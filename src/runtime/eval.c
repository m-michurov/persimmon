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
           || 0 == strcmp("while", name);
}

RuntimeObject *EvaluateVariableDefinition(Scope scope[static 1], AstNode node) {
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

RuntimeObject *EvaluateAssignment(Scope scope[static 1], AstNode node) {
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

RuntimeObject *EvaluateWhile(Scope scope[static 1], AstNode node) {
    auto const pattern =
            AstPattern_Expression(
                    AstPattern_Any(),
                    AstPattern_Any(),
                    AstPattern_Rest()
            );

    if (false == Ast_MatchByType((AstPattern *) pattern, node)) {
        fprintf(stdout, "Invalid usage of special form `for`\n");
        return RuntimeObject_Undefined();
    }

    auto const condition = pattern->ItemPatterns[1]->MatchedNode;
    auto const body = (AstPatternRest *) pattern->ItemPatterns[2];

    auto const runtimeFalse = RuntimeObject_NewInt(0);
    RuntimeObject_ReferenceCreated(runtimeFalse);

    while (true) {
        auto const conditionValue = Evaluate(scope, condition);
        RuntimeObject_ReferenceCreated(conditionValue);
        auto const conditionHolds = false == RuntimeObject_Equals(*runtimeFalse, *conditionValue);
        RuntimeObject_ReferenceDeleted(conditionValue);
        if (false == conditionHolds) {
            break;
        }

        auto bodyScope = Scope_WithParent(scope);

        for (size_t j = 0; j < body->NodesCount; j++) {
            auto const result = Evaluate(&bodyScope, body->Nodes[j]);
            RuntimeObject_ReferenceDeleted(result);
        }

        Scope_Free(&bodyScope);
    }
    RuntimeObject_ReferenceDeleted(runtimeFalse);

    return RuntimeObject_Undefined();
}

RuntimeObject *EvaluateSpecialForm(Scope scope[static 1], AstNode node) {
    auto const head = node.AsExpression.Items.Items[0].AsIdentifier;
    if (0 == strcmp(":=", head.Name)) {
        return EvaluateVariableDefinition(scope, node);
    }

    if (0 == strcmp("=", head.Name)) {
        return EvaluateAssignment(scope, node);
    }

    if (0 == strcmp("while", head.Name)) {
        return EvaluateWhile(scope, node);
    }

    return RuntimeObject_Undefined();
}

RuntimeObject *Evaluate(Scope scope[static 1], AstNode node) {
    switch (node.Type) {
        case AST_INT_LITERAL:
            return RuntimeObject_NewInt(node.AsIntLiteral.Value);
        case AST_STRING_LITERAL:
            return RuntimeObject_NewString(node.AsStringLiteral.Value);
        case AST_IDENTIFIER: {
            RuntimeObject *object;
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
            auto itemValues = (Vector_Of(RuntimeObject *)) {0};

            Vector_ForEach(itemPtr, items) {
                auto const itemValue = Evaluate(scope, *itemPtr);
                RuntimeObject_ReferenceCreated(itemValue);
                Vector_PushBack(&itemValues, itemValue);
            }

            if (Vector_Empty(itemValues)) {
                return RuntimeObject_Undefined();
            }

            auto const fn = itemValues.Items[0];
            if (RUNTIME_TYPE_NATIVE_FUNCTION != fn->Type) {
                fprintf(stdout, "Not a function\n");
                return RuntimeObject_Undefined();
            }

            auto const args = Vector_SliceAs(RuntimeObjectsSlice, itemValues, 1, itemValues.Size);
            auto const result = fn->AsNativeFunction(args);

            Vector_ForEach(itemPtr, itemValues) {
                RuntimeObject_ReferenceDeleted(*itemPtr);
            }
            Vector_Free(&itemValues);

            return result;
        }
    }

    Unreachable("%d", node.Type);
}
