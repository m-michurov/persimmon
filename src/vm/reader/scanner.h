#pragma once

#include <stdint.h>

#include "utility/arena.h"
#include "position.h"
#include "syntax_error.h"

typedef enum {
    TOKEN_EOF,
    TOKEN_INT,
    TOKEN_STRING,
    TOKEN_SYMBOL,
    TOKEN_OPEN_PAREN,
    TOKEN_CLOSE_PAREN,
    TOKEN_QUOTE
} Token_Type;

typedef struct {
    Token_Type type;
    Position pos;
    bool next_dot;
    union {
        int64_t as_int;
        char const *as_symbol;
        char const *as_string;
    };
} Token;

typedef enum {
    SCANNER_WS,
    SCANNER_INT,
    SCANNER_STRING,
    SCANNER_SYMBOL,
    SCANNER_OPEN_PAREN,
    SCANNER_CLOSE_PAREN,
    SCANNER_QUOTE,
    SCANNER_COMMENT,
    SCANNER_DOT
} Scanner_State;

typedef struct {
    bool has_token;
    Token token;

    Scanner_State _state;
    StringBuilder _sb;
    bool _escape_sequence;
    bool _string_terminated;
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
