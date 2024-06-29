#include "object_lists.h"

#include <stdarg.h>

#include "object_constructors.h"
#include "object_accessors.h"
#include "guards.h"

Object *object_list_(Object_Allocator *a, ...) {
    guard_is_not_null(a);

    auto head = object_nil();
    va_list args;
    va_start(args, a);
    while (true) {
        auto const arg = va_arg(args, Object *);
        if (nullptr == arg) {
            break;
        }

        head = object_cons(a, arg, head);
    }

    object_list_reverse(&head);
    return head;
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

Object *object_list_nth(Object *list, size_t n) {
    guard_is_not_null(list);
    guard_is_one_of(list->type, TYPE_CONS, TYPE_NIL);

    size_t i = 0;
    object_list_for(it, list) {
        if (i < n) {
            i++;
            continue;
        }

        return it;
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
