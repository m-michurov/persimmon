#include "primitives.h"

#include <stdio.h>

#include "object/lists.h"
#include "object/constructors.h"
#include "object/constructors_unchecked.h"
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

void define_primitives(ObjectAllocator *a, Object *env) {
    env_try_define(a, env, object_atom(a, "print"), object_primitive(a, prim_print));
    env_try_define(a, env, object_atom(a, "+"), object_primitive(a, prim_plus));
    env_try_define(a, env, object_atom(a, "-"), object_primitive(a, prim_minus));
    env_try_define(a, env, object_atom(a, "*"), object_primitive(a, prim_mul));
    env_try_define(a, env, object_atom(a, "/"), object_primitive(a, prim_div));
    env_try_define(a, env, object_atom(a, "list"), object_primitive(a, prim_list_list));
    env_try_define(a, env, object_atom(a, "first"), object_primitive(a, prim_list_first));
    env_try_define(a, env, object_atom(a, "rest"), object_primitive(a, prim_list_rest));
    env_try_define(a, env, object_atom(a, "prepend"), object_primitive(a, prim_list_prepend));
    env_try_define(a, env, object_atom(a, "eq?"), object_primitive(a, prim_eq));
}

