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
        auto const conditionHolds = false == RuntimeObject_Equals(*runtimeFalse, *conditionValue);
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

RuntimeObject *VariableDefinition(Scope scope[1], AstNode node) {
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
    Scope_Put(scope, pattern->ItemPatterns[1]->MatchedNode.AsIdentifier.Name, value);

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
    Scope_Update(scope, pattern->ItemPatterns[1]->MatchedNode.AsIdentifier.Name, value);

    return value;
}

static struct {
    char const *Name;
    RuntimeSpecialForm Implementation;
} const SpecialForms[] = {
        {.Name = "while", .Implementation = While},
        {.Name = ":=", .Implementation = VariableDefinition},
        {.Name = "=", .Implementation = VariableAssignment},
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
