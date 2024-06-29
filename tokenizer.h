#pragma once

#include <stdint.h>

#include "arena.h"
#include "position.h"
#include "syntax_error.h"

typedef enum {
    TOKEN_EOF,
    TOKEN_INT,
    TOKEN_STRING,
    TOKEN_ATOM,
    TOKEN_OPEN_PAREN,
    TOKEN_CLOSE_PAREN
} Token_Type;

char const *token_type_str(Token_Type type);

typedef struct {
    Token_Type type;
    Position pos;
    union {
        int64_t as_int;
        char const *as_atom;
        char const *as_string;
    };
} Token;

typedef struct Tokenizer Tokenizer;

Tokenizer *tokenizer_new(Arena *a);

void tokenizer_reset(Tokenizer *t);

bool tokenizer_try_accept(Tokenizer *t, Position pos, int c, SyntaxError *error);

Token const *tokenizer_token(Tokenizer const *t);
