#pragma once

#include "object.h"
#include "allocator.h"

bool object_try_make_list_(ObjectAllocator *a, Object **list, ...);

#define object_try_make_list(Object_Allocator_, List, ...) \
    object_try_make_list_((Object_Allocator_), (List), __VA_ARGS__, nullptr)

#define OBJECT__concat_(A, B) A ## B
#define OBJECT__concat(A, B) OBJECT__concat_(A, B)

#define object_list_for(It, List)                               \
for (                                                           \
    Object *OBJECT__concat(_l_, __LINE__) = (List),             \
           *It = TYPE_CONS == OBJECT__concat(_l_, __LINE__)->type   \
                ? OBJECT__concat(_l_, __LINE__)->as_cons.first  \
                : object_nil();                                 \
    TYPE_CONS == OBJECT__concat(_l_, __LINE__)->type;              \
    OBJECT__concat(_l_, __LINE__) =                             \
        OBJECT__concat(_l_, __LINE__)->as_cons.rest,            \
    It = TYPE_CONS == OBJECT__concat(_l_, __LINE__)->type          \
        ? OBJECT__concat(_l_, __LINE__)->as_cons.first          \
        : object_nil()                                          \
)

size_t object_list_count(Object *list);

void object_list_reverse(Object **list);

Object *object_list_nth(Object *list, size_t n);

Object *object_list_skip(Object *list, size_t n);

bool object_list_try_unpack_2(Object **_1, Object **_2, Object *list);
