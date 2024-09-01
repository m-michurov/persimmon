#include "dict.h"

#include "utility/guards.h"
#include "utility/exchange.h"
#include "utility/slice.h"
#include "allocator.h"
#include "constructors.h"
#include "object.h"
#include "compare.h"
#include "hash.h"

#define DEFAULT_CAPACITY 32

#define LOAD_FACTOR_THRESHOLD ((double) 0.5)

#define dict_error(ErrorType, Error)    \
do {                                    \
    *(Error) = (ErrorType);             \
    return false;                       \
} while (false)                         \

static double entries_load_factor(Object_DictEntries entries) {
    return ((double) entries.used) / ((double) entries.count);
}

#define entries_next_count(OldCapacity) (2 * (OldCapacity) + 10)

bool object_try_make_empty_dict(ObjectAllocator *a, Object **dict) {
    guard_is_not_null(a);
    guard_is_not_null(dict);

    return object_try_make_dict_entries(a, DEFAULT_CAPACITY, dict)
           && object_try_make_dict(a, *dict, dict);
}

static Object_DictEntry *entries_lookup(Object_DictEntries *entries, Object *key, size_t hash) {
    guard_is_not_null(entries);
    guard_is_not_null(key);
    guard_is_greater(entries->count, 0);
    guard_is_less(entries_load_factor(*entries), LOAD_FACTOR_THRESHOLD);

    auto i = hash % entries->count;
    while (slice_at(entries, i)->used && false == object_equals(key, slice_at(entries, i)->key)) {
        i = (i + 1) % entries->count;
    }

    return slice_at(entries, i);
}

static void entries_rebuild(Object_DictEntries const *entries, Object_DictEntries *new_entries) {
    guard_is_not_null(entries);
    guard_is_not_null(new_entries);
    guard_is_greater_or_equal(new_entries->count, entries->count);

    slice_for(entry, entries) {
        if (false == entry->used) {
            continue;
        }

        size_t hash;
        guard_is_true(object_try_hash(entry->key, &hash));

        *entries_lookup(new_entries, entry->key, hash) = *entry;
    }
}

bool object_dict_try_put(
        ObjectAllocator *a,
        Object *dict,
        Object *key,
        Object *value,
        Object_DictError *error
) {
    guard_is_not_null(a);
    guard_is_not_null(dict);
    guard_is_not_null(key);
    guard_is_not_null(value);
    guard_is_not_null(error);
    guard_is_equal(dict->type, TYPE_DICT);
    guard_is_equal(dict->as_dict.entries->type, TYPE_DICT_ENTRIES);

    auto const entries = &dict->as_dict.entries;

    if (entries_load_factor((*entries)->as_dict_entries) >= LOAD_FACTOR_THRESHOLD) {
        auto const next_count = entries_next_count((*entries)->as_dict_entries.count);
        if (false == object_try_make_dict_entries(a, next_count, &dict->as_dict.new_entries)) {
            dict_error(DICT_ALLOCATION_ERROR, error);
        }

        entries_rebuild(&(*entries)->as_dict_entries, &dict->as_dict.new_entries->as_dict_entries);
        dict->as_dict.entries = exchange(dict->as_dict.new_entries, object_nil());
    }

    size_t hash;
    if (false == object_try_hash(key, &hash)) {
        dict_error(DICT_KEY_UNHASHABLE, error);
    }

    auto const entry = entries_lookup(&(*entries)->as_dict_entries, key, hash);
    if (entry->used) {
        entry->value = value;
        return true;
    }

    (*entries)->as_dict_entries.used++;

    *entry = (Object_DictEntry) {
            .used = true,
            .key = key,
            .value = value
    };

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
    guard_is_not_null(error);
    guard_is_equal(dict->type, TYPE_DICT);
    guard_is_equal(dict->as_dict.entries->type, TYPE_DICT_ENTRIES);

    auto const entries = &dict->as_dict.entries->as_dict_entries;

    size_t hash;
    if (false == object_try_hash(key, &hash)) {
        dict_error(DICT_KEY_UNHASHABLE, error);
    }

    auto const entry = entries_lookup(entries, key, hash);
    if (entry->used) {
        *value = entry->value;
        return true;
    }

    dict_error(DICT_KEY_DOES_NOT_EXIST, error);
}

Object_DictEntry *OBJECT_DICT__find_next_used(Object_DictEntries *entries, Object_DictEntry *current) {
    guard_is_not_null(entries);
    guard_is_not_null(current);

    while (current < slice_end(entries)) {
        if (current->used) {
            break;
        }

        current++;
    }

    return current;
}