#include "bindings.h"

#include "object/list.h"
#include "utility/guards.h"
#include "variadic.h"

bool binding_is_valid_target(Object *target, BindingTargetError *error) { // NOLINT(*-no-recursion)
    guard_is_not_null(target);

    switch (target->type) {
        case TYPE_ATOM:
        case TYPE_NIL: {
            return true;
        }
        case TYPE_CONS: {
            auto is_varargs = false;
            while (OBJECT_NIL != target) {
                auto const it = object_list_shift(&target);

                if (is_varargs && (TYPE_ATOM != it->type || OBJECT_NIL != target)) {
                    *error = (BindingTargetError) {
                            .type = BINDING_INVALID_VARIADIC_SYNTAX
                    };
                    return false;
                }

                if (is_ampersand(it)) {
                    is_varargs = true;
                    continue;
                }

                if (binding_is_valid_target(it, error)) {
                    continue;
                }

                return false;
            }

            return true;
        }
        case TYPE_INT:
        case TYPE_STRING:
        case TYPE_DICT:
        case TYPE_PRIMITIVE:
        case TYPE_CLOSURE:
        case TYPE_MACRO: {
            *error = (BindingTargetError) {
                    .type = BINDING_INVALID_TARGET_TYPE,
                    .as_invalid_target = {
                            .target_type = target->type
                    }
            };
            return false;
        }
    }

    guard_unreachable();
}

typedef struct {
    size_t count;
    bool is_variadic;
} TargetsCount;

static TargetsCount count_targets(Object *target) {
    auto result = (TargetsCount) {0};

    while (OBJECT_NIL != target) {
        auto const it = object_list_shift(&target);

        if (result.is_variadic) {
            guard_is_true(TYPE_ATOM == it->type && OBJECT_NIL == target);
            return result;
        }

        if (is_ampersand(it)) {
            result.is_variadic = true;
            continue;
        }

        result.count++;
    }

    return result;
}

static bool is_valid_value(Object *target, Object *value, BindingValueError *error) { // NOLINT(*-no-recursion)
    guard_is_not_null(target);
    guard_is_not_null(value);
    guard_is_not_null(error);

    switch (target->type) {
        case TYPE_NIL:
        case TYPE_CONS: {
            if (TYPE_CONS != value->type && TYPE_NIL != value->type) {
                *error = (BindingValueError) {
                        .type = BINDING_CANNOT_UNPACK_VALUE,
                        .as_cannot_unpack = {
                                .value_type = value->type
                        }
                };
                return false;
            }

            auto const targets = count_targets(target);
            auto const values_count = object_list_count(value);
            if ((values_count < targets.count && targets.is_variadic) ||
                (targets.count != values_count && false == targets.is_variadic)) {
                *error = (BindingValueError) {
                        .type = BINDING_VALUES_COUNT_MISMATCH,
                        .as_count_mismatch = {
                                .expected = targets.count,
                                .is_variadic = targets.is_variadic,
                                .got = values_count
                        }
                };
                return false;
            }

            auto targets_left = targets.count;
            object_list_for(it, target) {
                if (0 == targets_left--) {
                    break;
                }

                if (is_valid_value(it, object_list_shift(&value), error)) {
                    continue;
                }

                return false;
            }

            guard_is_true(targets.is_variadic || OBJECT_NIL == value);
            return true;
        }
        case TYPE_ATOM: {
            return true;
        }
        case TYPE_INT:
        case TYPE_STRING:
        case TYPE_DICT:
        case TYPE_PRIMITIVE:
        case TYPE_CLOSURE:
        case TYPE_MACRO: {
            guard_unreachable();
        }
    }

    guard_unreachable();
}

static bool env_try_bind_(ObjectAllocator *a, Object *env, Object *target, Object *value) { // NOLINT(*-no-recursion)
    guard_is_not_null(a);
    guard_is_not_null(env);
    guard_is_not_null(target);
    guard_is_not_null(value);

    switch (target->type) {
        case TYPE_NIL: {
            guard_is_equal(value, OBJECT_NIL);
            return true;
        }
        case TYPE_ATOM: {
            return env_try_define(a, env, target, value);
        }
        case TYPE_CONS: {
            guard_is_one_of(value->type, TYPE_CONS, TYPE_NIL);

            auto is_varargs = false;
            object_list_for(it, target) {
                if (is_varargs) {
                    return env_try_bind_(a, env, it, value);
                }

                if (is_ampersand(it)) {
                    is_varargs = true;
                    continue;
                }

                if (env_try_bind_(a, env, it, object_list_shift(&value))) {
                    continue;
                }

                return false;
            }

            guard_is_equal(value, OBJECT_NIL);
            return true;
        }
        case TYPE_INT:
        case TYPE_STRING:
        case TYPE_DICT:
        case TYPE_PRIMITIVE:
        case TYPE_CLOSURE:
        case TYPE_MACRO: {
            guard_unreachable();
        }
    }

    guard_unreachable();
}

bool binding_try_create(ObjectAllocator *a, Object *env, Object *target, Object *value, BindingError *error) {
    guard_is_not_null(a);
    guard_is_not_null(env);
    guard_is_not_null(target);
    guard_is_not_null(value);
    guard_is_not_null(error);

    BindingTargetError target_error;
    if (false == binding_is_valid_target(target, &target_error)) {
        *error = (BindingError) {
                .type = BINDING_INVALID_TARGET,
                .as_target_error = target_error
        };
        return false;
    }

    BindingValueError value_error;
    if (false == is_valid_value(target, value, &value_error)) {
        *error = (BindingError) {
                .type = BINDING_INVALID_VALUE,
                .as_value_error = value_error
        };
        return false;
    }

    if (env_try_bind_(a, env, target, value)) {
        return true;
    }

    *error = (BindingError) {.type = BINDING_ALLOCATION_FAILED};
    return false;
}
