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

void AstNode_Print(FILE file[static 1], AstNode);
