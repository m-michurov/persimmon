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

    out_of_memory_error(vm, error);
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

    if (0 == strcmp("macro", atom_name)) {
        *type = FRAME_MACRO;
        return true;
    }

    if (0 == strcmp("import", atom_name)) {
        *type = FRAME_IMPORT;
        return true;
    }

    if (0 == strcmp("quote", atom_name)) {
        *type = FRAME_QUOTE;
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
        case TYPE_PRIMITIVE:
        case TYPE_CLOSURE:
        case TYPE_MACRO:
        case TYPE_NIL: {
            if (EVAL_FRAME_REMOVE == current) {
                return try_save_result_and_pop(vm, results_list, expr, error);
            }

            return try_save_result(vm, results_list, expr, error);
        }
        case TYPE_ATOM: {
            Object *value;
            if (false == env_try_find(env, expr, &value)) {
                name_error(vm, object_as_atom(expr), error);
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

            stack_overflow_error(vm, error);
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
            if (1 == object_list_count(frame->evaluated)) {
                auto const fn = frame->evaluated->as_cons.first;
                if (TYPE_MACRO == fn->type) {
                    auto args = frame->unevaluated;

                    auto const formal_args = fn->as_macro.args;
                    auto const actual_argc = object_list_count(args);
                    auto const formal_argc = object_list_count(formal_args);
                    if (actual_argc != formal_argc) {
                        call_error(vm, "<macro>", formal_argc, actual_argc, &frame->error);
                    }

                    Object **arg_bindings;
                    if (false == stack_try_create_local(s, &arg_bindings)) {
                        stack_overflow_error(vm, &frame->error);
                    }

                    if (false == env_try_create(a, fn->as_macro.env, arg_bindings)) {
                        out_of_memory_error(vm, &frame->error);
                    }

                    object_list_for(formal, formal_args) {
                        auto const actual = object_list_shift(&args);
                        if (false == env_try_define(a, *arg_bindings, formal, actual, nullptr)) {
                            out_of_memory_error(vm, &frame->error);
                        }
                    }

                    Object **do_atom;
                    if (false == stack_try_create_local(s, &do_atom)) {
                        stack_overflow_error(vm, &frame->error);
                    }

                    if (false == object_try_make_atom(a, "do", do_atom)) {
                        out_of_memory_error(vm, &frame->error);
                    }

                    Object **body;
                    if (false == stack_try_create_local(s, &body)) {
                        stack_overflow_error(vm, &frame->error);
                    }

                    if (false == object_try_make_cons(a, *do_atom, fn->as_closure.body, body)) {
                        out_of_memory_error(vm, &frame->error);
                    }

                    stack_swap_top(s, frame_make(
                            FRAME_DO,
                            object_nil(),
                            frame->env,
                            frame->results_list,
                            object_nil()
                    ));

                    return try_begin_eval(
                            vm, EVAL_FRAME_KEEP,
                            *arg_bindings, *body,
                            &frame->unevaluated, &frame->error
                    );
                }
            }

            if (object_nil() != frame->unevaluated) {
                auto const next = object_as_cons(frame->unevaluated).first;
                auto const ok = try_begin_eval(vm, EVAL_FRAME_KEEP, frame->env, next, &frame->evaluated, &frame->error);
                object_list_shift(&frame->unevaluated);
                return ok;
            }

            object_list_reverse(&frame->evaluated);
            guard_is_not_equal(frame->evaluated, object_nil());

            auto const fn = object_as_cons(frame->evaluated).first;
            auto args = object_as_cons(frame->evaluated).rest;

            if (TYPE_PRIMITIVE == fn->type) {
                Object **value;
                if (false == stack_try_create_local(s, &value)) {
                    stack_overflow_error(vm, &frame->error);
                }

                if (false == fn->as_primitive(vm, args, value, &frame->error)) {
                    return false;
                }

                return try_save_result_and_pop(vm, frame->results_list, *value, &frame->error);
            }

            if (TYPE_CLOSURE != fn->type) {
                type_error(vm, &frame->error, fn->type, TYPE_CLOSURE, TYPE_MACRO, TYPE_PRIMITIVE);
            }
            auto const formal_args = fn->as_closure.args;
            auto const actual_argc = object_list_count(args);
            auto const formal_argc = object_list_count(formal_args);
            if (actual_argc != formal_argc) {
                call_error(vm, "<closure>", formal_argc, actual_argc, &frame->error);
            }

            Object **arg_bindings;
            if (false == stack_try_create_local(s, &arg_bindings)) {
                stack_overflow_error(vm, &frame->error);
            }

            if (false == env_try_create(a, fn->as_closure.env, arg_bindings)) {
                out_of_memory_error(vm, &frame->error);
            }

            object_list_for(formal, formal_args) {
                auto const actual = object_list_shift(&args);
                if (false == env_try_define(a, *arg_bindings, formal, actual, nullptr)) {
                    out_of_memory_error(vm, &frame->error);
                }
            }

            Object **do_atom;
            if (false == stack_try_create_local(s, &do_atom)) {
                stack_overflow_error(vm, &frame->error);
            }

            if (false == object_try_make_atom(a, "do", do_atom)) {
                out_of_memory_error(vm, &frame->error);
            }

            Object **body;
            if (false == stack_try_create_local(s, &body)) {
                stack_overflow_error(vm, &frame->error);
            }

            if (false == object_try_make_cons(a, *do_atom, fn->as_closure.body, body)) {
                out_of_memory_error(vm, &frame->error);
            }

            stack_swap_top(s, frame_make(
                    FRAME_DO,
                    *body,
                    *arg_bindings,
                    frame->results_list,
                    (*body)->as_cons.rest
            ));
            return true;
        }
        case FRAME_FN: {
            auto const len = object_list_count(frame->unevaluated);
            if (len < 2) {
                fn_too_few_args_error(vm, &frame->error);
            }

            auto const args = object_as_cons(frame->unevaluated).first;
            if (TYPE_CONS != args->type && TYPE_NIL != args->type) {
                fn_args_type_error(vm, &frame->error);
            }

            object_list_for(it, args) {
                if (TYPE_ATOM == it->type) {
                    continue;
                }

                fn_args_type_error(vm, &frame->error);
            }

            auto const body = object_as_cons(frame->unevaluated).rest;

            Object **closure;
            if (false == stack_try_create_local(s, &closure)) {
                stack_overflow_error(vm, &frame->error);
            }
            if (object_try_make_closure(a, frame->env, args, body, closure)) {
                if (try_save_result_and_pop(vm, frame->results_list, *closure, &frame->error)) {
                    return true;
                }

                out_of_memory_error(vm, &frame->error);
            }

            out_of_memory_error(vm, &frame->error);
        }
        case FRAME_MACRO: {
            auto const len = object_list_count(frame->unevaluated);
            if (len < 2) {
                // FIXME macro
                fn_too_few_args_error(vm, &frame->error);
            }

            auto const args = object_as_cons(frame->unevaluated).first;
            if (TYPE_CONS != args->type && TYPE_NIL != args->type) {
                // FIXME macro
                fn_args_type_error(vm, &frame->error);
            }

            object_list_for(it, args) {
                if (TYPE_ATOM == it->type) {
                    continue;
                }

                // FIXME macro
                fn_args_type_error(vm, &frame->error);
            }

            auto const body = object_as_cons(frame->unevaluated).rest;

            Object **closure;
            if (false == stack_try_create_local(s, &closure)) {
                stack_overflow_error(vm, &frame->error);
            }
            if (object_try_make_macro(a, frame->env, args, body, closure)) {
                if (try_save_result_and_pop(vm, frame->results_list, *closure, &frame->error)) {
                    return true;
                }

                out_of_memory_error(vm, &frame->error);
            }

            out_of_memory_error(vm, &frame->error);
        }
        case FRAME_IF: {
            if (object_nil() == frame->evaluated) {
                auto const len = object_list_count(frame->unevaluated);
                if (len < 2) {
                    if_too_few_args_error(vm, &frame->error);
                }

                if (len > 3) {
                    if_too_many_args_error(vm, &frame->error);
                }

                auto const cond = object_as_cons(frame->unevaluated).first;
                auto const ok = try_begin_eval(vm, EVAL_FRAME_KEEP, frame->env, cond, &frame->evaluated, &frame->error);
                object_list_shift(&frame->unevaluated);
                return ok;
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
            auto const ok = try_begin_eval(vm, EVAL_FRAME_KEEP, frame->env, next, nullptr, &frame->error);
            object_list_shift(&frame->unevaluated);
            return ok;
        }
        case FRAME_DEFINE: {
            if (object_nil() == frame->evaluated) {
                auto const len = object_list_count(frame->unevaluated);
                if (2 != len) {
                    define_args_error(vm, &frame->error);
                }

                return try_begin_eval(
                        vm, EVAL_FRAME_KEEP,
                        frame->env, *object_list_nth(1, frame->unevaluated),
                        &frame->evaluated, &frame->error
                );
            }

            auto const name = object_as_cons(frame->unevaluated).first;
            if (TYPE_ATOM != name->type) {
                define_name_type_error(vm, &frame->error);
            }

            auto const value = object_as_cons(frame->evaluated).first;
            if (false == env_try_define(a, frame->env, name, value, nullptr)) {
                out_of_memory_error(vm, &frame->error);
            }

            return try_save_result_and_pop(vm, frame->results_list, value, &frame->error);
        }
        case FRAME_IMPORT: {
            auto const len = object_list_count(frame->unevaluated);
            if (len != 1) {
                import_args_error(vm, &frame->error);
            }

            auto const file_name = object_as_cons(frame->unevaluated).first;
            if (TYPE_STRING != file_name->type) {
                import_path_type_error(vm, &frame->error);
            }

            if (false == slice_try_append(vm_expressions_stack(vm), object_nil())) {
                import_nesting_too_deep_error(vm, &frame->error);
            }

            NamedFile file;
            if (false == named_file_try_open(file_name->as_string, "rb", &file)) {
                slice_try_pop(vm_expressions_stack(vm), nullptr);
                os_error(vm, errno, &frame->error);
            }

            auto const exprs = slice_last(*vm_expressions_stack(vm));

            auto const read_ok = reader_try_read_all(vm_reader(vm), file, exprs, &frame->error);
            named_file_close(&file);
            if (false == read_ok) {
                slice_try_pop(vm_expressions_stack(vm), nullptr);
                return false;
            }

            object_list_reverse(exprs);
            if (false == object_try_make_cons(a, object_nil(), *exprs, exprs)) {
                out_of_memory_error(vm, &frame->error);
                slice_try_pop(vm_expressions_stack(vm), nullptr);
            }
            object_list_reverse(exprs);

            Object **exprs_list;
            if (false == stack_try_create_local(s, &exprs_list)) {
                stack_overflow_error(vm, &frame->error);
            }

            object_list_for(it, *exprs) {
                if (object_try_make_cons(a, it, *exprs_list, exprs_list)) {
                    continue;
                }

                out_of_memory_error(vm, &frame->error);
                slice_try_pop(vm_expressions_stack(vm), nullptr);
            }
            object_list_reverse(exprs_list);

            stack_swap_top(s, frame_make(FRAME_DO, frame->expr, frame->env, frame->results_list, *exprs_list));
            slice_try_pop(vm_expressions_stack(vm), nullptr);
            return true;
        }
        case FRAME_QUOTE: {
            if (1 != object_list_count(frame->unevaluated)) {
                // FIXME quote error
                define_args_error(vm, &frame->error);
            }

            auto const value = object_as_cons(frame->unevaluated).first;
            return try_save_result_and_pop(vm, frame->results_list, value, &frame->error);
        }
    }

    guard_unreachable();
}

bool try_eval(VirtualMachine *vm, Object *env, Object *expr) {
    guard_is_not_null(vm);
    guard_is_not_null(env);
    guard_is_not_null(expr);
    guard_is_true(stack_is_empty(vm_stack(vm)));

    auto result = object_nil();
    if (false == try_begin_eval(vm, EVAL_FRAME_KEEP, env, expr, &result, vm_error(vm))) {
        return false;
    }

    while (false == stack_is_empty(vm_stack(vm))) {
        if (try_step(vm)) {
            continue;
        }

        guard_is_not_equal(stack_top(vm_stack(vm))->error, object_nil());
        *vm_error(vm) = stack_top(vm_stack(vm))->error;

        while (false == stack_is_empty(vm_stack(vm))) {
            stack_pop(vm_stack(vm));
        }

        return false;
    }
    guard_is_true(stack_is_empty(vm_stack(vm)));

    *vm_value(vm) = object_as_cons(result).first;
    return true;
}
