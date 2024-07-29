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
    return p.has_expr || false == slice_empty(p.exprs_stack);
}

static bool try_unwind_quotes(Parser *p, Parser_Result *result) {
    if (slice_empty(p->exprs_stack)) {
        return false;
    }

    while (slice_last(p->exprs_stack)->is_quote) {
        if (false == object_list_try_prepend(p->_a, p->expr, &slice_last(p->exprs_stack)->last)) {
            *result = PARSER_ALLOCATION_ERROR;
            return true;
        }

        p->expr = slice_last(p->exprs_stack)->last;
        object_list_reverse(&p->expr);
        slice_try_pop(&p->exprs_stack, nullptr);

        if (slice_empty(p->exprs_stack)) {
            p->has_expr = true;
            *result = PARSER_OK;
            return true;
        }
    }

    return false;
}

Parser_Result parser_try_accept(Parser *p, Token token, SyntaxError *syntax_error) {
    guard_is_not_null(p);
    guard_is_not_null(syntax_error);

    p->has_expr = false;

    switch (token.type) {
        case TOKEN_EOF: {
            if (slice_empty(p->exprs_stack)) {
                return PARSER_OK;
            }

            *syntax_error = (SyntaxError) {
                    .code = SYNTAX_ERROR_NO_MATCHING_CLOSE_PAREN,
                    .pos = slice_last(p->exprs_stack)->begin
            };
            return PARSER_SYNTAX_ERROR;
        }
        case TOKEN_INT: {
            if (false == object_try_make_int(p->_a, token.as_int, &p->expr)) {
                return PARSER_ALLOCATION_ERROR;
            }

            if (slice_empty(p->exprs_stack)) {
                p->has_expr = true;
                return PARSER_OK;
            }

            Parser_Result result;
            if (try_unwind_quotes(p, &result)) {
                return result;
            }

            return object_list_try_prepend(p->_a, p->expr, &slice_last(p->exprs_stack)->last)
                   ? PARSER_OK
                   : PARSER_ALLOCATION_ERROR;
        }
        case TOKEN_STRING: {
            if (false == object_try_make_string(p->_a, token.as_string, &p->expr)) {
                return PARSER_ALLOCATION_ERROR;
            }

            if (slice_empty(p->exprs_stack)) {
                p->has_expr = true;
                return PARSER_OK;
            }

            Parser_Result result;
            if (try_unwind_quotes(p, &result)) {
                return result;
            }

            return object_list_try_prepend(p->_a, p->expr, &slice_last(p->exprs_stack)->last)
                   ? PARSER_OK
                   : PARSER_ALLOCATION_ERROR;
        }
        case TOKEN_ATOM: {
            if (false == object_try_make_atom(p->_a, token.as_atom, &p->expr)) {
                return PARSER_ALLOCATION_ERROR;
            }

            if (slice_empty(p->exprs_stack)) {
                p->has_expr = true;
                return PARSER_OK;
            }

            Parser_Result result;
            if (try_unwind_quotes(p, &result)) {
                return result;
            }

            return object_list_try_prepend(p->_a, p->expr, &slice_last(p->exprs_stack)->last)
                   ? PARSER_OK
                   : PARSER_ALLOCATION_ERROR;
        }
        case TOKEN_OPEN_PAREN: {
            if (slice_try_append(&p->exprs_stack, ((Parser_Expression) {.last = object_nil(), .begin = token.pos}))) {
                return PARSER_OK;
            }

            *syntax_error = (SyntaxError) {
                    .code = SYNTAX_ERROR_NESTING_TOO_DEEP,
                    .pos = token.pos,
            };
            return PARSER_SYNTAX_ERROR;
        }
        case TOKEN_CLOSE_PAREN: {
            if (slice_empty(p->exprs_stack) || slice_last(p->exprs_stack)->is_quote) {
                *syntax_error = (SyntaxError) {
                        .code = SYNTAX_ERROR_UNEXPECTED_CLOSE_PAREN,
                        .pos = token.pos,
                };
                return PARSER_SYNTAX_ERROR;
            }

            p->expr = slice_last(p->exprs_stack)->last;
            object_list_reverse(&p->expr);

            if (1 == p->exprs_stack.count) {
                slice_try_pop(&p->exprs_stack, nullptr);
                p->has_expr = true;
                return PARSER_OK;
            }

            slice_try_pop(&p->exprs_stack, nullptr);

            Parser_Result result;
            if (try_unwind_quotes(p, &result)) {
                return result;
            }

            return object_list_try_prepend(p->_a, p->expr, &slice_last(p->exprs_stack)->last)
                   ? PARSER_OK
                   : PARSER_ALLOCATION_ERROR;
        }
        case TOKEN_QUOTE: {
            auto const expression = ((Parser_Expression) {
                    .last = object_nil(),
                    .begin = token.pos,
                    .is_quote = true
            });

            if (false == slice_try_append(&p->exprs_stack, expression)) {
                *syntax_error = (SyntaxError) {
                        .code = SYNTAX_ERROR_NESTING_TOO_DEEP,
                        .pos = token.pos,
                };
                return PARSER_SYNTAX_ERROR;
            }

            if (false == object_list_try_prepend(p->_a, object_nil(), &slice_last(p->exprs_stack)->last)) {
                return PARSER_ALLOCATION_ERROR;
            }

            if (false == object_try_make_atom(p->_a, "quote", &slice_last(p->exprs_stack)->last->as_cons.first)) {
                return PARSER_ALLOCATION_ERROR;
            }

            return PARSER_OK;
        }
    }

    guard_unreachable();
}
