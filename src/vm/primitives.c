#include "primitives.h"

#include <stdio.h>

#include "utility/guards.h"
#include "object/list.h"
#include "object/dict.h"
#include "object/constructors.h"
#include "object/accessors.h"
#include "object/repr.h"
#include "object/compare.h"
#include "env.h"
#include "traceback.h"
#include "errors.h"

static bool eq(VirtualMachine *vm, Object *args, Object **value) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_LIST, TYPE_NIL);
    guard_is_not_null(value);

    Object *lhs, *rhs;
    if (false == object_list_try_unpack_2(&lhs, &rhs, args)) {
        call_args_count_error(vm, "eq?", 2, object_list_count(args));
    }

    *value = object_equals(lhs, rhs)
             ? OBJECT_TRUE
             : OBJECT_NIL;
    return true;
}

static bool compare(VirtualMachine *vm, Object *args, Object **value) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_LIST, TYPE_NIL);
    guard_is_not_null(value);

    Object *lhs, *rhs;
    if (false == object_list_try_unpack_2(&lhs, &rhs, args)) {
        call_args_count_error(vm, "compare", 2, object_list_count(args));
    }

    auto const compare_result = object_compare(lhs, rhs);
    if (object_try_make_int(vm_allocator(vm), compare_result, value)) {
        return true;
    }

    out_of_memory_error(vm);
}

static bool str(VirtualMachine *vm, Object *args, Object **value) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_LIST, TYPE_NIL);
    guard_is_not_null(value);

    if (OBJECT_NIL == args) {
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
    guard_is_one_of(args->type, TYPE_LIST, TYPE_NIL);
    guard_is_not_null(value);

    auto const got = object_list_count(args);
    typeof(got) expected = 1;
    if (expected != got) {
        call_args_count_error(vm, "repr", expected, got);
    }

    auto sb = (StringBuilder) {0};

    errno_t error_code;
    if (false == object_try_repr(args->as_list.first, &sb, &error_code)) {
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
    guard_is_one_of(args->type, TYPE_LIST, TYPE_NIL);
    guard_is_not_null(value);

    if (OBJECT_NIL == args) {
        return true;
    }

    errno_t error_code;
    if (false == object_try_print(args->as_list.first, stdout, &error_code)) {
        os_error(vm, error_code);
    }

    object_list_for(it, args->as_list.rest) {
        printf(" ");
        if (false == object_try_print(it, stdout, &error_code)) {
            os_error(vm, error_code);
        }
    }
    printf("\n");

    *value = OBJECT_NIL;
    return true;
}

static bool plus(VirtualMachine *vm, Object *args, Object **value) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_LIST, TYPE_NIL);
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
    guard_is_one_of(args->type, TYPE_LIST, TYPE_NIL);
    guard_is_not_null(value);

    if (OBJECT_NIL == args) {
        if (object_try_make_int(vm_allocator(vm), 0, value)) {
            return true;
        }

        out_of_memory_error(vm);
    }

    auto const first = args->as_list.first;
    if (TYPE_INT != first->type) {
        type_error(vm, first->type, TYPE_INT);
    }

    auto acc = first->as_int;
    object_list_for(arg, args->as_list.rest) {
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
    guard_is_one_of(args->type, TYPE_LIST, TYPE_NIL);
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
    guard_is_one_of(args->type, TYPE_LIST, TYPE_NIL);
    guard_is_not_null(value);

    if (OBJECT_NIL == args) {
        if (object_try_make_int(vm_allocator(vm), 1, value)) {
            return true;
        }

        out_of_memory_error(vm);
    }

    auto const first = args->as_list.first;
    if (TYPE_INT != first->type) {
        type_error(vm, first->type, TYPE_INT);
    }

    auto acc = first->as_int;
    object_list_for(arg, args->as_list.rest) {
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
    guard_is_one_of(args->type, TYPE_LIST, TYPE_NIL);
    guard_is_not_null(value);

    *value = args;
    return true;
}

static bool list_first(VirtualMachine *vm, Object *args, Object **value) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_LIST, TYPE_NIL);
    guard_is_not_null(value);

    auto const got = object_list_count(args);
    typeof(got) expected = 1;
    if (expected != got) {
        call_args_count_error(vm, "first", expected, got);
    }

    auto const list = object_as_list(args).first;
    if (TYPE_LIST != list->type) {
        type_error(vm, list->type, TYPE_LIST);
    }

    *value = object_as_list(list).first;
    return true;
}

