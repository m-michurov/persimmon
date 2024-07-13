#pragma once

#include "object/object.h"
#include "object/allocator.h"

bool env_try_create(ObjectAllocator *a, Object *base_env, Object **env);

bool env_try_define(ObjectAllocator *a, Object *env, Object *name, Object *value, Object **binding);

typedef enum {
    Env_CannotUnpack,
    Env_CountMismatch,
    Env_InvalidTarget,
    Env_AllocationError
} Env_BindingErrorType;

typedef struct {
    Env_BindingErrorType type;

    union {
        struct {
            size_t expected;
            size_t got;
        } as_count_mismatch;

        struct {
            Object_Type target_type;
        } as_invalid_target;

        struct {
            Object_Type value_type;
        } as_cannot_unpack;
    };
} Env_BindingError;

bool env_is_valid_bind_target(Object *target);

bool env_try_bind(ObjectAllocator *a, Object *env, Object *target, Object *value, Env_BindingError *error);

bool env_try_find(Object *env, Object *name, Object **value);
