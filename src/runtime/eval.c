#include "eval.h"

#include "common/macros.h"

bool IsSpecialForm(AstNode node) {
    if (AST_EXPRESSION != node.Type
        || Vector_Empty(node.AsExpression.Items)
        || AST_IDENTIFIER != node.AsExpression.Items.Items[0].Type) {
        return false;
    }

    auto const name = node.AsExpression.Items.Items[0].AsIdentifier.Name;
    return 0 == strcmp(":=", name);
}

RuntimeObject EvaluateVariableDefinition(Scope scope[static 1], AstNode node) {
    Assert(AST_EXPRESSION == node.Type);

    auto const childNodes = node.AsExpression.Items;
    if (3 != childNodes.Size) {
        return RuntimeObject_Undefined();
    }

    auto const head = childNodes.Items[0];
    if (AST_IDENTIFIER != head.Type || 0 != strcmp(":=", head.AsIdentifier.Name)) {
        return RuntimeObject_Undefined();
    }

    auto const target = childNodes.Items[1];
    if (AST_IDENTIFIER != target.Type) {
        return RuntimeObject_Undefined();
    }

    auto const value = Evaluate(scope, childNodes.Items[2]);
    Scope_Put(scope, target.AsIdentifier.Name, value);

    return RuntimeObject_Undefined();
}

RuntimeObject EvaluateSpecialForm(Scope scope[static 1], AstNode node) {
    auto const head = node.AsExpression.Items.Items[0].AsIdentifier;
    if (0 == strcmp(":=", head.Name)) {
        return EvaluateVariableDefinition(scope, node);
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
