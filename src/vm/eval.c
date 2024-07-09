#include "eval.h"

#include <stdio.h>
#include <string.h>

#include "object/lists.h"
#include "object/accessors.h"
#include "object/constructors.h"
#include "object/env.h"
#include "utility/slice.h"
#include "utility/dynamic_array.h"
#include "utility/guards.h"
#include "reader/reader.h"
#include "stack.h"
#include "eval_errors.h"

typedef enum : bool {
    EVAL_FRAME_KEEP,
    EVAL_FRAME_REMOVE
} EvalFrameKeepOrRemove;

//#define EVAL_TRACE

static bool try_prepend(ObjectAllocator *a, Object **result, Object *value) {
    if (nullptr == result) {
        return true;
    }
    guard_is_not_null(*result);
    guard_is_not_null(value);

    if (object_try_make_cons(a, value, *result, result)) {
        return true;
    }

    print_out_of_memory_error(a);
    return false;
}

static bool try_pop_with(Stack *s, ObjectAllocator *a, Object *value) {
    guard_is_not_null(s);
    guard_is_not_null(a);
    guard_is_not_null(value);

    auto const frame = stack_top(s);
    if (nullptr == frame->results_list) {
        stack_pop(s);
        return true;
    }

    if (try_prepend(a, frame->results_list, value)) {
        stack_pop(s);
        return true;
    }

    print_out_of_memory_error(a);
    return false;
}

static bool try_get_special_type(Object *expr, Stack_FrameType *type) {
    if (TYPE_ATOM != object_as_cons(expr).first->type) {
        return false;
    }

    auto const atom_name = expr->as_cons.first->as_atom;

    if (0 == strcmp("if", atom_name)) {
        *type = FRAME_IF;
        return true;
    }

    if (0 == strcmp("do", atom_name)) {
        *type = FRAME_DO;
        return true;
    }

    if (0 == strcmp("define", atom_name)) {
        *type = FRAME_DEFINE;
        return true;
    }

    if (0 == strcmp("fn", atom_name)) {
        *type = FRAME_FN;
        return true;
    }

    if (0 == strcmp("import", atom_name)) {
        *type = FRAME_IMPORT;
        return true;
    }

    return false;
}

static bool try_begin_eval(
        VirtualMachine *vm,
        EvalFrameKeepOrRemove current,
        Object *env,
        Object *expr,
        Object **values_list
) {
    guard_is_not_null(vm);
    guard_is_not_null(env);
    guard_is_not_null(expr);

#ifdef EVAL_TRACE
    printf("[depth = %zu] evaluating %s\n", stack->count, object_repr(allocator, expr));
#endif

    auto const a = vm_allocator(vm);
    auto const s = vm_stack(vm);

    switch (expr->type) {
        case TYPE_INT:
        case TYPE_STRING:
        case TYPE_CLOSURE:
        case TYPE_PRIMITIVE:
        case TYPE_NIL: {
            if (EVAL_FRAME_REMOVE == current) {
                return try_pop_with(s, a, expr);
            }

            return try_prepend(a, values_list, expr);
        }
        case TYPE_ATOM: {
            Object *value;
            if (false == env_try_find(env, expr, &value)) {
                print_name_error(object_as_atom(expr));
                return false;
            }

            if (EVAL_FRAME_REMOVE == current) {
                return try_pop_with(s, a, value);
            }

            return try_prepend(a, values_list, value);
        }
        case TYPE_CONS: {
            Stack_FrameType type;
            auto const frame =
                    try_get_special_type(expr, &type)
                    ? frame_make(
                            type,
                            expr, env, values_list,
                            object_as_cons(expr).rest
                    )
                    : frame_make(
                            FRAME_CALL,
                            expr, env, values_list,
                            expr
                    );

            if (EVAL_FRAME_REMOVE == current) {
                stack_swap_top(s, frame);
                return true;
            }

            if (stack_try_push_frame(s, frame)) {
                return true;
            }

            print_stack_overflow_error();
            return false;
        }
    }

    guard_unreachable();
}

