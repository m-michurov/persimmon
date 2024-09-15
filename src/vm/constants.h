#pragma once

#include "object/object.h"
#include "object/allocator.h"

[[nodiscard]]
bool try_define_constants(ObjectAllocator *a, Object *env);
