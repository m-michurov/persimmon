#include "object.h"

#include <string.h>
#include <inttypes.h>
#include <ctype.h>

#include "utility/strings.h"
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

void object_print(Object *object, FILE *file) {
    guard_is_not_null(file);
    guard_is_not_null(object);

    switch (object->type) {
        case TYPE_STRING: {
            fprintf(file, "%s", object->as_string);
            return;
        }
        case TYPE_INT:
        case TYPE_ATOM:
        case TYPE_CONS:
        case TYPE_NIL:
        case TYPE_PRIMITIVE:
        case TYPE_CLOSURE:
        case TYPE_MACRO: {
            object_repr_print(object, file);
            return;
        }
    }

    guard_unreachable();
}

void object_repr_print(Object *object, FILE *file) { // NOLINT(*-no-recursion)
    guard_is_not_null(file);
    guard_is_not_null(object);

    switch (object->type) {
        case TYPE_INT: {
            fprintf(file, "%" PRId64, object->as_int);
            return;
        }
        case TYPE_STRING: {
            fprintf(file, "\"");

            string_for(it, object->as_string) {
                char const *escape_sequence;
                if (string_try_repr_escape_seq(*it, &escape_sequence)) {
                    fprintf(file, "%s", escape_sequence);
                    continue;
                }

                if (isprint(*it)) {
                    fprintf(file, "%c", *it);
                    continue;
                }

                fprintf(file, "\\0x%02hhX", *it);
            }

            fprintf(file, "\"");
            return;
        }
        case TYPE_ATOM: {
            fprintf(file, "%s", object->as_atom);
            return;
        }
        case TYPE_CONS: {
            fprintf(file, "(");
            object_repr_print(object->as_cons.first, file);
            auto it = object->as_cons.rest;
            while (true) {
                if (object_nil() == it) {
                    break;
                }

                fprintf(file, " ");

                if (TYPE_CONS == it->type) {
                    object_repr_print(it->as_cons.first, file);
                    it = it->as_cons.rest;
                    continue;
                }

                fprintf(file, ". ");
                object_repr_print(it, file);
                break;
            }
            fprintf(file, ")");

            return;
        }
        case TYPE_NIL: {
            fprintf(file, "()");
            return;
        }
        case TYPE_PRIMITIVE:
        case TYPE_CLOSURE:
        case TYPE_MACRO: {
            fprintf(file, "<%s>", object_type_str(object->type));
            return;
        }
    }

    guard_unreachable();
}
