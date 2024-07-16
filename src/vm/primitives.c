#include "primitives.h"

#include <stdio.h>

#include "utility/guards.h"
#include "object/lists.h"
#include "object/constructors.h"
#include "object/accessors.h"
#include "env.h"
#include "errors.h"

#define error(PrintErrorFn, Args) do { PrintErrorFn Args; return false; } while (false)

static bool prim_eq(VirtualMachine *vm, Object *args, Object **value) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    Object *lhs, *rhs;
    if (false == object_list_try_unpack_2(&lhs, &rhs, args)) {
        call_error(vm, "eq?", 2, object_list_count(args));
    }

    *value = vm_get(vm, object_equals(lhs, rhs) ? STATIC_TRUE : STATIC_FALSE);
    return true;
}

static bool prim_repr(VirtualMachine *vm, Object *args, Object **value) {
    guard_is_not_null(vm);
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

static bool prim_print(VirtualMachine *vm, Object *args, Object **value) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    object_list_for(it, args) {
        object_print(it, stdout);
        printf(" ");
    }
    printf("\n");

    *value = object_nil();
    return true;
}

static bool prim_plus(VirtualMachine *vm, Object *args, Object **value) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    int64_t acc = 0;
    object_list_for(arg, args) {
        if (TYPE_INT != arg->type) {
            type_error(vm, arg->type, TYPE_INT);
        }

        acc += arg->as_int;
    }

    if (object_try_make_int(vm_allocator(vm), acc, value)) {
        return true;
    }

    out_of_memory_error(vm);
}

static bool prim_minus(VirtualMachine *vm, Object *args, Object **value) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    if (object_nil() == args) {
        if (object_try_make_int(vm_allocator(vm), 0, value)) {
            return true;
        }

        out_of_memory_error(vm);
    }

    auto const first = args->as_cons.first;
    if (TYPE_INT != first->type) {
        type_error(vm, first->type, TYPE_INT);
    }

    auto acc = first->as_int;
    object_list_for(arg, args->as_cons.rest) {
        if (TYPE_INT != arg->type) {
            type_error(vm, arg->type, TYPE_INT);
        }

        acc -= arg->as_int;
    }

    if (object_try_make_int(vm_allocator(vm), acc, value)) {
        return true;
    }

    out_of_memory_error(vm);
}

static bool prim_mul(VirtualMachine *vm, Object *args, Object **value) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    int64_t acc = 1;
    object_list_for(arg, args) {
        if (TYPE_INT != arg->type) {
            type_error(vm, arg->type, TYPE_INT);
        }

        acc *= arg->as_int;
    }

    if (object_try_make_int(vm_allocator(vm), acc, value)) {
        return true;
    }

    out_of_memory_error(vm);
}

static bool prim_div(VirtualMachine *vm, Object *args, Object **value) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    if (object_nil() == args) {
        if (object_try_make_int(vm_allocator(vm), 1, value)) {
            return true;
        }

        out_of_memory_error(vm);
    }

    auto const first = args->as_cons.first;
    if (TYPE_INT != first->type) {
        type_error(vm, first->type, TYPE_INT);
    }

    auto acc = first->as_int;
    object_list_for(arg, args->as_cons.rest) {
        if (TYPE_INT != arg->type) {
            type_error(vm, arg->type, TYPE_INT);
        }

        if (0 == arg->as_int) {
            zero_division_error(vm);
        }

        acc /= arg->as_int;
    }

    if (object_try_make_int(vm_allocator(vm), acc, value)) {
        return true;
    }

    out_of_memory_error(vm);
}

static bool prim_list_list(VirtualMachine *vm, Object *args, Object **value) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    *value = args;
    return true;
}

static bool prim_list_first(VirtualMachine *vm, Object *args, Object **value) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    auto const got = object_list_count(args);
    typeof(got) expected = 1;
    if (expected != got) {
        call_error(vm, "first", expected, got);
    }

    auto const list = object_as_cons(args).first;
    if (TYPE_CONS != list->type) {
        type_error(vm, list->type, TYPE_CONS);
    }

    *value = object_as_cons(list).first;
    return true;
}

static bool prim_list_rest(VirtualMachine *vm, Object *args, Object **value) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    auto const got = object_list_count(args);
    typeof(got) expected = 1;
    if (expected != got) {
        call_error(vm, "rest", expected, got);
    }

    auto const list = object_as_cons(args).first;
    if (TYPE_CONS != list->type) {
        type_error(vm, list->type, TYPE_CONS);
    }

    *value = object_as_cons(list).rest;
    return true;
}

static bool prim_list_prepend(VirtualMachine *vm, Object *args, Object **value) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    Object *element, *list;
    if (false == object_list_try_unpack_2(&element, &list, args)) {
        call_error(vm, "prepend", 2, object_list_count(args));
    }

    if (list->type != TYPE_NIL && list->type != TYPE_CONS) {
        type_error(vm, list->type, TYPE_CONS);
    }

    if (object_try_make_cons(vm_allocator(vm), element, list, value)) {
        return true;
    }

    out_of_memory_error(vm);
}

static bool prim_list_reverse(VirtualMachine *vm, Object *args, Object **value) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    auto const got = object_list_count(args);
    typeof(got) expected = 1;
    if (expected != got) {
        call_error(vm, "reverse", expected, got);
    }

    auto const list = object_as_cons(args).first;
    if (TYPE_CONS != list->type && TYPE_NIL != list->type) {
        type_error(vm, list->type, TYPE_CONS, TYPE_NIL);
    }

    if (false == object_try_copy(vm_allocator(vm), list, value)) {
        out_of_memory_error(vm);
    }

    object_list_reverse(value);
    return true;
}

static bool prim_list_concat(VirtualMachine *vm, Object *args, Object **value) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    auto rest = value;
    object_list_for(it, args) {
        if (TYPE_CONS != it->type && TYPE_NIL != it->type) {
            type_error(vm, it->type, TYPE_CONS, TYPE_NIL);
        }

        if (false == object_try_copy(vm_allocator(vm), it, rest)) {
            out_of_memory_error(vm);
        }

        rest = object_list_last(value);
    }

    return true;
}

static bool prim_not(VirtualMachine *vm, Object *args, Object **value) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    auto const got = object_list_count(args);
    typeof(got) expected = 1;
    if (expected != got) {
        call_error(vm, "not", expected, got);
    }

    *value = vm_get(vm, object_nil() == args->as_cons.first ? STATIC_TRUE : STATIC_FALSE);

    return true;
}

static bool try_define(ObjectAllocator *a, Object *env, char const *name, Object_Primitive value) {
    Object *binding;
    return env_try_define(a, env, object_nil(), object_nil(), &binding)
           && object_try_make_atom(a, name, object_list_nth(0, binding))
           && object_try_make_primitive(a, value, object_list_nth(1, binding));
}

bool try_define_primitives(ObjectAllocator *a, Object *env) {
    return try_define(a, env, "repr", prim_repr)
           && try_define(a, env, "print", prim_print)
           && try_define(a, env, "+", prim_plus)
           && try_define(a, env, "-", prim_minus)
           && try_define(a, env, "*", prim_mul)
           && try_define(a, env, "/", prim_div)
           && try_define(a, env, "list", prim_list_list)
           && try_define(a, env, "first", prim_list_first)
           && try_define(a, env, "rest", prim_list_rest)
           && try_define(a, env, "prepend", prim_list_prepend)
           && try_define(a, env, "reverse", prim_list_reverse)
           && try_define(a, env, "concat", prim_list_concat)
           && try_define(a, env, "eq?", prim_eq)
           && try_define(a, env, "not", prim_not);
}

