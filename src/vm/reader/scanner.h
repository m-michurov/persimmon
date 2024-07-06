#pragma once

#include <stdint.h>

#include "utility/arena.h"
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

typedef struct Scanner Scanner;

Scanner *scanner_new(void);

void scanner_free(Scanner **s);

void scanner_reset(Scanner *s);

bool scanner_try_accept(Scanner *s, Position pos, int c, SyntaxError *error);

Token const *scanner_peek(Scanner const *s);
