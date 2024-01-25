#pragma once

#include <stdbool.h>

#include "token.h"

typedef struct CharBuffer {
    char *Data;
    size_t Size;
    size_t Capacity;
} CharBuffer;

typedef struct Lexer {
    FILE *file;
    long tokenStart;

    TokenType lastTokenType;
    CharBuffer buffer;
} Lexer;

Lexer Lexer_New(FILE file[static 1]);

void Lexer_Free(Lexer lexer[static 1]);

bool Lexer_SkipToken(Lexer lexer[static 1]);

bool Lexer_SyntaxError(Lexer lexer[static 1], const char *expected, int got, Token token[static 1]);

bool Lexer_ParseStringLiteral(Lexer lexer[static 1], Token token[static 1]);

bool Lexer_ParseIdentifier(Lexer lexer[static 1], Token token[static 1]);

bool Lexer_ParseIntLiteral(Lexer lexer[static 1], Token token[static 1]);

bool Lexer_NextToken(Lexer lexer[static 1], Token token[static 1]);
