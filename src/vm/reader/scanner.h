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
    TOKEN_CLOSE_PAREN,
    TOKEN_QUOTE
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

typedef struct {
    size_t max_token_size;
} Scanner_Config;

Scanner *scanner_new(Scanner_Config config);

void scanner_free(Scanner **s);

void scanner_reset(Scanner *s);

[[nodiscard]]

bool scanner_try_accept(Scanner *s, Position pos, int c, SyntaxError *error);

Token const *scanner_peek(Scanner const *s);
