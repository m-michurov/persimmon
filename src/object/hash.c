#include "hash.h"

#include "utility/guards.h"

size_t object_hash(Object *obj) {
    guard_is_not_null(obj);

    switch (obj->type) {
        case TYPE_INT:
        case TYPE_STRING:
        case TYPE_ATOM:
        case TYPE_CONS:
        case TYPE_NIL:
        case TYPE_DICT:
        case TYPE_PRIMITIVE:
        case TYPE_CLOSURE:
        case TYPE_MACRO: {
            return 42;
        }
    }

    guard_unreachable();
}
