#include "lists.h"

#include <stdarg.h>

#include "utility/guards.h"
#include "utility/exchange.h"
#include "constructors.h"
#include "accessors.h"

bool object_list_try_prepend(ObjectAllocator *a, Object *value, Object **list) {
    guard_is_not_null(a);
    guard_is_not_null(value);
    guard_is_not_null(list);
    guard_is_not_null(*list);

    return object_try_make_cons(a, value, *list, list);
}

Object *object_list_shift(Object **list) {
    guard_is_not_null(list);
    guard_is_not_null(*list);
    guard_is_equal((*list)->type, TYPE_CONS);

    auto const first = (*list)->as_cons.first;
    *list = (*list)->as_cons.rest;
    return first;
}

bool object_try_make_list_(ObjectAllocator *a, Object **list, ...) {
    guard_is_not_null(a);
    guard_is_not_null(list);

    *list = object_nil();

    va_list args;
    va_start(args, list);
    while (true) {
        auto const arg = va_arg(args, Object *);
        if (nullptr == arg) {
            break;
        }

        if (false == object_try_make_cons(a, arg, *list, list)) {
            return false;
        }
    }

    object_list_reverse(list);
    return true;
}

size_t object_list_count(Object *list) {
    guard_is_not_null(list);

    auto count = (size_t) {0};
    object_list_for(it, list) {
        (void) it;
        count++;
    }

    return count;
}

void object_list_reverse(Object **list) {
    guard_is_not_null(list);
    guard_is_not_null(*list);
    guard_is_one_of((*list)->type, TYPE_CONS, TYPE_NIL);

    auto prev = object_nil();
    auto current = *list;
    while (object_nil() != current) {
        auto const next = exchange(current->as_cons.rest, prev);
        prev = exchange(current, next);
    }

    *list = prev;
}

Object **object_list_nth(size_t n, Object *list) {
    guard_is_not_null(list);
    guard_is_one_of(list->type, TYPE_CONS, TYPE_NIL);

    size_t i = 0;
    for (auto it = list; object_nil() != it; it = it->as_cons.rest) {
        if (TYPE_CONS != it->type) {
            break;
        }

        if (i < n) {
            i++;
            continue;
        }

        return &it->as_cons.first;
    }

    guard_assert(false, "list index %zu is out of range for prim_list_list of %zu elements", n, i);
}

Object *object_list_skip(Object *list, size_t n) {
    guard_is_not_null(list);
    guard_is_one_of(list->type, TYPE_CONS, TYPE_NIL);

    size_t i = 0;
    for (; object_nil() != list; list = object_as_cons(list).rest) {
        guard_is_equal(list->type, TYPE_CONS);

        if (i < n) {
            i++;
            continue;
        }

        return list;
    }

    if (i == n && object_nil() == list) {
        return object_nil();
    }

    guard_assert(false, "cannot skip %zu elements for prim_list_list of %zu elements", n, i);
}

bool object_list_try_unpack_2(Object **_1, Object **_2, Object *list) {
    guard_is_not_null(_1);
    guard_is_not_null(_2);
    guard_is_not_null(list);
    guard_is_one_of(list->type, TYPE_CONS, TYPE_NIL);

    if (2 != object_list_count(list)) {
        return false;
    }

    *_1 = list->as_cons.first;
    list = list->as_cons.rest;
    *_2 = list->as_cons.first;
    list = list->as_cons.rest;
    (void) list;
    return true;
}

bool object_list_try_get_tagged(Object *list, char const *tag, Object **value) {
    guard_is_not_null(list);
    guard_is_not_null(tag);
    guard_is_not_null(value);

    object_list_for(it, list) {
        if (TYPE_CONS != it->type) {
            continue;
        }

        Object *key;
        if (false == object_list_try_unpack_2(&key, value, it)) {
            continue;
        }

        if (TYPE_ATOM == key->type && 0 == strcmp(tag, key->as_atom)) {
            return true;
        }
    }

    return false;
}
