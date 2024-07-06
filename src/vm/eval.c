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

//#define EVAL_TRACE

static bool try_push_result(ObjectAllocator *a, Object **result, Object *value) {
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

static bool try_begin_eval(
        VirtualMachine *vm,
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
            return try_push_result(a, values_list, expr);
        }
        case TYPE_ATOM: {
            Object *value;
            if (false == env_try_find(env, expr, &value)) {
                print_name_error(object_as_atom(expr));
                return false;
            }

            return try_push_result(a, values_list, value);
        }
        case TYPE_CONS: {
            if (TYPE_ATOM == expr->as_cons.first->type) {
                auto const atom_name = expr->as_cons.first->as_atom;

                if (0 == strcmp("if", atom_name)) {
                    auto const len = object_list_count(expr->as_cons.rest);
                    if (2 != len && 3 != len) {
                        printf("SpecialFormError: if takes 2 or 3 arguments but %zu were given\n", len);
                        printf("Usage:\n");
                        printf("    (if cond then)\n");
                        printf("    (if cond then else)\n");
                        return false;
                    }

                    auto const no_overflow = stack_try_push_frame(
                            s,
                            frame_make(FRAME_IF, expr, env, values_list, object_as_cons(expr).rest)
                    );
                    if (false == no_overflow) {
                        print_stack_overflow_error();
                        return false;
                    }
                    return true;
                }

                if (0 == strcmp("do", atom_name)) {
                    auto const no_overflow = stack_try_push_frame(
                            s,
                            frame_make(FRAME_DO, expr, env, values_list, object_as_cons(expr).rest)
                    );
                    if (false == no_overflow) {
                        print_stack_overflow_error();
                        return false;
                    }
                    return true;
                }

                if (0 == strcmp("define", atom_name)) {
                    auto const len = object_list_count(expr->as_cons.rest);
                    if (2 != len) {
                        printf("SpecialFormError: define takes 2 arguments but %zu were given\n", len);
                        printf("Usage:\n");
                        printf("    (define name value)\n");
                        return false;
                    }

                    auto const no_overflow = stack_try_push_frame(
                            s,
                            frame_make(FRAME_DEFINE, expr, env, values_list, object_as_cons(expr).rest)
                    );
                    if (false == no_overflow) {
                        print_stack_overflow_error();
                        return false;
                    }
                    return true;
                }

                if (0 == strcmp("fn", atom_name)) {
                    auto const len = object_list_count(expr->as_cons.rest);
                    if (len < 2) {
                        printf("SpecialFormError: fn takes at least 2 arguments but %zu were given\n", len);
                        printf("Usage:\n");
                        printf("    (fn (args*) x & more)\n");
                        return false;
                    }

                    auto const args = object_list_nth(expr, 1);
                    if (TYPE_CONS != args->type && TYPE_NIL != args->type) {
                        printf("SpecialFormError: invalid fn syntax\n");
                        printf("Usage:\n");
                        printf("    (fn (args*) x & more)\n");
                        return false;
                    }

                    object_list_for(it, args) {
                        if (TYPE_ATOM == it->type) {
                            continue;
                        }
                        printf(
                                "SpecialFormError: fn'stack args* must consist of atoms (got %s)\n",
                                object_type_str(it->type)
                        );
                        printf("Usage:\n");
                        printf("    (fn (args*) x & more)\n");
                        return false;
                    }

                    auto const body = object_list_skip(expr, 2);

                    Object *closure;
                    if (object_try_make_closure(a, env, args, body, &closure)) {
                        return try_push_result(a, values_list, closure);
                    }

                    print_out_of_memory_error(a);
                    return false;
                }

                if (0 == strcmp("import", atom_name)) {
                    auto const len = object_list_count(expr->as_cons.rest);
                    if (len != 1) {
                        printf("SpecialFormError: import takes 1 argument but %zu were given\n", len);
                        printf("Usage:\n");
                        printf("    (import path)\n");
                        return false;
                    }

                    auto const file_name = object_list_nth(expr, 1);
                    if (TYPE_STRING != file_name->type) {
                        printf(
                                "SpecialFormError: import'stack path argument must be allocator string (got %s)\n",
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

                    auto exprs_list = object_nil();
                    slice_for(it, exprs) {
                        if (object_try_make_cons(a, *it, exprs_list, &exprs_list)) {
                            continue;
                        }

                        print_out_of_memory_error(a);
                        return false;
                    }
                    object_list_reverse(&exprs_list);

                    auto const no_overflow = stack_try_push_frame(
                            s,
                            frame_make(FRAME_DO, expr, env, values_list, exprs_list)
                    );
                    if (false == no_overflow) {
                        print_stack_overflow_error();
                        return false;
                    }
                    return true;
                }
            }
            auto const no_overflow = stack_try_push_frame(
                    s,
                    frame_make(FRAME_CALL, expr, env, values_list, expr)
            );
            if (false == no_overflow) {
                print_stack_overflow_error();
                return false;
            }
            return true;
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
                return try_begin_eval(vm, frame->env, next, &frame->evaluated);
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
                auto const ok = try_push_result(a, frame->results_list, value);
                stack_pop(s);
                return ok;
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

            stack_pop(s);
            auto const no_overflow = stack_try_push_frame(
                    s,
                    frame_make(FRAME_DO, body, arg_bindings, frame->results_list, body->as_cons.rest)
            );
            if (false == no_overflow) {
                print_stack_overflow_error();
                return false;
            }

            return true;
        }
        case FRAME_IF: {
            if (object_nil() == frame->evaluated) {
                auto const next = object_as_cons(frame->unevaluated).first;
                frame->unevaluated = object_as_cons(frame->unevaluated).rest;
                return try_begin_eval(vm, frame->env, next, &frame->evaluated);
            }

            auto const cond_value = object_as_cons(frame->evaluated).first;

            stack_pop(s);
            if (object_nil() != cond_value) {
                return try_begin_eval(vm, frame->env, object_as_cons(frame->unevaluated).first, frame->results_list);
            }

            frame->unevaluated = object_as_cons(frame->unevaluated).rest; // skip `then`
            if (object_nil() == frame->unevaluated) {
                return try_push_result(a, frame->results_list, object_nil());
            }

            return try_begin_eval(vm, frame->env, object_as_cons(frame->unevaluated).first, frame->results_list);
        }
        case FRAME_DO: {
            if (object_nil() == frame->unevaluated) {
                auto const ok = try_push_result(a, frame->results_list, object_nil());
                stack_pop(s);
                return ok;
            }

            if (object_nil() == object_as_cons(frame->unevaluated).rest) {
                stack_pop(s);
                return try_begin_eval(vm, frame->env, frame->unevaluated->as_cons.first, frame->results_list);
            }

            auto const next = object_as_cons(frame->unevaluated).first;
            frame->unevaluated = object_as_cons(frame->unevaluated).rest;
            return try_begin_eval(vm, frame->env, next, nullptr);
        }
        case FRAME_DEFINE: {
            if (object_nil() == frame->evaluated) {
                return try_begin_eval(vm, frame->env, object_list_nth(frame->unevaluated, 1), &frame->evaluated);
            }

            env_try_define(a, frame->env, object_as_cons(frame->unevaluated).first,
                           object_as_cons(frame->evaluated).first);
            auto const ok = try_push_result(a, frame->results_list, object_as_cons(frame->evaluated).first);
            stack_pop(s);
            return ok;
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
    if (false == try_begin_eval(vm, env, expr, result)) {
        return false;
    }

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
