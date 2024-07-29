#include "scanner.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "utility/exchange.h"
#include "utility/math.h"
#include "utility/guards.h"
#include "utility/strings.h"

#define SINGLE_QUOTE    '\''
#define DOUBLE_QUOTE    '"'
#define OPEN_PAREN      '('
#define CLOSE_PAREN     ')'
#define SEMICOLON       ';'
#define BACKSLASH       '\\'

static bool is_name_char(int c) {
    return isalnum(c) || ('\0' != c && nullptr != strchr("~!@#$%^&*_+-=./<>?", c));
}

static bool is_whitespace(int c) {
    return isspace(c) || ',' == c || EOF == c;
}

static bool is_printable(int c) {
    return isprint(c) || ' ' == c;
}

static bool is_newline(int c) {
    return '\r' == c || '\n' == c;
}

static void clear(Scanner *s) {
    sb_clear(&s->_sb);
    s->_escape_sequence = false;
    s->_int_value = 0;
}

static void transition(Scanner *s, Position pos, Scanner_State new_state) {
    auto const state = exchange(s->_state, new_state);
    auto const token_pos = exchange(s->_token_pos, pos);

    switch (state) {
        case SCANNER_WS:
        case SCANNER_COMMENT: {
            s->has_token = false;
            clear(s);
            return;
        }
        case SCANNER_INT: {
            s->has_token = true;
            s->token = (Token) {
                    .type = TOKEN_INT,
                    .pos = token_pos,
                    .as_int = s->_int_value
            };
            clear(s);
            return;
        }
        case SCANNER_STRING: {
            s->has_token = true;
            s->token = (Token) {
                    .type = TOKEN_STRING,
                    .pos = token_pos,
                    .as_string = s->_sb.str
            };
//            tokenizer_clear(t);
            return;
        }
        case SCANNER_ATOM: {
            guard_is_greater(s->_sb.length, 0);

            s->has_token = true;
            s->token = (Token) {
                    .type = TOKEN_ATOM,
                    .pos = token_pos,
                    .as_atom = s->_sb.str
            };
//            tokenizer_clear(t);
            return;
        }
        case SCANNER_OPEN_PAREN: {
            s->has_token = true;
            s->token = (Token) {.type = TOKEN_OPEN_PAREN, .pos = token_pos};
            clear(s);
            return;
        }
        case SCANNER_CLOSE_PAREN: {
            s->has_token = true;
            s->token = (Token) {.type = TOKEN_CLOSE_PAREN, .pos = token_pos};
            clear(s);
            return;
        }
        case SCANNER_QUOTE: {
            s->has_token = true;
            s->token = (Token) {.type = TOKEN_QUOTE, .pos = token_pos};
            clear(s);
            return;
        }
    }

    guard_unreachable();
}

static bool try_add_digit(int64_t int_value, int64_t digit, int64_t *result) {
    guard_is_not_null(result);

    int64_t shifted;
    if (__builtin_mul_overflow(int_value, 10, &shifted)) {
        return false;
    }

    int64_t signed_digit;
    if (__builtin_mul_overflow(sign(int_value), digit, &signed_digit)) {
        return false;
    }

    return false == __builtin_add_overflow(shifted, signed_digit, result);
}

static bool any_accept(Scanner *s, Position pos, int c, SyntaxError *error) {
    if (SINGLE_QUOTE == c) {
        transition(s, pos, SCANNER_QUOTE);
        return true;
    }

    if (isdigit(c)) {
        transition(s, pos, SCANNER_INT);
        s->_int_value = c - '0';
        return true;
    }

    if (is_name_char(c)) {
        transition(s, pos, SCANNER_ATOM);

        if (false == sb_try_printf(&s->_sb, "%c", (char) c)) {
            *error = (SyntaxError) {
                    .code = SYNTAX_ERROR_TOKEN_TOO_LONG,
                    .pos = {.lineno = pos.lineno, s->_token_pos.col, pos.end_col}
            };
            return false;
        }

        return true;
    }

    if (is_whitespace(c)) {
        transition(s, pos, SCANNER_WS);
        return true;
    }

    if (OPEN_PAREN == c) {
        transition(s, pos, SCANNER_OPEN_PAREN);
        return true;
    }

    if (CLOSE_PAREN == c) {
        transition(s, pos, SCANNER_CLOSE_PAREN);
        return true;
    }

    if (DOUBLE_QUOTE == c) {
        transition(s, pos, SCANNER_STRING);
        return true;
    }

    if (SEMICOLON == c) {
        transition(s, pos, SCANNER_COMMENT);
        return true;
    }

    *error = (SyntaxError) {
            .code = SYNTAX_ERROR_INVALID_CHARACTER,
            .pos = pos,
            .bad_chr = c
    };
    scanner_reset(s);
    return false;
}

