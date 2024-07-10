#include "eval.h"

#include <string.h>

#include "object/lists.h"
#include "object/accessors.h"
#include "object/constructors.h"
#include "object/env.h"
#include "utility/slice.h"
#include "utility/guards.h"
#include "reader/reader.h"
#include "stack.h"
#include "errors.h"

#undef printf

typedef enum : bool {
    EVAL_FRAME_KEEP,
    EVAL_FRAME_REMOVE
} EvalFrameKeepOrRemove;

//#define EVAL_TRACE

static bool try_save_result(VirtualMachine *vm, Object **results_list, Object *value, Object **error) {
    guard_is_not_null(value);
    guard_is_not_null(error);

    if (nullptr == results_list) {
        return true;
    }
    guard_is_not_null(*results_list);

    if (object_try_make_cons(vm_allocator(vm), value, *results_list, results_list)) {
        return true;
    }

    create_out_of_memory_error(vm, error);
    return false;
}

static bool try_save_result_and_pop(VirtualMachine *vm, Object **results_list, Object *value, Object **error) {
    guard_is_not_null(vm);
    guard_is_not_null(value);
    guard_is_not_null(error);

    auto const frame = stack_top(vm_stack(vm));
    if (nullptr == frame->results_list) {
        stack_pop(vm_stack(vm));
        return true;
    }

    if (try_save_result(vm, results_list, value, error)) {
        stack_pop(vm_stack(vm));
        return true;
    }

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
        Object **results_list,
        Object **error
) {
    guard_is_not_null(vm);
    guard_is_not_null(env);
    guard_is_not_null(expr);

#ifdef EVAL_TRACE
    printf("[depth = %zu] evaluating %s\n", stack->count, object_repr(allocator, expr));
#endif

    auto const s = vm_stack(vm);

    switch (expr->type) {
        case TYPE_INT:
        case TYPE_STRING:
        case TYPE_CLOSURE:
        case TYPE_PRIMITIVE:
        case TYPE_NIL: {
            if (EVAL_FRAME_REMOVE == current) {
                return try_save_result_and_pop(vm, results_list, expr, error);
            }

            return try_save_result(vm, results_list, expr, error);
        }
        case TYPE_ATOM: {
            Object *value;
            if (false == env_try_find(env, expr, &value)) {
                create_name_error(vm, object_as_atom(expr), error);
                return false;
            }

            if (EVAL_FRAME_REMOVE == current) {
                return try_save_result_and_pop(vm, results_list, value, error);
            }

            return try_save_result(vm, results_list, value, error);
        }
        case TYPE_CONS: {
            Stack_FrameType type;
            auto const frame =
                    try_get_special_type(expr, &type)
                    ? frame_make(
                            type,
                            expr, env, results_list,
                            object_as_cons(expr).rest
                    )
                    : frame_make(
                            FRAME_CALL,
                            expr, env, results_list,
                            expr
                    );

            if (EVAL_FRAME_REMOVE == current) {
                stack_swap_top(s, frame);
                return true;
            }

            if (stack_try_push_frame(s, frame)) {
                return true;
            }

            create_stack_overflow_error(vm, error);
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
                return try_begin_eval(vm, EVAL_FRAME_KEEP, frame->env, next, &frame->evaluated, &frame->error);
            }

            object_list_reverse(&frame->evaluated);
            guard_is_not_equal(frame->evaluated, object_nil());

            auto const fn = object_as_cons(frame->evaluated).first;
            auto args = object_as_cons(frame->evaluated).rest;
            if (TYPE_PRIMITIVE == fn->type) {
                Object *value;
                if (false == fn->as_primitive(vm, args, &value, &frame->error)) {
                    return false;
                }
                return try_save_result_and_pop(vm, frame->results_list, value, &frame->error);
            }

            if (TYPE_CLOSURE != fn->type) {
                create_type_error(vm, &frame->error, fn->type, TYPE_CLOSURE, TYPE_PRIMITIVE);
                return false;
            }
            auto const formal_args = fn->as_closure.args;
            auto const actual_argc = object_list_count(args);
            auto const formal_argc = object_list_count(formal_args);
            if (actual_argc != formal_argc) {
                create_call_error(vm, "<closure>", formal_argc, actual_argc, &frame->error);
                return false;
            }

            Object *arg_bindings;
            if (false == env_try_create(a, fn->as_closure.env, &arg_bindings)) {
                create_out_of_memory_error(vm, &frame->error);
                return false;
            }
            object_list_for(formal, formal_args) {
                auto const actual = object_as_cons(args).first;
                if (false == env_try_define(a, arg_bindings, formal, actual, nullptr)) {
                    create_out_of_memory_error(vm, &frame->error);
                    return false;
                }
                args = object_as_cons(args).rest;
            }

            Object *do_atom;
            if (false == object_try_make_atom(a, "do", &do_atom)) {
                create_out_of_memory_error(vm, &frame->error);
                return false;
            }

            Object *body;
            if (false == object_try_make_cons(a, do_atom, fn->as_closure.body, &body)) {
                create_out_of_memory_error(vm, &frame->error);
                return false;
            }

            stack_swap_top(s, frame_make(FRAME_DO, body, arg_bindings, frame->results_list, body->as_cons.rest));
            return true;
        }
        case FRAME_FN: {
            auto const len = object_list_count(frame->unevaluated);
            if (len < 2) {
                create_fn_too_few_args_error(vm, &frame->error);
                return false;
            }

            auto const args = object_as_cons(frame->unevaluated).first;
            if (TYPE_CONS != args->type && TYPE_NIL != args->type) {
                create_fn_args_type_error(vm, &frame->error);
                return false;
            }

            object_list_for(it, args) {
                if (TYPE_ATOM == it->type) {
                    continue;
                }

                create_fn_args_type_error(vm, &frame->error);
                return false;
            }

            auto const body = object_as_cons(frame->unevaluated).rest;

            Object *closure;
            if (object_try_make_closure(a, frame->env, args, body, &closure)) {
                if (try_save_result_and_pop(vm, frame->results_list, closure, &frame->error)) {
                    return true;
                }

                create_out_of_memory_error(vm, &frame->error);
                return false;
            }

            create_out_of_memory_error(vm, &frame->error);
            return false;
        }
        case FRAME_IF: {
            if (object_nil() == frame->evaluated) {
                auto const len = object_list_count(frame->unevaluated);
                if (len < 2) {
                    create_if_too_few_args_error(vm, &frame->error);
                    return false;
                }

                if (len > 3) {
                    create_if_too_many_args_error(vm, &frame->error);
                    return false;
                }

                auto const cond = object_as_cons(frame->unevaluated).first;
                frame->unevaluated = object_as_cons(frame->unevaluated).rest;
                return try_begin_eval(vm, EVAL_FRAME_KEEP, frame->env, cond, &frame->evaluated, &frame->error);
            }

            auto const cond_value = object_as_cons(frame->evaluated).first;
            if (object_nil() != cond_value) {
                return try_begin_eval(
                        vm, EVAL_FRAME_REMOVE,
                        frame->env, object_as_cons(frame->unevaluated).first,
                        frame->results_list, &frame->error
                );
            }

            frame->unevaluated = object_as_cons(frame->unevaluated).rest; // skip `then`
            if (object_nil() == frame->unevaluated) {
                return try_save_result_and_pop(vm, frame->results_list, object_nil(), &frame->error);
            }

            return try_begin_eval(
                    vm, EVAL_FRAME_REMOVE,
                    frame->env, object_as_cons(frame->unevaluated).first,
                    frame->results_list, &frame->error
            );
        }
        case FRAME_DO: {
            if (object_nil() == frame->unevaluated) {
                return try_save_result_and_pop(vm, frame->results_list, object_nil(), &frame->error);
            }

            if (object_nil() == object_as_cons(frame->unevaluated).rest) {
                return try_begin_eval(
                        vm, EVAL_FRAME_REMOVE,
                        frame->env, frame->unevaluated->as_cons.first,
                        frame->results_list, &frame->error
                );
            }

            auto const next = object_as_cons(frame->unevaluated).first;
            frame->unevaluated = object_as_cons(frame->unevaluated).rest;
            return try_begin_eval(vm, EVAL_FRAME_KEEP, frame->env, next, nullptr, &frame->error);
        }
        case FRAME_DEFINE: {
            if (object_nil() == frame->evaluated) {
                auto const len = object_list_count(frame->unevaluated);
                if (2 != len) {
                    create_define_args_error(vm, &frame->error);
                    return false;
                }

                return try_begin_eval(
                        vm, EVAL_FRAME_KEEP,
                        frame->env, *object_list_nth(1, frame->unevaluated),
                        &frame->evaluated, &frame->error
                );
            }

            auto const name = object_as_cons(frame->unevaluated).first;
            if (TYPE_ATOM != name->type) {
                create_define_name_type_error(vm, &frame->error);
                return false;
            }

            auto const value = object_as_cons(frame->evaluated).first;
            if (false == env_try_define(a, frame->env, name, value, nullptr)) {
                create_out_of_memory_error(vm, &frame->error);
                return false;
            }

            return try_save_result_and_pop(vm, frame->results_list, value, &frame->error);
        }
        case FRAME_IMPORT: {
            auto const len = object_list_count(frame->unevaluated);
            if (len != 1) {
                create_import_args_error(vm, &frame->error);
                return false;
            }

            auto const file_name = object_as_cons(frame->unevaluated).first;
            if (TYPE_STRING != file_name->type) {
                create_import_path_type_error(vm, &frame->error);
                return false;
            }

            if (false == slice_try_append(vm_expressions_stack(vm), object_nil())) {
                create_import_nesting_too_deep_error(vm, &frame->error);
                return false;
            }

            auto const handle = fopen(file_name->as_string, "rb");
            if (nullptr == handle) {
                create_os_error(vm, errno, &frame->error);
                return false;
            }
            auto const exprs = slice_last(*vm_expressions_stack(vm));
            auto const file = (NamedFile) {.name = file_name->as_string, .handle = handle};
            if (false == reader_try_read_all(vm_reader(vm), file, exprs, &frame->error)) {
                fclose(handle);
                return false;
            }
            fclose(handle);

            object_list_reverse(exprs);
            if (false == object_try_make_cons(a, object_nil(), *exprs, exprs)) {
                create_out_of_memory_error(vm, &frame->error);
                return false;
            }
            object_list_reverse(exprs);

            auto exprs_list = object_nil();
            object_list_for(it, *exprs) {
                if (object_try_make_cons(a, it, exprs_list, &exprs_list)) {
                    continue;
                }

                create_out_of_memory_error(vm, &frame->error);
                return false;
            }
            object_list_reverse(&exprs_list);

            stack_swap_top(s, frame_make(FRAME_DO, frame->expr, frame->env, frame->results_list, exprs_list));
            slice_try_pop(vm_expressions_stack(vm), nullptr);
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
    guard_is_true(stack_is_empty(vm_stack(vm)));

    auto result = object_nil();
    if (false == try_begin_eval(vm, EVAL_FRAME_KEEP, env, expr, &result, error)) {
        return false;
    }

    while (false == stack_is_empty(vm_stack(vm))) {
        if (try_step(vm)) {
            continue;
        }

        guard_is_not_equal(stack_top(vm_stack(vm))->error, object_nil());
        *error = stack_top(vm_stack(vm))->error;

        while (false == stack_is_empty(vm_stack(vm))) {
            stack_pop(vm_stack(vm));
        }

        return false;
    }
    guard_is_true(stack_is_empty(vm_stack(vm)));

    *value = object_as_cons(result).first;
    return true;
}
