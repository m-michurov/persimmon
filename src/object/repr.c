#include "repr.h"

#include <inttypes.h>
#include <ctype.h>

#include "utility/guards.h"
#include "utility/strings.h"
#include "utility/writer.h"
#include "utility/slice.h"
#include "lists.h"

static bool is_quote(Object *expr, Object **quoted) {
    guard_is_not_null(expr);
    guard_is_not_null(quoted);

    if (TYPE_CONS != expr->type) {
        return false;
    }

    Object *tag;
    if (false == object_list_try_unpack_2(&tag, quoted, expr)) {
        return false;
    }

    return TYPE_ATOM == tag->type && 0 == strcmp("quote", tag->as_atom);
}

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
            Object *quoted;
            if (is_quote(obj, &quoted)) {
                return writer_try_printf(w, error_code, "'")
                       && object_try_write_repr(w, quoted, error_code);
            }

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
        case TYPE_DICT: {
            if (slice_empty(obj->as_dict.entries->as_dict_entries)) {
                return writer_try_printf(w, error_code, "{}");
            }

            if (false == writer_try_printf(w, error_code, "{")) {
                return false;
            }

            auto first_entry = true;
            slice_for(entry, &obj->as_dict.entries->as_dict_entries) {
                if (false == entry->used) {
                    continue;
                }

                if (false == first_entry) {
                    if (false == writer_try_printf(w, error_code, ", ")) {
                        return false;
                    }
                }

                if (false == object_try_write_repr(w, entry->key, error_code)) {
                    return false;
                }

                if (false == writer_try_printf(w, error_code, " ")) {
                    return false;
                }

                if (false == object_try_write_repr(w, entry->value, error_code)) {
                    return false;
                }

                first_entry = false;
            }

            return writer_try_printf(w, error_code, "}");
        }
        case TYPE_NIL: {
            return writer_try_printf(w, error_code, "()");
        }
        case TYPE_DICT_ENTRIES:
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
        case TYPE_DICT_ENTRIES:
        case TYPE_DICT:
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
