#pragma once

#include "object.h"

typedef enum {
    OBJECT_LESS = -1,
    OBJECT_EQUALS = 0,
    OBJECT_GREATER = 1
} Object_CompareResult;

Object_CompareResult object_compare(Object *a, Object *b);

bool object_equals(Object *a, Object *b);
