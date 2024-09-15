#include "parser.h"

#include <stdio.h>

#include "utility/guards.h"
#include "utility/slice.h"
#include "object/list.h"
#include "object/constructors.h"

static auto const SYMBOL_GET = &(Object) {.type = TYPE_SYMBOL, .as_symbol = "get"};
static auto const SYMBOL_QUOTE = &(Object) {.type = TYPE_SYMBOL, .as_symbol = "quote"};

bool parser_try_init(Parser *p, ObjectAllocator *a, Parser_Config config, errno_t *error_code) {
    guard_is_not_null(p);
    guard_is_not_null(a);
    guard_is_not_null(error_code);

    errno = 0;
    auto const exprs = calloc(config.max_nesting_depth, sizeof(Parser_Expression));
    if (nullptr == exprs) {
        *error_code = errno;
        return false;
    }

    *p = (Parser) {
            .expr = OBJECT_NIL,
            ._a = a,
            .exprs_stack = (Parser_ExpressionsStack) {
                    .data = exprs,
                    .capacity = config.max_nesting_depth
            }
    };

    return true;
}

void parser_free(Parser *p) {
    guard_is_not_null(p);

    free(p->exprs_stack.data);
    *p = (Parser) {0};
}

void parser_reset(Parser *p) {
    slice_clear(&p->exprs_stack);
    p->has_expr = false;
}

bool parser_is_inside_expression(Parser p) {
    return false == slice_empty(p.exprs_stack);
}

#define parser_syntax_error(ErrorCode, Pos, Error)  \
do {                                                \
    *(Error) = ((Parser_Error) {                    \
        .type = PARSER_SYNTAX_ERROR,                \
        .as_syntax_error = (SyntaxError) {          \
            .code = (ErrorCode),                    \
            .pos = (Pos)                            \
        }                                           \
    });                                             \
    return false;                                   \
} while (false)

#define parser_allocation_error(Error)      \
do {                                        \
    *(Error) = ((Parser_Error) {            \
        .type = PARSER_ALLOCATION_ERROR     \
    });                                     \
    return false;                           \
} while (false)

static bool try_complete_dot(Parser *p, Parser_Error *error) {
    guard_is_not_null(p);
    guard_is_not_null(error);
    guard_is_equal(slice_last(&p->exprs_stack)->type, PARSER_DOT);

    auto const last = slice_last(&p->exprs_stack)->last;
    auto const quoted_key = object_list_nth_mutable(1, last);

    if (false == object_try_make_list_of(p->_a, quoted_key, SYMBOL_QUOTE, p->expr)) {
        parser_allocation_error(error);
    }

    p->expr = slice_last(&p->exprs_stack)->last;
    object_list_reverse_inplace(&p->expr);
    slice_try_pop(&p->exprs_stack, nullptr);

    if (slice_empty(p->exprs_stack)) {
        p->has_expr = true;
    }

    return true;
}

static bool try_unwind_quotes_(Parser *p, Parser_Error *error) {
    if (slice_empty(p->exprs_stack)) {
        return true;
    }

    while (PARSER_QUOTE == slice_last(&p->exprs_stack)->type) {
        if (false == object_list_try_prepend(p->_a, p->expr, &slice_last(&p->exprs_stack)->last)) {
            parser_allocation_error(error);
        }

        p->expr = slice_last(&p->exprs_stack)->last;
        object_list_reverse_inplace(&p->expr);
        slice_try_pop(&p->exprs_stack, nullptr);

        if (slice_empty(p->exprs_stack)) {
            p->has_expr = true;
            return true;
        }
    }

    return true;
}

static bool try_make_dot(Parser *p, Position pos, Parser_Error *error) {
    guard_is_not_null(p);
    guard_is_not_null(error);

    if (false == slice_try_append(&p->exprs_stack, ((Parser_Expression) {
            .last = OBJECT_NIL,
            .begin = pos,
            .type = PARSER_DOT
    }))) {
        parser_syntax_error(SYNTAX_ERROR_NESTING_TOO_DEEP, pos, error);
    }

    auto const dot_expr = &slice_last(&p->exprs_stack)->last;
    auto const ok = object_list_try_prepend(p->_a, SYMBOL_GET, dot_expr)
                    && object_list_try_prepend(p->_a, OBJECT_NIL, dot_expr)
                    && object_list_try_prepend(p->_a, p->expr, dot_expr);
    if (false == ok) {
        parser_allocation_error(error);
    }

    return true;
}

