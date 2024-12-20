#include "eval.h"

#include <string.h>

#include "utility/guards.h"
#include "utility/exchange.h"
#include "object/list.h"
#include "object/accessors.h"
#include "object/constructors.h"
#include "reader/reader.h"
#include "env.h"
#include "bindings.h"
#include "stack.h"
#include "errors.h"
#include "variadic.h"

static auto const SYMBOL_DO = &(Object) {.type = TYPE_SYMBOL, .as_symbol = "do"};

typedef enum : bool {
    EVAL_FRAME_KEEP,
    EVAL_FRAME_REMOVE
} EvalFrameKeepOrRemove;

static bool try_save_result(VirtualMachine *vm, Object **results_list, Object *value) {
    guard_is_not_null(value);

    if (nullptr == results_list) {
        return true;
    }
    guard_is_not_null(*results_list);

    if (object_try_make_list(&vm->allocator, value, *results_list, results_list)) {
        return true;
    }

    out_of_memory_error(vm);
}

static bool try_save_result_and_pop(
        VirtualMachine *vm,
        Object **results_list,
        Object *value
) {
    guard_is_not_null(vm);
    guard_is_not_null(value);

    auto const frame = stack_top(&vm->stack);
    if (nullptr == frame->results_list) {
        stack_pop(&vm->stack);
        return true;
    }

    if (try_save_result(vm, results_list, value)) {
        stack_pop(&vm->stack);
        return true;
    }

    return false;
}

