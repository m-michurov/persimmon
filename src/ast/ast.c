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

static void Indent(FILE file[static 1], size_t depth) {
    for (size_t i = 0; i < depth; i++) {
        fprintf(file, "  ");
    }
}

static void PrettyPrint(FILE file[static 1], AstNode node, size_t depth) {
    Indent(file, depth);
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
            fprintf(file, ".AsExpression={[\n");
            Vector_ForEach(childNodePtr, node.AsExpression.Items) {
                PrettyPrint(file, *childNodePtr, depth + 1);
                fprintf(file, ",\n");
            }
            Indent(file, depth);
            fprintf(file, "]}");
            break;
        }
        default:
            Unreachable("%d", node.Type);
    }

    fprintf(file, "}");
}

void AstNode_PrettyPrint(FILE file[static 1], AstNode node) {
    PrettyPrint(file, node, 0);
}