static bool try_allocate_object(Parser *p, Token token) {
    guard_is_not_null(p);

    switch (token.type) {
        case TOKEN_EOF: {
            guard_unreachable();
        }
        case TOKEN_INT: {
            return object_try_make_int(p->_a, token.as_int, &p->expr);
        }
        case TOKEN_STRING: {
            return object_try_make_string(p->_a, token.as_string, &p->expr);
        }
        case TOKEN_SYMBOL: {
            return object_try_make_symbol(p->_a, token.as_symbol, &p->expr);
        }
        case TOKEN_OPEN_PAREN: {
            guard_unreachable();
        }
        case TOKEN_CLOSE_PAREN: {
            guard_unreachable();
        }
        case TOKEN_QUOTE: {
            guard_unreachable();
        }
    }

    guard_unreachable();
}

bool parser_try_accept(Parser *p, Token token, Parser_Error *error) {
    guard_is_not_null(p);
    guard_is_not_null(error);

    p->has_expr = false;

    switch (token.type) {
        case TOKEN_EOF: {
            if (slice_empty(p->exprs_stack)) {
                return true;
            }

            parser_syntax_error(SYNTAX_ERROR_NO_MATCHING_CLOSE_PAREN, slice_last(&p->exprs_stack)->begin, error);
        }
        case TOKEN_INT:
        case TOKEN_STRING:
        case TOKEN_SYMBOL: {
            if (false == try_allocate_object(p, token)) {
                parser_allocation_error(error);
            }

            if (slice_empty(p->exprs_stack) && false == token.next_dot) {
                p->has_expr = true;
                return true;
            }

            if (false == slice_empty(p->exprs_stack) && PARSER_DOT == slice_last(&p->exprs_stack)->type) {
                if (false == try_complete_dot(p, error)) {
                    return false;
                }
            }

            if (token.next_dot) {
                return try_make_dot(p, token.pos, error);
            }

            if (false == try_unwind_quotes_(p, error)) {
                return false;
            }

            if (p->has_expr) {
                return true;
            }

            if (object_list_try_prepend(p->_a, p->expr, &slice_last(&p->exprs_stack)->last)) {
                return true;
            }

            parser_allocation_error(error);
        }
        case TOKEN_OPEN_PAREN: {
            if (slice_try_append(&p->exprs_stack, ((Parser_Expression) {.last = OBJECT_NIL, .begin = token.pos}))) {
                return true;
            }

            parser_syntax_error(SYNTAX_ERROR_NESTING_TOO_DEEP, token.pos, error);
        }
        case TOKEN_CLOSE_PAREN: {
            if (slice_empty(p->exprs_stack) || PARSER_QUOTE == slice_last(&p->exprs_stack)->type) {
                parser_syntax_error(SYNTAX_ERROR_UNEXPECTED_CLOSE_PAREN, token.pos, error);
            }

            p->expr = slice_last(&p->exprs_stack)->last;
            object_list_reverse_inplace(&p->expr);
            slice_try_pop(&p->exprs_stack, nullptr);

            if (slice_empty(p->exprs_stack) && false == token.next_dot) {
                p->has_expr = true;
                return true;
            }

            if (false == slice_empty(p->exprs_stack) && PARSER_DOT == slice_last(&p->exprs_stack)->type) {
                if (false == try_complete_dot(p, error)) {
                    return false;
                }
            }

            if (token.next_dot) {
                return try_make_dot(p, token.pos, error);
            }

            if (false == try_unwind_quotes_(p, error)) {
                return false;
            }

            if (p->has_expr) {
                return true;
            }

            if (object_list_try_prepend(p->_a, p->expr, &slice_last(&p->exprs_stack)->last)) {
                return true;
            }

            parser_allocation_error(error);
        }
        case TOKEN_QUOTE: {
            if (false == slice_try_append(&p->exprs_stack, ((Parser_Expression) {
                    .last = OBJECT_NIL,
                    .begin = token.pos,
                    .type = PARSER_QUOTE
            }))) {
                parser_syntax_error(SYNTAX_ERROR_NESTING_TOO_DEEP, token.pos, error);
            }

            if (false == object_try_make_list_of(p->_a, &slice_last(&p->exprs_stack)->last, SYMBOL_QUOTE)) {
                parser_allocation_error(error);
            }

            return true;
        }
    }

    guard_unreachable();
}
