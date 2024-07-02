#pragma once

#include "object.h"
#include "object_allocator.h"
#include "stack.h"

bool try_eval(
        ObjectAllocator *a,
        Stack *s,
        Object *env,
        Object *expr,
        Object **value
);
