#include "compare.h"

#include "utility/guards.h"
#include "lists.h"
#include "dict.h"

static bool equals(Object *a, Object *b) { // NOLINT(*-no-recursion)
    guard_is_not_null(a);
    guard_is_not_null(b);

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
            while (object_nil() != a && object_nil() != b) {
                if (false == equals(a->as_cons.first, b->as_cons.first)) {
                    return false;
                }

                object_list_shift(&a);
                object_list_shift(&b);
            }

            return object_nil() == a && object_nil() == b;
        }
        case TYPE_DICT: {
            if (a->as_dict.size != b->as_dict.size) {
                return false;
            }

            for (auto entry = a->as_dict.entries; object_nil() != entry; entry = object_dict_entry_next(entry)) {
                auto const key = object_dict_entry_key(entry);
                auto const value = object_dict_entry_value(entry);

                Object *other_value;
                Object_DictError error;
                guard_is_true(object_dict_try_get(b, key, &other_value, &error));

                if (false == equals(value, other_value)) {
                    return false;
                }
            }

            return true;
        }
        case TYPE_PRIMITIVE: {
            return a->as_primitive == b->as_primitive;
        }
        case TYPE_CLOSURE:
        case TYPE_MACRO: {
            return equals(a->as_closure.env, b->as_closure.env)
                   && equals(a->as_closure.args, b->as_closure.args)
                   && equals(a->as_closure.body, b->as_closure.body);
        }
        case TYPE_NIL: {
            return true;
        }
    }

    guard_unreachable();
}

static bool try_compare_less(Object *a, Object *b, bool *result) {
    guard_is_not_null(a);
    guard_is_not_null(b);
    guard_is_not_null(result);

    if (a->type != b->type) {
        switch (a->type) {
            case TYPE_INT:
            case TYPE_STRING:
            case TYPE_ATOM:
            case TYPE_CONS: {
                switch (b->type) {
                    case TYPE_INT:
                    case TYPE_STRING:
                    case TYPE_ATOM:
                    case TYPE_CONS: {
                        *result = a->type < b->type;
                        return true;
                    }
                    case TYPE_DICT:
                    case TYPE_PRIMITIVE:
                    case TYPE_CLOSURE:
                    case TYPE_MACRO:
                    case TYPE_NIL: {
                        return false;
                    }
                }

                guard_unreachable();
            }
            case TYPE_DICT:
            case TYPE_PRIMITIVE:
            case TYPE_CLOSURE:
            case TYPE_MACRO:
            case TYPE_NIL: {
                return false;
            }
        }
        return false;
    }

    switch (a->type) {
        case TYPE_INT: {
            *result = a->as_int < b->as_int;
            return true;
        }
        case TYPE_STRING: {
            *result = strcmp(a->as_string, b->as_string) < 0;
            return true;
        }
        case TYPE_ATOM: {
            *result = strcmp(a->as_atom, b->as_atom) < 0;
            return true;
        }
        case TYPE_CONS: {
            while (object_nil() != a && object_nil() != b) {
                bool elements_less;
                if (false == try_compare_less(a->as_cons.first, b->as_cons.first, &elements_less)) {
                    return false;
                }

                if (false == elements_less) {
                    *result = false;
                    return true;
                }

                object_list_shift(&a);
                object_list_shift(&b);
            }

            *result = object_nil() == a && object_nil() != b;
            return true;
        }
        case TYPE_DICT:
        case TYPE_PRIMITIVE:
        case TYPE_CLOSURE:
        case TYPE_MACRO:
        case TYPE_NIL: {
            return false;
        }
    }

    guard_unreachable();
}

static bool try_compare_greater(Object *a, Object *b, bool *result) {
    guard_is_not_null(a);
    guard_is_not_null(b);
    guard_is_not_null(result);

    return try_compare_less(b, a, result);
}

bool object_try_compare(Object *a, Object *b, Object_CompareKind kind, bool *result) {
    guard_is_not_null(a);
    guard_is_not_null(b);
    guard_is_not_null(result);

    switch (kind) {
        case COMPARE_LESS: {
            return try_compare_less(a, b, result);
        }
        case COMPARE_LESS_OR_EQUAL: {
            return false == try_compare_greater(a, b, result);
        }
        case COMPARE_GREATER: {
            return try_compare_greater(a, b, result);
        }
        case COMPARE_GREATER_OR_EQUAL: {
            return false == try_compare_less(a, b, result);
        }
        case COMPARE_EQUAL: {
            *result = equals(a, b);
            return true;
        }
    }

    guard_unreachable();
}

bool object_equals(Object *a, Object *b) {
    bool equals;
    guard_is_true(object_try_compare(a, b, COMPARE_EQUAL, &equals));

    return equals;
}
