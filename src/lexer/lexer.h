#pragma once

#include <stdbool.h>

#include "token.h"

typedef struct LexerError {
    long StreamPosition;
    int BadChar;
    char const *Why;
} LexerError;

typedef struct Lexer Lexer;

Lexer *Lexer_Init(FILE file[static 1]);

void Lexer_Free(Lexer *);

typedef enum LexerResultType {
    LEXER_ERROR,
    LEXER_TOKEN
} LexerResultType;

typedef struct LexerResult {
    LexerResultType Type;
    union {
        Token AsToken;
        LexerError AsError;
    };
} LexerResult;

LexerResult Lexer_Next(Lexer *);

bool Lexer_HasNext(Lexer *);
