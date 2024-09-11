#include "accessors.h"

#include "utility/guards.h"

char const *object_as_atom(Object *obj) {
    guard_is_not_null(obj);
    guard_is_equal(obj->type, TYPE_ATOM);

    return obj->as_atom;
}

Object_Cons object_as_cons(Object *obj) {
    guard_is_not_null(obj);
    guard_is_equal(obj->type, TYPE_CONS);

    return obj->as_cons;
}

