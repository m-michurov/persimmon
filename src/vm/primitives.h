#pragma once

#include "object/object.h"
#include "object/allocator.h"

[[nodiscard]]
bool try_define_primitives(ObjectAllocator *a, Object **key_root, Object **value_root, Object *env);
