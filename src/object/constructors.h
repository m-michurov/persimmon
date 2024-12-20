#pragma once

#include "object.h"
#include "allocator.h"

[[nodiscard]]
bool object_try_make_int(ObjectAllocator *a, int64_t value, Object **obj);

[[nodiscard]]
bool object_try_make_string(ObjectAllocator *a, char const *s, Object **obj);

[[nodiscard]]
bool object_try_make_symbol(ObjectAllocator *a, char const *s, Object **obj);

[[nodiscard]]
bool object_try_make_list(ObjectAllocator *a, Object *first, Object *rest, Object **obj);

[[nodiscard]]
bool object_try_make_primitive(ObjectAllocator *a, Object_Primitive fn, Object **obj);

[[nodiscard]]
bool object_try_make_closure(ObjectAllocator *a, Object *env, Object *args, Object *body, Object **obj);

[[nodiscard]]
bool object_try_make_macro(ObjectAllocator *a, Object *env, Object *args, Object *body, Object **obj);

[[nodiscard]]
bool object_try_make_dict(ObjectAllocator *a, Object *key, Object *value, Object *left, Object *right, Object **obj);

[[nodiscard]]
bool object_try_deep_copy(ObjectAllocator *a, Object *obj, Object **copy);

[[nodiscard]]
bool object_try_shallow_copy(ObjectAllocator *a, Object *obj, Object **copy);