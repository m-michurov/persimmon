#include "constants.h"

#include "utility/guards.h"
#include "object/lists.h"
#include "object/constructors.h"
#include "object/accessors.h"
#include "env.h"

bool try_define_constants(VirtualMachine *vm, Object *env) {
    guard_is_not_null(vm);
    guard_is_not_null(env);

    auto const a = vm_allocator(vm);

    Object *binding;
    auto ok =
            env_try_define(a, env, object_nil(), object_nil(), &binding)
            && object_try_make_atom(a, "nil", object_list_nth(0, binding))

            && env_try_define(a, env, object_nil(), vm_get(vm, STATIC_FALSE), &binding)
            && object_try_make_atom(a, "false", object_list_nth(0, binding))

            && env_try_define(a, env, object_nil(), vm_get(vm, STATIC_TRUE), &binding)
            && object_try_make_atom(a, "true", object_list_nth(0, binding));

    return ok;
}
