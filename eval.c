#include "eval.h"

#include <stdio.h>
#include <string.h>

#include "object_lists.h"
#include "object_accessors.h"
#include "object_constructors.h"
#include "object_env.h"
#include "guards.h"
#include "stack.h"
#include "reader.h"
#include "slice.h"

//#define EVAL_TRACE

static void eval_push_result(Object_Allocator *a, Object **result, Object *value) {
    if (nullptr == result) {
        return;
    }
    guard_is_not_null(*result);
    guard_is_not_null(value);

    *result = object_cons(a, value, *result);
}

static bool try_begin_eval(
        Object_Allocator *a,
        Stack *stack,
        Object *env,
        Object *expr,
        Object **values_list,
        Object **error
) {
    guard_is_not_null(a);
    guard_is_not_null(stack);
    guard_is_not_null(env);
    guard_is_not_null(expr);
    guard_is_not_null(error);

#ifdef EVAL_TRACE
    printf("[depth = %zu] evaluating %s\n", stack->count, object_repr(a, expr));
#endif

    switch (expr->type) {
        case TYPE_INT:
        case TYPE_STRING:
        case TYPE_CLOSURE:
        case TYPE_PRIMITIVE:
        case TYPE_NIL: {
            eval_push_result(a, values_list, expr);
            return true;
        }
        case TYPE_ATOM: {
            Object *value;
            if (false == env_try_find(env, expr, &value)) {
                *error = object_list(a, object_atom(a, "NameError"), expr);
                return false;
            }

            eval_push_result(a, values_list, value);
            return true;
        }
        case TYPE_CONS: {
            if (TYPE_ATOM == expr->as_cons.first->type) {
                auto const atom_name = expr->as_cons.first->as_atom;

                if (0 == strcmp("if", atom_name)) {
                    auto const len = object_list_count(expr->as_cons.rest);
                    if (2 != len && 3 != len) {
                        *error = object_list(
                                a,
                                object_atom(a, "IfSyntaxError"),
                                object_list(a, object_atom(a, "expected-args"), object_int(a, 2), object_int(a, 3)),
                                object_list(a, object_atom(a, "got-args"), object_int(a, len))
                        );
                        return false;
                    }

                    auto const no_overflow = stack_try_push(
                            stack,
                            frame_new(FRAME_IF, env, values_list, object_as_cons(expr).rest)
                    );
                    if (false == no_overflow) {
                        *error = object_list(a, object_atom(a, "StackOverflowError"));
                        return false;
                    }
                    return true;
                }

                if (0 == strcmp("do", atom_name)) {
                    auto const no_overflow = stack_try_push(
                            stack,
                            frame_new(FRAME_DO, env, values_list, object_as_cons(expr).rest)
                    );
                    if (false == no_overflow) {
                        *error = object_list(a, object_atom(a, "StackOverflowError"));
                        return false;
                    }
                    return true;
                }

                if (0 == strcmp("define", atom_name)) {
                    auto const no_overflow = stack_try_push(
                            stack,
                            frame_new(FRAME_DEFINE, env, values_list, object_as_cons(expr).rest)
                    );
                    if (false == no_overflow) {
                        *error = object_list(a, object_atom(a, "StackOverflowError"));
                        return false;
                    }
                    return true;
                }

                if (0 == strcmp("fn", atom_name)) {
                    auto const len = object_list_count(expr->as_cons.rest);
                    if (len < 2) {
                        *error = object_list(
                                a,
                                object_atom(a, "FnSyntaxError"),
                                object_list(a, object_atom(a, "expected-args"), object_int(a, 2), object_atom(a, "+")),
                                object_list(a, object_atom(a, "got-args"), object_int(a, len))
                        );
                        return false;
                    }

                    auto const args = object_list_nth(expr, 1);
                    object_list_for(it, args) {
                        if (TYPE_ATOM == it->type) {
                            continue;
                        }
                        *error = object_list(
                                a,
                                object_atom(a, "TypeError"),
                                object_list(a, object_atom(a, "in"), expr),
                                it,
                                object_list(a, object_atom(a, "expected"), object_atom(a, object_type_str(TYPE_ATOM))),
                                object_list(a, object_atom(a, "got"), object_atom(a, object_type_str(it->type)))
                        );
                        return false;
                    }

                    auto const body = object_list_skip(expr, 2);
                    eval_push_result(a, values_list, object_closure(a, env, args, body));
                    return true;
                }

                if (0 == strcmp("import", atom_name)) {
                    auto const len = object_list_count(expr->as_cons.rest);
                    if (len != 1) {
                        *error = object_list(
                                a,
                                object_atom(a, "ImportSyntaxError"),
                                object_list(a, object_atom(a, "expected-args"), object_int(a, 1)),
                                object_list(a, object_atom(a, "got-args"), object_int(a, len))
                        );
                        return false;
                    }

                    auto const file_name = object_list_nth(expr, 1);
                    if (TYPE_STRING != file_name->type) {
                        *error = object_list(
                                a,
                                object_atom(a, "TypeError"),
                                object_atom(a, "import"),
                                object_list(
                                        a,
                                        object_atom(a, "expected"),
                                        object_atom(a, object_type_str(TYPE_STRING))
                                ),
                                object_list(
                                        a,
                                        object_atom(a, "got"),
                                        object_atom(a, object_type_str(file_name->type))
                                )
                        );
                        return false;
                    }

                    auto const handle = fopen(file_name->as_string, "rb");
                    if (nullptr == handle) {
                        auto const saved_errno = errno;
                        *error = object_list(
                                a,
                                object_atom(a, "IOError"),
                                file_name,
                                object_string(a, strerror(saved_errno))
                        );
                        return false;
                    }

                    auto arena = &(Arena) {0};
                    auto r = reader_open(
                            arena,
                            (NamedFile) {.handle = handle, .name = file_name->as_string},
                            a
                    );
                    auto exprs = (Objects) {0};
                    ReaderError reader_error;
                    if (false == reader_try_read_all(arena, r, &exprs, &reader_error)) {
                        *error = object_list(
                                a,
                                object_atom(a, "ImportSyntaxError"),
                                object_list(a, object_atom(a, "line"), object_int(a, reader_error.base.pos.lineno)),
                                object_atom(a, syntax_error_code_desc(reader_error.base.code))
                        );
                        arena_free(arena);
                        fclose(handle);
                        return false;
                    }

                    auto exprs_list = object_nil();
                    slice_for(it, exprs) {
                        exprs_list = object_cons(a, *it, exprs_list);
                    }
                    object_list_reverse(&exprs_list);

                    auto const no_overflow = stack_try_push(
                            stack,
                            frame_new(FRAME_DO, env, values_list, exprs_list)
                    );
                    if (false == no_overflow) {
                        *error = object_list(a, object_atom(a, "StackOverflowError"));
                        arena_free(arena);
                        fclose(handle);
                        return false;
                    }
                    arena_free(arena);
                    fclose(handle);
                    return true;
                }
            }
            auto const no_overflow = stack_try_push(
                    stack,
                    frame_new(FRAME_CALL, env, values_list, expr)
            );
            if (false == no_overflow) {
                *error = object_list(a, object_atom(a, "StackOverflowError"));
                return false;
            }
            return true;
        }
        case TYPE_MOVED: {
            guard_unreachable();
        }
    }

    guard_unreachable();
}

