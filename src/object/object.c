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

bool object_equals(Object *a, Object *b) { // NOLINT(*-no-recursion)
    if (a->type != b->type) {
        return false;
    }

    switch (a->type) {
        case TYPE_INT: {
            return a->as_int == b->as_int;
        }
        case TYPE_STRING: {
            return 0 == strcmp(a->as_string, b->as_string);
        }
        case TYPE_ATOM: {
            return 0 == strcmp(a->as_atom, b->as_atom);
        }
        case TYPE_CONS: {
            return object_equals(a->as_cons.first, b->as_cons.first)
                   && object_equals(a->as_cons.rest, b->as_cons.rest);
        }
        case TYPE_PRIMITIVE: {
            return a->as_primitive == b->as_primitive;
        }
        case TYPE_CLOSURE:
        case TYPE_MACRO: {
            return object_equals(a->as_closure.env, b->as_closure.env)
                   && object_equals(a->as_closure.args, b->as_closure.args)
                   && object_equals(a->as_closure.body, b->as_closure.body);
        }
        case TYPE_NIL: {
            return true;
        }
    }

    guard_unreachable();
}
