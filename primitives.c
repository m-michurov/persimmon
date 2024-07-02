#include "primitives.h"

#include <stdio.h>

#include "object_lists.h"
#include "object_constructors.h"
#include "object_accessors.h"
#include "guards.h"
#include "object_env.h"
#include "eval_errors.h"

static bool prim_eq(ObjectAllocator *a, Object *args, Object **value) {
    guard_is_not_null(a);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    Object *lhs, *rhs;
    if (false == object_list_try_unpack_2(&lhs, &rhs, args)) {
        print_args_error("prim_eq?", object_list_count(args), 2);
        return false;
    }

    *value = object_equals(lhs, rhs) ? object_atom(a, "true") : object_nil();
    return true;
}

static bool prim_print(ObjectAllocator *a, Object *args, Object **value) {
    guard_is_not_null(a);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    auto arena = &(Arena) {0};

    if (TYPE_CONS != args->type) {
        printf("%s\n", object_repr(arena, args));

        *value = object_nil();
        return true;
    }

    object_list_for(it, args) {
        printf("%s ", object_repr(arena, it));
    }
    printf("\n");

    arena_free(arena);

    *value = object_nil();
    return true;
}

static bool prim_plus(ObjectAllocator *a, Object *args, Object **value) {
    guard_is_not_null(a);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    auto acc = object_int(a, 0);
    object_list_for(arg, args) {
        if (TYPE_INT != arg->type) {
            print_type_error(arg->type, TYPE_INT);
            return false;
        }

        acc = object_int(a, acc->as_int + arg->as_int);
    }

    *value = acc;
    return true;
}

static bool prim_minus(ObjectAllocator *a, Object *args, Object **value) {
    guard_is_not_null(a);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    if (object_nil() == args) {
        *value = object_int(a, 0);
        return true;
    }

    auto acc = object_nil();
    object_list_for(arg, args) {
        if (TYPE_INT != arg->type) {
            print_type_error(arg->type, TYPE_INT);
            return false;
        }

        acc = object_nil() != acc
              ? object_int(a, acc->as_int - arg->as_int)
              : arg;
    }

    *value = acc;
    return true;
}

static bool prim_mul(ObjectAllocator *a, Object *args, Object **value) {
    guard_is_not_null(a);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    auto acc = object_int(a, 1);
    object_list_for(arg, args) {
        if (TYPE_INT != arg->type) {
            print_type_error(arg->type, TYPE_INT);
            return false;
        }

        acc = object_int(a, acc->as_int * arg->as_int);
    }

    *value = acc;
    return true;
}

static bool prim_div(ObjectAllocator *a, Object *args, Object **value) {
    guard_is_not_null(a);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    if (object_nil() == args) {
        *value = object_int(a, 1);
        return true;
    }

    auto acc = object_nil();
    object_list_for(arg, args) {
        if (TYPE_INT != arg->type) {
            print_type_error(arg->type, TYPE_INT);
            return false;
        }

        if (object_nil() != acc && 0 == arg->as_int) {
            print_zero_division_error();
            return false;
        }

        acc = object_nil() != acc
              ? object_int(a, acc->as_int / arg->as_int)
              : arg;
    }

    *value = acc;
    return true;
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
        print_args_error("first", len, 1);
        return false;
    }

    auto const list = object_as_cons(args).first;
    if (TYPE_CONS != list->type) {
        print_type_error(list->type, TYPE_CONS);
        return false;
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
        print_args_error("rest", len, 1);
        return false;
    }

    auto const list = object_as_cons(args).first;
    if (TYPE_CONS != list->type) {
        print_type_error(list->type, TYPE_CONS);
        return false;
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
        print_args_error("prepend", object_list_count(args), 2);
        return false;
    }

    if (list->type != TYPE_NIL && list->type != TYPE_CONS) {
        print_type_error(list->type, TYPE_CONS, TYPE_NIL);
        return false;
    }

    *value = object_cons(a, element, list);
    return true;
}

static bool prim_exit(ObjectAllocator *a, Object *args, Object **value) {
    guard_is_not_null(a);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    return false;
}

void define_primitives(ObjectAllocator *a, Object *env) {
    env_define(a, env, object_atom(a, "print"), object_primitive(a, prim_print));
    env_define(a, env, object_atom(a, "+"), object_primitive(a, prim_plus));
    env_define(a, env, object_atom(a, "-"), object_primitive(a, prim_minus));
    env_define(a, env, object_atom(a, "*"), object_primitive(a, prim_mul));
    env_define(a, env, object_atom(a, "/"), object_primitive(a, prim_div));
    env_define(a, env, object_atom(a, "list"), object_primitive(a, prim_list_list));
    env_define(a, env, object_atom(a, "first"), object_primitive(a, prim_list_first));
    env_define(a, env, object_atom(a, "rest"), object_primitive(a, prim_list_rest));
    env_define(a, env, object_atom(a, "prepend"), object_primitive(a, prim_list_prepend));
    env_define(a, env, object_atom(a, "eq?"), object_primitive(a, prim_eq));
    env_define(a, env, object_atom(a, "exit"), object_primitive(a, prim_exit));
}

