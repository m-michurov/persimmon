#include "scanner.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "exchange.h"
#include "sign.h"
#include "guards.h"
#include "string_builder.h"
#include "strings.h"

typedef enum {
    TOKENIZER_WS,
    TOKENIZER_INT,
    TOKENIZER_STRING,
    TOKENIZER_ATOM,
    TOKENIZER_OPEN_PAREN,
    TOKENIZER_CLOSE_PAREN,
} State;

struct Scanner {
    State state;
    StringBuilder *sb;
    bool escape_sequence;
    int64_t int_value;
    Position token_pos;
    bool has_token;
    Token token;
};

char const *token_type_str(Token_Type type) {
    switch (type) {
        case TOKEN_EOF: {
            return "TOKEN_EOF";
        }
        case TOKEN_INT: {
            return "TOKEN_INT";
        }
        case TOKEN_STRING: {
            return "TOKEN_STRING";
        }
        case TOKEN_ATOM: {
            return "TOKEN_ATOM";
        }
        case TOKEN_OPEN_PAREN: {
            return "TOKEN_OPEN_PAREN";
        }
        case TOKEN_CLOSE_PAREN: {
            return "TOKEN_CLOSE_PAREN";
        }
    }

    guard_unreachable();
}

static bool is_name_char(int c) {
    return isalnum(c) || ('\0' != c && nullptr != strchr("~!@#$%^&*_+-=./<>?", c));
}

static bool is_whitespace(int c) {
    return isspace(c) || ',' == c || EOF == c;
}

static void tokenizer_clear(Scanner *t) {
    sb_clear(t->sb);
    t->escape_sequence = false;

    t->int_value = 0;
}

