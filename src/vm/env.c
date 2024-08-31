#include "env.h"

#include "utility/guards.h"
#include "object/lists.h"
#include "object/constructors.h"
#include "object/compare.h"

bool env_try_create(ObjectAllocator *a, Object *base_env, Object **env) {
    guard_is_not_null(a);
    guard_is_not_null(base_env);

    return object_try_make_cons(a, object_nil(), base_env, env);
}

bool env_try_define(ObjectAllocator *a, Object *env, Object *name, Object *value) {
    guard_is_not_null(a);
    guard_is_not_null(env);
    guard_is_not_null(name);
    guard_is_not_null(value);
    guard_is_equal(env->type, TYPE_CONS);

    auto const current_env = &env->as_cons.first;
    object_list_for(it, *current_env) {
        if (false == object_equals(name, *object_list_nth(0, it))) {
            continue;
        }

        *object_list_nth(1, it) = value;
        return true;
    }

    if (false == object_list_try_prepend(a, object_nil(), current_env)) {
        return false;
    }

    auto const ok = object_try_make_list(a, &(*current_env)->as_cons.first, name, value);
    return ok;
}

bool env_try_find(Object *env, Object *name, Object **value) {
    guard_is_not_null(env);
    guard_is_not_null(name);
    guard_is_not_null(value);
    guard_is_equal(env->type, TYPE_CONS);

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
