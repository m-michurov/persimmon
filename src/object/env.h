#pragma once

#include "object.h"
#include "allocator.h"

bool env_try_create(ObjectAllocator *a, Object *base_env, Object **env);

bool env_try_define(ObjectAllocator *a, Object *env, Object *name, Object *value, Object **binding);

bool env_try_find(Object *env, Object *name, Object **value);