static bool try_get_special_type(Object *expr, Stack_FrameType *type) {
    if (TYPE_SYMBOL != object_as_list(expr).first->type) {
        return false;
    }

    auto const symbol_name = expr->as_list.first->as_symbol;

    if (0 == strcmp("if", symbol_name)) {
        *type = FRAME_IF;
        return true;
    }

    if (0 == strcmp("do", symbol_name)) {
        *type = FRAME_DO;
        return true;
    }

    if (0 == strcmp("define", symbol_name)) {
        *type = FRAME_DEFINE;
        return true;
    }

    if (0 == strcmp("fn", symbol_name)) {
        *type = FRAME_FN;
        return true;
    }

    if (0 == strcmp("macro", symbol_name)) {
        *type = FRAME_MACRO;
        return true;
    }

    if (0 == strcmp("import", symbol_name)) {
        *type = FRAME_IMPORT;
        return true;
    }

    if (0 == strcmp("quote", symbol_name)) {
        *type = FRAME_QUOTE;
        return true;
    }

    if (0 == strcmp("catch", symbol_name)) {
        *type = FRAME_CATCH;
        return true;
    }

    if (0 == strcmp("and", symbol_name)) {
        *type = FRAME_AND;
        return true;
    }

    if (0 == strcmp("or", symbol_name)) {
        *type = FRAME_OR;
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

    auto const s = &vm->stack;

    switch (expr->type) {
        case TYPE_INT:
        case TYPE_STRING:
        case TYPE_DICT:
        case TYPE_PRIMITIVE:
        case TYPE_CLOSURE:
        case TYPE_MACRO:
        case TYPE_NIL: {
            if (EVAL_FRAME_REMOVE == current) {
                return try_save_result_and_pop(vm, results_list, expr);
            }

            return try_save_result(vm, results_list, expr);
        }
        case TYPE_SYMBOL: {
            Object *value;
            if (false == env_try_find(env, expr, &value)) {
                name_error(vm, object_as_symbol(expr));
            }

            if (EVAL_FRAME_REMOVE == current) {
                return try_save_result_and_pop(vm, results_list, value);
            }

            return try_save_result(vm, results_list, value);
        }
        case TYPE_LIST: {
            Stack_FrameType type;
            auto const frame =
                    try_get_special_type(expr, &type)
                    ? frame_make(
                            type,
                            expr, env, results_list,
                            object_as_list(expr).rest
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

static bool try_step_call(VirtualMachine *vm) {
    guard_is_not_null(vm);

    auto const s = &vm->stack;
    auto const a = &vm->allocator;
    auto const frame = stack_top(s);
    guard_is_equal(frame->type, FRAME_CALL);

    if (1 == object_list_count(frame->evaluated) && TYPE_MACRO == frame->evaluated->as_list.first->type) {
        auto const fn = frame->evaluated->as_list.first;
        auto actual_args = frame->unevaluated;

        auto const formal_args = fn->as_closure.args;

        Object **arg_bindings;
        if (false == stack_try_create_local(stack_locals(s), &arg_bindings)) {
            stack_overflow_error(vm);
        }

        if (false == env_try_create(a, fn->as_closure.env, arg_bindings)) {
            out_of_memory_error(vm);
        }

        BindingError error;
        if (false == binding_try_create(&vm->allocator, *arg_bindings, formal_args, actual_args, &error)) {
            binding_error(vm, error);
        }

        stack_swap_top(s, frame_make(
                FRAME_DO,
                frame->expr,
                frame->env,
                frame->results_list,
                OBJECT_NIL
        ));

        return try_begin_eval(
                vm, EVAL_FRAME_KEEP,
                *arg_bindings, fn->as_closure.body,
                &frame->unevaluated
        );
    }

    if (is_ampersand(frame->unevaluated)) {
        guard_is_not_equal(frame->evaluated, OBJECT_NIL);

        auto const extra_args = object_list_nth_mutable(0, frame->evaluated);
        if (TYPE_LIST != (*extra_args)->type && TYPE_NIL != (*extra_args)->type) {
            type_error(vm, (*extra_args)->type, TYPE_LIST);
        }

        object_list_reverse_inplace(extra_args);
        object_list_concat_inplace(extra_args, object_list_skip(1, frame->evaluated));

        frame->evaluated = object_list_nth(0, frame->evaluated);
        frame->unevaluated = OBJECT_NIL;

        return true;
    }

    if (OBJECT_NIL != frame->unevaluated) {
        auto const next = object_list_nth(0, frame->unevaluated);

        if (is_ampersand(next)) {
            auto const rest = object_list_skip(1, frame->unevaluated);
            if (1 != object_list_count(rest)) {
                variadic_syntax_error(vm);
            }

            frame->unevaluated = next;
            return try_begin_eval(
                    vm, EVAL_FRAME_KEEP,
                    frame->env,
                    object_as_list(rest).first,
                    &frame->evaluated
            );
        }

        auto const ok = try_begin_eval(vm, EVAL_FRAME_KEEP, frame->env, next, &frame->evaluated);
        object_list_shift(&frame->unevaluated);
        return ok;
    }

    object_list_reverse_inplace(&frame->evaluated);
    guard_is_not_equal(frame->evaluated, OBJECT_NIL);

    auto const fn = object_as_list(frame->evaluated).first;
    auto actual_args = object_as_list(frame->evaluated).rest;

    if (TYPE_PRIMITIVE == fn->type) {
        Object **value;
        if (false == stack_try_create_local(stack_locals(s), &value)) {
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
    if (false == stack_try_create_local(stack_locals(s), &arg_bindings)) {
        stack_overflow_error(vm);
    }

    if (false == env_try_create(a, fn->as_closure.env, arg_bindings)) {
        out_of_memory_error(vm);
    }

    BindingError error;
    if (false == binding_try_create(&vm->allocator, *arg_bindings, formal_args, actual_args, &error)) {
        binding_error(vm, error);
    }

    return try_begin_eval(vm, EVAL_FRAME_REMOVE, *arg_bindings, fn->as_closure.body, frame->results_list);
}

static bool is_parameters_declaration_valid(Object *args) {
    BindingTargetError error;
    return (TYPE_LIST == args->type || TYPE_NIL == args->type)
           && binding_is_valid_target(args, &error);
}

// TODO make fn take an optional name to allow recursion BUT NOT MACROS
static bool try_step_macro_or_fn(VirtualMachine *vm) {
    guard_is_not_null(vm);

    auto const s = &vm->stack;
    auto const a = &vm->allocator;
    auto const frame = stack_top(s);
    guard_is_one_of(frame->type, FRAME_MACRO, FRAME_FN);

    auto const len = object_list_count(frame->unevaluated);
    auto const args = len > 0 ? object_as_list(frame->unevaluated).first : OBJECT_NIL;

    if (len < 2 || false == is_parameters_declaration_valid(args)) {
        if (FRAME_MACRO == frame->type) {
            special_syntax_error(vm, "macro", "(macro (" VARIADIC_AMPERSAND " args) " VARIADIC_AMPERSAND " body)");
        }

        special_syntax_error(vm, "fn", "(fn (" VARIADIC_AMPERSAND " args) " VARIADIC_AMPERSAND " body)");
    }

    auto const body_items = object_as_list(frame->unevaluated).rest;

    Object **body;
    if (false == stack_try_create_local(stack_locals(s), &body)) {
        stack_overflow_error(vm);
    }

    if (false == object_try_make_list(a, SYMBOL_DO, body_items, body)) {
        out_of_memory_error(vm);
    }

    Object **closure;
    if (false == stack_try_create_local(stack_locals(s), &closure)) {
        stack_overflow_error(vm);
    }

    auto const make_closure =
            FRAME_MACRO == frame->type
            ? object_try_make_macro
            : object_try_make_closure;
    if (false == make_closure(a, frame->env, args, *body, closure)) {
        out_of_memory_error(vm);
    }

    return try_save_result_and_pop(vm, frame->results_list, *closure);
}

static bool try_step_if(VirtualMachine *vm) {
    guard_is_not_null(vm);

    auto const s = &vm->stack;
    auto const frame = stack_top(s);
    guard_is_equal(frame->type, FRAME_IF);

    if (OBJECT_NIL == frame->evaluated) {
        auto const len = object_list_count(frame->unevaluated);
        if (len < 2 || len > 3) {
            special_syntax_error(vm, "if", "(if cond then)", "(if cond then else)");
        }

        auto const cond = object_as_list(frame->unevaluated).first;
        auto const ok = try_begin_eval(vm, EVAL_FRAME_KEEP, frame->env, cond, &frame->evaluated);
        object_list_shift(&frame->unevaluated);
        return ok;
    }

    auto const cond_value = object_as_list(frame->evaluated).first;
    if (OBJECT_NIL != cond_value) {
        return try_begin_eval(
                vm, EVAL_FRAME_REMOVE,
                frame->env, object_as_list(frame->unevaluated).first,
                frame->results_list
        );
    }

    frame->unevaluated = object_as_list(frame->unevaluated).rest; // skip `then`
    if (OBJECT_NIL == frame->unevaluated) {
        return try_save_result_and_pop(vm, frame->results_list, OBJECT_NIL);
    }

    return try_begin_eval(
            vm, EVAL_FRAME_REMOVE,
            frame->env, object_as_list(frame->unevaluated).first,
            frame->results_list
    );
}

static bool try_step_do(VirtualMachine *vm) {
    guard_is_not_null(vm);

    auto const s = &vm->stack;
    auto const frame = stack_top(s);
    guard_is_equal(frame->type, FRAME_DO);

    if (OBJECT_NIL == frame->unevaluated) {
        return try_save_result_and_pop(vm, frame->results_list, OBJECT_NIL);
    }

    if (OBJECT_NIL == object_as_list(frame->unevaluated).rest) {
        return try_begin_eval(
                vm, EVAL_FRAME_REMOVE,
                frame->env, frame->unevaluated->as_list.first,
                frame->results_list
        );
    }

    auto const next = object_as_list(frame->unevaluated).first;
    auto const ok = try_begin_eval(vm, EVAL_FRAME_KEEP, frame->env, next, nullptr);
    object_list_shift(&frame->unevaluated);
    return ok;
}

static bool try_step_define(VirtualMachine *vm) {
    guard_is_not_null(vm);

    auto const s = &vm->stack;
    auto const frame = stack_top(s);
    guard_is_equal(frame->type, FRAME_DEFINE);

    if (OBJECT_NIL == frame->evaluated) {
        size_t const expected = 2;
        if (expected != object_list_count(frame->unevaluated)) {
            special_syntax_error(vm, "define", "(define target value)");
        }

        return try_begin_eval(
                vm, EVAL_FRAME_KEEP,
                frame->env,
                object_list_nth(1, frame->unevaluated),
                &frame->evaluated
        );
    }

    auto const target = object_as_list(frame->unevaluated).first;
    auto const value = object_as_list(frame->evaluated).first;

    BindingError error;
    if (false == binding_try_create(&vm->allocator, frame->env, target, value, &error)) {
        binding_error(vm, error);
    }

    return try_save_result_and_pop(vm, frame->results_list, value);
}

static bool try_step_import(VirtualMachine *vm) {
    guard_is_not_null(vm);

    auto const a = &vm->allocator;
    auto const s = &vm->stack;
    auto const frame = stack_top(s);
    guard_is_equal(frame->type, FRAME_IMPORT);

    size_t const expected = 1;
    if (expected != object_list_count(frame->unevaluated)) {
        special_syntax_error(vm, "import", "(import path)");
    }

    auto const file_name = object_as_list(frame->unevaluated).first;
    if (TYPE_STRING != file_name->type) {
        type_error(vm, file_name->type, TYPE_STRING);
    }

    Object **exprs;
    if (false == stack_try_create_local(stack_locals(s), &exprs)) {
        stack_overflow_error(vm);
    }

    NamedFile file;
    if (false == named_file_try_open(file_name->as_string, "rb", &file)) {
        os_error(vm, errno);
    }

    auto const read_ok = object_reader_try_read_all(&vm->reader, file, exprs);
    named_file_close(&file);
    if (false == read_ok) {
        return false;
    }

    if (false == object_list_try_append_inplace(a, OBJECT_NIL, exprs)) {
        out_of_memory_error(vm);
    }

    stack_swap_top(s, frame_make(FRAME_DO, frame->expr, frame->env, frame->results_list, *exprs));
    return true;
}

static bool try_step_quote(VirtualMachine *vm) {
    guard_is_not_null(vm);

    auto const frame = stack_top(&vm->stack);
    guard_is_equal(frame->type, FRAME_QUOTE);

    size_t const expected = 1;
    if (expected != object_list_count(frame->unevaluated)) {
        special_syntax_error(vm, "quote", "(quote expr)");
    }

    auto const value = object_as_list(frame->unevaluated).first;
    return try_save_result_and_pop(vm, frame->results_list, value);
}

static bool try_step_catch(VirtualMachine *vm) {
    guard_is_not_null(vm);

    auto const s = &vm->stack;
    auto const a = &vm->allocator;
    auto const frame = stack_top(s);
    guard_is_equal(frame->type, FRAME_CATCH);

    if (OBJECT_NIL != vm->error) {
        guard_is_equal(frame->unevaluated, OBJECT_NIL);
        guard_is_equal(frame->evaluated, OBJECT_NIL);

        Object **error;
        if (false == stack_try_create_local(stack_locals(s), &error)) {
            stack_overflow_error(vm);
        }

        Object **result;
        if (false == stack_try_create_local(stack_locals(s), &result)) {
            stack_overflow_error(vm);
        }

        *error = exchange(vm->error, OBJECT_NIL);

        if (false == object_try_make_list_of(a, result, OBJECT_NIL, *error)) {
            out_of_memory_error(vm);
        }

        return try_save_result_and_pop(vm, frame->results_list, *result);
    }

    if (OBJECT_NIL == frame->evaluated) {
        guard_is_equal(vm->error, OBJECT_NIL);

        Object **body;
        if (false == stack_try_create_local(stack_locals(s), &body)) {
            stack_overflow_error(vm);
        }

        if (false == object_try_make_list(a, SYMBOL_DO, frame->unevaluated, body)) {
            out_of_memory_error(vm);
        }

        frame->unevaluated = OBJECT_NIL;

        return try_begin_eval(vm, EVAL_FRAME_KEEP, frame->env, *body, &frame->evaluated);
    }

    guard_is_equal(vm->error, OBJECT_NIL);
    guard_is_equal(frame->unevaluated, OBJECT_NIL);
    guard_is_equal(frame->unevaluated, OBJECT_NIL);

    Object **result;
    if (false == stack_try_create_local(stack_locals(s), &result)) {
        stack_overflow_error(vm);
    }

    if (false == object_try_make_list_of(a, result, object_as_list(frame->evaluated).first, OBJECT_NIL)) {
        out_of_memory_error(vm);
    }

    return try_save_result_and_pop(vm, frame->results_list, *result);
}

static bool try_step_and(VirtualMachine *vm) {
    guard_is_not_null(vm);

    auto const frame = stack_top(&vm->stack);
    guard_is_equal(frame->type, FRAME_AND);

    if (OBJECT_NIL == frame->evaluated) {
        if (OBJECT_NIL == frame->unevaluated) {
            return try_save_result_and_pop(vm, frame->results_list, OBJECT_NIL);
        }

        if (1 == object_list_count(frame->unevaluated)) {
            auto const result = object_as_list(frame->unevaluated).first;
            return try_begin_eval(vm, EVAL_FRAME_REMOVE, frame->env, result, frame->results_list);
        }

        auto const next = object_as_list(frame->unevaluated).first;
        auto const ok = try_begin_eval(vm, EVAL_FRAME_KEEP, frame->env, next, &frame->evaluated);
        object_list_shift(&frame->unevaluated);
        return ok;
    }

    auto const value = object_as_list(frame->evaluated).first;
    if (OBJECT_NIL == value) {
        return try_save_result_and_pop(vm, frame->results_list, OBJECT_NIL);
    }

    frame->evaluated = OBJECT_NIL;
    return true;
}

static bool try_step_or(VirtualMachine *vm) {
    guard_is_not_null(vm);

    auto const frame = stack_top(&vm->stack);
    guard_is_equal(frame->type, FRAME_OR);

    if (OBJECT_NIL == frame->evaluated) {
        if (OBJECT_NIL == frame->unevaluated) {
            return try_save_result_and_pop(vm, frame->results_list, OBJECT_NIL);
        }

        if (1 == object_list_count(frame->unevaluated)) {
            auto const result = object_as_list(frame->unevaluated).first;
            return try_begin_eval(vm, EVAL_FRAME_REMOVE, frame->env, result, frame->results_list);
        }

        auto const next = object_as_list(frame->unevaluated).first;
        auto const ok = try_begin_eval(vm, EVAL_FRAME_KEEP, frame->env, next, &frame->evaluated);
        object_list_shift(&frame->unevaluated);
        return ok;
    }

    auto const value = object_as_list(frame->evaluated).first;
    if (OBJECT_NIL != value) {
        return try_save_result_and_pop(vm, frame->results_list, value);
    }

    frame->evaluated = OBJECT_NIL;
    return true;
}

static bool try_step(VirtualMachine *vm) {
    guard_is_not_null(vm);

    switch (stack_top(&vm->stack)->type) {
        case FRAME_CALL: {
            return try_step_call(vm);
        }
        case FRAME_FN:
        case FRAME_MACRO: {
            return try_step_macro_or_fn(vm);
        }
        case FRAME_IF: {
            return try_step_if(vm);
        }
        case FRAME_DO: {
            return try_step_do(vm);
        }
        case FRAME_DEFINE: {
            return try_step_define(vm);
        }
        case FRAME_IMPORT: {
            return try_step_import(vm);
        }
        case FRAME_QUOTE: {
            return try_step_quote(vm);
        }
        case FRAME_CATCH: {
            return try_step_catch(vm);
        }
        case FRAME_AND: {
            return try_step_and(vm);
        }
        case FRAME_OR: {
            return try_step_or(vm);
        }
    }

    guard_unreachable();
}

bool try_eval(VirtualMachine *vm, Object *env, Object *expr) {
    guard_is_not_null(vm);
    guard_is_not_null(env);
    guard_is_not_null(expr);

    auto const s = &vm->stack;
    guard_is_true(stack_is_empty(s));

    vm->value = OBJECT_NIL;
    vm->error = OBJECT_NIL;
    if (false == try_begin_eval(vm, EVAL_FRAME_KEEP, env, expr, &vm->value)) {
        return false;
    }

    while (false == stack_is_empty(s)) {
        if (try_step(vm)) {
            continue;
        }

        auto const error_frame = stack_top(s);
        guard_is_not_equal(vm->error, OBJECT_NIL);

        while (false == stack_is_empty(s)) {
            auto const current_frame = stack_top(s);
            if (FRAME_CATCH == current_frame->type && error_frame != current_frame) {
                break;
            }

            stack_pop(s);
        }

        if (stack_is_empty(s)) {
            return false;
        }

        if (FRAME_CATCH == stack_top(s)->type) {
            continue;
        }

        guard_unreachable();
    }
    guard_is_true(stack_is_empty(s));

    vm->value = object_as_list(vm->value).first;
    return true;
}
