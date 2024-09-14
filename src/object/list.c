#include "list.h"

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

bool object_list_try_append_inplace(ObjectAllocator *a, Object *value, Object **list) {
    guard_is_not_null(a);
    guard_is_not_null(value);
    guard_is_not_null(list);
    guard_is_not_null(*list);
    guard_is_one_of((*list)->type, TYPE_NIL, TYPE_CONS);

    while (OBJECT_NIL != *list) {
        list = (Object **) &(*list)->as_cons.rest;
    }

    return object_try_make_cons(a, value, OBJECT_NIL, list);
}

void object_list_concat_inplace(Object **head, Object *tail) {
    guard_is_not_null(head);
    guard_is_not_null(*head);
    guard_is_not_null(tail);
    guard_is_one_of((*head)->type, TYPE_NIL, TYPE_CONS);
    guard_is_one_of(tail->type, TYPE_NIL, TYPE_CONS);

    while (OBJECT_NIL != *head) {
        head = (Object **) &(*head)->as_cons.rest;
    }

    *head = tail;
}

static bool try_shift(Object **list, Object **head) {
    guard_is_not_null(list);
    guard_is_not_null(*list);
    guard_is_not_null(head);
    guard_is_one_of((*list)->type, TYPE_NIL, TYPE_CONS);

    if (OBJECT_NIL == *list) {
        return false;
    }

    *head = (*list)->as_cons.first;
    *list = (*list)->as_cons.rest;

    return true;
}

Object *object_list_shift(Object **list) {
    guard_is_not_null(list);
    guard_is_not_null(*list);
    guard_is_equal((*list)->type, TYPE_CONS);

    Object *head;
    guard_is_true(try_shift(list, &head));
    return head;
}

bool object_try_make_list_(ObjectAllocator *a, Object **list, ...) {
    guard_is_not_null(a);
    guard_is_not_null(list);

    *list = OBJECT_NIL;

    va_list args;
    va_start(args, list);
    while (true) {
        auto const arg = va_arg(args, Object *);
        if (nullptr == arg) {
            break;
        }

        if (false == object_list_try_prepend(a, arg, list)) {
            return false;
        }
    }

    object_list_reverse_inplace(list);
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

void object_list_reverse_inplace(Object **list) {
    guard_is_not_null(list);
    guard_is_not_null(*list);
    guard_is_one_of((*list)->type, TYPE_CONS, TYPE_NIL);

    auto prev = OBJECT_NIL;
    auto current = *list;
    while (OBJECT_NIL != current) {
        auto const next = exchange(current->as_cons.rest, prev); // NOLINT(*-sizeof-expression)
        prev = exchange(current, next);
    }

    *list = prev;
}

Object *object_list_nth(size_t n, Object *list) {
    return *object_list_nth_mutable(n, list);
}

Object **object_list_nth_mutable(size_t n, Object *list) {
    guard_is_not_null(list);
    guard_is_one_of(list->type, TYPE_CONS, TYPE_NIL);

    size_t i = 0;
    for (auto it = list; OBJECT_NIL != it; it = it->as_cons.rest) {
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

Object **object_list_end_mutable(Object **list) {
    guard_is_not_null(list);
    guard_is_not_null(*list);
    guard_is_one_of((*list)->type, TYPE_CONS, TYPE_NIL);

    while (OBJECT_NIL != *list) {
        list = &(*list)->as_cons.rest;
    }

    return list;
}

Object *object_list_skip(size_t n, Object *list) {
    guard_is_not_null(list);
    guard_is_one_of(list->type, TYPE_CONS, TYPE_NIL);

    size_t i = 0;
    for (; OBJECT_NIL != list; list = object_as_cons(list).rest) {
        guard_is_equal(list->type, TYPE_CONS);

        if (i < n) {
            i++;
            continue;
        }

        return list;
    }

    if (i == n && OBJECT_NIL == list) {
        return OBJECT_NIL;
    }

    guard_assert(false, "cannot skip %zu elements for prim_list_list of %zu elements", n, i);
}

bool object_list_try_unpack_2(Object **_1, Object **_2, Object *list) {
    guard_is_not_null(_1);
    guard_is_not_null(_2);
    guard_is_not_null(list);
    guard_is_one_of(list->type, TYPE_CONS, TYPE_NIL);

    return try_shift(&list, _1)
           && try_shift(&list, _2)
           && OBJECT_NIL == list;
}

bool object_list_try_unpack_3(Object **_1, Object **_2, Object **_3, Object *list) {
    guard_is_not_null(_1);
    guard_is_not_null(_2);
    guard_is_not_null(_3);
    guard_is_not_null(list);
    guard_is_one_of(list->type, TYPE_CONS, TYPE_NIL);

    return try_shift(&list, _1)
           && try_shift(&list, _2)
           && try_shift(&list, _3)
           && OBJECT_NIL == list;
}

bool object_list_is_tagged(Object *list, char const **tag) {
    guard_is_not_null(list);
    guard_is_not_null(tag);

    if (TYPE_CONS != list->type) {
        return false;
    }

    auto const first = list->as_cons.first;
    if (TYPE_ATOM != first->type) {
        return false;
    }

    *tag = first->as_atom;
    return true;
}

bool object_list_try_get_tagged_field(Object *list, char const *tag, Object **value) {
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
