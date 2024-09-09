#pragma once

#include "utility/slice.h"
#include "utility/macros.h"
#include "object.h"
#include "accessors.h"
#include "allocator.h"

size_t object_dict_size(Object *dict);

[[nodiscard]]
bool object_dict_try_put(ObjectAllocator *a, Object *dict, Object *key, Object *value, Object **out);

[[nodiscard]]
bool object_dict_try_get(Object *dict, Object *key, Object **value);
