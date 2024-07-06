#include "accessors.h"

#include "utility/guards.h"

int64_t object_as_int(Object *obj) {
    guard_is_not_null(obj);
    guard_is_equal(obj->type, TYPE_INT);

    return obj->as_int;
}

char const *object_as_string(Object *obj) {
    guard_is_not_null(obj);
    guard_is_equal(obj->type, TYPE_STRING);

    return obj->as_string;
}

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

Object_Primitive object_as_primitive(Object *obj) {
    guard_is_not_null(obj);
    guard_is_equal(obj->type, TYPE_PRIMITIVE);

    return obj->as_primitive;
}

Object_Closure object_as_closure(Object *obj) {
    guard_is_not_null(obj);
    guard_is_equal(obj->type, TYPE_CLOSURE);

    return obj->as_closure;
}
