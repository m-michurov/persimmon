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

typedef struct {
    Token_Type type;
    Position pos;
    union {
        int64_t as_int;
        char const *as_atom;
        char const *as_string;
    };
} Token;

typedef enum {
    SCANNER_WS,
    SCANNER_INT,
    SCANNER_STRING,
    SCANNER_ATOM,
    SCANNER_OPEN_PAREN,
    SCANNER_CLOSE_PAREN,
    SCANNER_QUOTE,
    SCANNER_COMMENT
} Scanner_State;

typedef struct {
    bool has_token;
    Token token;

    Scanner_State _state;
    StringBuilder _sb;
    bool _escape_sequence;
    int64_t _int_value;
    Position _token_pos;
} Scanner;

typedef struct {
    size_t max_token_length;
} Scanner_Config;

bool scanner_try_init(Scanner *s, Scanner_Config config, errno_t *error_code);

void scanner_free(Scanner *s);

void scanner_reset(Scanner *s);

[[nodiscard]]
bool scanner_try_accept(Scanner *s, Position pos, int c, SyntaxError *error);
