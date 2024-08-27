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

bool object_type_is_mutable(Object_Type type) {
    switch (type) {
        case TYPE_DICT: {
            return true;
        }
        case TYPE_INT:
        case TYPE_STRING:
        case TYPE_ATOM:
        case TYPE_CONS:
        case TYPE_PRIMITIVE:
        case TYPE_CLOSURE:
        case TYPE_MACRO:
        case TYPE_NIL: {
            return false;
        }
    }

    guard_unreachable();
}

Object *object_nil() { return &NIL; }
