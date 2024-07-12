#include "parser.h"

#include "utility/guards.h"
#include "utility/slice.h"
#include "object/lists.h"
#include "object/constructors.h"

struct Parser {
    ObjectAllocator *a;
    Parser_ExpressionsStack stack;
    bool has_expr;
    Object *expr;
};

Parser *parser_new(ObjectAllocator *a, Parser_Config config) {
    guard_is_not_null(a);
    guard_is_greater(config.max_nesting_depth, 0);

    auto const p = (Parser *) guard_succeeds(calloc, (1, sizeof(Parser)));
    memcpy(p, &((Parser) {
            .a = a,
            .stack = {
                    .data = guard_succeeds(calloc, (config.max_nesting_depth, sizeof(Parser_Expression))),
                    .capacity = config.max_nesting_depth
            },
            .expr = object_nil()
    }), sizeof(Parser));
    return p;
}

void parser_free(Parser **p) {
    guard_is_not_null(p);
    guard_is_not_null(*p);

    free((void *) (*p)->stack.data);
    free(*p);
    *p = nullptr;
}

void parser_reset(Parser *p) {
    slice_clear(&p->stack);
    p->has_expr = false;
}

Parser_ExpressionsStack const *parser_stack(Parser const *p) {
    return &p->stack;
}

bool parser_is_inside_expression(Parser const *p) {
    return p->has_expr || false == slice_empty(p->stack);
}

