#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#include "arena.h"
#include "arena_append.h"
#include "tokenizer.h"
#include "object.h"
#include "object_allocator.h"
#include "object_constructors.h"
#include "object_lists.h"
#include "eval.h"
#include "strings.h"
#include "guards.h"
#include "parser.h"
#include "primitives.h"
#include "stack.h"
#include "object_env.h"
#include "object_arena_allocator.h"

bool try_parse(Arena *arena, Object_Allocator *allocator, Tokens tokens, Objects *exprs) {
    *exprs = (Objects) {0};

    auto const p = parser_new(arena, allocator);
    for (auto it = tokens.data; it != tokens.data + tokens.count; it++) {
        Parser_Error error;
        if (false == parser_try_accept(p, *it, &error)) {
            printf(
                    "Syntax error at %zu..%zu: %s\n",
                    error.pos.first, error.pos.last, syntax_error_str(error.error_code)
            );
            return false;
        }

        Object *expr;
        if (parser_try_get_expression(p, &expr)) {
            arena_append(arena, exprs, expr); // NOLINT(*-sizeof-expression)
        }
    }

    return true;
}

[[maybe_unused]]

static int parse_demo(Arena *arena, Object_Allocator *allocator) {
    auto const code_str =
            "(define apply (fn (func col)\n    (if col (do (func (first col)) (apply func (rest col))))))"
            "\n(define print-squares (fn (col) (apply (fn (x) (print (* x x))) col)))"
            "\n(print-squares (list 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25))"
            "\n(define adder (fn (n) (fn (x) (+ x n))))"
            "\n(define plus-3 (adder 3))"
            "\n(print (plus-3 39))"
            "\n(first nil)"
            "\n(define nil (list))"
            "\n(rest 42)"
            "\n(first (list 1) (list 2))"
            "\n(first (rest (list 1)))"
            "\n(+ 1 \"hello\")"
            "\n(fn)"
            "\n(fn ())"
            "\n(fn () 1)"
            "\n(fn (a b \"s\") 1)";
    printf("code:\n%s\n", code_str);

    auto const t = tokenizer_new(arena);
    auto tokens = (Tokens) {0};
    int c = '\0';
    for (auto it = code_str; EOF != c; it++) {
        c = '\0' == *it ? EOF : *it;
        Tokenizer_Error error;
        if (false == tokenizer_try_accept(t, it - code_str, c, &error)) {
            printf(
                    "Syntax error at %zu..%zu: %s - '%c' (%d)\n",
                    error.pos.first, error.pos.last, syntax_error_str(error.error_code),
                    error.chr, error.chr
            );
            continue;
        }

        Token token;
        if (tokenizer_try_get_token(t, &token)) {
            arena_append(arena, &tokens, token);
        }
    }
    arena_append(arena, &tokens, ((Token) {.type = TOKEN_EOF}));

    Objects exprs;
    if (false == try_parse(arena, allocator, tokens, &exprs)) {
        return EXIT_FAILURE;
    }

    auto env = env_new(allocator, object_nil());
    define_primitives(allocator, env);

    auto stack = stack_new(arena, 7);

    for (auto it = exprs.data; it != exprs.data + exprs.count; it++) {
        printf("code: %s\n", object_repr(arena, *it));

        Object *value, *error;
        if (false == try_eval(allocator, stack, env, *it, &value, &error)) {
            printf("ERROR: %s\n", object_repr(arena, error));
        }
        guard_is_equal(stack->count, 0);
    }

    printf("env: %s\n", object_repr(arena, env));

    return EXIT_SUCCESS;
}

[[maybe_unused]]

static void tokenize(Arena *arena, FILE *f) {
    guard_is_not_null(f);

    auto t = tokenizer_new(arena);
    auto tokens = (Tokens) {0};
    size_t pos = 0;

    for (auto eof = false; false == eof;) {
        auto const c = fgetc(f);
        eof = EOF == c;

        Tokenizer_Error error;
        if (false == tokenizer_try_accept(t, pos++, c, &error)) {
            printf(
                    "Syntax error at %zu..%zu: %s - '%c' (%d)\n",
                    error.pos.first, error.pos.last, syntax_error_str(error.error_code),
                    error.chr, error.chr
            );
            continue;
        }

        Token token;
        if (tokenizer_try_get_token(t, &token)) {
            arena_append(arena, &tokens, token);
            continue;
        }
    }
    fclose(f);

    for (auto it = tokens.data; it != tokens.data + tokens.count; it++) {
        switch (it->type) {
            case TOKEN_EOF: {
                guard_unreachable();
            }
            case TOKEN_INT: {
                printf("[int, %" PRId64 ", %zu..%zu]\n", it->as_int, it->pos.first, it->pos.last);
                break;
            }
            case TOKEN_STRING: {
                auto const begin = it->as_string;
                auto const end = it->as_string + strlen(it->as_string);

                printf("[string, \"");
                for (auto c = begin; c < end; c++) {
                    char const *escape_sequence;
                    if (string_try_repr_escape_seq(*c, &escape_sequence)) {
                        printf("%s", escape_sequence);
                    } else {
                        printf("%c", *c);
                    }
                }
                printf("\", %zu..%zu]\n", it->pos.first, it->pos.last);
                break;
            }
            case TOKEN_ATOM: {
                printf("[name, %s, %zu..%zu]\n", it->as_atom, it->pos.first, it->pos.last);
                break;
            }
            case TOKEN_OPEN_PAREN: {
                printf("['(', %zu..%zu]\n", it->pos.first, it->pos.last);
                break;
            }
            case TOKEN_CLOSE_PAREN: {
                printf("[')', %zu..%zu]\n", it->pos.first, it->pos.last);
                break;
            }
        }
    }

    auto stats = arena_statistics(arena);
    printf("  regions: %zu\n", stats.regions);
    printf("   system: %zu bytes\n", stats.memory_used_bytes);
    printf("allocated: %zu bytes\n", stats.allocated_bytes);
    printf("   wasted: %zu bytes\n", stats.wasted_bytes);
}