static bool try_step(Object_Allocator *a, Stack *stack, Object **error) {
    guard_is_not_null(a);
    guard_is_not_null(stack);
    guard_is_greater(stack->count, 0);
    guard_is_not_null(error);

    auto const frame = stack_top(stack);
    switch (frame->type) {
        case FRAME_CALL: {
            if (object_nil() != frame->unevaluated) {
                auto const next = object_as_cons(frame->unevaluated).first;
                frame->unevaluated = object_as_cons(frame->unevaluated).rest;
                return try_begin_eval(a, stack, frame->env, next, &frame->evaluated, error);
            }

            object_list_reverse(&frame->evaluated);
            guard_is_not_equal(frame->evaluated, object_nil());

            auto const fn = object_as_cons(frame->evaluated).first;
            auto args = object_as_cons(frame->evaluated).rest;
            if (TYPE_PRIMITIVE == fn->type) {
                Object *value;
                if (false == fn->as_primitive(a, args, &value, error)) {
                    return false;
                }
                eval_push_result(a, frame->result, value);
                stack_pop(stack);
                return true;
            }

            if (TYPE_CLOSURE != fn->type) {
                *error = object_list(
                        a,
                        object_atom(a, "TypeError"),
                        fn,
                        object_list(a, object_atom(a, "expected"), object_atom(a, object_type_str(TYPE_CLOSURE))),
                        object_list(a, object_atom(a, "got"), object_atom(a, object_type_str(fn->type)))
                );
                return false;
            }
            auto const formal_args = fn->as_closure.args;
            auto const actual_argc = object_list_count(args);
            auto const formal_argc = object_list_count(formal_args);
            if (actual_argc != formal_argc) {
                *error = object_list(
                        a,
                        object_atom(a, "TypeError"),
                        fn,
                        object_list(a, object_atom(a, "expected-args"), object_int(a, formal_argc)),
                        object_list(a, object_atom(a, "got-args"), object_int(a, actual_argc))
                );
                return false;
            }

            auto const arg_bindings = env_new(a, fn->as_closure.env); // NOLINT(*-sizeof-expression)
            object_list_for(formal, formal_args) {
                auto const actual = object_as_cons(args).first;
                env_define(a, arg_bindings, formal, actual);
                args = object_as_cons(args).rest;
            }

            stack_pop(stack);
            auto const next = object_cons(a, object_atom(a, "do"), fn->as_closure.body);
            return try_begin_eval(a, stack, arg_bindings, next, frame->result, error);
        }
        case FRAME_IF: {
            if (object_nil() == frame->evaluated) {
                auto const next = object_as_cons(frame->unevaluated).first;
                frame->unevaluated = object_as_cons(frame->unevaluated).rest;
                return try_begin_eval(a, stack, frame->env, next, &frame->evaluated, error);
            }

            auto const cond_value = object_as_cons(frame->evaluated).first;

            stack_pop(stack);
            if (object_nil() != cond_value) {
                return try_begin_eval(a, stack, frame->env, object_as_cons(frame->unevaluated).first, frame->result,
                                      error);
            }

            frame->unevaluated = object_as_cons(frame->unevaluated).rest; // skip `then`
            if (object_nil() == frame->unevaluated) {
                eval_push_result(a, frame->result, object_nil());
                return true;
            }

            return try_begin_eval(a, stack, frame->env, object_as_cons(frame->unevaluated).first, frame->result, error);
        }
        case FRAME_DO: {
            if (object_nil() == frame->unevaluated) {
                eval_push_result(a, frame->result, object_nil());
                stack_pop(stack);
                return true;
            }

            if (object_nil() == object_as_cons(frame->unevaluated).rest) {
                stack_pop(stack);
                return try_begin_eval(a, stack, frame->env, frame->unevaluated->as_cons.first, frame->result, error);
            }

            auto const next = object_as_cons(frame->unevaluated).first;
            frame->unevaluated = object_as_cons(frame->unevaluated).rest;
            return try_begin_eval(a, stack, frame->env, next, nullptr, error);
        }
        case FRAME_DEFINE: {
            if (object_nil() == frame->evaluated) {
                return try_begin_eval(a, stack, frame->env, object_list_nth(frame->unevaluated, 1), &frame->evaluated,
                                      error);
            }

            env_define(a, frame->env, object_as_cons(frame->unevaluated).first, object_as_cons(frame->evaluated).first);
            eval_push_result(a, frame->result, object_as_cons(frame->evaluated).first);
            stack_pop(stack);
            return true;
        }
    }

    guard_unreachable();
}

bool try_eval(
        Object_Allocator *allocator,
        Stack *stack,
        Object *env,
        Object *expression,
        Object **value,
        Object **error
) {
    guard_is_not_null(allocator);
    guard_is_not_null(stack);
    guard_is_not_null(env);
    guard_is_not_null(expression);
    guard_is_not_null(value);
    guard_is_not_null(error);

    *error = object_nil();
    Object *result = object_nil();
    if (false == try_begin_eval(allocator, stack, env, expression, &result, error)) {
        return false;
    }

    while (stack->count > 0) {
        if (try_step(allocator, stack, error)) {
            continue;
        }

        while (stack->count > 0) { stack_pop(stack); }
        return false;
    }
    guard_is_equal(stack->count, 0);

    *value = object_as_cons(result).first;
    return true;
}
