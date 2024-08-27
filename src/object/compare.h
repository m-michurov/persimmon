#pragma once

#include "object.h"

typedef enum {
    COMPARE_LESS,
    COMPARE_LESS_OR_EQUAL,
    COMPARE_GREATER,
    COMPARE_GREATER_OR_EQUAL,
    COMPARE_EQUAL
} Object_CompareKind;

bool object_try_compare(Object *a, Object *b, Object_CompareKind kind, bool *result);

bool object_equals(Object *a, Object *b);
