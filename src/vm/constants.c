#include "constants.h"

#include "utility/guards.h"
#include "object/accessors.h"
#include "env.h"

static auto const ATOM_TRUE = &(Object) {.type = TYPE_ATOM, .as_atom = "true"};
static auto const ATOM_FALSE = &(Object) {.type = TYPE_ATOM, .as_atom = "false"};
static auto const ATOM_NIL = &(Object) {.type = TYPE_ATOM, .as_atom = "nil"};

bool try_define_constants(ObjectAllocator *a, Object *env) {
    guard_is_not_null(a);
    guard_is_not_null(env);

    return env_try_define(a, env, ATOM_NIL, OBJECT_NIL)
           && env_try_define(a, env, ATOM_TRUE, ATOM_TRUE)
           && env_try_define(a, env, ATOM_FALSE, OBJECT_NIL);
}
