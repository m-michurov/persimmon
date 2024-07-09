#include "env.h"

#include "utility/guards.h"
#include "lists.h"
#include "constructors.h"
#include "accessors.h"

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
    if (false == object_try_make_cons(a, object_nil(), *current_env, current_env)) {
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
