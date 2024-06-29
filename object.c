#include "object.h"

#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <ctype.h>
#include <stdarg.h>

#include "string_builder.h"
#include "strings.h"
#include "guards.h"
#include "object_lists.h"

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
        case TYPE_NIL: {
            return "nil";
        }
        case TYPE_MOVED: {
            guard_unreachable();
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
        case TYPE_CLOSURE: {
            return object_equals(a->as_closure.env, b->as_closure.env)
                   && object_equals(a->as_closure.args, b->as_closure.args)
                   && object_equals(a->as_closure.body, b->as_closure.body);
        }
        case TYPE_NIL: {
            return true;
        }
        case TYPE_MOVED: {
            guard_unreachable();
        }
    }

    guard_unreachable();
}

static void sb_object_repr(StringBuilder *sb, Object *object) { // NOLINT(*-no-recursion)
    switch (object->type) {
        case TYPE_INT: {
            sb_sprintf(sb, "%" PRId64, object->as_int);
            return;
        }
        case TYPE_STRING: {
            sb_append_char(sb, '"');

            string_for(it, object->as_string) {
                char const *escape_sequence;
                if (string_try_repr_escape_seq(*it, &escape_sequence)) {
                    sb_append(sb, escape_sequence);
                    continue;
                }

                if (isprint(*it)) {
                    sb_append_char(sb, *it);
                    continue;
                }

                sb_sprintf(sb, "\\0x%02hhX", *it);
            }

            sb_append_char(sb, '"');
            return;
        }
        case TYPE_ATOM: {
            sb_append(sb, object->as_atom);
            return;
        }
        case TYPE_CONS: {
            sb_append_char(sb, '(');
            sb_object_repr(sb, object->as_cons.first);
            auto it = object->as_cons.rest;
            while (true) {
                if (object_nil() == it) {
                    break;
                }

                sb_append_char(sb, ' ');

                if (TYPE_CONS == it->type) {
                    sb_object_repr(sb, it->as_cons.first);
                    it = it->as_cons.rest;
                    continue;
                }

                sb_append(sb, ". ");
                sb_object_repr(sb, it);
                break;
            }
            sb_append_char(sb, ')');

            return;
        }
        case TYPE_NIL: {
            sb_append(sb, "()");
            return;
        }
        case TYPE_PRIMITIVE:
        case TYPE_CLOSURE: {
            sb_sprintf(sb, "<%s>", object_type_str(object->type));
            return;
        }
        case TYPE_MOVED: {
            guard_unreachable();
        }
    }

    guard_unreachable();
}

char *object_repr(Arena *a, Object *object) {
    guard_is_not_null(a);
    guard_is_not_null(object);

    auto sb = sb_new(a);
    sb_object_repr(sb, object);
    return sb_str(sb);
}
