#pragma once

#include <stdbool.h>

#include "token.h"

typedef struct LexerError {
    long StreamPosition;
    int BadChar;
    char const *Why;
} LexerError;

typedef struct Lexer Lexer;

Lexer *Lexer_Init(FILE file[1]);

void Lexer_Free(Lexer *lexer);

typedef enum LexerResultType {
    LEXER_ERROR,
    LEXER_TOKEN
} LexerResultType;

typedef struct LexerResult {
    LexerResultType Type;
    union {
        Token Token;
        LexerError Error;
    };
} LexerResult;

LexerResult Lexer_Next(Lexer *lexer);

bool Lexer_HasNext(Lexer *lexer);
