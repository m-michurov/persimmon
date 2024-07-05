#pragma once

#include "object.h"
#include "object_allocator.h"

bool object_try_make_int(ObjectAllocator *a, int64_t value, Object **obj);

bool object_try_make_string(ObjectAllocator *a, char const *s, Object **obj);

bool object_try_make_atom(ObjectAllocator *a, char const *s, Object **obj);

bool object_try_make_cons(ObjectAllocator *a, Object *first, Object *rest, Object **obj);

bool object_try_make_primitive(ObjectAllocator *a, Object_Primitive fn, Object **obj);

bool object_try_make_closure(ObjectAllocator *a, Object *env, Object *args, Object *body, Object **obj);
