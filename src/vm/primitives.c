#include "primitives.h"

#include <stdio.h>

#include "utility/guards.h"
#include "object/lists.h"
#include "object/dict.h"
#include "object/constructors.h"
#include "object/accessors.h"
#include "object/repr.h"
#include "object/compare.h"
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

static bool str(VirtualMachine *vm, Object *args, Object **value) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    if (object_nil() == args) {
        if (false == object_try_make_string(vm_allocator(vm), "", value)) {
            out_of_memory_error(vm);
        }

        return true;
    }

    auto sb = (StringBuilder) {0};
    object_list_for(it, args) {
        errno_t error_code;
        if (false == object_try_print(it, &sb, &error_code)) {
            sb_free(&sb);
            os_error(vm, error_code);
        }
    }

    if (false == object_try_make_string(vm_allocator(vm), sb.str, value)) {
        sb_free(&sb);
        out_of_memory_error(vm);
    }

    sb_free(&sb);
    return true;
}

static bool repr(VirtualMachine *vm, Object *args, Object **value) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    auto const got = object_list_count(args);
    typeof(got) expected = 1;
    if (expected != got) {
        call_error(vm, "repr", expected, false, got);
    }

    auto sb = (StringBuilder) {0};

    errno_t error_code;
    if (false == object_try_repr(args->as_cons.first, &sb, &error_code)) {
        sb_free(&sb);
        os_error(vm, error_code);
    }

    if (false == object_try_make_string(vm_allocator(vm), sb.str, value)) {
        sb_free(&sb);
        out_of_memory_error(vm);
    }

    sb_free(&sb);
    return true;
}

static bool print(VirtualMachine *vm, Object *args, Object **value) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    if (object_nil() == args) {
        return true;
    }

    errno_t error_code;
    if (false == object_try_print(args->as_cons.first, stdout, &error_code)) {
        os_error(vm, error_code);
    }

    object_list_for(it, args->as_cons.rest) {
        printf(" ");
        if (false == object_try_print(it, stdout, &error_code)) {
            os_error(vm, error_code);
        }
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

    if (false == traceback_try_get(vm_allocator(vm), *vm_stack(vm), value)) {
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

    auto const error = args->as_cons.first;
    if (TYPE_NIL == error->type) {
        type_error_unexpected(vm, error->type);
    }

    *vm_error(vm) = error;
    return false;
}

static bool dict_get(VirtualMachine *vm, Object *args, Object **value) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(value);

    Object *key, *dict;
    if (false == object_list_try_unpack_2(&key, &dict, args)) {
        call_error(vm, "get", 2, false, object_list_count(args));
    }

    if (TYPE_DICT != dict->type) {
        type_error(vm, dict->type, TYPE_DICT);
    }

    Object_DictError error;
    if (false == object_dict_try_get(dict, key, value, &error)) {
        key_error(vm, key);
    }

    return true;
}

static bool dict_dict(VirtualMachine *vm, Object *args, Object **result) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(result);

    auto const got = object_list_count(args);
    if (0 != got % 2) {
        call_parity_error(vm, "dict", true);
    }

    if (false == object_try_make_dict_entries(vm_allocator(vm), /* TODO named constant */ 32, result)) {
        out_of_memory_error(vm);
    }

    if (false == object_try_make_dict(vm_allocator(vm), *result, result)) {
        out_of_memory_error(vm);
    }

    while (object_nil() != args) {
        auto const key = object_list_shift(&args);
        auto const value = object_list_shift(&args);

        Object_DictError error;
        if (false == object_dict_try_put(vm_allocator(vm), *result, key, value, &error)) {
            // FIXME type error, key error or allocation error
            key_error(vm, key);
        }
    }

    return true;
}

static bool dict_put(VirtualMachine *vm, Object *args, Object **result) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_CONS, TYPE_NIL);
    guard_is_not_null(result);

    auto const got = object_list_count(args);
    typeof(got) expected = 3;
    if (expected != got) {
        call_error(vm, "put", expected, false, got);
    }

    auto const key = *object_list_nth(0, args);
    auto const value = *object_list_nth(1, args);
    auto const dict = *object_list_nth(2, args);
    if (TYPE_DICT != dict->type) {
        type_error(vm, dict->type, TYPE_DICT);
    }

    Object_DictError error;
    if (false == object_dict_try_put(vm_allocator(vm), dict, key, value, &error)) {
        // FIXME type error, key error or allocation error
        out_of_memory_error(vm);
    }

    *result = dict;
    return true;
}

static bool try_define(ObjectAllocator *a, Object *env, char const *name, Object_Primitive value) {
    Object *binding;
    return env_try_define(a, env, object_nil(), object_nil(), &binding)
           && object_try_make_atom(a, name, object_list_nth(0, binding))
           && object_try_make_primitive(a, value, object_list_nth(1, binding));
}

bool try_define_primitives(ObjectAllocator *a, Object *env) {
    return try_define(a, env, "eq?", eq)
           && try_define(a, env, "str", str)
           && try_define(a, env, "repr", repr)
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
           && try_define(a, env, "dict", dict_dict)
           && try_define(a, env, "get", dict_get)
           && try_define(a, env, "put", dict_put)
           && try_define(a, env, "not", not)
           && try_define(a, env, "type", type)
           && try_define(a, env, "traceback", traceback)
           && try_define(a, env, "throw", throw);
}

