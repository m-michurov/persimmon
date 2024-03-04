#pragma once

#include <stdint.h>
#include <stdio.h>

typedef enum TokenType {
    TOKEN_NONE = 0,
    TOKEN_INVALID,
    TOKEN_OPEN_PAREN,
    TOKEN_CLOSE_PAREN,
    TOKEN_IDENTIFIER,
    TOKEN_INT_LITERAL,
    TOKEN_STRING_LITERAL,
} TokenType;

const char *TokenType_Name(TokenType tokenType);

typedef struct Token {
    TokenType Type;
    long Start;
    long End;

    union {
        const char *AsIdentifier;
        const char *AsStringLiteral;
        int64_t AsIntLiteral;
    } Value;
} Token;

#define Token_Identifier(Start_, End_, Name)    \
((Token) {                                      \
    .Type=TOKEN_IDENTIFIER,                     \
    .Start=(Start_),                            \
    .End=(End_),                                \
    .Value.AsIdentifier=(Name)                  \
})

#define Token_StringLiteral(Start_, End_, Value_)   \
((Token) {                                          \
    .Type=TOKEN_STRING_LITERAL,                     \
    .Start=(Start_),                                \
    .End=(End_),                                    \
    .Value.AsStringLiteral=(Value_)                 \
})

#define Token_IntLiteral(Start_, End_, Value_)  \
((Token) {                                      \
    .Type=TOKEN_INT_LITERAL,                    \
    .Start=(Start_),                            \
    .End=(End_),                                \
    .Value.AsIntLiteral=(Value_)                \
})

#define Token_OpenParenthesis(Start_)   ((Token) {.Type=TOKEN_OPEN_PAREN, .Start=(Start_), .End=(Start_)+1})
#define Token_CloseParenthesis(Start_)  ((Token) {.Type=TOKEN_CLOSE_PAREN, .Start=(Start_), .End=(Start_)+1})

void Token_Print(FILE file[static 1], Token token);
