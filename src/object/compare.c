#include "compare.h"

#include "utility/guards.h"
#include "list.h"
#include "dict.h"

static Object_CompareResult compare_closure(Object_Closure a, Object_Closure b) { // NOLINT(*-no-recursion)
    int result;
    if (OBJECT_EQUALS != (result = object_compare(a.args, b.args))) {
        return result;
    }

    if (OBJECT_EQUALS != (result = object_compare(a.body, b.body))) {
        return result;
    }

    if (OBJECT_EQUALS != (result = object_compare(a.env, b.env))) {
        return result;
    }

    return 0;
}

Object_CompareResult object_compare(Object *a, Object *b) { // NOLINT(*-no-recursion)
    guard_is_not_null(a);
    guard_is_not_null(b);

    if (a->type != b->type) {
        return a->type > b->type ? OBJECT_GREATER : OBJECT_LESS;
    }

    switch (a->type) {
        case TYPE_NIL: {
            return OBJECT_EQUALS;
        }
        case TYPE_INT: {
            if (a->as_int == b->as_int) {
                return OBJECT_EQUALS;
            }

            return a->as_int > b->as_int ? OBJECT_GREATER : OBJECT_LESS;
        }
        case TYPE_STRING: {
            return strcmp(a->as_string, b->as_string);
        }
        case TYPE_SYMBOL: {
            return strcmp(a->as_symbol, b->as_symbol);
        }
        case TYPE_LIST: {
            while (OBJECT_NIL != a && OBJECT_NIL != b) {
                auto const element_compare_result = object_compare(a->as_list.first, b->as_list.first);
                if (0 != element_compare_result) {
                    return element_compare_result;
                }

                object_list_shift(&a);
                object_list_shift(&b);
            }

            if (OBJECT_NIL == a && OBJECT_NIL == b) {
                return OBJECT_EQUALS;
            }

            return OBJECT_NIL == b ? OBJECT_GREATER : OBJECT_LESS;
        }
        case TYPE_DICT: {
            return object_dict_compare(a, b);
        }
        case TYPE_PRIMITIVE: {
            return (uintptr_t) a->as_primitive > (uintptr_t) b->as_primitive ? OBJECT_GREATER : OBJECT_LESS;
        }
        case TYPE_CLOSURE:
        case TYPE_MACRO: {
            return compare_closure(a->as_closure, b->as_closure);
        }
    }

    guard_unreachable();
}

bool object_equals(Object *a, Object *b) {
    return 0 == object_compare(a, b);
}
