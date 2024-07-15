#include "eval.h"

#include <string.h>

#include "utility/slice.h"
#include "utility/guards.h"
#include "utility/exchange.h"
#include "object/lists.h"
#include "object/accessors.h"
#include "object/constructors.h"
#include "reader/reader.h"
#include "env.h"
#include "stack.h"
#include "errors.h"

typedef enum : bool {
    EVAL_FRAME_KEEP,
    EVAL_FRAME_REMOVE
} EvalFrameKeepOrRemove;

//#define EVAL_TRACE

static bool try_save_result(VirtualMachine *vm, Object **results_list, Object *value) {
    guard_is_not_null(value);

    if (nullptr == results_list) {
        return true;
    }
    guard_is_not_null(*results_list);

    if (object_try_make_cons(vm_allocator(vm), value, *results_list, results_list)) {
        return true;
    }

    out_of_memory_error(vm);
}

static bool try_save_result_and_pop(
        VirtualMachine *vm,
        Object **results_list, // TODO this is always frame->results_list of the top frame
        Object *value
) {
    guard_is_not_null(vm);
    guard_is_not_null(value);

    auto const frame = stack_top(vm_stack(vm));
    if (nullptr == frame->results_list) {
        stack_pop(vm_stack(vm));
        return true;
    }

    if (try_save_result(vm, results_list, value)) {
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

    if (0 == strcmp("try", atom_name)) {
        *type = FRAME_TRY;
        return true;
    }

    return false;
}

static bool try_begin_eval(
        VirtualMachine *vm,
        EvalFrameKeepOrRemove current,
        Object *env,
        Object *expr,
        Object **results_list
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
                return try_save_result_and_pop(vm, results_list, expr);
            }

            return try_save_result(vm, results_list, expr);
        }
        case TYPE_ATOM: {
            Object *value;
            if (false == env_try_find(env, expr, &value)) {
                name_error(vm, object_as_atom(expr));
            }

            if (EVAL_FRAME_REMOVE == current) {
                return try_save_result_and_pop(vm, results_list, value);
            }

            return try_save_result(vm, results_list, value);
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

            stack_overflow_error(vm);
        }
    }

    guard_unreachable();
}

static bool validate_args(Object *args) {
    return env_is_valid_bind_target(args)
           && (TYPE_CONS == args->type || TYPE_NIL == args->type);
}

static bool try_bind(VirtualMachine *vm, Object *env, Object *target, Object *value) {
    Env_BindingError binding_error;
    if (env_try_bind(vm_allocator(vm), env, target, value, &binding_error)) {
        return true;
    }

    switch (binding_error.type) {
        case Env_CannotUnpack: {
            binding_unpack_error(vm, binding_error.as_cannot_unpack.value_type);
        }
        case Env_CountMismatch: {
            binding_count_error(
                    vm,
                    binding_error.as_count_mismatch.expected,
                    binding_error.as_count_mismatch.is_varargs,
                    binding_error.as_count_mismatch.got
            );
        }
        case Env_InvalidTarget: {
            binding_target_error(vm, binding_error.as_invalid_target.target_type);
        }
        case Env_InvalidVarargsFormat: {
            binding_varargs_error(vm);
        }
        case Env_AllocationError: {
            out_of_memory_error(vm);
        }
    }

    guard_unreachable();
}

