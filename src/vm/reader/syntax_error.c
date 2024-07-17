#include "syntax_error.h"

#include <ctype.h>

#include "utility/guards.h"
#include "utility/strings.h"

char const *syntax_error_str(SyntaxError_Code error_type) {
    switch (error_type) {
        case SYNTAX_ERROR_INTEGER_INVALID: {
            return "invalid integer literal";
        }
        case SYNTAX_ERROR_INTEGER_TOO_LARGE: {
            return "integer literal is too large";
        }
        case SYNTAX_ERROR_INTEGER_LEADING_ZERO: {
            return "integer literal cannot have leading zeros";
        }
        case SYNTAX_ERROR_STRING_UNKNOWN_ESCAPE_SEQUENCE: {
            return "unknown escape sequence";
        }
        case SYNTAX_ERROR_STRING_UNTERMINATED: {
            return "unterminated string literal";
        }
        case SYNTAX_ERROR_INVALID_CHARACTER: {
            return "invalid character";
        }
        case SYNTAX_ERROR_UNEXPECTED_CLOSE_PAREN: {
            return "unexpected ')'";
        }
        case SYNTAX_ERROR_NO_MATCHING_CLOSE_PAREN: {
            return "'(' was never closed";
        }
        case SYNTAX_ERROR_NESTING_TOO_DEEP: {
            return "too many nesting levels";
        }
    }

    guard_unreachable();
}