[[maybe_unused]]

static int tokenize_demo(Arena *arena) {
    auto const path = "in.pmn";
    auto const in = fopen(path, "rb");
    if (nullptr == in) {
        perror(path);
        return EXIT_FAILURE;
    }

    tokenize(arena, in);

    fclose(in);

    return EXIT_SUCCESS;
}

[[maybe_unused]]

static int list_for_demo(Arena *arena, Object_Allocator *allocator) {
    (void) arena;

    auto list1 = object_cons(
            allocator,
            object_int(allocator, 42),
            object_cons(
                    allocator,
                    object_string(allocator, "hello"),
                    object_cons(
                            allocator,
                            object_atom(allocator, "++"),
                            object_nil()
                    )));
    auto list3 = object_nil();

    printf("list ending with nil\n");
    printf("%s\n", object_repr(arena, list1));
    object_list_for(it, list1) {
        printf("%s\n", object_repr(arena, it));
    }
    object_list_reverse(&list1);
    printf("reversed: ");
    printf("%s\n", object_repr(arena, list1));

    printf("empty list\n");
    printf("%s\n", object_repr(arena, list3));
    object_list_for(it, list3) {
        printf("%s\n", object_repr(arena, it));
    }
    object_list_reverse(&list3);
    printf("reversed: ");
    printf("%s\n", object_repr(arena, list3));
    return EXIT_SUCCESS;
}

[[maybe_unused]]

static int eval_demo(Arena *arena, Object_Allocator *allocator) {
    auto env = env_new(allocator, object_nil());
    define_primitives(allocator, env);

    env_define(allocator, env, object_atom(allocator, "x"), object_int(allocator, 42));
    env_define(allocator, env, object_atom(allocator, "yy"), object_int(allocator, -7));
    env_define(allocator, env, object_atom(allocator, "x"), object_int(allocator, 42));
    env_define(allocator, env, object_atom(allocator, "s1"), object_string(allocator, "hello"));
    printf("env: %s\n", object_repr(arena, env));

    auto const expr = object_list(
            allocator,
            object_atom(allocator, "print"),
            object_list(
                    allocator,
                    object_atom(allocator, "if"),
                    object_atom(allocator, "x"),
                    object_list(
                            allocator,
                            object_atom(allocator, "+"),
                            object_atom(allocator, "x"),
                            object_atom(allocator, "x"),
                            object_atom(allocator, "yy")
                    )
            ),
            object_list(allocator, object_atom(allocator, "define"), object_atom(allocator, "nil"), object_nil()),
            object_list(
                    allocator,
                    object_atom(allocator, "if"),
                    object_atom(allocator, "nil"),
                    object_list(
                            allocator,
                            object_atom(allocator, "+"),
                            object_atom(allocator, "x"),
                            object_atom(allocator, "yy")
                    ),
                    object_list(
                            allocator,
                            object_atom(allocator, "define"),
                            object_atom(allocator, "z"),
                            object_list(
                                    allocator,
                                    object_atom(allocator, "+"),
                                    object_atom(allocator, "x"),
                                    object_int(allocator, 3)
                            )
                    )
            ),
            object_list(
                    allocator,
                    object_atom(allocator, "if"),
                    object_atom(allocator, "nil"),
                    object_int(allocator, 1),
                    object_list(
                            allocator,
                            object_atom(allocator, "do"),
                            object_list(allocator, object_atom(allocator, "print"), object_string(allocator, "z"),
                                        object_atom(allocator, "z")),
                            object_list(allocator, object_atom(allocator, "print"), object_int(allocator, 11)),
                            object_atom(allocator, "s1")
                    )
            )
    );

    auto stack = stack_new(arena, 10);

    printf("code: %s\n", object_repr(arena, expr));

    Object *value, *error;
    if (try_eval(allocator, stack, env, expr, &value, &error)) {
        printf("%s\n", object_repr(arena, value));
    } else {
        printf("error: %s\n", object_repr(arena, error));
    }
    printf("env: %s\n", object_repr(arena, env));

    return EXIT_SUCCESS;
}

[[maybe_unused]]

