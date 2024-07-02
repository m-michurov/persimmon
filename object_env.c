#include "object_env.h"

#include "guards.h"
#include "object_lists.h"
#include "object_constructors.h"
#include "object_accessors.h"

Object *env_new(ObjectAllocator *a, Object *base_env) {
    guard_is_not_null(a);
    guard_is_not_null(base_env);

    return object_cons(a, object_nil(), base_env);
}

void env_define(ObjectAllocator *a, Object *env, Object *name, Object *value) {
    guard_is_not_null(a);
    guard_is_not_null(env);
    guard_is_not_null(name);
    guard_is_not_null(value);
    guard_is_equal(env->type, TYPE_CONS);

    env->as_cons.first = object_cons(a, object_list(a, name, value), env->as_cons.first);
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
