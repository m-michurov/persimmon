#pragma once

#include "object/object.h"
#include "object/allocator.h"

[[nodiscard]]
bool env_try_create(ObjectAllocator *a, Object *base_env, Object **env);

[[nodiscard]]
bool env_try_define(ObjectAllocator *a, Object *env, Object *name, Object *value);

[[nodiscard]]
bool env_try_find(Object *env, Object *name, Object **value);
