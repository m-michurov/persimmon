#include "constants.h"

#include "utility/guards.h"
#include "object/constructors.h"
#include "object/accessors.h"
#include "env.h"

bool try_define_constants(ObjectAllocator *a, Object **key_root, Object **value_root, Object *env) {
    guard_is_not_null(a);
    guard_is_not_null(key_root);
    guard_is_not_null(value_root);
    guard_is_not_null(env);

    (void) value_root; // might be used later

    auto ok =
            object_try_make_atom(a, "nil", key_root)
            && env_try_define(a, env, *key_root, object_nil())

            && object_try_make_atom(a, "false", key_root)
            && env_try_define(a, env, *key_root, object_nil())

            && object_try_make_atom(a, "true", key_root)
            && env_try_define(a, env, *key_root, *key_root);

    return ok;
}
