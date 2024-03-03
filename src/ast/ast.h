#pragma once

#include <stdint.h>

#include "call_checked.h"
#include "collections/vector.h"

#include "lexer/lexer.h"

typedef enum AstNodeType {
    AST_INT_LITERAL,
    AST_STRING_LITERAL,
    AST_IDENTIFIER,
    AST_EXPRESSION,
} AstNodeType;

char const *AstNodeType_Name(AstNodeType);

typedef struct AstNode AstNode;
typedef Vector_Of(AstNode) AstNodes;

typedef struct AstIntLiteral {
    int64_t Value;
} AstIntLiteral;

typedef struct AstStringLiteral {
    char const *Value;
} AstStringLiteral;

typedef struct AstIdentifier {
    char const *Name;
} AstIdentifier;

typedef struct AstExpression {
    AstNodes Items;
} AstExpression;

struct AstNode {
    AstNodeType Type;
    union {
        AstIntLiteral AsIntLiteral;
        AstStringLiteral AsStringLiteral;
        AstIdentifier AsIdentifier;
        AstExpression AsExpression;
    };
};

#define AstNode_IntLiteral(Value)       ((AstNode) {AST_INT_LITERAL, .AsIntLiteral={(Value)}})
#define AstNode_StringLiteral(Value)    ((AstNode) {AST_STRING_LITERAL, .AsStringLiteral={(Value)}})
#define AstNode_Identifier(Name)        ((AstNode) {AST_IDENTIFIER, .AsIdentifier={(Name)}})
#define AstNode_Expression(Items)       ((AstNode) {AST_EXPRESSION, .AsExpression={(Items)}})

void AstNode_PrettyPrint(FILE file[static 1], AstNode);
