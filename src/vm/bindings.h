#pragma once

#include "env.h"

typedef enum {
    BINDING_INVALID_TARGET_TYPE,
    BINDING_INVALID_VARIADIC_SYNTAX,
} BindingTargetError_Type;

typedef struct {
    BindingTargetError_Type type;

    struct {
        Object_Type target_type;
    } as_invalid_target;
} BindingTargetError;

bool binding_is_valid_target(Object *target, BindingTargetError *error);

typedef enum {
    BINDING_VALUES_COUNT_MISMATCH,
    BINDING_CANNOT_UNPACK_VALUE,
} BindingValueError_Type;

typedef struct {
    BindingValueError_Type type;

    union {
        struct {
            size_t expected;
            bool is_variadic;
            size_t got;
        } as_count_mismatch;

        struct {
            Object_Type value_type;
        } as_cannot_unpack;
    };
} BindingValueError;

typedef enum {
    BINDING_INVALID_TARGET,
    BINDING_INVALID_VALUE,
    BINDING_ALLOCATION_FAILED
} BindingError_Type;

typedef struct {
    BindingError_Type type;
    union {
        BindingTargetError as_target_error;
        BindingValueError as_value_error;
    };
} BindingError;

[[nodiscard]]
bool binding_try_create(
        ObjectAllocator *a,
        Object *env,
        Object *target,
        Object *value,
        BindingError *error
);