static bool try_step(VirtualMachine *vm) {
    guard_is_not_null(vm);

    auto const s = vm_stack(vm);
    auto const a = vm_allocator(vm);
    auto const frame = stack_top(s);

    object_repr_print(frame->expr, stdout);
    printf("\n");
    object_repr_print(frame->unevaluated, stdout);
    printf("\n");
    object_repr_print(frame->evaluated, stdout);
    printf("\n");
    printf("\n");

    switch (frame->type) {
        case FRAME_CALL: {
            if (1 == object_list_count(frame->evaluated) && TYPE_MACRO == frame->evaluated->as_cons.first->type) {
                auto const fn = frame->evaluated->as_cons.first;
                auto actual_args = frame->unevaluated;

                auto const formal_args = fn->as_closure.args;

                Object **arg_bindings;
                if (false == stack_try_create_local(s, &arg_bindings)) {
                    stack_overflow_error(vm);
                }

                if (false == env_try_create(a, fn->as_closure.env, arg_bindings)) {
                    out_of_memory_error(vm);
                }

                if (false == try_bind(vm, *arg_bindings, formal_args, actual_args)) {
                    return false;
                }

                Object **do_atom;
                if (false == stack_try_create_local(s, &do_atom)) {
                    stack_overflow_error(vm);
                }

                if (false == object_try_make_atom(a, "do", do_atom)) {
                    out_of_memory_error(vm);
                }

                Object **body;
                if (false == stack_try_create_local(s, &body)) {
                    stack_overflow_error(vm);
                }

                if (false == object_try_make_cons(a, *do_atom, fn->as_closure.body, body)) {
                    out_of_memory_error(vm);
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
                        &frame->unevaluated
                );
            }

            if (object_nil() != frame->unevaluated) {
                auto const next = object_as_cons(frame->unevaluated).first;
                auto const ok = try_begin_eval(vm, EVAL_FRAME_KEEP, frame->env, next, &frame->evaluated);
                object_list_shift(&frame->unevaluated);
                return ok;
            }

            object_list_reverse(&frame->evaluated);
            guard_is_not_equal(frame->evaluated, object_nil());

            auto const fn = object_as_cons(frame->evaluated).first;
            auto actual_args = object_as_cons(frame->evaluated).rest;

            if (TYPE_PRIMITIVE == fn->type) {
                Object **value;
                if (false == stack_try_create_local(s, &value)) {
                    stack_overflow_error(vm);
                }

                if (false == fn->as_primitive(vm, actual_args, value)) {
                    return false;
                }

                return try_save_result_and_pop(vm, frame->results_list, *value);
            }

            if (TYPE_CLOSURE != fn->type) {
                type_error(vm, fn->type, TYPE_CLOSURE, TYPE_MACRO, TYPE_PRIMITIVE);
            }
            auto const formal_args = fn->as_closure.args;

            Object **arg_bindings;
            if (false == stack_try_create_local(s, &arg_bindings)) {
                stack_overflow_error(vm);
            }

            if (false == env_try_create(a, fn->as_closure.env, arg_bindings)) {
                out_of_memory_error(vm);
            }

            if (false == try_bind(vm, *arg_bindings, formal_args, actual_args)) {
                return false;
            }

            Object **do_atom;
            if (false == stack_try_create_local(s, &do_atom)) {
                stack_overflow_error(vm);
            }

            if (false == object_try_make_atom(a, "do", do_atom)) {
                out_of_memory_error(vm);
            }

            Object **body;
            if (false == stack_try_create_local(s, &body)) {
                stack_overflow_error(vm);
            }

            if (false == object_try_make_cons(a, *do_atom, fn->as_closure.body, body)) {
                out_of_memory_error(vm);
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
                too_few_args_error(vm, "fn");
            }

            auto const args = object_as_cons(frame->unevaluated).first;
            if (false == validate_args(args)) {
                parameters_type_error(vm);
            }

            auto const body = object_as_cons(frame->unevaluated).rest;

            Object **closure;
            if (false == stack_try_create_local(s, &closure)) {
                stack_overflow_error(vm);
            }
            if (object_try_make_closure(a, frame->env, args, body, closure)) {
                if (try_save_result_and_pop(vm, frame->results_list, *closure)) {
                    return true;
                }

                out_of_memory_error(vm);
            }

            out_of_memory_error(vm);
        }
        case FRAME_MACRO: {
            auto const len = object_list_count(frame->unevaluated);
            if (len < 2) {
                too_few_args_error(vm, "macro");
            }

            auto const args = object_as_cons(frame->unevaluated).first;
            if (false == validate_args(args)) {
                parameters_type_error(vm);
            }

            auto const body = object_as_cons(frame->unevaluated).rest;

            Object **closure;
            if (false == stack_try_create_local(s, &closure)) {
                stack_overflow_error(vm);
            }
            if (object_try_make_macro(a, frame->env, args, body, closure)) {
                if (try_save_result_and_pop(vm, frame->results_list, *closure)) {
                    return true;
                }

                out_of_memory_error(vm);
            }

            out_of_memory_error(vm);
        }
        case FRAME_IF: {
            if (object_nil() == frame->evaluated) {
                auto const len = object_list_count(frame->unevaluated);
                if (len < 2) {
                    too_few_args_error(vm, "if");
                }

                if (len > 3) {
                    too_many_args_error(vm, "if");
                }

                auto const cond = object_as_cons(frame->unevaluated).first;
                auto const ok = try_begin_eval(vm, EVAL_FRAME_KEEP, frame->env, cond, &frame->evaluated);
                object_list_shift(&frame->unevaluated);
                return ok;
            }

            auto const cond_value = object_as_cons(frame->evaluated).first;
            if (object_nil() != cond_value) {
                return try_begin_eval(
                        vm, EVAL_FRAME_REMOVE,
                        frame->env, object_as_cons(frame->unevaluated).first,
                        frame->results_list
                );
            }

            frame->unevaluated = object_as_cons(frame->unevaluated).rest; // skip `then`
            if (object_nil() == frame->unevaluated) {
                return try_save_result_and_pop(vm, frame->results_list, object_nil());
            }

            return try_begin_eval(
                    vm, EVAL_FRAME_REMOVE,
                    frame->env, object_as_cons(frame->unevaluated).first,
                    frame->results_list
            );
        }
        case FRAME_DO: {
            if (object_nil() == frame->unevaluated) {
                return try_save_result_and_pop(vm, frame->results_list, object_nil());
            }

            if (object_nil() == object_as_cons(frame->unevaluated).rest) {
                return try_begin_eval(
                        vm, EVAL_FRAME_REMOVE,
                        frame->env, frame->unevaluated->as_cons.first,
                        frame->results_list
                );
            }

            auto const next = object_as_cons(frame->unevaluated).first;
            auto const ok = try_begin_eval(vm, EVAL_FRAME_KEEP, frame->env, next, nullptr);
            object_list_shift(&frame->unevaluated);
            return ok;
        }
        case FRAME_DEFINE: {
            if (object_nil() == frame->evaluated) {
                size_t const expected = 2;
                if (expected != object_list_count(frame->unevaluated)) {
                    args_count_error(vm, "define", expected);
                }

                return try_begin_eval(
                        vm, EVAL_FRAME_KEEP,
                        frame->env, *object_list_nth(1, frame->unevaluated),
                        &frame->evaluated
                );
            }

            auto const target = object_as_cons(frame->unevaluated).first;
            auto const value = object_as_cons(frame->evaluated).first;
            if (false == try_bind(vm, frame->env, target, value)) {
                return false;
            }

            return try_save_result_and_pop(vm, frame->results_list, value);
        }
        case FRAME_IMPORT: {
            size_t const expected = 1;
            if (expected != object_list_count(frame->unevaluated)) {
                args_count_error(vm, "import", expected);
            }

            auto const file_name = object_as_cons(frame->unevaluated).first;
            if (TYPE_STRING != file_name->type) {
                import_path_type_error(vm);
            }

            if (false == slice_try_append(vm_expressions_stack(vm), object_nil())) {
                import_nesting_too_deep_error(vm);
            }

            NamedFile file;
            if (false == named_file_try_open(file_name->as_string, "rb", &file)) {
                slice_try_pop(vm_expressions_stack(vm), nullptr);
                os_error(vm, errno);
            }

            auto const exprs = slice_last(*vm_expressions_stack(vm));

            auto const read_ok = reader_try_read_all(vm_reader(vm), file, exprs);
            named_file_close(&file);
            if (false == read_ok) {
                slice_try_pop(vm_expressions_stack(vm), nullptr);
                return false;
            }

            object_list_reverse(exprs);
            if (false == object_try_make_cons(a, object_nil(), *exprs, exprs)) {
                out_of_memory_error(vm);
                slice_try_pop(vm_expressions_stack(vm), nullptr);
            }
            object_list_reverse(exprs);

            Object **exprs_list;
            if (false == stack_try_create_local(s, &exprs_list)) {
                stack_overflow_error(vm);
            }

            object_list_for(it, *exprs) {
                if (object_try_make_cons(a, it, *exprs_list, exprs_list)) {
                    continue;
                }

                out_of_memory_error(vm);
                slice_try_pop(vm_expressions_stack(vm), nullptr);
            }
            object_list_reverse(exprs_list);

            stack_swap_top(s, frame_make(FRAME_DO, frame->expr, frame->env, frame->results_list, *exprs_list));
            slice_try_pop(vm_expressions_stack(vm), nullptr);
            return true;
        }
        case FRAME_QUOTE: {
            size_t const expected = 1;
            if (expected != object_list_count(frame->unevaluated)) {
                args_count_error(vm, "quote", expected);
            }

            auto const value = object_as_cons(frame->unevaluated).first;
            return try_save_result_and_pop(vm, frame->results_list, value);
        }
        case FRAME_TRY: {
            if (object_nil() != *vm_error(vm)) {
                guard_is_equal(object_nil(), frame->unevaluated);
                guard_is_equal(object_nil(), frame->evaluated);

                Object **error, **result;
                if (false == stack_try_create_local(s, &error)
                    || false == stack_try_create_local(s, &result)) {
                    stack_overflow_error(vm);
                }

                *error = exchange(*vm_error(vm), object_nil());

                if (false == object_try_make_list(a, result, object_nil(), error)) {
                    out_of_memory_error(vm);
                }

                return try_save_result_and_pop(vm, frame->results_list, *result);
            }

            if (object_nil() == frame->evaluated) {
                guard_is_equal(object_list_count(frame->unevaluated), 1);
                guard_is_equal(object_nil(), *vm_error(vm));

                auto const next = object_as_cons(frame->unevaluated).first;
                auto const ok = try_begin_eval(vm, EVAL_FRAME_KEEP, frame->env, next, &frame->evaluated);
                object_list_shift(&frame->unevaluated);
                return ok;
            }


            Object **result;
            if (false == stack_try_create_local(s, &result)) {
                stack_overflow_error(vm);
            }

            if (false == object_try_make_list(a, result, object_as_cons(frame->evaluated).first, object_nil())) {
                out_of_memory_error(vm);
            }

            return try_save_result_and_pop(vm, frame->results_list, *result);
        }
    }

    guard_unreachable();
}

bool try_eval(VirtualMachine *vm, Object *env, Object *expr) {
    guard_is_not_null(vm);
    guard_is_not_null(env);
    guard_is_not_null(expr);

    auto const s = vm_stack(vm);
    guard_is_true(stack_is_empty(s));

    *vm_value(vm) = object_nil();
    *vm_error(vm) = object_nil();
    if (false == try_begin_eval(vm, EVAL_FRAME_KEEP, env, expr, vm_value(vm))) {
        return false;
    }

    while (false == stack_is_empty(s)) {
        if (try_step(vm)) {
            continue;
        }

        guard_is_not_equal(*vm_error(vm), object_nil());

        while (false == stack_is_empty(s) && FRAME_TRY != stack_top(s)->type) {
            stack_pop(s);
        }

        if (stack_is_empty(s)) {
            return false;
        }

        if (FRAME_TRY == stack_top(s)->type) {
            continue;
        }

        guard_unreachable();
    }
    guard_is_true(stack_is_empty(s));

    *vm_value(vm) = object_as_cons(*vm_value(vm)).first;
    return true;
}