static bool int_accept(Scanner *s, Position pos, int c, SyntaxError *error) {
    guard_is_equal(s->_state, SCANNER_INT);

    if (isdigit(c)) {
        auto const digit = c - '0';
        if (0 == s->_int_value) {
            *error = (SyntaxError) {
                    .code = SYNTAX_ERROR_INTEGER_LEADING_ZERO,
                    .pos = {.lineno = pos.lineno, s->_token_pos.col, pos.end_col},
                    .bad_chr = c
            };
            scanner_reset(s);
            return false;
        }

        s->_token_pos.end_col = pos.end_col;
        if (false == try_add_digit(s->_int_value, digit, &s->_int_value)) {
            *error = (SyntaxError) {
                    .code = SYNTAX_ERROR_INTEGER_TOO_LARGE,
                    .pos = {.lineno = pos.lineno, s->_token_pos.col, pos.end_col},
                    .bad_chr = c
            };
            scanner_reset(s);
            return false;
        }

        return true;
    }

    if (is_whitespace(c)) {
        transition(s, pos, SCANNER_WS);
        return true;
    }

    if (OPEN_PAREN == c) {
        transition(s, pos, SCANNER_OPEN_PAREN);
        return true;
    }

    if (CLOSE_PAREN == c) {
        transition(s, pos, SCANNER_CLOSE_PAREN);
        return true;
    }

    if (SEMICOLON == c) {
        transition(s, pos, SCANNER_COMMENT);
        return true;
    }

    *error = (SyntaxError) {
            .code = SYNTAX_ERROR_INTEGER_INVALID,
            .pos = {.lineno = pos.lineno, s->_token_pos.col, pos.end_col},
            .bad_chr = c
    };
    scanner_reset(s);
    return false;
}

static bool string_accept(Scanner *s, Position pos, int c, SyntaxError *error) {
    guard_is_equal(s->_state, SCANNER_STRING);

    if (s->_escape_sequence) {
        s->_escape_sequence = false;

        char represented_char;
        if (string_try_get_escape_seq_value((char) c, &represented_char)) {

            if (false == sb_try_printf(&s->_sb, "%c", represented_char)) {
                *error = (SyntaxError) {
                        .code = SYNTAX_ERROR_TOKEN_TOO_LONG,
                        .pos = {.lineno = pos.lineno, s->_token_pos.col, pos.end_col}
                };
                return false;
            }

            return true;
        }

        *error = (SyntaxError) {
                .code = SYNTAX_ERROR_STRING_UNKNOWN_ESCAPE_SEQUENCE,
                .pos = {.lineno = pos.lineno, .col = pos.col - 1, .end_col = pos.end_col},
                .bad_chr = c
        };
        scanner_reset(s);
        return false;
    }

    if (BACKSLASH == c) {
        s->_escape_sequence = true;
        return true;
    }

    if (DOUBLE_QUOTE == c) {
        s->_token_pos.end_col = pos.end_col;
        transition(s, pos, SCANNER_WS);
        return true;
    }

    if (is_printable(c)) {
        s->_token_pos.end_col = pos.end_col;

        if (false == sb_try_printf(&s->_sb, "%c", (char) c)) {
            *error = (SyntaxError) {
                    .code = SYNTAX_ERROR_TOKEN_TOO_LONG,
                    .pos = {.lineno = pos.lineno, s->_token_pos.col, pos.end_col}
            };
            return false;
        }

        return true;
    }

    if (is_newline(c)) {
        *error = (SyntaxError) {
                .code = SYNTAX_ERROR_STRING_UNTERMINATED,
                .pos = pos,
                .bad_chr = c
        };
        scanner_reset(s);
        return false;
    }

    if (SEMICOLON == c) {
        transition(s, pos, SCANNER_COMMENT);
        return true;
    }

    *error = (SyntaxError) {
            .code = SYNTAX_ERROR_INVALID_CHARACTER,
            .pos = pos,
            .bad_chr = c
    };
    scanner_reset(s);
    return false;
}

