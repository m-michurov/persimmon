#include "parser.h"

#include "utility/guards.h"
#include "utility/slice.h"
#include "object/lists.h"
#include "object/constructors.h"

bool parser_try_init(Parser *p, ObjectAllocator *a, Parser_Config config, errno_t *error_code) {
    guard_is_not_null(p);
    guard_is_not_null(error_code);

    *p = (Parser) {
            .expr = object_nil(),
            ._a = a,
    };

    errno = 0;
    auto const exprs = calloc(config.max_nesting_depth, sizeof(Parser_Expression));
    if (nullptr == exprs) {
        *error_code = errno;
        return false;
    }

    p->exprs_stack = (Parser_ExpressionsStack) {
            .data = exprs,
            .capacity = config.max_nesting_depth
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

static bool try_unwind_quotes_(Parser *p, Parser_Error *error) {
    if (slice_empty(p->exprs_stack)) {
        return true;
    }

    while (slice_last(p->exprs_stack)->is_quote) {
        if (false == object_list_try_prepend(p->_a, p->expr, &slice_last(p->exprs_stack)->last)) {
            parser_allocation_error(error);
        }

        p->expr = slice_last(p->exprs_stack)->last;
        object_list_reverse(&p->expr);
        slice_try_pop(&p->exprs_stack, nullptr);

        if (slice_empty(p->exprs_stack)) {
            p->has_expr = true;
            return true;
        }
    }

    return true;
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

            parser_syntax_error(SYNTAX_ERROR_NO_MATCHING_CLOSE_PAREN, slice_last(p->exprs_stack)->begin, error);
        }
        case TOKEN_INT: {
            if (false == object_try_make_int(p->_a, token.as_int, &p->expr)) {
                parser_allocation_error(error);
            }

            if (slice_empty(p->exprs_stack)) {
                p->has_expr = true;
                return true;
            }

            if (false == try_unwind_quotes_(p, error)) {
                return false;
            }

            if (p->has_expr) {
                return true;
            }

            if (object_list_try_prepend(p->_a, p->expr, &slice_last(p->exprs_stack)->last)) {
                return true;
            }

            parser_allocation_error(error);
        }
        case TOKEN_STRING: {
            if (false == object_try_make_string(p->_a, token.as_string, &p->expr)) {
                parser_allocation_error(error);
            }

            if (slice_empty(p->exprs_stack)) {
                p->has_expr = true;
                return true;
            }

            if (false == try_unwind_quotes_(p, error)) {
                return false;
            }

            if (p->has_expr) {
                return true;
            }

            if (object_list_try_prepend(p->_a, p->expr, &slice_last(p->exprs_stack)->last)) {
                return true;
            }

            parser_allocation_error(error);
        }
        case TOKEN_ATOM: {
            if (false == object_try_make_atom(p->_a, token.as_atom, &p->expr)) {
                parser_allocation_error(error);
            }

            if (slice_empty(p->exprs_stack)) {
                p->has_expr = true;
                return true;
            }

            if (false == try_unwind_quotes_(p, error)) {
                return false;
            }

            if (p->has_expr) {
                return true;
            }

            if (object_list_try_prepend(p->_a, p->expr, &slice_last(p->exprs_stack)->last)) {
                return true;
            }

            parser_allocation_error(error);
        }
        case TOKEN_OPEN_PAREN: {
            if (slice_try_append(&p->exprs_stack, ((Parser_Expression) {.last = object_nil(), .begin = token.pos}))) {
                return true;
            }

            parser_syntax_error(SYNTAX_ERROR_NESTING_TOO_DEEP, token.pos, error);
        }
        case TOKEN_CLOSE_PAREN: {
            if (slice_empty(p->exprs_stack) || slice_last(p->exprs_stack)->is_quote) {
                parser_syntax_error(SYNTAX_ERROR_UNEXPECTED_CLOSE_PAREN, token.pos, error);
            }

            p->expr = slice_last(p->exprs_stack)->last;
            object_list_reverse(&p->expr);

            if (1 == p->exprs_stack.count) {
                slice_try_pop(&p->exprs_stack, nullptr);
                p->has_expr = true;
                return true;
            }

            slice_try_pop(&p->exprs_stack, nullptr);

            if (false == try_unwind_quotes_(p, error)) {
                return false;
            }

            if (p->has_expr) {
                return true;
            }

            if (object_list_try_prepend(p->_a, p->expr, &slice_last(p->exprs_stack)->last)) {
                return true;
            }

            parser_allocation_error(error);
        }
        case TOKEN_QUOTE: {
            auto const expression = ((Parser_Expression) {
                    .last = object_nil(),
                    .begin = token.pos,
                    .is_quote = true
            });

            if (false == slice_try_append(&p->exprs_stack, expression)) {
                parser_syntax_error(SYNTAX_ERROR_NESTING_TOO_DEEP, token.pos, error);
            }

            if (false == object_list_try_prepend(p->_a, object_nil(), &slice_last(p->exprs_stack)->last)) {
                parser_allocation_error(error);
            }

            if (false == object_try_make_atom(p->_a, "quote", &slice_last(p->exprs_stack)->last->as_cons.first)) {
                parser_allocation_error(error);
            }

            return true;
        }
    }

    guard_unreachable();
}
