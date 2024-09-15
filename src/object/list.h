#pragma once

#include "utility/macros.h"
#include "object.h"
#include "allocator.h"

[[nodiscard]]
bool object_list_try_prepend(ObjectAllocator *a, Object *value, Object **list);

[[nodiscard]]
bool object_list_try_append_inplace(ObjectAllocator *a, Object *value, Object **list);

void object_list_concat_inplace(Object **head, Object *tail);

Object *object_list_shift(Object **list);

[[nodiscard]]
bool object_try_make_list_of_(ObjectAllocator *a, Object **list, ...);

#define object_try_make_list_of(ObjectAllocator_, List, ...) \
    object_try_make_list_of_((ObjectAllocator_), (List), __VA_ARGS__, nullptr)

#define object_list_for(It, List)                                       \
for (                                                                   \
    Object *concat_identifiers(_l_, __LINE__) = (List),                 \
           *It = TYPE_LIST == concat_identifiers(_l_, __LINE__)->type   \
                ? concat_identifiers(_l_, __LINE__)->as_list.first      \
                : OBJECT_NIL;                                           \
    TYPE_LIST == concat_identifiers(_l_, __LINE__)->type;               \
    concat_identifiers(_l_, __LINE__) =                                 \
        concat_identifiers(_l_, __LINE__)->as_list.rest,                \
    It = TYPE_LIST == concat_identifiers(_l_, __LINE__)->type           \
        ? concat_identifiers(_l_, __LINE__)->as_list.first              \
        : OBJECT_NIL                                                    \
)

size_t object_list_count(Object *list);

void object_list_reverse_inplace(Object **list);

Object *object_list_nth(size_t n, Object *list);

Object **object_list_nth_mutable(size_t n, Object *list);

Object **object_list_end_mutable(Object **list);

Object *object_list_skip(size_t n, Object *list);

[[nodiscard]]
bool object_list_try_unpack_2(Object **_1, Object **_2, Object *list);

[[nodiscard]]
bool object_list_try_unpack_3(Object **_1, Object **_2, Object **_3, Object *list);

[[nodiscard]]
bool object_list_is_tagged(Object *list, char const **tag);

[[nodiscard]]
bool object_list_try_get_tagged_field(Object *list, char const *tag, Object **value);
