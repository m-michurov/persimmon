#include "syntax_error.h"

#include <ctype.h>

#include "guards.h"
#include "strings.h"

char const *syntax_error_code_desc(SyntaxError_Code error_type) {
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

void syntax_error_print(FILE *file, SyntaxError error) {
    guard_is_not_null(file);

    fprintf(file, "SyntaxError: %s", syntax_error_code_desc(error.code));
    if (SYNTAX_ERROR_INVALID_CHARACTER != error.code) {
        printf("\n");
        return;
    }

    printf(" - ");
    if (isprint(error.bad_chr)) {
        printf("'%c'\n", error.bad_chr);
        return;
    }

    char const *escape_seq;
    if (string_try_repr_escape_seq((char) error.bad_chr, &escape_seq)) {
        printf("'%s'\n", escape_seq);
        return;
    }

    printf("\\0x%02hhX\n", error.bad_chr);
}
