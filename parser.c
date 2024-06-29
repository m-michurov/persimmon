#include "parser.h"

#include "arena_append.h"
#include "guards.h"
#include "object_lists.h"
#include "object_allocator.h"
#include "object_constructors.h"
#include "slice.h"

typedef struct {
    Object *expr;
    Position begin_pos;
} ExpressionStack_Item;

typedef struct {
    ExpressionStack_Item *data;
    size_t count;
    size_t capacity;
} ExpressionStack;

struct Parser {
    Arena *a;
    Object_Allocator *allocator;
    ExpressionStack exprs;
    bool has_expr;
    Object *expr;
};

Parser *parser_new(Arena *a, Object_Allocator *allocator) {
    guard_is_not_null(a);
    guard_is_not_null(allocator);

    return arena_emplace(a, ((Parser) {
            .a = a,
            .allocator = allocator,
            .exprs = (ExpressionStack) {0}
    }));
}

void parser_reset(Parser *p) {
    p->exprs.count = 0;
    p->has_expr = false;
}

bool parser_is_inside_expression(Parser const *p) {
    return p->has_expr || p->exprs.count > 0;
}

bool parser_try_accept(Parser *p, Token token, SyntaxError *error) {
    guard_is_not_null(p);
    guard_is_not_null(error);

    switch (token.type) {
        case TOKEN_EOF: {
            if (slice_empty(p->exprs)) {
                return true;
            }

            *error = (SyntaxError) {
                    .code = SYNTAX_ERROR_NO_MATCHING_CLOSE_PAREN,
                    .pos = slice_last(p->exprs)->begin_pos
            };
            return false;
        }
        case TOKEN_INT: {
            auto const expr = object_int(p->allocator, token.as_int);
            if (slice_empty(p->exprs)) {
                p->expr = expr;
                p->has_expr = true;
                return true;
            }

            slice_last(p->exprs)->expr = object_cons(p->allocator, expr, slice_last(p->exprs)->expr);
            return true;
        }
        case TOKEN_STRING: {
            auto const expr = object_string(p->allocator, token.as_string);
            if (slice_empty(p->exprs)) {
                p->expr = expr;
                p->has_expr = true;
                return true;
            }

            slice_last(p->exprs)->expr = object_cons(p->allocator, expr, slice_last(p->exprs)->expr);
            return true;
        }
        case TOKEN_ATOM: {
            auto const expr = object_atom(p->allocator, token.as_atom);
            if (slice_empty(p->exprs)) {
                p->expr = expr;
                p->has_expr = true;
                return true;
            }

            slice_last(p->exprs)->expr = object_cons(p->allocator, expr, slice_last(p->exprs)->expr);
            return true;
        }
        case TOKEN_OPEN_PAREN: {
            arena_append(p->a, &p->exprs, ((ExpressionStack_Item) {
                    .expr = object_nil(),
                    .begin_pos = token.pos
            }));
            return true;
        }
        case TOKEN_CLOSE_PAREN: {
            if (slice_empty(p->exprs)) {
                *error = (SyntaxError) {
                        .code = SYNTAX_ERROR_UNEXPECTED_CLOSE_PAREN,
                        .pos = token.pos,
                };
                return false;
            }

            if (1 == p->exprs.count) {
                p->expr = slice_last(p->exprs)->expr;
                slice_try_pop(&p->exprs, nullptr);
                object_list_reverse(&p->expr);
                p->has_expr = true;
                return true;
            }

            auto expr = slice_last(p->exprs)->expr;
            slice_try_pop(&p->exprs, nullptr);
            object_list_reverse(&expr);
            slice_last(p->exprs)->expr = object_cons(p->allocator, expr, slice_last(p->exprs)->expr);
            return true;
        }
    }

    guard_unreachable();
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