static int fn_demo(Arena *arena, Object_Allocator *allocator) {
    auto env = env_new(allocator, object_nil());
    define_primitives(allocator, env);

    printf("env: %s\n", object_repr(arena, env));

    auto const expr = object_list(
            allocator,
            object_atom(allocator, "do"),
            object_list(allocator, object_atom(allocator, "define"), object_atom(allocator, "x"),
                        object_int(allocator, 3)),
            object_list(allocator, object_atom(allocator, "define"), object_atom(allocator, "y"),
                        object_int(allocator, 4)),
            object_list(
                    allocator,
                    object_atom(allocator, "print"),
                    object_list(
                            allocator,
                            object_atom(allocator, "+"),
                            object_atom(allocator, "x"),
                            object_atom(allocator, "y"),
                            object_int(allocator, 10)
                    )
            ),
            object_list(
                    allocator,
                    object_atom(allocator, "define"),
                    object_atom(allocator, "sum-plus-10"),
                    object_list(
                            allocator,
                            object_atom(allocator, "fn"),
                            object_list(allocator, object_atom(allocator, "x"), object_atom(allocator, "y")),
                            object_list(allocator, object_atom(allocator, "print"), object_string(allocator, "x"),
                                        object_atom(allocator, "x")),
                            object_list(allocator, object_atom(allocator, "print"), object_string(allocator, "y"),
                                        object_atom(allocator, "y")),
                            object_list(
                                    allocator,
                                    object_atom(allocator, "+"),
                                    object_atom(allocator, "x"),
                                    object_atom(allocator, "y"),
                                    object_int(allocator, 10)
                            )
                    )
            ),
            object_list(
                    allocator,
                    object_atom(allocator, "print"),
                    object_list(
                            allocator,
                            object_atom(allocator, "sum-plus-10"),
                            object_int(allocator, 1),
                            object_int(allocator, 2)
                    )
            )
    );

    auto stack = stack_new(arena, 10);

    printf("> %s\n", object_repr(arena, expr));

    Object *value, *error;
    if (try_eval(allocator, stack, env, expr, &value, &error)) {
        printf("%s\n", object_repr(arena, value));
    } else {
        printf("error: %s\n", object_repr(arena, error));
    }
    printf("env: %s\n", object_repr(arena, env));

    return EXIT_SUCCESS;
}

[[maybe_unused]]

static int do_tco_demo(Arena *arena, Object_Allocator *allocator) {
    auto env = env_new(allocator, object_nil());
    define_primitives(allocator, env);

    printf("env: %s\n", object_repr(arena, env));

    auto nums = object_nil();
    for (int i = 30; i >= 0; i--) {
        nums = object_cons(allocator, object_int(allocator, i), nums);
    }

    auto const expr = object_list(
            allocator,
            object_atom(allocator, "do"),
            object_list(
                    allocator,
                    object_atom(allocator, "define"),
                    object_atom(allocator, "print-list"),
                    object_list(
                            allocator,
                            object_atom(allocator, "fn"),
                            object_list(allocator, object_atom(allocator, "head")),
                            object_list(allocator, object_atom(allocator, "print"),
                                        object_atom(allocator, "head")),
                            object_list(
                                    allocator,
                                    object_atom(allocator, "if"),
                                    object_atom(allocator, "head"),
                                    object_list(
                                            allocator,
                                            object_atom(allocator, "print-list"),
                                            object_list(allocator, object_atom(allocator, "rest"),
                                                        object_atom(allocator, "head"))
                                    )
                            )
                    )
            ),
            object_list(
                    allocator,
                    object_atom(allocator, "print-list"),
                    object_cons(allocator, object_atom(allocator, "list"), nums)
            )
    );

    auto stack = stack_new(arena, 10);

    printf("> %s\n", object_repr(arena, expr));

    Object *value, *error;
    if (try_eval(allocator, stack, env, expr, &value, &error)) {
        printf("%s\n", object_repr(arena, value));
    } else {
        printf("error: %s\n", object_repr(arena, error));
    }
    printf("env: %s\n", object_repr(arena, env));

    return EXIT_SUCCESS;
}

#define run(Fn, Args)               \
({                                  \
    printf("running '" #Fn "':\n"); \
    auto const _r = Fn Args;        \
    printf("finished '" #Fn "'\n"); \
    _r;                             \
})

int main(void) {
    auto arena = &(Arena) {0};
    auto allocator = object_arena_allocator_new(arena);

    auto const exit_code = run(parse_demo, (arena, allocator));
//    auto const exit_code = run(tokenize_demo, (a, allocator));
//    auto const exit_code = run(list_for_demo, (a, allocator));
//    auto const exit_code = run(eval_demo, (a, allocator));
//    auto const exit_code = run(fn_demo, (a, allocator));
//    auto const exit_code = run(do_tco_demo, (a, allocator));

    auto stats = arena_statistics(arena);
    printf("  regions: %zu\n", stats.regions);
    printf("   system: %zu bytes\n", stats.memory_used_bytes);
    printf("allocated: %zu bytes\n", stats.allocated_bytes);
    printf("   wasted: %zu bytes\n", stats.wasted_bytes);

    object_allocator_free(&allocator);
    arena_free(arena);

    return exit_code;
}
