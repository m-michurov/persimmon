#include "eval.h"

#include "common/macros.h"

#include "ast/match.h"

bool IsSpecialForm(AstNode node) {
    auto const pattern =
            AstPattern_Expression(
                    AstPattern_Type(AST_IDENTIFIER),
                    AstPattern_Rest()
            );

    if (false == Ast_MatchByType(&pattern->Base, node)) {
        return false;
    }

    auto const name = pattern->ItemPatterns[0]->MatchedNode.AsIdentifier.Name;
    return 0 == strcmp(":=", name)
           || 0 == strcmp("=", name)
           || 0 == strcmp("for", name);
}

RuntimeObject EvaluateVariableDefinition(Scope scope[static 1], AstNode node) {
    auto const pattern =
            AstPattern_Expression(
                    AstPattern_Any(),
                    AstPattern_Type(AST_IDENTIFIER),
                    AstPattern_Any()
            );

    if (false == Ast_MatchByType((AstPattern *) pattern, node)) {
        fprintf(stdout, "Invalid usage of special form `:=`\n");
        return RuntimeObject_Undefined();
    }

    auto const target = pattern->ItemPatterns[1]->MatchedNode.AsIdentifier.Name;
    auto const value = Evaluate(scope, pattern->ItemPatterns[2]->MatchedNode);

    Scope_Put(scope, target, value);

    return RuntimeObject_Undefined();
}

RuntimeObject EvaluateAssignment(Scope scope[static 1], AstNode node) {
    auto const pattern =
            AstPattern_Expression(
                    AstPattern_Any(),
                    AstPattern_Type(AST_IDENTIFIER),
                    AstPattern_Any()
            );

    if (false == Ast_MatchByType((AstPattern *) pattern, node)) {
        fprintf(stdout, "Invalid usage of special form `=`\n");
        return RuntimeObject_Undefined();
    }

    auto const target = pattern->ItemPatterns[1]->MatchedNode.AsIdentifier.Name;
    auto const value = Evaluate(scope, pattern->ItemPatterns[2]->MatchedNode);

    Scope_Update(scope, target, value);

    return RuntimeObject_Undefined();
}

RuntimeObject EvaluateFor(Scope scope[static 1], AstNode node) {
    auto const pattern =
            AstPattern_Expression(
                    AstPattern_Any(),
                    AstPattern_Type(AST_IDENTIFIER),
                    AstPattern_Any(),
                    AstPattern_Any(),
                    AstPattern_Rest()
            );

    if (false == Ast_MatchByType((AstPattern *) pattern, node)) {
        fprintf(stdout, "Invalid usage of special form `for`\n");
        return RuntimeObject_Undefined();
    }

    auto const counter = pattern->ItemPatterns[1]->MatchedNode.AsIdentifier.Name;

    auto const lower = Evaluate(scope, pattern->ItemPatterns[2]->MatchedNode);
    if (RUNTIME_TYPE_INT != lower.Type) {
        return RuntimeObject_Undefined();
    }

    auto const upper = Evaluate(scope, pattern->ItemPatterns[3]->MatchedNode);
    if (RUNTIME_TYPE_INT != upper.Type) {
        return RuntimeObject_Undefined();
    }

    auto result = RuntimeObject_Undefined();
    auto const body = (AstPatternRest *) pattern->ItemPatterns[4];

    for (int64_t i = lower.AsInt; i <= upper.AsInt; i++) {
        auto bodyScope = Scope_WithParent(scope);

        Scope_Put(&bodyScope, counter, RuntimeObject_Int(i));

        for (size_t j = 0; j < body->NodesCount; j++) {
            result = Evaluate(&bodyScope, body->Nodes[j]);
        }

        Scope_Free(&bodyScope);
    }

    return result;
}

RuntimeObject EvaluateSpecialForm(Scope scope[static 1], AstNode node) {
    auto const head = node.AsExpression.Items.Items[0].AsIdentifier;
    if (0 == strcmp(":=", head.Name)) {
        return EvaluateVariableDefinition(scope, node);
    }

    if (0 == strcmp("=", head.Name)) {
        return EvaluateAssignment(scope, node);
    }

    if (0 == strcmp("for", head.Name)) {
        return EvaluateFor(scope, node);
    }

    return RuntimeObject_Undefined();
}

RuntimeObject Evaluate(Scope scope[static 1], AstNode node) {
    switch (node.Type) {
        case AST_INT_LITERAL:
            return RuntimeObject_Int(node.AsIntLiteral.Value);
        case AST_STRING_LITERAL:
            return RuntimeObject_String(node.AsStringLiteral.Value);
        case AST_IDENTIFIER: {
            RuntimeObject object;
            if (Scope_TryResolve(*scope, node.AsIdentifier.Name, &object)) {
                return object;
            }

            return RuntimeObject_Undefined();
        }
        case AST_EXPRESSION: {
            if (IsSpecialForm(node)) {
                return EvaluateSpecialForm(scope, node);
            }

            auto const items = node.AsExpression.Items;
            auto itemValues = (Vector_Of(RuntimeObject)) {0};

            Vector_ForEach(itemPtr, items) {
                Vector_PushBack(&itemValues, Evaluate(scope, *itemPtr));
            }

            if (Vector_Empty(itemValues)) {
                return RuntimeObject_Undefined();
            }

            auto const fn = itemValues.Items[0];
            if (RUNTIME_TYPE_NATIVE_FUNCTION != fn.Type) {
                return RuntimeObject_Undefined();
            }

            RuntimeObject result;
            fn.AsNativeFunction(itemValues.Size - 1, itemValues.Items + 1, &result);
            return result;
        }
    }

    Unreachable("%d", node.Type);
}