static bool try_step(VirtualMachine *vm) {
    guard_is_not_null(vm);

    auto const s = vm_stack(vm);
    auto const a = vm_allocator(vm);
    auto const frame = stack_top(s);

    switch (frame->type) {
        case FRAME_CALL: {
            if (object_nil() != frame->unevaluated) {
                auto const next = object_as_cons(frame->unevaluated).first;
                frame->unevaluated = object_as_cons(frame->unevaluated).rest;
                return try_begin_eval(vm, EVAL_FRAME_KEEP, frame->env, next, &frame->evaluated);
            }

            object_list_reverse(&frame->evaluated);
            guard_is_not_equal(frame->evaluated, object_nil());

            auto const fn = object_as_cons(frame->evaluated).first;
            auto args = object_as_cons(frame->evaluated).rest;
            if (TYPE_PRIMITIVE == fn->type) {
                Object *value;
                if (false == fn->as_primitive(a, args, &value)) {
                    return false;
                }
                return try_pop_with(s, a, value);
            }

            if (TYPE_CLOSURE != fn->type) {
                print_type_error(fn->type, TYPE_CLOSURE);
                return false;
            }
            auto const formal_args = fn->as_closure.args;
            auto const actual_argc = object_list_count(args);
            auto const formal_argc = object_list_count(formal_args);
            if (actual_argc != formal_argc) {
                print_args_error("<closure>", actual_argc, formal_argc);
                return false;
            }

            Object *arg_bindings;
            if (false == env_try_create(a, fn->as_closure.env, &arg_bindings)) {
                print_out_of_memory_error(a);
                return false;
            }
            object_list_for(formal, formal_args) {
                auto const actual = object_as_cons(args).first;
                env_try_define(a, arg_bindings, formal, actual);
                args = object_as_cons(args).rest;
            }

            Object *do_atom;
            if (false == object_try_make_atom(a, "do", &do_atom)) {
                print_out_of_memory_error(a);
                return false;
            }

            Object *body;
            if (false == object_try_make_cons(a, do_atom, fn->as_closure.body, &body)) {
                print_out_of_memory_error(a);
                return false;
            }

            stack_swap_top(s, frame_make(FRAME_DO, body, arg_bindings, frame->results_list, body->as_cons.rest));
            return true;
        }
        case FRAME_FN: {
            auto const len = object_list_count(frame->unevaluated);
            if (len < 2) {
                printf("SpecialFormError: fn takes at least 2 arguments but %zu were given\n", len);
                printf("Usage:\n");
                printf("    (fn (args*) x & more)\n");

                return false;
            }

            auto const args = object_as_cons(frame->unevaluated).first;
            if (TYPE_CONS != args->type && TYPE_NIL != args->type) {
                printf("SpecialFormError: invalid fn syntax\n");
                printf("Usage:\n");
                printf("    (fn (args*) x & more)\n");
                printf("\n");

                return false;
            }

            object_list_for(it, args) {
                if (TYPE_ATOM == it->type) {
                    continue;
                }
                printf(
                        "SpecialFormError: fn's args* must consist of atoms (got %s)\n",
                        object_type_str(it->type)
                );
                printf("Usage:\n");
                printf("    (fn (args*) x & more)\n");

                return false;
            }

            auto const body = object_as_cons(frame->unevaluated).rest;

            Object *closure;
            if (object_try_make_closure(a, frame->env, args, body, &closure)) {
                if (try_prepend(a, frame->results_list, closure)) {
                    stack_pop(s);
                    return true;
                }
                return false;
            }

            print_out_of_memory_error(a);
            return false;
        }
        case FRAME_IF: {
            if (object_nil() == frame->evaluated) {
                auto const len = object_list_count(frame->unevaluated);
                if (2 != len && 3 != len) {
                    printf("SpecialFormError: if takes 2 or 3 arguments but %zu were given\n", len);
                    printf("Usage:\n");
                    printf("    (if cond then)\n");
                    printf("    (if cond then else)\n");

                    return false;
                }

                auto const cond = object_as_cons(frame->unevaluated).first;
                frame->unevaluated = object_as_cons(frame->unevaluated).rest;
                return try_begin_eval(vm, EVAL_FRAME_KEEP, frame->env, cond, &frame->evaluated);
            }

            auto const cond_value = object_as_cons(frame->evaluated).first;
            if (object_nil() != cond_value) {
                return try_begin_eval(
                        vm, EVAL_FRAME_REMOVE,
                        frame->env, object_as_cons(frame->unevaluated).first, frame->results_list
                );
            }

            frame->unevaluated = object_as_cons(frame->unevaluated).rest; // skip `then`
            if (object_nil() == frame->unevaluated) {
                return try_pop_with(s, a, object_nil());
            }

            return try_begin_eval(
                    vm, EVAL_FRAME_REMOVE,
                    frame->env, object_as_cons(frame->unevaluated).first, frame->results_list
            );
        }
        case FRAME_DO: {
            if (object_nil() == frame->unevaluated) {
                return try_pop_with(s, a, object_nil());
            }

            if (object_nil() == object_as_cons(frame->unevaluated).rest) {
                return try_begin_eval(
                        vm, EVAL_FRAME_REMOVE,
                        frame->env, frame->unevaluated->as_cons.first, frame->results_list
                );
            }

            auto const next = object_as_cons(frame->unevaluated).first;
            frame->unevaluated = object_as_cons(frame->unevaluated).rest;
            return try_begin_eval(vm, EVAL_FRAME_KEEP, frame->env, next, nullptr);
        }
        case FRAME_DEFINE: {
            if (object_nil() == frame->evaluated) {
                auto const len = object_list_count(frame->unevaluated);
                if (2 != len) {
                    printf("SpecialFormError: define takes 2 arguments but %zu were given\n", len);
                    printf("Usage:\n");
                    printf("    (define name value)\n");

                    return false;
                }

                return try_begin_eval(
                        vm, EVAL_FRAME_KEEP,
                        frame->env, object_list_nth(frame->unevaluated, 1), &frame->evaluated
                );
            }

            auto const name = object_as_cons(frame->unevaluated).first;
            auto const value = object_as_cons(frame->evaluated).first;
            if (false == env_try_define(a, frame->env, name, value)) {
                print_out_of_memory_error(a);
                return false;
            }

            return try_pop_with(s, a, value);
        }
        case FRAME_IMPORT: {
            auto const len = object_list_count(frame->unevaluated);
            if (len != 1) {
                printf("SpecialFormError: import takes 1 argument but %zu were given\n", len);
                printf("Usage:\n");
                printf("    (import path)\n");
                return false;
            }

            auto const file_name = object_as_cons(frame->unevaluated).first;
            if (TYPE_STRING != file_name->type) {
                printf(
                        "SpecialFormError: import's path argument must be allocator string (got %s)\n",
                        object_type_str(file_name->type)
                );
                printf("Usage:\n");
                printf("    (import path)\n");
                return false;
            }

            auto const handle = fopen(file_name->as_string, "rb");
            if (nullptr == handle) {
                printf("ImportError: %s - %s\n", file_name->as_string, strerror(errno));
                return false;
            }
            auto const file = (NamedFile) {.name = file_name->as_string, .handle = handle};

            auto exprs = (Objects) {0};
            if (false == reader_try_read_all(vm_reader(vm), file, &exprs)) {
                fclose(handle);
                return false;
            }
            fclose(handle);
            da_append(&exprs, object_nil());

            auto exprs_list = object_nil();
            slice_for(it, exprs) {
                if (object_try_make_cons(a, *it, exprs_list, &exprs_list)) {
                    continue;
                }

                print_out_of_memory_error(a);
                return false;
            }
            object_list_reverse(&exprs_list);

            stack_swap_top(s, frame_make(FRAME_DO, frame->expr, frame->env, frame->results_list, exprs_list));
            return true;
        }
    }

    guard_unreachable();
}

bool try_eval(
        VirtualMachine *vm,
        Object *env,
        Object *expr,
        Object **value,
        Object **error
) {
    guard_is_not_null(vm);
    guard_is_not_null(env);
    guard_is_not_null(expr);
    guard_is_not_null(value);
    guard_is_not_null(error);

    slice_clear(vm_temporaries(vm));
    da_append(vm_temporaries(vm), object_nil());
    auto result = slice_last(*vm_temporaries(vm));
    if (false == try_begin_eval(vm, EVAL_FRAME_KEEP, env, expr, result)) {
        return false;
    }

    // TODO create a stacktrace on error
    while (false == stack_is_empty(vm_stack(vm))) {
        if (try_step(vm)) {
            continue;
        }

        return false;
    }
    guard_is_true(stack_is_empty(vm_stack(vm)));

    *value = object_as_cons(*result).first;
    return true;
}
