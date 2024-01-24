#pragma once

#include <stdint.h>
#include <stdio.h>

typedef enum TokenType {
    TOKEN_NONE = 0,
    TOKEN_INVALID,
    TOKEN_DOT,
    TOKEN_OPEN_PAREN,
    TOKEN_CLOSE_PAREN,
    TOKEN_IDENTIFIER,
    TOKEN_INT_LITERAL,
    TOKEN_STRING_LITERAL,
} TokenType;

const char *TokenType_Name(TokenType tokenType);

typedef struct Token {
    TokenType Type;
    union {
        const char *Identifier;
        const char *StringLiteral;
        int64_t IntLiteral;
    };
} Token;

void Token_Print(FILE file[static 1], Token token);
