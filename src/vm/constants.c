#include "constants.h"

#include "utility/guards.h"
#include "object/accessors.h"
#include "env.h"

static auto const SYMBOL_TRUE = &(Object) {.type = TYPE_SYMBOL, .as_symbol = "true"};
static auto const SYMBOL_FALSE = &(Object) {.type = TYPE_SYMBOL, .as_symbol = "false"};
static auto const SYMBOL_NIL = &(Object) {.type = TYPE_SYMBOL, .as_symbol = "nil"};

bool try_define_constants(ObjectAllocator *a, Object *env) {
    guard_is_not_null(a);
    guard_is_not_null(env);

    return env_try_define(a, env, SYMBOL_NIL, OBJECT_NIL)
           && env_try_define(a, env, SYMBOL_TRUE, OBJECT_TRUE)
           && env_try_define(a, env, SYMBOL_FALSE, OBJECT_NIL);
}
