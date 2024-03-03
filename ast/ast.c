#include "ast.h"

#include <inttypes.h>

#include "common/macros.h"

char const *AstNodeType_Name(AstNodeType nodeType) {
    switch (nodeType) {
        case AST_INT_LITERAL:
            return "AST_INT_LITERAL";
        case AST_STRING_LITERAL:
            return "AST_STRING_LITERAL";
        case AST_IDENTIFIER:
            return "AST_IDENTIFIER";
        case AST_EXPRESSION:
            return "AST_EXPRESSION";
        default:
            Unreachable("%d", nodeType);
    }

    Unreachable();
}

void AstNode_Print(FILE file[static 1], AstNode node) {
    fprintf(file, "AstNode{.Type=%s, ", AstNodeType_Name(node.Type));

    switch (node.Type) {
        case AST_INT_LITERAL:
            fprintf(file, ".AsIntLiteral={%" PRId64 "}", node.AsIntLiteral.Value);
            break;
        case AST_STRING_LITERAL:
            fprintf(file, ".AsStringLiteral={\"%s\"}", node.AsStringLiteral.Value);
            break;
        case AST_IDENTIFIER:
            fprintf(file, ".AsIdentifier={%s}", node.AsIdentifier.Name);
            break;
        case AST_EXPRESSION: {
            fprintf(file, ".AsExpression={[");
            Vector_ForEach(childNodePtr, node.AsExpression.Items) {
                AstNode_Print(file, *childNodePtr);
                fprintf(file, ", ");
            }
            fprintf(file, "]}");
            break;
        }
        default:
            Unreachable("%d", node.Type);
    }

    fprintf(file, "}");
}