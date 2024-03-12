#include "eval.h"

#include "common/macros.h"

#include "ast/match.h"
#include "runtime/special_forms.h"

RuntimeObject *TemporaryReferences_Add(TemporaryReferences references[static 1], RuntimeObject *object) {
    Vector_PushBack(&references->Objects, object);
    RuntimeObject_ReferenceCreated(object);
    return object;
}

void TemporaryReferences_Free(TemporaryReferences references[static 1]) {
    Vector_ForEach(objectPtr, references->Objects) {
        RuntimeObject_ReferenceDeleted(*objectPtr);
    }
    Vector_Free(&references->Objects);
    *references = TemporaryReferences_Empty();
}

RuntimeObject *Apply(RuntimeObject *fn, RuntimeObjectsSlice args) {
    if (RUNTIME_TYPE_NATIVE_FUNCTION == fn->Type) {
        return fn->AsNativeFunction(args);
    }

    if (RUNTIME_TYPE_FUNCTION == fn->Type) {
        auto function = fn->AsFunction;
        if (args.Size != function.ArgumentNames.Size) {
            fprintf(
                    stdout,
                    "Wrong number of arguments: expected %zu, got %zu\n",
                    function.ArgumentNames.Size, args.Size
            );
            return RuntimeObject_Undefined();
        }

        auto scope = Scope_WithParent(&function.CapturedVariables);
        for (size_t i = 0; i < function.ArgumentNames.Size; i++) {
            Scope_Put(&scope, function.ArgumentNames.Items[i], args.Items[i]);
        }

        for (size_t i = 0; i + 1 < function.Body.Size; i++) {
            auto result = RuntimeObject_ReferenceCreated(Evaluate(&scope, function.Body.Items[i]));
            RuntimeObject_ReferenceDeleted(result);
        }

        auto result = RuntimeObject_ReferenceCreated(Evaluate(&scope, *Vector_At(function.Body, -1)));

        Scope_Free(&scope);
        result->ReferencesCount--;

        return result;
    }

    fprintf(stdout, "Not a function\n");
    return RuntimeObject_Undefined();
}

static RuntimeObject *EvaluateIdentifier(Scope scope[static 1], AstNode node) {
    Assert(AST_IDENTIFIER == node.Type);

    RuntimeObject *value;
    if (Scope_TryResolve(*scope, node.AsIdentifier.NameChars, &value)) {
        return value;
    }

    fprintf(stdout, "Unresolved identifier `%s`\n", node.AsIdentifier.NameChars);
    return RuntimeObject_Undefined();
}

static RuntimeObject *EvaluateCall(Scope scope[static 1], AstNode node) {
    Assert(AST_EXPRESSION == node.Type);

    auto itemValues = TemporaryReferences_Empty();
    Vector_ForEach(itemPtr, node.AsExpression) {
        TemporaryReferences_Add(
                &itemValues,
                Evaluate(scope, *itemPtr)
        );
    }

    auto const result = Vector_Empty(itemValues.Objects)
                        ? RuntimeObject_Undefined()
                        : Apply(
                    itemValues.Objects.Items[0],
                    Vector_SliceAs(RuntimeObjectsSlice, itemValues.Objects, 1, itemValues.Objects.Size)
            );

    TemporaryReferences_Free(&itemValues);
    return result;
}

static RuntimeObject *EvaluateExpression(Scope scope[static 1], AstNode node) {
    auto const pattern = AstPattern_Expression(
            AstPattern_Type(AST_IDENTIFIER),
            AstPattern_Rest()
    );

    RuntimeSpecialForm specialForm;
    if (Ast_MatchByType((AstPattern *) pattern, node)
        && SpecialForm_TryGet(pattern->ItemPatterns[0]->MatchedNode.AsIdentifier.NameChars, &specialForm)) {
        return specialForm(scope, node);
    }

    return EvaluateCall(scope, node);
}

RuntimeObject *Evaluate(Scope scope[static 1], AstNode node) {
    switch (node.Type) {
        case AST_INT_LITERAL:
            return RuntimeObject_NewInt(node.AsInt.AsInt64);
        case AST_STRING_LITERAL:
            return RuntimeObject_NewString(node.AsString.Chars);
        case AST_IDENTIFIER:
            return EvaluateIdentifier(scope, node);
        case AST_EXPRESSION:
            return EvaluateExpression(scope, node);
    }

    Unreachable("%d", node.Type);
}