// FIXME use checked constructors
// TODO find a way to report OOM
Parser_Result parser_try_accept(Parser *p, Token token, SyntaxError *syntax_error) {
    guard_is_not_null(p);
    guard_is_not_null(syntax_error);

    switch (token.type) {
        case TOKEN_EOF: {
            if (slice_empty(p->stack)) {
                return PARSER_OK;
            }

            *syntax_error = (SyntaxError) {
                    .code = SYNTAX_ERROR_NO_MATCHING_CLOSE_PAREN,
                    .pos = slice_last(p->stack)->begin
            };
            return PARSER_SYNTAX_ERROR;
        }
        case TOKEN_INT: {
            if (false == object_try_make_int(p->a, token.as_int, &p->expr)) {
                return PARSER_ALLOCATION_ERROR;
            }

            if (slice_empty(p->stack)) {
                p->has_expr = true;
                return PARSER_OK;
            }

            if (slice_last(p->stack)->is_quote) {
                if (false == object_list_try_prepend(p->a, p->expr, &slice_last(p->stack)->last)) {
                    return PARSER_ALLOCATION_ERROR;
                }

                p->expr = slice_last(p->stack)->last;
                object_list_reverse(&p->expr);
                slice_try_pop(&p->stack, nullptr);

                if (slice_empty(p->stack)) {
                    p->has_expr = true;
                    return PARSER_OK;
                }
            }

            return object_list_try_prepend(p->a, p->expr, &slice_last(p->stack)->last)
                   ? PARSER_OK
                   : PARSER_ALLOCATION_ERROR;
        }
        case TOKEN_STRING: {
            if (false == object_try_make_string(p->a, token.as_string, &p->expr)) {
                return PARSER_ALLOCATION_ERROR;
            }

            if (slice_empty(p->stack)) {
                p->has_expr = true;
                return PARSER_OK;
            }

            if (slice_last(p->stack)->is_quote) {
                if (false == object_list_try_prepend(p->a, p->expr, &slice_last(p->stack)->last)) {
                    return PARSER_ALLOCATION_ERROR;
                }

                p->expr = slice_last(p->stack)->last;
                object_list_reverse(&p->expr);
                slice_try_pop(&p->stack, nullptr);

                if (slice_empty(p->stack)) {
                    p->has_expr = true;
                    return PARSER_OK;
                }
            }

            return object_list_try_prepend(p->a, p->expr, &slice_last(p->stack)->last)
                   ? PARSER_OK
                   : PARSER_ALLOCATION_ERROR;
        }
        case TOKEN_ATOM: {
            if (false == object_try_make_atom(p->a, token.as_atom, &p->expr)) {
                return PARSER_ALLOCATION_ERROR;
            }

            if (slice_empty(p->stack)) {
                p->has_expr = true;
                return PARSER_OK;
            }

            if (slice_last(p->stack)->is_quote) {
                if (false == object_list_try_prepend(p->a, p->expr, &slice_last(p->stack)->last)) {
                    return PARSER_ALLOCATION_ERROR;
                }

                p->expr = slice_last(p->stack)->last;
                object_list_reverse(&p->expr);
                slice_try_pop(&p->stack, nullptr);

                if (slice_empty(p->stack)) {
                    p->has_expr = true;
                    return PARSER_OK;
                }
            }

            return object_list_try_prepend(p->a, p->expr, &slice_last(p->stack)->last)
                   ? PARSER_OK
                   : PARSER_ALLOCATION_ERROR;
        }
        case TOKEN_OPEN_PAREN: {
            if (slice_try_append(&p->stack, ((Parser_Expression) {.last = object_nil(), .begin = token.pos}))) {
                return PARSER_OK;
            }

            *syntax_error = (SyntaxError) {
                    .code = SYNTAX_ERROR_NESTING_TOO_DEEP,
                    .pos = token.pos,
            };
            return PARSER_SYNTAX_ERROR;
        }
        case TOKEN_CLOSE_PAREN: {
            if (slice_empty(p->stack) || slice_last(p->stack)->is_quote) {
                *syntax_error = (SyntaxError) {
                        .code = SYNTAX_ERROR_UNEXPECTED_CLOSE_PAREN,
                        .pos = token.pos,
                };
                return PARSER_SYNTAX_ERROR;
            }

            if (slice_last(p->stack)->is_quote) {
                if (false == object_list_try_prepend(p->a, p->expr, &slice_last(p->stack)->last)) {
                    return PARSER_ALLOCATION_ERROR;
                }

                p->expr = slice_last(p->stack)->last;
                object_list_reverse(&p->expr);
                slice_try_pop(&p->stack, nullptr);

                if (slice_empty(p->stack)) {
                    p->has_expr = true;
                    return PARSER_OK;
                }
            }

            p->expr = slice_last(p->stack)->last;
            object_list_reverse(&p->expr);

            if (1 == p->stack.count) {
                slice_try_pop(&p->stack, nullptr);
                p->has_expr = true;
                return PARSER_OK;
            }

            slice_try_pop(&p->stack, nullptr);

            if (false == slice_empty(p->stack) && slice_last(p->stack)->is_quote) {
                if (false == object_list_try_prepend(p->a, p->expr, &slice_last(p->stack)->last)) {
                    return PARSER_ALLOCATION_ERROR;
                }

                p->expr = slice_last(p->stack)->last;
                object_list_reverse(&p->expr);
                slice_try_pop(&p->stack, nullptr);

                if (slice_empty(p->stack)) {
                    p->has_expr = true;
                    return PARSER_OK;
                }
            }

            return object_list_try_prepend(p->a, p->expr, &slice_last(p->stack)->last)
                   ? PARSER_OK
                   : PARSER_ALLOCATION_ERROR;
        }
        case TOKEN_QUOTE: {
            auto const expression = ((Parser_Expression) {
                    .last = object_nil(),
                    .begin = token.pos,
                    .is_quote = true
            });

            if (false == slice_try_append(&p->stack, expression)) {
                *syntax_error = (SyntaxError) {
                        .code = SYNTAX_ERROR_NESTING_TOO_DEEP,
                        .pos = token.pos,
                };
                return PARSER_SYNTAX_ERROR;
            }

            if (false == object_list_try_prepend(p->a, object_nil(), &slice_last(p->stack)->last)) {
                return PARSER_ALLOCATION_ERROR;
            }

            if (false == object_try_make_atom(p->a, "quote", &slice_last(p->stack)->last->as_cons.first)) {
                return PARSER_ALLOCATION_ERROR;
            }

            return PARSER_OK;
        }
    }

    guard_unreachable();
}

Object *const *parser_peek(Parser const *p) {
    guard_is_not_null(p);

    return &p->expr;
}

bool parser_try_get_expression(Parser *p, Object **expression) {
    guard_is_not_null(p);
    guard_is_not_null(expression);

    if (false == p->has_expr) {
        return false;
    }

    *expression = p->expr;
    p->has_expr = false;
    return true;
}
