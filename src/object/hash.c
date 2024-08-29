#include "hash.h"

#include "utility/guards.h"

bool object_try_hash(Object *obj, size_t *hash) {
    guard_is_not_null(obj);
    guard_is_not_null(hash);

    switch (obj->type) {
        case TYPE_INT:
        case TYPE_STRING:
        case TYPE_ATOM:
        case TYPE_CONS:
        case TYPE_NIL: {
            *hash = 43; // FIXME
            return true;
        }
        case TYPE_DICT_ENTRIES:
        case TYPE_DICT:
        case TYPE_PRIMITIVE:
        case TYPE_CLOSURE:
        case TYPE_MACRO: {
            return false;
        }
    }

    guard_unreachable();
}
