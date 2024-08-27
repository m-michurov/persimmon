#include "dict.h"

#include "utility/guards.h"
#include "utility/exchange.h"
#include "allocator.h"
#include "object.h"
#include "lists.h"
#include "compare.h"

static bool entry_try_init(ObjectAllocator *a, Object *key, Object *value, Object **entry) {
    return object_try_make_list(a, entry, object_nil(), object_nil(), object_nil(), object_nil(), key, value);
}

static Object **entry_prev(Object *entry) {
    guard_is_not_null(entry);

    return object_list_nth(0, entry);
}

static Object **entry_next(Object *entry) {
    guard_is_not_null(entry);

    return object_list_nth(1, entry);
}

static Object **entry_left(Object *entry) {
    guard_is_not_null(entry);

    return object_list_nth(2, entry);
}

static Object **entry_right(Object *entry) {
    guard_is_not_null(entry);

    return object_list_nth(3, entry);
}

static Object **entry_key(Object *entry) {
    guard_is_not_null(entry);

    return object_list_nth(4, entry);
}

static Object **entry_value(Object *entry) {
    guard_is_not_null(entry);

    return object_list_nth(5, entry);
}

Object *object_dict_entry_next(Object *entry) {
    return *entry_next(entry);
}

Object *object_dict_entry_key(Object *entry) {
    return *entry_key(entry);
}

Object *object_dict_entry_value(Object *entry) {
    return *entry_value(entry);
}

static bool try_find_entry_or_nil(Object *dict, Object *key, Object ***entry) {
    guard_is_not_null(dict);
    guard_is_not_null(key);
    guard_is_equal(dict->type, TYPE_DICT);

    *entry = &dict->as_dict.root;
    while (object_nil() != **entry) {
        if (object_equals(key, *entry_key(**entry))) {
            return true;
        }

        bool is_less;
        if (false == object_try_compare(key, *entry_key(**entry), COMPARE_LESS, &is_less)) {
            return false;
        }

        *entry = is_less ? entry_left(**entry) : entry_right(**entry);
    }

    return true;
}

bool object_dict_try_put(
        ObjectAllocator *a,
        Object *dict,
        Object *key,
        Object *value,
        Object **key_value_pair,
        Object_DictError *error
) {
    guard_is_not_null(a);
    guard_is_not_null(dict);
    guard_is_not_null(key);
    guard_is_not_null(value);
    guard_is_not_null(key_value_pair);
    guard_is_not_null(error);
    guard_is_one_of(dict->type, TYPE_DICT);

    *key_value_pair = object_nil();

    Object **entry;
    if (false == try_find_entry_or_nil(dict, key, &entry)) {
        *error = DICT_INVALID_KEY;
        return false;
    }

    if (object_nil() != *entry) {
        *entry_value(*entry) = value;
        return true;
    }

    if (false == entry_try_init(a, key, value, entry)) {
        *error = DICT_ALLOCATION_ERROR;
        return false;
    }

    // FIXME rebalance

    auto const entries = exchange(dict->as_dict.entries, *entry);
    if (object_nil() != entries) {
        *entry_next(*entry) = entries;
        *entry_prev(entries) = *entry;
    }

    return true;
}

bool object_dict_try_get(
        Object *dict,
        Object *key,
        Object **value,
        Object_DictError *error
) {
    guard_is_not_null(dict);
    guard_is_not_null(key);
    guard_is_not_null(value);
    guard_is_equal(dict->type, TYPE_DICT);

    Object **entry;
    if (false == try_find_entry_or_nil(dict, key, &entry)) {
        *error = DICT_INVALID_KEY;
        return false;
    }

    if (object_nil() == *entry) {
        *error = DICT_KEY_DOES_NOT_EXIST;
        return false;
    }

    *value = *entry_value(*entry);
    return true;
}