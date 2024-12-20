#pragma once

#include <stdio.h>

#include "position.h"
#include "line_reader.h"

typedef enum {
    SYNTAX_ERROR_INTEGER_INVALID,
    SYNTAX_ERROR_INTEGER_TOO_LARGE,
    SYNTAX_ERROR_INTEGER_LEADING_ZERO,
    SYNTAX_ERROR_STRING_UNKNOWN_ESCAPE_SEQUENCE,
    SYNTAX_ERROR_STRING_UNTERMINATED,
    SYNTAX_ERROR_INVALID_CHARACTER,
    SYNTAX_ERROR_UNEXPECTED_CLOSE_PAREN,
    SYNTAX_ERROR_NO_MATCHING_CLOSE_PAREN,
    SYNTAX_ERROR_NESTING_TOO_DEEP,
    SYNTAX_ERROR_TOKEN_TOO_LONG
} SyntaxError_Code;

char const *syntax_error_str(SyntaxError_Code error_type);

typedef struct {
    SyntaxError_Code code;
    Position pos;
} SyntaxError;

