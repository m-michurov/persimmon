#include "repr.h"

#include <inttypes.h>
#include <ctype.h>

#include "utility/guards.h"
#include "utility/strings.h"
#include "utility/writer.h"
#include "lists.h"

static bool object_try_write_repr(Writer w, Object *obj, errno_t *error_code) {
    guard_is_not_null(obj);
    guard_is_not_null(error_code);

    switch (obj->type) {
        case TYPE_INT: {
            return writer_try_printf(w, error_code, "%" PRId64, obj->as_int);
        }
        case TYPE_STRING: {
            if (false == writer_try_printf(w, error_code, "\"")) {
                return false;
            }

            string_for(it, obj->as_string) {
                char const *escape_sequence;
                if (string_try_repr_escape_seq(*it, &escape_sequence)) {
                    if (false == writer_try_printf(w, error_code, "%s", escape_sequence)) {
                        return false;
                    }

                    continue;
                }

                if (isprint(*it)) {
                    if (false == writer_try_printf(w, error_code, "%c", *it)) {
                        return false;
                    }

                    continue;
                }

                if (false == writer_try_printf(w, error_code, "\\0x%02hhX", *it)) {
                    return false;
                }
            }

            return writer_try_printf(w, error_code, "\"");
        }
        case TYPE_ATOM: {
            return writer_try_printf(w, error_code, "%s", obj->as_atom);
        }
        case TYPE_CONS: {
            if (false == writer_try_printf(w, error_code, "(")) {
                return false;
            }

            if (false == object_try_write_repr(w, obj->as_cons.first, error_code)) {
                return false;
            }

            object_list_for(it, obj->as_cons.rest) {
                if (false == writer_try_printf(w, error_code, " ")) {
                    return false;
                }

                if (false == object_try_write_repr(w, it, error_code)) {
                    return false;
                }
            }

            return writer_try_printf(w, error_code, ")");
        }
        case TYPE_NIL: {
            return writer_try_printf(w, error_code, "()");
        }
        case TYPE_PRIMITIVE:
        case TYPE_CLOSURE:
        case TYPE_MACRO: {
            return writer_try_printf(w, error_code, "<%s>", object_type_str(obj->type));
        }
    }

    guard_unreachable();
}

static bool object_try_write_str(Writer w, Object *obj, errno_t *error_code) {
    guard_is_not_null(obj);
    guard_is_not_null(error_code);

    switch (obj->type) {
        case TYPE_STRING: {
            return writer_try_printf(w, error_code, "%s", obj->as_string);
        }
        case TYPE_INT:
        case TYPE_ATOM:
        case TYPE_CONS:
        case TYPE_NIL:
        case TYPE_PRIMITIVE:
        case TYPE_CLOSURE:
        case TYPE_MACRO: {
            return object_try_write_repr(w, obj, error_code);
        }
    }

    guard_unreachable();
}

bool object_try_repr_sb(Object *obj, StringBuilder *sb, errno_t *error_code) {
    return object_try_write_repr(writer_make(sb), obj, error_code);
}

bool object_try_repr_file(Object *obj, FILE *file, errno_t *error_code) {
    return object_try_write_repr(writer_make(file), obj, error_code);
}

bool object_try_print_sb(Object *obj, StringBuilder *sb, errno_t *error_code) {
    return object_try_write_str(writer_make(sb), obj, error_code);
}

bool object_try_print_file(Object *obj, FILE *file, errno_t *error_code) {
    return object_try_write_str(writer_make(file), obj, error_code);
}
