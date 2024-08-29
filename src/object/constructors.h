#pragma once

#include "object.h"
#include "allocator.h"

[[nodiscard]]
bool object_try_make_int(ObjectAllocator *a, int64_t value, Object **obj);

[[nodiscard]]
bool object_try_make_string(ObjectAllocator *a, char const *s, Object **obj);

[[nodiscard]]
bool object_try_make_atom(ObjectAllocator *a, char const *s, Object **obj);

[[nodiscard]]
bool object_try_make_cons(ObjectAllocator *a, Object *first, Object *rest, Object **obj);

[[nodiscard]]
bool object_try_make_primitive(ObjectAllocator *a, Object_Primitive fn, Object **obj);

[[nodiscard]]
bool object_try_make_closure(ObjectAllocator *a, Object *env, Object *args, Object *body, Object **obj);

[[nodiscard]]
bool object_try_make_macro(ObjectAllocator *a, Object *env, Object *args, Object *body, Object **obj);

[[nodiscard]]
bool object_try_make_dict_entries(ObjectAllocator *a, size_t count, Object **obj);

[[nodiscard]]
bool object_try_make_dict(ObjectAllocator *a, Object *entries, Object **obj);

[[nodiscard]]
bool object_try_copy(ObjectAllocator *a, Object *obj, Object **copy);