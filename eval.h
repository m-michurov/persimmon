#pragma once

#include "object.h"
#include "object_allocator.h"
#include "stack.h"

bool try_eval(
        Object_Allocator *allocator,
        Stack *stack,
        Object *env,
        Object *expression,
        Object **value,
        Object **error
);