static bool list_rest(VirtualMachine *vm, Object *args, Object **value) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_LIST, TYPE_NIL);
    guard_is_not_null(value);

    auto const got = object_list_count(args);
    typeof(got) expected = 1;
    if (expected != got) {
        call_args_count_error(vm, "rest", expected, got);
    }

    auto const list = object_as_list(args).first;
    if (TYPE_LIST != list->type) {
        type_error(vm, list->type, TYPE_LIST);
    }

    *value = object_as_list(list).rest;
    return true;
}

static bool list_prepend(VirtualMachine *vm, Object *args, Object **value) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_LIST, TYPE_NIL);
    guard_is_not_null(value);

    Object *element, *list;
    if (false == object_list_try_unpack_2(&element, &list, args)) {
        call_args_count_error(vm, "prepend", 2, object_list_count(args));
    }

    if (list->type != TYPE_NIL && list->type != TYPE_LIST) {
        type_error(vm, list->type, TYPE_LIST, TYPE_NIL);
    }

    if (object_try_make_list(vm_allocator(vm), element, list, value)) {
        return true;
    }

    out_of_memory_error(vm);
}

static bool list_reverse(VirtualMachine *vm, Object *args, Object **value) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_LIST, TYPE_NIL);
    guard_is_not_null(value);

    auto const got = object_list_count(args);
    typeof(got) expected = 1;
    if (expected != got) {
        call_args_count_error(vm, "reverse", expected, got);
    }

    auto const list = object_as_list(args).first;
    if (TYPE_LIST != list->type && TYPE_NIL != list->type) {
        type_error(vm, list->type, TYPE_LIST, TYPE_NIL);
    }

    if (false == object_try_deep_copy(vm_allocator(vm), list, value)) {
        out_of_memory_error(vm);
    }

    object_list_reverse_inplace(value);
    return true;
}

static bool list_concat(VirtualMachine *vm, Object *args, Object **value) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_LIST, TYPE_NIL);
    guard_is_not_null(value);

    auto rest = value;
    object_list_for(it, args) {
        if (TYPE_LIST != it->type && TYPE_NIL != it->type) {
            type_error(vm, it->type, TYPE_LIST, TYPE_NIL);
        }

        if (false == object_try_deep_copy(vm_allocator(vm), it, rest)) {
            out_of_memory_error(vm);
        }

        rest = object_list_end_mutable(value);
    }

    return true;
}

static bool not(VirtualMachine *vm, Object *args, Object **value) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_LIST, TYPE_NIL);
    guard_is_not_null(value);

    auto const got = object_list_count(args);
    typeof(got) expected = 1;
    if (expected != got) {
        call_args_count_error(vm, "not", expected, got);
    }

    *value = OBJECT_NIL == args->as_list.first ? OBJECT_TRUE : OBJECT_NIL;

    return true;
}

static bool type(VirtualMachine *vm, Object *args, Object **value) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_LIST, TYPE_NIL);
    guard_is_not_null(value);

    auto const got = object_list_count(args);
    typeof(got) expected = 1;
    if (expected != got) {
        call_args_count_error(vm, "type", expected, got);
    }

    auto const arg = args->as_list.first;
    return object_try_make_symbol(vm_allocator(vm), object_type_str(arg->type), value);
}

