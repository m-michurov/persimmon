#include "object.h"

#include "utility/guards.h"

static auto NIL = (Object) {.type = TYPE_NIL};

char const *object_type_str(Object_Type type) {
    switch (type) {
        case TYPE_INT: {
            return "integer";
        }
        case TYPE_STRING: {
            return "string";
        }
        case TYPE_ATOM: {
            return "atom";
        }
        case TYPE_CONS: {
            return "cons";
        }
        case TYPE_DICT_ENTRIES: {
            return "dict_entries";
        }
        case TYPE_DICT: {
            return "dict";
        }
        case TYPE_PRIMITIVE: {
            return "primitive";
        }
        case TYPE_CLOSURE: {
            return "closure";
        }
        case TYPE_MACRO: {
            return "macro";
        }
        case TYPE_NIL: {
            return "nil";
        }
    }

    guard_unreachable();
}

Object *object_nil() { return &NIL; }
