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

typedef struct AstInt {
    int64_t AsInt64;
} AstInt;

typedef struct AstString {
    char const *Chars;
} AstString;

typedef struct AstIdentifier {
    char const *Name;
} AstIdentifier;

typedef struct AstExpression {
    AstNodes Items;
} AstExpression;

struct AstNode {
    AstNodeType Type;
    union {
        AstInt AsInt;
        AstString AsString;
        AstIdentifier AsIdentifier;
        AstExpression AsExpression;
    };
};

#define AstNode_Int(Value)          ((AstNode) {AST_INT_LITERAL, .AsInt={(Value)}})
#define AstNode_String(Value)       ((AstNode) {AST_STRING_LITERAL, .AsString={(Value)}})
#define AstNode_Identifier(Name)    ((AstNode) {AST_IDENTIFIER, .AsIdentifier={(Name)}})
#define AstNode_Expression(Items)   ((AstNode) {AST_EXPRESSION, .AsExpression={(Items)}})

void AstNode_PrettyPrint(FILE file[static 1], AstNode);

void AstNode_Free(AstNode node[static 1]);