static bool traceback(VirtualMachine *vm, Object *args, Object **value) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_LIST, TYPE_NIL);
    guard_is_not_null(value);

    auto const got = object_list_count(args);
    typeof(got) expected = 0;
    if (expected != got) {
        call_args_count_error(vm, "traceback", expected, got);
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
    guard_is_one_of(args->type, TYPE_LIST, TYPE_NIL);
    guard_is_not_null(value);

    auto const got = object_list_count(args);
    typeof(got) expected = 1;
    if (expected != got) {
        call_args_count_error(vm, "throw", expected, got);
    }

    auto const error = args->as_list.first;
    if (TYPE_NIL == error->type) {
        type_error(vm, error->type);
    }

    *vm_error(vm) = error;
    return false;
}

static bool dict_dict(VirtualMachine *vm, Object *args, Object **result) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_LIST, TYPE_NIL);
    guard_is_not_null(result);

    *result = OBJECT_NIL;

    while (OBJECT_NIL != args) {
        auto const key = object_list_shift(&args);
        auto const value = OBJECT_NIL == args ? OBJECT_NIL : object_list_shift(&args);

        if (object_dict_try_put(vm_allocator(vm), *result, key, value, result)) {
            continue;
        }

        out_of_memory_error(vm);
    }

    return true;
}

static bool dict_get(VirtualMachine *vm, Object *args, Object **value) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_LIST, TYPE_NIL);
    guard_is_not_null(value);

    Object *key, *dict;
    if (false == object_list_try_unpack_2(&key, &dict, args)) {
        call_args_count_error(vm, "get", 2, object_list_count(args));
    }

    if (TYPE_NIL != dict->type && TYPE_DICT != dict->type) {
        type_error(vm, dict->type, TYPE_NIL, TYPE_DICT);
    }

    if (object_dict_try_get(dict, key, value)) {
        return true;
    }

    key_error(vm, key);
}

static bool dict_put(VirtualMachine *vm, Object *args, Object **result) {
    guard_is_not_null(vm);
    guard_is_not_null(args);
    guard_is_one_of(args->type, TYPE_LIST, TYPE_NIL);
    guard_is_not_null(result);

    Object *key, *value, *dict;
    if (false == object_list_try_unpack_3(&key, &value, &dict, args)) {
        call_args_count_error(vm, "put", 3, object_list_count(args));
    }

    if (TYPE_NIL != dict->type && TYPE_DICT != dict->type) {
        type_error(vm, dict->type, TYPE_NIL, TYPE_DICT);
    }

    if (object_dict_try_put(vm_allocator(vm), dict, key, value, result)) {
        return true;
    }

    out_of_memory_error(vm);
}

typedef struct {
    Object *name;
    Object *value;
} Primitive;

#define primitive(Name, Fn)                                             \
((Primitive) {                                                          \
    .name = &(Object) {.type = TYPE_SYMBOL, .as_symbol = (Name)},       \
    .value = &(Object) {.type = TYPE_PRIMITIVE, .as_primitive = (Fn)}   \
})

static Primitive const PRIMITIVES[] = {
        primitive("eq?", eq),
        primitive("compare", compare),
        primitive("str", str),
        primitive("repr", repr),
        primitive("print", print),
        primitive("+", plus),
        primitive("-", minus),
        primitive("*", multiply),
        primitive("/", divide),
        primitive("list", list_list),
        primitive("first", list_first),
        primitive("rest", list_rest),
        primitive("prepend", list_prepend),
        primitive("reverse", list_reverse),
        primitive("concat", list_concat),
        primitive("not", not),
        primitive("type", type),
        primitive("traceback", traceback),
        primitive("throw", throw),
        primitive("dict", dict_dict),
        primitive("get", dict_get),
        primitive("put", dict_put),
};

static size_t const PRIMITIVES_COUNT = sizeof(PRIMITIVES) / sizeof(PRIMITIVES[0]);

bool try_define_primitives(ObjectAllocator *a, Object *env) {
    guard_is_not_null(a);
    guard_is_not_null(env);

    for (auto it = PRIMITIVES; it < PRIMITIVES + PRIMITIVES_COUNT; it++) {
        if (false == env_try_define(a, env, it->name, it->value)) {
            return false;
        }
    }

    return true;
}

