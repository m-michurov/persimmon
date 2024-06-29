#pragma once

#include "object.h"
#include "object_allocator.h"

Object *env_new(Object_Allocator *a, Object *base_env);

void env_define(Object_Allocator *a, Object *env, Object *name, Object *value);

bool env_try_find(Object *env, Object *name, Object **value);
