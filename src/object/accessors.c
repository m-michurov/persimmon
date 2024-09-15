#include "accessors.h"

#include "utility/guards.h"

char const *object_as_symbol(Object *obj) {
    guard_is_not_null(obj);
    guard_is_equal(obj->type, TYPE_SYMBOL);

    return obj->as_symbol;
}

Object_List object_as_list(Object *obj) {
    guard_is_not_null(obj);
    guard_is_equal(obj->type, TYPE_LIST);

    return obj->as_list;
}

