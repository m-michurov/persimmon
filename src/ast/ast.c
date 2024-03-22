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
            fprintf(file, ".AsInt={%" PRId64 "}", node.AsInt.AsInt64);
            break;
        case AST_STRING_LITERAL:
            fprintf(file, ".AsString={\"%s\"}", node.AsString.Chars);
            break;
        case AST_IDENTIFIER:
            fprintf(file, ".AsIdentifier={%s}", node.AsIdentifier.NameChars);
            break;
        case AST_EXPRESSION: {
            fprintf(file, ".AsExpression={[\n");
            Vector_ForEach(childNodePtr, node.AsExpression) {
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

AstNode AstNode_Copy(AstNode node) {
    switch (node.Type) {
        case AST_INT_LITERAL:
            return AstNode_Int(node.AsInt.AsInt64);
        case AST_STRING_LITERAL:
            return AstNode_String(strdup(node.AsString.Chars));
        case AST_IDENTIFIER:
            return AstNode_Identifier(strdup(node.AsIdentifier.NameChars));
        case AST_EXPRESSION: {
            auto copy = (AstExpression) {0};
            Vector_ForEach(nodeItemPtr, node.AsExpression) {
                Vector_PushBack(&copy, AstNode_Copy(*nodeItemPtr));
            }

            return AstNode_Expression(copy);
        }
    }

    Unreachable("%d", node.Type);
}

void AstNode_PrettyPrint(FILE file[static 1], AstNode node) {
    PrettyPrint(file, node, 0);
}

void AstNode_Free(AstNode node[static 1]) {
    switch (node->Type) {
        case AST_INT_LITERAL:
            break;
        case AST_STRING_LITERAL: {
            free((void *) node->AsString.Chars);
            break;
        }
        case AST_IDENTIFIER: {
            free((void *) node->AsIdentifier.NameChars);
            break;
        }
        case AST_EXPRESSION: {
            Vector_ForEach(childNode, node->AsExpression) {
                AstNode_Free(childNode);
            }
            Vector_Free(&node->AsExpression);
            break;
        }
        default:
            Unreachable("%d", node->Type);
    }
}