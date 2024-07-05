#include "parser.h"

#include "arena_append.h"
#include "guards.h"
#include "object_lists.h"
#include "object_allocator.h"
#include "object_constructors.h"
#include "slice.h"
#include "dynamic_array.h"

struct Parser {
    ObjectAllocator *a;
    Parser_Stack stack;
    bool has_expr;
    Object *expr;
};

Parser *parser_new(ObjectAllocator *a) {
    guard_is_not_null(a);

    auto const p = (Parser *) guard_succeeds(calloc, (1, sizeof(Parser)));
    *p = (Parser) {.a = a};
    return p;
}

void parser_free(Parser **p) {
    guard_is_not_null(p);
    guard_is_not_null(*p);

    da_free(&(*p)->stack);
    free(*p);
    *p = nullptr;
}

void parser_reset(Parser *p) {
    slice_clear(&p->stack);
    p->has_expr = false;
}

Parser_Stack const *parser_stack(Parser const *p) {
    return &p->stack;
}

bool parser_is_inside_expression(Parser const *p) {
    return p->has_expr || false == slice_empty(p->stack);
}

bool parser_try_accept(Parser *p, Token token, SyntaxError *error) {
    guard_is_not_null(p);
    guard_is_not_null(error);

    switch (token.type) {
        case TOKEN_EOF: {
            if (slice_empty(p->stack)) {
                return true;
            }

            *error = (SyntaxError) {
                    .code = SYNTAX_ERROR_NO_MATCHING_CLOSE_PAREN,
                    .pos = slice_last(p->stack)->begin
            };
            return false;
        }
        case TOKEN_INT: {
            auto const expr = object_int(p->a, token.as_int);
            if (slice_empty(p->stack)) {
                p->expr = expr;
                p->has_expr = true;
                return true;
            }

            slice_last(p->stack)->last = object_cons(p->a, expr, slice_last(p->stack)->last);
            return true;
        }
        case TOKEN_STRING: {
            auto const expr = object_string(p->a, token.as_string);
            if (slice_empty(p->stack)) {
                p->expr = expr;
                p->has_expr = true;
                return true;
            }

            slice_last(p->stack)->last = object_cons(p->a, expr, slice_last(p->stack)->last);
            return true;
        }
        case TOKEN_ATOM: {
            auto const expr = object_atom(p->a, token.as_atom);
            if (slice_empty(p->stack)) {
                p->expr = expr;
                p->has_expr = true;
                return true;
            }

            slice_last(p->stack)->last = object_cons(p->a, expr, slice_last(p->stack)->last);
            return true;
        }
        case TOKEN_OPEN_PAREN: {
            da_append(&p->stack, ((Parser_Expression) {.last = object_nil(), .begin = token.pos}));
            return true;
        }
        case TOKEN_CLOSE_PAREN: {
            if (slice_empty(p->stack)) {
                *error = (SyntaxError) {
                        .code = SYNTAX_ERROR_UNEXPECTED_CLOSE_PAREN,
                        .pos = token.pos,
                };
                return false;
            }

            if (1 == p->stack.count) {
                p->expr = slice_last(p->stack)->last;
                slice_try_pop(&p->stack, nullptr);
                object_list_reverse(&p->expr);
                p->has_expr = true;
                return true;
            }

            auto expr = slice_last(p->stack)->last;
            slice_try_pop(&p->stack, nullptr);
            object_list_reverse(&expr);
            slice_last(p->stack)->last = object_cons(p->a, expr, slice_last(p->stack)->last);
            return true;
        }
    }

    guard_unreachable();
}

Object *const *parser_expression(Parser const *p) {
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
