#include "bindings.h"

#include "object/lists.h"
#include "utility/guards.h"

static bool is_dot(Object *obj) {
    return TYPE_ATOM == obj->type && 0 == strcmp(".", obj->as_atom);
}

bool binding_is_valid_target(Object *target, Binding_TargetError *error) {
    guard_is_not_null(target);

    switch (target->type) {
        case TYPE_ATOM:
        case TYPE_NIL: {
            return true;
        }
        case TYPE_CONS: {
            auto is_varargs = false;
            while (object_nil() != target) {
                auto const it = object_list_shift(&target);

                if (is_varargs && (TYPE_ATOM != it->type || object_nil() != target)) {
                    *error = (Binding_TargetError) {
                            .type = Binding_InvalidVariadicSyntax
                    };
                    return false;
                }

                if (is_dot(it)) {
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
        case TYPE_PRIMITIVE:
        case TYPE_CLOSURE:
        case TYPE_MACRO: {
            *error = (Binding_TargetError) {
                    .type = Binding_InvalidTargetType,
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

    while (object_nil() != target) {
        auto const it = object_list_shift(&target);

        if (result.is_variadic) {
            guard_is_true(TYPE_ATOM == it->type && object_nil() == target);
            return result;
        }

        if (is_dot(it)) {
            result.is_variadic = true;
            continue;
        }

        result.count++;
    }

    return result;
}

static bool is_valid_value(Object *target, Object *value, Binding_ValueError *error) {
    guard_is_not_null(target);
    guard_is_not_null(value);
    guard_is_not_null(error);

    switch (target->type) {
        case TYPE_NIL:
        case TYPE_CONS: {
            if (TYPE_CONS != value->type && TYPE_NIL != value->type) {
                *error = (Binding_ValueError) {
                        .type = Binding_CannotUnpackValue,
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
                *error = (Binding_ValueError) {
                        .type = Binding_ValueCountMismatch,
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

            guard_is_true(targets.is_variadic || object_nil() == value);
            return true;
        }
        case TYPE_ATOM: {
            return true;
        }
        case TYPE_INT:
        case TYPE_STRING:
        case TYPE_PRIMITIVE:
        case TYPE_CLOSURE:
        case TYPE_MACRO: {
            guard_unreachable();
        }
    }

    guard_unreachable();
}

static bool env_try_bind_(ObjectAllocator *a, Object *env, Object *target, Object *value) {
    guard_is_not_null(a);
    guard_is_not_null(env);
    guard_is_not_null(target);
    guard_is_not_null(value);

    switch (target->type) {
        case TYPE_NIL: {
            guard_is_equal(value, object_nil());
            return true;
        }
        case TYPE_ATOM: {
            return env_try_define(a, env, target, value, nullptr);
        }
        case TYPE_CONS: {
            guard_is_one_of(value->type, TYPE_CONS, TYPE_NIL);

            auto is_varargs = false;
            object_list_for(it, target) {
                if (is_varargs) {
                    return env_try_bind_(a, env, it, value);
                }

                if (is_dot(it)) {
                    is_varargs = true;
                    continue;
                }

                if (env_try_bind_(a, env, it, object_list_shift(&value))) {
                    continue;
                }

                return false;
            }

            guard_is_equal(value, object_nil());
            return true;
        }
        case TYPE_INT:
        case TYPE_STRING:
        case TYPE_PRIMITIVE:
        case TYPE_CLOSURE:
        case TYPE_MACRO: {
            guard_unreachable();
        }
    }

    guard_unreachable();
}

bool binding_try_create(ObjectAllocator *a, Object *env, Object *target, Object *value, Binding_Error *error) {
    guard_is_not_null(a);
    guard_is_not_null(env);
    guard_is_not_null(target);
    guard_is_not_null(value);
    guard_is_not_null(error);

    Binding_TargetError target_error;
    if (false == binding_is_valid_target(target, &target_error)) {
        *error = (Binding_Error) {
                .type = Binding_InvalidTarget,
                .as_target_error = target_error
        };
        return false;
    }

    Binding_ValueError value_error;
    if (false == is_valid_value(target, value, &value_error)) {
        *error = (Binding_Error) {
                .type = Binding_InvalidValue,
                .as_value_error = value_error
        };
        return false;
    }

    return env_try_bind_(a, env, target, value);
}
