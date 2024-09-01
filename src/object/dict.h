#pragma once

#include "utility/slice.h"
#include "utility/macros.h"
#include "object.h"
#include "accessors.h"
#include "allocator.h"

typedef enum {
    DICT_KEY_UNHASHABLE,
    DICT_KEY_DOES_NOT_EXIST,
    DICT_ALLOCATION_ERROR
} Object_DictError;

[[nodiscard]]
bool object_try_make_empty_dict(ObjectAllocator *a, Object **dict);

[[nodiscard]]
bool object_dict_try_put(
        ObjectAllocator *a,
        Object *dict,
        Object *key,
        Object *value,
        Object_DictError *error
);

[[nodiscard]]
bool object_dict_try_get(
        Object *dict,
        Object *key,
        Object **value,
        Object_DictError *error
);

Object_DictEntry *OBJECT_DICT__find_next_used(Object_DictEntries *entries, Object_DictEntry *current);

#define object_dict_for(EntryIt, Dict)                                                      \
auto concat_identifiers(_e_, __LINE__) = object_as_dict_entries((Dict)->as_dict.entries);   \
for (                                                                                       \
    Object_DictEntry *EntryIt = OBJECT_DICT__find_next_used(                                \
        concat_identifiers(_e_, __LINE__),                                                  \
        slice_at(concat_identifiers(_e_, __LINE__), 0)                                      \
    );                                                                                      \
    EntryIt != slice_end(concat_identifiers(_e_, __LINE__));                                \
    EntryIt = OBJECT_DICT__find_next_used(concat_identifiers(_e_, __LINE__), EntryIt + 1)   \
)
