#include "primitives.h"

#include <stdio.h>

#include "object_lists.h"
#include "object_constructors.h"
#include "object_accessors.h"
#include "guards.h"
#include "object_env.h"

static bool prim_eq(Object_Allocator *a, Object *args, Object **value, Object **error) {
    guard_is_not_null(a);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);
    guard_is_not_null(error);

    Object *lhs, *rhs;
    if (false == object_list_try_unpack_2(&lhs, &rhs, args)) {
        *error = object_list(
                a,
                object_atom(a, "InvalidCallError"),
                object_atom(a, "prim_eq?"),
                object_list(a, object_atom(a, "expected-args"), object_int(a, 2)),
                object_list(a, object_atom(a, "got-args"), object_int(a, object_list_count(args)))
        );
        return false;
    }

    *value = object_equals(lhs, rhs) ? object_atom(a, "true") : object_nil();
    return true;
}

static bool prim_print(Object_Allocator *a, Object *args, Object **value, Object **error) {
    guard_is_not_null(a);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);
    guard_is_not_null(error);

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

static bool prim_plus(Object_Allocator *a, Object *args, Object **value, Object **error) {
    guard_is_not_null(a);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);
    guard_is_not_null(error);

    auto acc = object_int(a, 0);
    object_list_for(arg, args) {
        if (TYPE_INT != arg->type) {
            *error = object_list(
                    a,
                    object_atom(a, "TypeError"),
                    object_atom(a, "+"),
                    object_list(a, object_atom(a, "expected"), object_atom(a, object_type_str(TYPE_INT))),
                    object_list(a, object_atom(a, "got"), object_atom(a, object_type_str(arg->type)))
            );
            return false;
        }

        acc = object_int(a, acc->as_int + arg->as_int);
    }

    *value = acc;
    return true;
}

static bool prim_minus(Object_Allocator *a, Object *args, Object **value, Object **error) {
    guard_is_not_null(a);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);
    guard_is_not_null(error);

    if (object_nil() == args) {
        *value = object_int(a, 0);
        return true;
    }

    auto acc = object_nil();
    object_list_for(arg, args) {
        if (TYPE_INT != arg->type) {
            *error = object_list(
                    a,
                    object_atom(a, "TypeError"),
                    object_atom(a, "-"),
                    object_list(a, object_atom(a, "expected"), object_atom(a, object_type_str(TYPE_INT))),
                    object_list(a, object_atom(a, "got"), object_atom(a, object_type_str(arg->type)))
            );
            return false;
        }

        acc = object_nil() != acc
              ? object_int(a, acc->as_int - arg->as_int)
              : arg;
    }

    *value = acc;
    return true;
}

static bool prim_mul(Object_Allocator *a, Object *args, Object **value, Object **error) {
    guard_is_not_null(a);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);
    guard_is_not_null(error);

    auto acc = object_int(a, 1);
    object_list_for(arg, args) {
        if (TYPE_INT != arg->type) {
            *error = object_list(
                    a,
                    object_atom(a, "TypeError"),
                    object_atom(a, "*"),
                    object_list(a, object_atom(a, "expected"), object_atom(a, object_type_str(TYPE_INT))),
                    object_list(a, object_atom(a, "got"), object_atom(a, object_type_str(arg->type)))
            );
            return false;
        }

        acc = object_int(a, acc->as_int * arg->as_int);
    }

    *value = acc;
    return true;
}

static bool prim_div(Object_Allocator *a, Object *args, Object **value, Object **error) {
    guard_is_not_null(a);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);
    guard_is_not_null(error);

    if (object_nil() == args) {
        *value = object_int(a, 1);
        return true;
    }

    auto acc = object_nil();
    object_list_for(arg, args) {
        if (TYPE_INT != arg->type) {
            *error = object_list(
                    a,
                    object_atom(a, "TypeError"),
                    object_atom(a, "/"),
                    object_list(a, object_atom(a, "expected"), object_atom(a, object_type_str(TYPE_INT))),
                    object_list(a, object_atom(a, "got"), object_atom(a, object_type_str(arg->type)))
            );
            return false;
        }

        if (object_nil() != acc && 0 == arg->as_int) {
            *error = object_list(
                    a,
                    object_atom(a, "ZeroDivisionError"),
                    object_atom(a, "division by zero")
            );
            return false;
        }

        acc = object_nil() != acc
              ? object_int(a, acc->as_int / arg->as_int)
              : arg;
    }

    *value = acc;
    return true;
}

