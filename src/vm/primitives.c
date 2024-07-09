#include "primitives.h"

#include <stdio.h>

#include "object/lists.h"
#include "object/constructors.h"
#include "object/accessors.h"
#include "object/env.h"
#include "utility/guards.h"
#include "eval_errors.h"

#define error(PrintErrorFn, Args) do { PrintErrorFn Args; return false; } while (false)

static bool prim_eq(ObjectAllocator *a, Object *args, Object **value) {
    guard_is_not_null(a);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    Object *lhs, *rhs;
    if (false == object_list_try_unpack_2(&lhs, &rhs, args)) {
        error(print_args_error, ("prim_eq?", object_list_count(args), 2));
    }

    if (false == object_equals(lhs, rhs)) {
        *value = object_nil();
        return true;
    }

    if (object_try_make_atom(a, "true", value)) {
        return true;
    }

    error(print_out_of_memory_error, (a));
}

static bool prim_print(ObjectAllocator *a, Object *args, Object **value) {
    guard_is_not_null(a);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    object_list_for(it, args) {
        object_repr_print(it, stdout);
        printf(" ");
    }
    printf("\n");

    *value = object_nil();
    return true;
}

static bool prim_plus(ObjectAllocator *a, Object *args, Object **value) {
    guard_is_not_null(a);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    int64_t acc = 0;
    object_list_for(arg, args) {
        if (TYPE_INT != arg->type) {
            error(print_type_error, (arg->type, TYPE_INT));
        }

        acc += arg->as_int;
    }

    if (object_try_make_int(a, acc, value)) {
        return true;
    }

    error(print_out_of_memory_error, (a));
}

static bool prim_minus(ObjectAllocator *a, Object *args, Object **value) {
    guard_is_not_null(a);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    if (object_nil() == args) {
        if (object_try_make_int(a, 0, value)) {
            return true;
        }

        error(print_out_of_memory_error, (a));
    }

    auto const first = args->as_cons.first;
    if (TYPE_INT != first->type) {
        error(print_type_error, (first->type, TYPE_INT));
    }

    auto acc = first->as_int;
    object_list_for(arg, args->as_cons.rest) {
        if (TYPE_INT != arg->type) {
            error(print_type_error, (arg->type, TYPE_INT));
        }

        acc -= arg->as_int;
    }

    if (object_try_make_int(a, acc, value)) {
        return true;
    }

    error(print_out_of_memory_error, (a));
}

static bool prim_mul(ObjectAllocator *a, Object *args, Object **value) {
    guard_is_not_null(a);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    int64_t acc = 1;
    object_list_for(arg, args) {
        if (TYPE_INT != arg->type) {
            error(print_type_error, (arg->type, TYPE_INT));
        }

        acc *= arg->as_int;
    }

    if (object_try_make_int(a, acc, value)) {
        return true;
    }

    error(print_out_of_memory_error, (a));
}

static bool prim_div(ObjectAllocator *a, Object *args, Object **value) {
    guard_is_not_null(a);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    if (object_nil() == args) {
        if (object_try_make_int(a, 1, value)) {
            return true;
        }

        error(print_out_of_memory_error, (a));
    }

    auto const first = args->as_cons.first;
    if (TYPE_INT != first->type) {
        error(print_type_error, (first->type, TYPE_INT));
    }

    auto acc = first->as_int;
    object_list_for(arg, args->as_cons.rest) {
        if (TYPE_INT != arg->type) {
            error(print_type_error, (arg->type, TYPE_INT));
        }

        if (0 == arg->as_int) {
            error(print_zero_division_error, ());
        }

        acc /= arg->as_int;
    }

    if (object_try_make_int(a, acc, value)) {
        return true;
    }

    error(print_out_of_memory_error, (a));
}

static bool prim_list_list(ObjectAllocator *a, Object *args, Object **value) {
    guard_is_not_null(a);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    *value = args;
    return true;
}

static bool prim_list_first(ObjectAllocator *a, Object *args, Object **value) {
    guard_is_not_null(a);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    auto const len = object_list_count(args);
    if (1 != len) {
        error(print_args_error, ("first", len, 1));
    }

    auto const list = object_as_cons(args).first;
    if (TYPE_CONS != list->type) {
        error(print_type_error, (list->type, TYPE_CONS));
    }

    *value = object_as_cons(list).first;
    return true;
}

static bool prim_list_rest(ObjectAllocator *a, Object *args, Object **value) {
    guard_is_not_null(a);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    auto const len = object_list_count(args);
    if (1 != len) {
        error(print_args_error, ("rest", len, 1));
    }

    auto const list = object_as_cons(args).first;
    if (TYPE_CONS != list->type) {
        error(print_type_error, (list->type, TYPE_CONS));
    }

    *value = object_as_cons(list).rest;
    return true;
}

static bool prim_list_prepend(ObjectAllocator *a, Object *args, Object **value) {
    guard_is_not_null(a);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    Object *element, *list;
    if (false == object_list_try_unpack_2(&element, &list, args)) {
        error(print_args_error, ("prepend", object_list_count(args), 1));
    }

    if (list->type != TYPE_NIL && list->type != TYPE_CONS) {
        error(print_type_error, (list->type, TYPE_CONS));
    }

    if (object_try_make_cons(a, element, list, value)) {
        return true;
    }

    error(print_out_of_memory_error, (a));
}

static bool try_define(ObjectAllocator *a, Object *env, char const *name, Object_Primitive value) {
    Object *binding;
    return env_try_define(a, env, object_nil(), object_nil(), &binding)
           && object_try_make_atom(a, name, object_list_nth(binding, 0))
           && object_try_make_primitive(a, value, object_list_nth(binding, 1));
}

bool try_define_primitives(ObjectAllocator *a, Object *env) {
    return try_define(a, env, "print", prim_print)
           && try_define(a, env, "+", prim_plus)
           && try_define(a, env, "-", prim_minus)
           && try_define(a, env, "*", prim_mul)
           && try_define(a, env, "/", prim_div)
           && try_define(a, env, "list", prim_list_list)
           && try_define(a, env, "first", prim_list_first)
           && try_define(a, env, "rest", prim_list_rest)
           && try_define(a, env, "prepend", prim_list_prepend)
           && try_define(a, env, "eq?", prim_eq);
}

