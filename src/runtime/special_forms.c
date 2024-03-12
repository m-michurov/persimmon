#include "special_forms.h"

#include "common/macros.h"
#include "ast/match.h"

RuntimeObject *While(Scope scope[static 1], AstNode node) {
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

    auto const runtimeFalse = RuntimeObject_ReferenceCreated(RuntimeObject_NewInt(0));

    while (true) {
        auto const conditionValue = RuntimeObject_ReferenceCreated(Evaluate(scope, condition));
        auto const conditionHolds = false == RuntimeObject_Equals(runtimeFalse, conditionValue);
        RuntimeObject_ReferenceDeleted(conditionValue);

        if (false == conditionHolds) {
            break;
        }

        auto bodyScope = Scope_WithParent(scope);

        for (size_t j = 0; j < body->NodesCount; j++) {
            auto const value =
                    RuntimeObject_ReferenceCreated(Evaluate(&bodyScope, body->Nodes[j]));
            RuntimeObject_ReferenceDeleted(value);
        }

        Scope_Free(&bodyScope);
    }
    RuntimeObject_ReferenceDeleted(runtimeFalse);

    return RuntimeObject_Undefined();
}

RuntimeObject *VariableDefinition(Scope scope[static 1], AstNode node) {
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

    auto const value = Evaluate(scope, pattern->ItemPatterns[2]->MatchedNode);
    Scope_Put(scope, pattern->ItemPatterns[1]->MatchedNode.AsIdentifier.NameChars, value);

    return value;
}

RuntimeObject *VariableAssignment(Scope scope[static 1], AstNode node) {
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

    auto const value = Evaluate(scope, pattern->ItemPatterns[2]->MatchedNode);
    Scope_Update(scope, pattern->ItemPatterns[1]->MatchedNode.AsIdentifier.NameChars, value);

    return value;
}

void CaptureFreeVariables(
        ArgumentNames argumentNames,
        AstNode node,
        Scope from,
        Scope to[static 1]
) {
    switch (node.Type) {
        case AST_INT_LITERAL:
        case AST_STRING_LITERAL:
            return;
        case AST_IDENTIFIER: {
            auto const name = node.AsIdentifier.NameChars;
            auto isFree = true;
            Vector_ForEach(argNamePtr, argumentNames) {
                if (0 != strcmp(*argNamePtr, name)) {
                    continue;
                }

                isFree = false;
                break;
            }

            RuntimeObject *object;
            if (isFree && false == Scope_TryResolve(*to, name, &object)) {
                if (Scope_TryResolve(from, name, &object)) {
                    Scope_Put(to, name, object);
                }
            }

            return;
        }
        case AST_EXPRESSION: {
            Vector_ForEach(itemPtr, node.AsExpression) {
                CaptureFreeVariables(argumentNames, *itemPtr, from, to);
            }

            return;
        }
    }

    Unreachable("%d", node.Type);
}

RuntimeObject *Function(Scope scope[static 1], AstNode node) {
    auto const pattern =
            AstPattern_Expression(
                    AstPattern_Any(),
                    (AstPattern *) AstPattern_Expression(
                            AstPattern_Rest()
                    ),
                    AstPattern_Rest()
            );

    if (false == Ast_MatchByType((AstPattern *) pattern, node)) {
        fprintf(stdout, "Invalid usage of special form `fn`\n");
        return RuntimeObject_Undefined();
    }

    auto const argNodes = (AstPatternRest *) ((AstPatternExpression *) pattern->ItemPatterns[1])->ItemPatterns[0];
    auto argNames = (ArgumentNames) {0};

    for (size_t i = 0; i < argNodes->NodesCount; i++) {
        auto const argNode = argNodes->Nodes[i];
        if (AST_IDENTIFIER != argNode.Type) {
            fprintf(stdout, "Invalid usage of special form `fn`: arguments must be identifiers\n");
            Vector_Free(&argNames);
            return RuntimeObject_Undefined();
        }

        Vector_PushBack(&argNames, strdup(argNode.AsIdentifier.NameChars));
    }

    auto const bodyNodes = (AstPatternRest *) pattern->ItemPatterns[2];
    auto body = (FunctionBody) {0};
    auto capturedVariables = Scope_Empty();

    for (size_t i = 0; i < bodyNodes->NodesCount; i++) {
        auto const bodyNode = bodyNodes->Nodes[i];
        Vector_PushBack(&body, AstNode_Copy(bodyNode));
        CaptureFreeVariables(argNames, bodyNode, *scope, &capturedVariables);
    }

    return RuntimeObject_NewFunction(argNames, body, capturedVariables);
}

static struct {
    char const *Name;
    RuntimeSpecialForm Implementation;
} const SpecialForms[] = {
        {.Name = "while", .Implementation = While},
        {.Name = ":=", .Implementation = VariableDefinition},
        {.Name = "=", .Implementation = VariableAssignment},
        {.Name = "fn", .Implementation = Function},
};

static const auto SpecialFormsCount = sizeof(SpecialForms) / sizeof(SpecialForms[0]);

bool SpecialForm_TryGet(char const *name, RuntimeSpecialForm specialForm[static 1]) {
    for (size_t i = 0; i < SpecialFormsCount; i++) {
        if (0 != strcmp(name, SpecialForms[i].Name)) {
            continue;
        }

        *specialForm = SpecialForms[i].Implementation;
        return true;
    }

    return false;
}