static bool prim_list_list(Object_Allocator *a, Object *args, Object **value, Object **error) {
    guard_is_not_null(a);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);
    guard_is_not_null(error);

    *value = args;
    return true;
}

static bool prim_list_first(Object_Allocator *a, Object *args, Object **value, Object **error) {
    guard_is_not_null(a);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);
    guard_is_not_null(error);

    auto const len = object_list_count(args);
    if (1 != len) {
        *error = object_list(
                a,
                object_atom(a, "InvalidCallError"),
                object_atom(a, "first"),
                object_list(a, object_atom(a, "expected-args"), object_int(a, 1)),
                object_list(a, object_atom(a, "got-args"), object_int(a, len))
        );
        return false;
    }

    auto const list = object_as_cons(args).first;
    if (TYPE_CONS != list->type) {
        *error = object_list(
                a,
                object_atom(a, "TypeError"),
                object_list(a, object_atom(a, "expected"), object_atom(a, object_type_str(TYPE_CONS))),
                object_list(a, object_atom(a, "got"), object_atom(a, object_type_str(list->type)))
        );
        return false;
    }

    *value = object_as_cons(list).first;
    return true;
}

static bool prim_list_rest(Object_Allocator *a, Object *args, Object **value, Object **error) {
    guard_is_not_null(a);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);
    guard_is_not_null(error);

    auto const len = object_list_count(args);
    if (1 != len) {
        *error = object_list(
                a,
                object_atom(a, "InvalidCallError"),
                object_atom(a, "rest"),
                object_list(a, object_atom(a, "expected-args"), object_int(a, 1)),
                object_list(a, object_atom(a, "got-args"), object_int(a, len))
        );
        return false;
    }

    auto const list = object_as_cons(args).first;
    if (TYPE_CONS != list->type) {
        *error = object_list(
                a,
                object_atom(a, "TypeError"),
                object_list(a, object_atom(a, "expected"), object_atom(a, object_type_str(TYPE_CONS))),
                object_list(a, object_atom(a, "got"), object_atom(a, object_type_str(list->type)))
        );
        return false;
    }

    *value = object_as_cons(list).rest;
    return true;
}

static bool prim_list_prepend(Object_Allocator *a, Object *args, Object **value, Object **error) {
    guard_is_not_null(a);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);
    guard_is_not_null(error);

    Object *element, *list;
    if (false == object_list_try_unpack_2(&element, &list, args)) {
        *error = object_list(
                a,
                object_atom(a, "InvalidCallError"),
                object_atom(a, "prepend"),
                object_list(a, object_atom(a, "expected-args"), object_int(a, 2)),
                object_list(a, object_atom(a, "got-args"), object_int(a, object_list_count(args)))
        );
        return false;
    }

    if (list->type != TYPE_NIL && list->type != TYPE_CONS) {
        *error = object_list(
                a,
                object_atom(a, "TypeError"),
                object_list(
                        a,
                        object_atom(a, "expected"),
                        object_atom(a, object_type_str(TYPE_CONS)),
                        object_atom(a, object_type_str(TYPE_NIL))
                ),
                object_list(a, object_atom(a, "got"), object_atom(a, object_type_str(list->type)))
        );
        return false;
    }

    *value = object_cons(a, element, list);
    return true;
}

static bool prim_exit(Object_Allocator *a, Object *args, Object **value, Object **error) {
    guard_is_not_null(a);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);
    guard_is_not_null(error);

    *error = object_list(a, object_atom(a, "Exit"));
    return false;
}

void define_primitives(Object_Allocator *a, Object *env) {
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

