#include "primitives.h"

#include <stdio.h>

#include "utility/guards.h"
#include "object/lists.h"
#include "object/constructors.h"
#include "object/accessors.h"
#include "env.h"
#include "traceback.h"
#include "errors.h"

#define error(PrintErrorFn, Args) do { PrintErrorFn Args; return false; } while (false)

static bool eq(VirtualMachine *vm, Object *args, Object **value) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    Object *lhs, *rhs;
    if (false == object_list_try_unpack_2(&lhs, &rhs, args)) {
        call_error(vm, "eq?", 2, false, object_list_count(args));
    }

    *value = vm_get(vm, object_equals(lhs, rhs) ? STATIC_TRUE : STATIC_FALSE);
    return true;
}

static bool repr(VirtualMachine *vm, Object *args, Object **value) {
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

static bool print(VirtualMachine *vm, Object *args, Object **value) {
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

static bool plus(VirtualMachine *vm, Object *args, Object **value) {
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

static bool minus(VirtualMachine *vm, Object *args, Object **value) {
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

static bool multiply(VirtualMachine *vm, Object *args, Object **value) {
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

static bool divide(VirtualMachine *vm, Object *args, Object **value) {
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

static bool list_list(VirtualMachine *vm, Object *args, Object **value) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    *value = args;
    return true;
}

static bool list_first(VirtualMachine *vm, Object *args, Object **value) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    auto const got = object_list_count(args);
    typeof(got) expected = 1;
    if (expected != got) {
        call_error(vm, "first", expected, false, got);
    }

    auto const list = object_as_cons(args).first;
    if (TYPE_CONS != list->type) {
        type_error(vm, list->type, TYPE_CONS);
    }

    *value = object_as_cons(list).first;
    return true;
}

static bool list_rest(VirtualMachine *vm, Object *args, Object **value) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    auto const got = object_list_count(args);
    typeof(got) expected = 1;
    if (expected != got) {
        call_error(vm, "rest", expected, false, got);
    }

    auto const list = object_as_cons(args).first;
    if (TYPE_CONS != list->type) {
        type_error(vm, list->type, TYPE_CONS);
    }

    *value = object_as_cons(list).rest;
    return true;
}

static bool list_prepend(VirtualMachine *vm, Object *args, Object **value) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    Object *element, *list;
    if (false == object_list_try_unpack_2(&element, &list, args)) {
        call_error(vm, "prepend", 2, false, object_list_count(args));
    }

    if (list->type != TYPE_NIL && list->type != TYPE_CONS) {
        type_error(vm, list->type, TYPE_CONS);
    }

    if (object_try_make_cons(vm_allocator(vm), element, list, value)) {
        return true;
    }

    out_of_memory_error(vm);
}

static bool list_reverse(VirtualMachine *vm, Object *args, Object **value) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    auto const got = object_list_count(args);
    typeof(got) expected = 1;
    if (expected != got) {
        call_error(vm, "reverse", expected, false, got);
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

static bool list_concat(VirtualMachine *vm, Object *args, Object **value) {
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

static bool not(VirtualMachine *vm, Object *args, Object **value) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    auto const got = object_list_count(args);
    typeof(got) expected = 1;
    if (expected != got) {
        call_error(vm, "not", expected, false, got);
    }

    *value = vm_get(vm, object_nil() == args->as_cons.first ? STATIC_TRUE : STATIC_FALSE);

    return true;
}

static bool type(VirtualMachine *vm, Object *args, Object **value) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    auto const got = object_list_count(args);
    typeof(got) expected = 1;
    if (expected != got) {
        call_error(vm, "type", expected, false, got);
    }

    auto const arg = args->as_cons.first;
    return object_try_make_atom(vm_allocator(vm), object_type_str(arg->type), value);
}

static bool traceback(VirtualMachine *vm, Object *args, Object **value) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    auto const got = object_list_count(args);
    typeof(got) expected = 0;
    if (expected != got) {
        call_error(vm, "traceback", expected, false, got);
    }

    if (false == traceback_try_get(vm_allocator(vm), vm_stack(vm), value)) {
        out_of_memory_error(vm);
    }
    object_list_shift(value);

    return true;
}

static bool throw(VirtualMachine *vm, Object *args, Object **value) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    auto const got = object_list_count(args);
    typeof(got) expected = 1;
    if (expected != got) {
        call_error(vm, "throw", expected, false, got);
    }

    *vm_error(vm) = args->as_cons.first;
    return false;
}

static bool try_define(ObjectAllocator *a, Object *env, char const *name, Object_Primitive value) {
    Object *binding;
    return env_try_define(a, env, object_nil(), object_nil(), &binding)
           && object_try_make_atom(a, name, object_list_nth(0, binding))
           && object_try_make_primitive(a, value, object_list_nth(1, binding));
}

bool try_define_primitives(ObjectAllocator *a, Object *env) {
    return try_define(a, env, "repr", repr)
           && try_define(a, env, "print", print)
           && try_define(a, env, "+", plus)
           && try_define(a, env, "-", minus)
           && try_define(a, env, "*", multiply)
           && try_define(a, env, "/", divide)
           && try_define(a, env, "list", list_list)
           && try_define(a, env, "first", list_first)
           && try_define(a, env, "rest", list_rest)
           && try_define(a, env, "prepend", list_prepend)
           && try_define(a, env, "reverse", list_reverse)
           && try_define(a, env, "concat", list_concat)
           && try_define(a, env, "eq?", eq)
           && try_define(a, env, "not", not)
           && try_define(a, env, "type", type)
           && try_define(a, env, "traceback", traceback)
           && try_define(a, env, "throw", throw);
}

