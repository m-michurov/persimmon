#include "env.h"

#include "utility/guards.h"
#include "object/lists.h"
#include "object/constructors.h"
#include "object/accessors.h"

bool env_try_create(ObjectAllocator *a, Object *base_env, Object **env) {
    guard_is_not_null(a);
    guard_is_not_null(base_env);

    return object_try_make_cons(a, object_nil(), base_env, env);
}

bool env_try_define(ObjectAllocator *a, Object *env, Object *name, Object *value, Object **binding) {
    guard_is_not_null(a);
    guard_is_not_null(env);
    guard_is_not_null(name);
    guard_is_not_null(value);
    guard_is_equal(env->type, TYPE_CONS);

    auto const current_env = &env->as_cons.first;
    if (false == object_list_try_prepend(a, object_nil(), current_env)) {
        return false;
    }

    auto const new_binding = &(*current_env)->as_cons.first;
    if (nullptr == binding) {
        return object_try_make_list(a, new_binding, name, value);
    }

    auto const ok = object_try_make_list(a, new_binding, name, value);
    *binding = *new_binding;
    return ok;
}

bool env_is_valid_bind_target(Object *target) {
    guard_is_not_null(target);

    switch (target->type) {
        case TYPE_ATOM:
        case TYPE_NIL: {
            return true;
        }
        case TYPE_CONS: {
            object_list_for(it, target) {
                if (env_is_valid_bind_target(it)) {
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
            return false;
        }
    }

    guard_unreachable();
}

static bool validate_binding(Object *target, Object *value, Env_BindingError *error) {
    guard_is_not_null(target);
    guard_is_not_null(value);
    guard_is_not_null(error);

    switch (target->type) {
        case TYPE_NIL: {
            if (object_nil() == value) {
                return true;
            }

            if (TYPE_CONS != value->type) {
                *error = (Env_BindingError) {.type = Env_CannotUnpack};
                return false;
            }

            *error = (Env_BindingError) {
                    .type = Env_CountMismatch,
                    .as_count_mismatch = {
                            .expected = 0,
                            .got = object_list_count(value)
                    }
            };
            return false;
        }
        case TYPE_ATOM: {
            return true;
        }
        case TYPE_CONS: {
            if (TYPE_CONS != value->type) {
                *error = (Env_BindingError) {.type = Env_CannotUnpack};
                return false;
            }

            auto const expected = object_list_count(target);
            auto const got = object_list_count(value);
            if (expected != got) {
                *error = (Env_BindingError) {
                        .type = Env_CountMismatch,
                        .as_count_mismatch = {
                                .expected = expected,
                                .got = got
                        }
                };
                return false;
            }

            object_list_for(it, target) {
                if (validate_binding(it, object_list_shift(&value), error)) {
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
            *error = (Env_BindingError) {
                    .type = Env_InvalidTarget,
                    .as_invalid_target ={
                            .target_type = target->type
                    }
            };
            return false;
        }
    }

    guard_unreachable();
}

static bool env_try_bind_(ObjectAllocator *a, Object *env, Object *target, Object *value, Env_BindingError *error) {
    guard_is_not_null(a);
    guard_is_not_null(env);
    guard_is_not_null(target);
    guard_is_not_null(value);
    guard_is_not_null(error);

    switch (target->type) {
        case TYPE_NIL: {
            guard_is_equal(object_nil(), value);
            return true;
        }
        case TYPE_ATOM: {
            if (env_try_define(a, env, target, value, nullptr)) {
                return true;
            }

            *error = (Env_BindingError) {.type = Env_AllocationError};
            return false;
        }
        case TYPE_CONS: {
            guard_is_equal(value->type, TYPE_CONS);
            guard_is_equal(object_list_count(target), object_list_count(value));

            object_list_for(it, target) {
                if (env_try_bind_(a, env, it, object_list_shift(&value), error)) {
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

bool env_try_bind(ObjectAllocator *a, Object *env, Object *target, Object *value, Env_BindingError *error) {
    guard_is_not_null(a);
    guard_is_not_null(env);
    guard_is_not_null(target);
    guard_is_not_null(value);
    guard_is_not_null(error);

    if (false == validate_binding(target, value, error)) {
        return false;
    }

    return env_try_bind_(a, env, target, value, error);
}

bool env_try_find(Object *env, Object *name, Object **value) {
    guard_is_not_null(env);
    guard_is_not_null(name);
    guard_is_not_null(value);

    if (TYPE_ATOM != name->type) { return false; }

    object_list_for(bindings, env) {
        object_list_for(binding, bindings) {
            Object *key;
            if (object_list_try_unpack_2(&key, value, binding) && object_equals(key, name)) {
                return true;
            }
        }
    }

    return false;
}