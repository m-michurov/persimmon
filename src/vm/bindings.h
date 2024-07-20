#pragma once

#include "env.h"

typedef enum {
    Binding_InvalidTargetType,
    Binding_InvalidVariadicSyntax,
} Binding_TargetErrorType;

typedef struct {
    Binding_TargetErrorType type;

    union {
        struct {
            Object_Type target_type;
        } as_invalid_target;
    };
} Binding_TargetError;

bool binding_is_valid_target(Object *target, Binding_TargetError *error);

typedef enum {
    Binding_ValueCountMismatch,
    Binding_CannotUnpackValue,
} Binding_ValueErrorType;

typedef struct {
    Binding_ValueErrorType type;

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
} Binding_ValueError;

typedef enum {
    Binding_InvalidTarget,
    Binding_InvalidValue,
    Binding_AllocationFailed
} Binding_ErrorType;

typedef struct {
    Binding_ErrorType type;
    union {
        Binding_TargetError as_target_error;
        Binding_ValueError as_value_error;
    };
} Binding_Error;

bool binding_try_create(
        ObjectAllocator *a,
        Object *env,
        Object *target,
        Object *value,
        Binding_Error *error
);
