#include "dict.h"

#include "utility/guards.h"
#include "compare.h"
#include "constructors.h"
#include "hash.h"

size_t object_dict_size(Object *dict) {
    guard_is_not_null(dict);
    guard_is_one_of(dict->type, TYPE_NIL, TYPE_DICT);
    guard_is_not_null(dict);
    guard_is_one_of(dict->type, TYPE_NIL, TYPE_DICT);

    if (object_nil() == dict) {
        return 0;
    }

    return 1 + object_dict_size(dict->as_dict.right) + object_dict_size(dict->as_dict.right);
}

bool object_dict_try_put(ObjectAllocator *a, Object *dict, Object *key, Object *value, Object **out) {
    guard_is_not_null(a);
    guard_is_not_null(dict);
    guard_is_not_null(key);
    guard_is_not_null(value);
    guard_is_not_null(out);
    guard_is_one_of(dict->type, TYPE_NIL, TYPE_DICT);

    if (object_nil() == dict) {
        return object_try_make_dict(a, key, value, object_nil(), object_nil(), out);
    }

    auto const key_hash = object_hash(key);
    auto const root_hash = object_hash(dict->as_dict.key);

    if (key_hash > root_hash) {
        return object_try_make_dict(
                a,
                dict->as_dict.key,
                dict->as_dict.value,
                dict->as_dict.left,
                dict->as_dict.right,
                out
        ) && object_dict_try_put(a, dict->as_dict.right, key, value, &(*out)->as_dict.right);
    }

    if (key_hash == root_hash && object_equals(key, dict->as_dict.key)) {
        return object_try_make_dict(a, key, value, dict->as_dict.left, dict->as_dict.right, out);
    }

    return object_try_make_dict(
            a,
            dict->as_dict.key,
            dict->as_dict.value,
            dict->as_dict.left,
            dict->as_dict.right,
            out
    ) && object_dict_try_put(a, dict->as_dict.left, key, value, &(*out)->as_dict.left);
}

bool object_dict_try_get(Object *dict, Object *key, Object **value) {
    guard_is_not_null(dict);
    guard_is_not_null(key);
    guard_is_not_null(value);
    guard_is_one_of(dict->type, TYPE_NIL, TYPE_DICT);

    auto const key_hash = object_hash(key);

    while (object_nil() != dict) {
        guard_is_one_of(dict->type, TYPE_DICT);
        auto const root_hash = object_hash(dict->as_dict.key);

        if (key_hash > root_hash) {
            dict = dict->as_dict.right;
            continue;
        }

        if (key_hash == root_hash && object_equals(key, dict->as_dict.key)) {
            *value = dict->as_dict.value;
            return true;
        }

        dict = dict->as_dict.left;
    }

    return false;
}
