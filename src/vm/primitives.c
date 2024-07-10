#include "primitives.h"

#include <stdio.h>

#include "utility/guards.h"
#include "object/lists.h"
#include "object/constructors.h"
#include "object/accessors.h"
#include "object/env.h"
#include "vm/errors.h"

#define error(PrintErrorFn, Args) do { PrintErrorFn Args; return false; } while (false)

static bool prim_eq(VirtualMachine *vm, Object *args, Object **value, Object **error) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);
    guard_is_not_null(error);

    Object *lhs, *rhs;
    if (false == object_list_try_unpack_2(&lhs, &rhs, args)) {
        create_call_error(vm, "prim_eq?", 2, object_list_count(args), error);
        return false;
    }

    if (false == object_equals(lhs, rhs)) {
        *value = object_nil();
        return true;
    }

    if (object_try_make_atom(vm_allocator(vm), "true", value)) {
        return true;
    }

    create_out_of_memory_error(vm, error);
    return false;
}

static bool prim_print(VirtualMachine *vm, Object *args, Object **value, Object **error) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);
    guard_is_not_null(error);

    object_list_for(it, args) {
        object_repr_print(it, stdout);
        printf(" ");
    }
    printf("\n");

    *value = object_nil();
    return true;
}

static bool prim_plus(VirtualMachine *vm, Object *args, Object **value, Object **error) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);
    guard_is_not_null(error);

    int64_t acc = 0;
    object_list_for(arg, args) {
        if (TYPE_INT != arg->type) {
            create_type_error(vm, error, arg->type, TYPE_INT);
            return false;
        }

        acc += arg->as_int;
    }

    if (object_try_make_int(vm_allocator(vm), acc, value)) {
        return true;
    }

    create_out_of_memory_error(vm, error);
    return false;
}

static bool prim_minus(VirtualMachine *vm, Object *args, Object **value, Object **error) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);
    guard_is_not_null(error);

    if (object_nil() == args) {
        if (object_try_make_int(vm_allocator(vm), 0, value)) {
            return true;
        }

        create_out_of_memory_error(vm, error);
        return false;
    }

    auto const first = args->as_cons.first;
    if (TYPE_INT != first->type) {
        create_type_error(vm, error, first->type, TYPE_INT);
        return false;
    }

    auto acc = first->as_int;
    object_list_for(arg, args->as_cons.rest) {
        if (TYPE_INT != arg->type) {
            create_type_error(vm, error, arg->type, TYPE_INT);
            return false;
        }

        acc -= arg->as_int;
    }

    if (object_try_make_int(vm_allocator(vm), acc, value)) {
        return true;
    }

    create_out_of_memory_error(vm, error);
    return false;
}

static bool prim_mul(VirtualMachine *vm, Object *args, Object **value, Object **error) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);
    guard_is_not_null(error);

    int64_t acc = 1;
    object_list_for(arg, args) {
        if (TYPE_INT != arg->type) {
            create_type_error(vm, error, arg->type, TYPE_INT);
            return false;
        }

        acc *= arg->as_int;
    }

    if (object_try_make_int(vm_allocator(vm), acc, value)) {
        return true;
    }

    create_out_of_memory_error(vm, error);
    return false;
}

static bool prim_div(VirtualMachine *vm, Object *args, Object **value, Object **error) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);
    guard_is_not_null(error);

    if (object_nil() == args) {
        if (object_try_make_int(vm_allocator(vm), 1, value)) {
            return true;
        }

        create_out_of_memory_error(vm, error);
        return false;
    }

    auto const first = args->as_cons.first;
    if (TYPE_INT != first->type) {
        create_type_error(vm, error, first->type, TYPE_INT);
        return false;
    }

    auto acc = first->as_int;
    object_list_for(arg, args->as_cons.rest) {
        if (TYPE_INT != arg->type) {
            create_type_error(vm, error, arg->type, TYPE_INT);
            return false;
        }

        if (0 == arg->as_int) {
            create_zero_division_error(vm, error);
            return false;
        }

        acc /= arg->as_int;
    }

    if (object_try_make_int(vm_allocator(vm), acc, value)) {
        return true;
    }

    create_out_of_memory_error(vm, error);
    return false;
}

static bool prim_list_list(VirtualMachine *vm, Object *args, Object **value, Object **error) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);
    guard_is_not_null(error);

    *value = args;
    return true;
}

static bool prim_list_first(VirtualMachine *vm, Object *args, Object **value, Object **error) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);
    guard_is_not_null(error);

    auto const got = object_list_count(args);
    typeof(got) expected = 1;
    if (expected != got) {
        create_call_error(vm, "first", expected, got, error);
        return false;
    }

    auto const list = object_as_cons(args).first;
    if (TYPE_CONS != list->type) {
        create_type_error(vm, error, list->type, TYPE_CONS);
        return false;
    }

    *value = object_as_cons(list).first;
    return true;
}

static bool prim_list_rest(VirtualMachine *vm, Object *args, Object **value, Object **error) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);
    guard_is_not_null(error);

    auto const got = object_list_count(args);
    typeof(got) expected = 1;
    if (expected != got) {
        create_call_error(vm, "rest", expected, got, error);
    }

    auto const list = object_as_cons(args).first;
    if (TYPE_CONS != list->type) {
        create_type_error(vm, error, list->type, TYPE_CONS);
        return false;
    }

    *value = object_as_cons(list).rest;
    return true;
}

static bool prim_list_prepend(VirtualMachine *vm, Object *args, Object **value, Object **error) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);
    guard_is_not_null(error);

    Object *element, *list;
    if (false == object_list_try_unpack_2(&element, &list, args)) {
        create_call_error(vm, "prepend", 2, object_list_count(args), error);
        return false;
    }

    if (list->type != TYPE_NIL && list->type != TYPE_CONS) {
        create_type_error(vm, error, list->type, TYPE_CONS);
        return false;
    }

    if (object_try_make_cons(vm_allocator(vm), element, list, value)) {
        return true;
    }

    create_out_of_memory_error(vm, error);
    return false;
}

static bool try_define(ObjectAllocator *a, Object *env, char const *name, Object_Primitive value) {
    Object *binding;
    return env_try_define(a, env, object_nil(), object_nil(), &binding)
           && object_try_make_atom(a, name, object_list_nth(0, binding))
           && object_try_make_primitive(a, value, object_list_nth(1, binding));
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

