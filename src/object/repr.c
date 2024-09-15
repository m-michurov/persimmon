#include "repr.h"

#include <inttypes.h>
#include <ctype.h>

#include "utility/guards.h"
#include "utility/strings.h"
#include "utility/writer.h"
#include "list.h"

static bool is_quote(Object *expr, Object **quoted) {
    guard_is_not_null(expr);
    guard_is_not_null(quoted);

    if (TYPE_LIST != expr->type) {
        return false;
    }

    Object *tag;
    if (false == object_list_try_unpack_2(&tag, quoted, expr)) {
        return false;
    }

    return TYPE_SYMBOL == tag->type && 0 == strcmp("quote", tag->as_symbol);
}

static bool object_try_write_repr(Writer w, Object *obj, errno_t *error_code);

static bool dict_try_write_repr_(Writer w, Object *obj, bool *is_min, errno_t *error_code) { // NOLINT(*-no-recursion)
    guard_is_not_null(obj);
    guard_is_not_null(is_min);
    guard_is_not_null(error_code);
    guard_is_one_of(obj->type, TYPE_NIL, TYPE_DICT);

    if (OBJECT_NIL == obj) {
        return true;
    }

    if (false == dict_try_write_repr_(w, obj->as_dict.left, is_min, error_code)) {
        return false;
    }

    if (false == *is_min && false == writer_try_printf(w, error_code, ", ")) {
        return false;
    }

    if (OBJECT_NIL == obj->as_dict.left) {
        *is_min = false;
    }

    return object_try_write_repr(w, obj->as_dict.key, error_code)
           && writer_try_printf(w, error_code, " ")
           && object_try_write_repr(w, obj->as_dict.value, error_code)
           && dict_try_write_repr_(w, obj->as_dict.right, is_min, error_code);
}

static bool dict_try_write_repr(Writer w, Object *obj, errno_t *error_code) { // NOLINT(*-no-recursion)
    auto is_min = true;
    return dict_try_write_repr_(w, obj, &is_min, error_code);
}

static bool object_try_write_repr(Writer w, Object *obj, errno_t *error_code) { // NOLINT(*-no-recursion)
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
        case TYPE_SYMBOL: {
            return writer_try_printf(w, error_code, "%s", obj->as_symbol);
        }
        case TYPE_LIST: {
            Object *quoted;
            if (is_quote(obj, &quoted)) {
                return writer_try_printf(w, error_code, "'")
                       && object_try_write_repr(w, quoted, error_code);
            }

            if (false == writer_try_printf(w, error_code, "(")) {
                return false;
            }

            if (false == object_try_write_repr(w, obj->as_list.first, error_code)) {
                return false;
            }

            object_list_for(it, obj->as_list.rest) {
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
            return writer_try_printf(w, error_code, "{")
                   && dict_try_write_repr(w, obj, error_code)
                   && writer_try_printf(w, error_code, "}");
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
        case TYPE_SYMBOL:
        case TYPE_LIST:
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
