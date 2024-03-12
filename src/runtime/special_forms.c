#include "special_forms.h"

#include "common/macros.h"
#include "ast/match.h"

RuntimeObject *While(Scope scope[static 1], AstNode node) {
    auto const pattern =
            AstPattern_MatchExpression(
                    AstPattern_MatchAny(),
                    AstPattern_MatchAny(),
                    AstPattern_MatchRest()
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

        Vector_ForEach(bodyNodePtr, body->Nodes) {
            auto const value =
                    RuntimeObject_ReferenceCreated(Evaluate(&bodyScope, *bodyNodePtr));
            RuntimeObject_ReferenceDeleted(value);
        }

        Scope_Free(&bodyScope);
    }
    RuntimeObject_ReferenceDeleted(runtimeFalse);

    return RuntimeObject_Undefined();
}

RuntimeObject *VariableDefinition(Scope scope[static 1], AstNode node) {
    auto const pattern =
            AstPattern_MatchExpression(
                    AstPattern_MatchAny(),
                    AstPattern_MatchByType(AST_IDENTIFIER),
                    AstPattern_MatchAny()
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
            AstPattern_MatchExpression(
                    AstPattern_MatchAny(),
                    AstPattern_MatchByType(AST_IDENTIFIER),
                    AstPattern_MatchAny()
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
            auto const isFree = false == Vector_Any(argumentNames, it, 0 == strcmp(it, name));

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
            AstPattern_MatchExpression(
                    AstPattern_MatchAny(),
                    (AstPattern *) AstPattern_MatchExpression(
                            AstPattern_MatchRestByType(AST_IDENTIFIER)
                    ),
                    AstPattern_MatchRest()
            );

    if (false == Ast_MatchByType((AstPattern *) pattern, node)) {
        fprintf(stdout, "Invalid usage of special form `fn`\n");
        return RuntimeObject_Undefined();
    }

    auto const argNodes = (AstPatternRestByType *) ((AstPatternExpression *) pattern->ItemPatterns[1])->ItemPatterns[0];
    auto argNames = (ArgumentNames) {0};

    Vector_ForEach(argNodePtr, argNodes->Nodes) {
        Vector_PushBack(&argNames, strdup(argNodePtr->AsIdentifier.NameChars));
    }

    auto const bodyNodes = (AstPatternRest *) pattern->ItemPatterns[2];
    auto body = (FunctionBody) {0};
    auto capturedVariables = Scope_Empty();

    Vector_ForEach(bodyNodePtr, bodyNodes->Nodes) {
        Vector_PushBack(&body, AstNode_Copy(*bodyNodePtr));
        CaptureFreeVariables(argNames, *bodyNodePtr, *scope, &capturedVariables);
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