static bool name_accept(Scanner *s, Position pos, int c, SyntaxError *error) {
    guard_is_equal(s->_state, SCANNER_ATOM);

    if (isdigit(c) && 1 == s->_sb.length) {
        auto digit = c - '0';
        if (0 == digit) {
            *error = (SyntaxError) {
                    .code = SYNTAX_ERROR_INTEGER_LEADING_ZERO,
                    .pos = {.lineno = pos.lineno, .col = s->_token_pos.col, .end_col = pos.end_col},
                    .bad_chr = c
            };
            scanner_reset(s);
            return false;
        }

        switch (s->_sb.str[0]) {
            case '-': {
                digit = -digit;
                [[fallthrough]];
            }
            case '+': {
                s->_int_value = digit;
                s->_token_pos.end_col = pos.end_col;
                s->_state = SCANNER_INT;
                return true;
            }
        }
    }

    if (is_name_char(c)) {
        s->_token_pos.end_col = pos.end_col;

        if (false == sb_try_printf(&s->_sb, "%c", (char) c)) {
            *error = (SyntaxError) {
                    .code = SYNTAX_ERROR_TOKEN_TOO_LONG,
                    .pos = {.lineno = pos.lineno, s->_token_pos.col, pos.end_col}
            };
            return false;
        }

        return true;
    }

    if (is_whitespace(c)) {
        transition(s, pos, SCANNER_WS);
        return true;
    }

    if (OPEN_PAREN == c) {
        transition(s, pos, SCANNER_OPEN_PAREN);
        return true;
    }

    if (CLOSE_PAREN == c) {
        transition(s, pos, SCANNER_CLOSE_PAREN);
        return true;
    }

    if (SEMICOLON == c) {
        transition(s, pos, SCANNER_COMMENT);
        return true;
    }

    *error = (SyntaxError) {
            .code = SYNTAX_ERROR_INVALID_CHARACTER,
            .pos = pos,
            .bad_chr = c
    };
    scanner_reset(s);
    return false;
}

static bool comment_accept(Scanner *s, Position pos, int c, SyntaxError *error) {
    guard_is_equal(s->_state, SCANNER_COMMENT);

    if (is_newline(c)) {
        transition(s, pos, SCANNER_WS);
        return true;
    }

    if (is_printable(c)) {
        transition(s, pos, SCANNER_COMMENT);
        return true;
    }

    *error = (SyntaxError) {
            .code = SYNTAX_ERROR_INVALID_CHARACTER,
            .pos = pos,
            .bad_chr = c
    };
    scanner_reset(s);
    return false;
}

bool scanner_try_accept(Scanner *s, Position pos, int c, SyntaxError *error) {
    guard_is_not_null(s);
    guard_is_not_null(error);
    if (EOF != c) {
        guard_is_in_range(c, 0, UCHAR_MAX);
    }

    s->has_token = false;

    switch (s->_state) {
        case SCANNER_OPEN_PAREN:
        case SCANNER_CLOSE_PAREN:
        case SCANNER_WS: {
            return any_accept(s, pos, c, error);
        }
        case SCANNER_INT: {
            return int_accept(s, pos, c, error);
        }
        case SCANNER_STRING: {
            return string_accept(s, pos, c, error);
        }
        case SCANNER_ATOM: {
            return name_accept(s, pos, c, error);
        }
        case SCANNER_QUOTE: {
            return any_accept(s, pos, c, error);
        }
        case SCANNER_COMMENT: {
            return comment_accept(s, pos, c, error);
        }
    }

    guard_unreachable();
}

bool scanner_try_init(Scanner *s, Scanner_Config config, errno_t *error_code) {
    guard_is_not_null(s);
    guard_is_not_null(error_code);

    *s = (Scanner) {0};
    return sb_try_reserve(&s->_sb, config.max_token_length, error_code);
}

void scanner_free(Scanner *s) {
    guard_is_not_null(s);

    sb_free(&s->_sb);
    *s = (Scanner) {0};
}

void scanner_reset(Scanner *s) {
    clear(s);
    s->_state = SCANNER_WS;
}