static void tokenizer_transition(Scanner *t, Position pos, State new_state) {
    auto const state = exchange(t->state, new_state);
    auto const token_pos = exchange(t->token_pos, pos);

    switch (state) {
        case TOKENIZER_WS: {
            tokenizer_clear(t);
            return;
        }
        case TOKENIZER_INT: {
            t->has_token = true;
            t->token = (Token) {
                    .type = TOKEN_INT,
                    .pos = token_pos,
                    .as_int = t->int_value
            };
            tokenizer_clear(t);
            return;
        }
        case TOKENIZER_STRING: {
            t->has_token = true;
            t->token = (Token) {
                    .type = TOKEN_STRING,
                    .pos = token_pos,
                    .as_string = sb_str(t->sb)
            };
            tokenizer_clear(t);
            return;
        }
        case TOKENIZER_ATOM: {
            guard_is_greater(sb_length(t->sb), 0);

            t->has_token = true;
            t->token = (Token) {
                    .type = TOKEN_ATOM,
                    .pos = token_pos,
                    .as_atom = sb_str(t->sb)
            };
            tokenizer_clear(t);
            return;
        }
        case TOKENIZER_OPEN_PAREN: {
            t->has_token = true;
            t->token = (Token) {.type = TOKEN_OPEN_PAREN, .pos = token_pos};
            tokenizer_clear(t);
            return;
        }
        case TOKENIZER_CLOSE_PAREN: {
            t->has_token = true;
            t->token = (Token) {.type = TOKEN_CLOSE_PAREN, .pos = token_pos};
            tokenizer_clear(t);
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

static bool tokenizer_any_accept(Scanner *t, Position pos, int c, SyntaxError *error) {
    if (isdigit(c)) {
        tokenizer_transition(t, pos, TOKENIZER_INT);
        t->int_value = c - '0';
        return true;
    }

    if (is_name_char(c)) {
        tokenizer_transition(t, pos, TOKENIZER_ATOM);
        sb_append_char(t->sb, (char) c);
        return true;
    }

    if (is_whitespace(c)) {
        tokenizer_transition(t, pos, TOKENIZER_WS);
        return true;
    }

    if ('(' == c) {
        tokenizer_transition(t, pos, TOKENIZER_OPEN_PAREN);
        return true;
    }

    if (')' == c) {
        tokenizer_transition(t, pos, TOKENIZER_CLOSE_PAREN);
        return true;
    }

    if ('"' == c) {
        tokenizer_transition(t, pos, TOKENIZER_STRING);
        return true;
    }

    *error = (SyntaxError) {
            .code = SYNTAX_ERROR_INVALID_CHARACTER,
            .pos = pos,
            .bad_chr = c
    };
    scanner_reset(t);
    return false;
}

static bool tokenizer_int_accept(Scanner *t, Position pos, int c, SyntaxError *error) {
    guard_is_equal(t->state, TOKENIZER_INT);

    if (isdigit(c)) {
        auto const digit = c - '0';
        if (0 == t->int_value) {
            *error = (SyntaxError) {
                    .code = SYNTAX_ERROR_INTEGER_LEADING_ZERO,
                    .pos = {.lineno = pos.lineno, t->token_pos.col, pos.end_col},
                    .bad_chr = c
            };
            scanner_reset(t);
            return false;
        }

        t->token_pos.end_col = pos.end_col;
        if (false == try_add_digit(t->int_value, digit, &t->int_value)) {
            *error = (SyntaxError) {
                    .code = SYNTAX_ERROR_INTEGER_TOO_LARGE,
                    .pos = {.lineno = pos.lineno, t->token_pos.col, pos.end_col},
                    .bad_chr = c
            };
            scanner_reset(t);
            return false;
        }

        return true;
    }

    if (is_whitespace(c)) {
        tokenizer_transition(t, pos, TOKENIZER_WS);
        return true;
    }

    if ('(' == c) {
        tokenizer_transition(t, pos, TOKENIZER_OPEN_PAREN);
        return true;
    }

    if (')' == c) {
        tokenizer_transition(t, pos, TOKENIZER_CLOSE_PAREN);
        return true;
    }

    *error = (SyntaxError) {
            .code = SYNTAX_ERROR_INTEGER_INVALID,
            .pos = {.lineno = pos.lineno, t->token_pos.col, pos.end_col},
            .bad_chr = c
    };
    scanner_reset(t);
    return false;
}

static bool tokenizer_string_accept(Scanner *t, Position pos, int c, SyntaxError *error) {
    guard_is_equal(t->state, TOKENIZER_STRING);

    if (t->escape_sequence) {
        t->escape_sequence = false;

        char represented_char;
        if (string_try_get_escape_seq_value((char) c, &represented_char)) {
            sb_append_char(t->sb, represented_char);
            return true;
        }

        *error = (SyntaxError) {
                .code = SYNTAX_ERROR_STRING_UNKNOWN_ESCAPE_SEQUENCE,
                .pos = {.lineno = pos.lineno, .col = pos.col - 1, .end_col = pos.end_col},
                .bad_chr = c
        };
        scanner_reset(t);
        return false;
    }

    if ('\\' == c) {
        t->escape_sequence = true;
        return true;
    }

    if ('"' == c) {
        t->token_pos.end_col = pos.end_col;
        tokenizer_transition(t, pos, TOKENIZER_WS);
        return true;
    }

    if (isprint(c) || ' ' == c) {
        t->token_pos.end_col = pos.end_col;
        sb_append_char(t->sb, (char) c);
        return true;
    }

    if ('\n' == c || '\r' == c) {
        *error = (SyntaxError) {
                .code = SYNTAX_ERROR_STRING_UNTERMINATED,
                .pos = pos,
                .bad_chr = c
        };
        scanner_reset(t);
        return false;
    }

    *error = (SyntaxError) {
            .code = SYNTAX_ERROR_INVALID_CHARACTER,
            .pos = pos,
            .bad_chr = c
    };
    scanner_reset(t);
    return false;
}

static bool tokenizer_name_accept(Scanner *t, Position pos, int c, SyntaxError *error) {
    guard_is_equal(t->state, TOKENIZER_ATOM);

    if (isdigit(c) && 1 == sb_length(t->sb)) {
        auto digit = c - '0';
        if (0 == digit) {
            *error = (SyntaxError) {
                    .code = SYNTAX_ERROR_INTEGER_LEADING_ZERO,
                    .pos = {.lineno = pos.lineno, .col = t->token_pos.col, .end_col = pos.end_col},
                    .bad_chr = c
            };
            scanner_reset(t);
            return false;
        }

        switch (sb_str(t->sb)[0]) {
            case '-': {
                digit = -digit;
                [[fallthrough]];
            }
            case '+': {
                t->int_value = digit;
                t->token_pos.end_col = pos.end_col;
                t->state = TOKENIZER_INT;
                return true;
            }
        }
    }

    if (is_name_char(c)) {
        t->token_pos.end_col = pos.end_col;
        sb_append_char(t->sb, (char) c);
        return true;
    }

    if (is_whitespace(c)) {
        tokenizer_transition(t, pos, TOKENIZER_WS);
        return true;
    }

    if ('(' == c) {
        tokenizer_transition(t, pos, TOKENIZER_OPEN_PAREN);
        return true;
    }

    if (')' == c) {
        tokenizer_transition(t, pos, TOKENIZER_CLOSE_PAREN);
        return true;
    }

    *error = (SyntaxError) {
            .code = SYNTAX_ERROR_INVALID_CHARACTER,
            .pos = pos,
            .bad_chr = c
    };
    scanner_reset(t);
    return false;
}

bool scanner_try_accept(Scanner *s, Position pos, int c, SyntaxError *error) {
    guard_is_not_null(s);
    guard_is_not_null(error);
    if (EOF != c) {
        guard_is_in_range(c, 0, UCHAR_MAX);
    }

    s->has_token = false;

    switch (s->state) {
        case TOKENIZER_OPEN_PAREN:
        case TOKENIZER_CLOSE_PAREN:
        case TOKENIZER_WS: {
            return tokenizer_any_accept(s, pos, c, error);
        }
        case TOKENIZER_INT: {
            return tokenizer_int_accept(s, pos, c, error);
        }
        case TOKENIZER_STRING: {
            return tokenizer_string_accept(s, pos, c, error);
        }
        case TOKENIZER_ATOM: {
            return tokenizer_name_accept(s, pos, c, error);
        }
    }

    guard_unreachable();
}

Scanner *scanner_new(void) {
    auto const s = (Scanner *) guard_succeeds(calloc, (1, sizeof(Scanner)));
    *s = (Scanner) {.sb = sb_new()};
    return s;
}

void scanner_free(Scanner **s) {
    guard_is_not_null(s);
    guard_is_not_null(*s);

    sb_free(&(*s)->sb);
    free(*s);
    *s = nullptr;
}

void scanner_reset(Scanner *s) {
    tokenizer_clear(s);
    s->state = TOKENIZER_WS;
}

Token const *scanner_peek(Scanner const *s) {
    guard_is_not_null(s);

    if (false == s->has_token) {
        return nullptr;
    }

    return &s->token;
}
