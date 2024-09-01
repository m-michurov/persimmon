#include "env.h"

#include "utility/guards.h"
#include "object/lists.h"
#include "object/dict.h"
#include "object/compare.h"
#include "object/constructors.h"

bool env_try_create(ObjectAllocator *a, Object *base_env, Object **env) {
    guard_is_not_null(a);
    guard_is_not_null(base_env);

    return object_try_make_empty_dict(a, env)
           && object_try_make_cons(a, *env, base_env, env);
}

bool env_try_define(ObjectAllocator *a, Object *env, Object *name, Object *value) {
    guard_is_not_null(a);
    guard_is_not_null(env);
    guard_is_not_null(name);
    guard_is_not_null(value);
    guard_is_equal(name->type, TYPE_ATOM);
    guard_is_equal(env->type, TYPE_CONS);

    auto const scope = env->as_cons.first;
    guard_is_equal(scope->type, TYPE_DICT);

    Object_DictError error;
    if (object_dict_try_put(a, scope, name, value, &error)) {
        return true;
    }

    guard_is_equal(error, DICT_ALLOCATION_ERROR);
    return false;
}

bool env_try_find(Object *env, Object *name, Object **value) {
    guard_is_not_null(env);
    guard_is_not_null(name);
    guard_is_not_null(value);
    guard_is_equal(name->type, TYPE_ATOM);
    guard_is_equal(env->type, TYPE_CONS);

    object_list_for(scope, env) {
        Object_DictError error;
        if (object_dict_try_get(scope, name, value, &error)) {
            return true;
        }

        guard_is_equal(error, DICT_KEY_DOES_NOT_EXIST);
    }

    return false;
}
