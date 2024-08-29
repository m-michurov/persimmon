#pragma once

#include "object.h"
#include "allocator.h"

typedef enum {
    DICT_KEY_UNHASHABLE,
    DICT_KEY_DOES_NOT_EXIST,
    DICT_ALLOCATION_ERROR
} Object_DictError;

[[nodiscard]]
bool object_dict_try_put(
        ObjectAllocator *a,
        Object *dict,
        Object *key,
        Object *value,
        Object **key_value_pair,
        Object_DictError *error
);

[[nodiscard]]
bool object_dict_try_get(
        Object *dict,
        Object *key,
        Object **value,
        Object_DictError *error
);
