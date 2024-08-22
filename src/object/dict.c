#include "dict.h"

#include "utility/guards.h"
#include "allocator.h"
#include "object.h"
#include "lists.h"

bool object_dict_try_put(
        ObjectAllocator *a,
        Object *dict,
        Object *key,
        Object *value,
        Object **result
) {
    guard_is_not_null(a);
    guard_is_not_null(dict);
    guard_is_not_null(key);
    guard_is_not_null(value);
    guard_is_not_null(result);
    guard_is_one_of(dict->type, TYPE_NIL, TYPE_CONS);

    *result = object_nil();

    object_list_for(it, dict) {
        Object *existing_key, *existing_value;
        if (false == object_list_try_unpack_2(&existing_key, &existing_value, it)) {
            continue;
        }

        if (object_equals(key, existing_key)) {
            continue;
        }

        if (false == object_list_try_prepend(a, object_nil(), result)) {
            return false;
        }

        auto const binding = &(*result)->as_cons.first;
        if (false == object_try_make_list(a, binding, existing_key, existing_value)) {
            return false;
        }
    }

    if (false == object_list_try_prepend(a, object_nil(), result)) {
        return false;
    }

    auto const new_binding = &(*result)->as_cons.first;
    return object_try_make_list(a, new_binding, key, value);
}

bool object_dict_try_get(Object *dict, Object *key, Object **value) {
    guard_is_not_null(dict);
    guard_is_not_null(key);
    guard_is_not_null(value);

    object_list_for(binding, dict) {
        Object *existing_key;
        if (object_list_try_unpack_2(&existing_key, value, binding) && object_equals(existing_key, key)) {
            return true;
        }
    }

    return false;
}