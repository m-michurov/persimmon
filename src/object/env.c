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

bool env_try_define(ObjectAllocator *a, Object *env, Object *name, Object *value) {
    guard_is_not_null(a);
    guard_is_not_null(env);
    guard_is_not_null(name);
    guard_is_not_null(value);
    guard_is_equal(env->type, TYPE_CONS);

    // FIXME make a root
    Object *binding;
    if (false == object_try_make_list(a, &binding, name, value)) {
        return false;
    }
    return object_try_make_cons(a, binding, env->as_cons.first, &env->as_cons.first);
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
