#pragma once

#include "object.h"
#include "allocator.h"

Object *object_int(ObjectAllocator *a, int64_t value);

Object *object_string(ObjectAllocator *a, char const *s);

Object *object_atom(ObjectAllocator *a, char const *s);

Object *object_cons(ObjectAllocator *a, Object *first, Object *rest);

Object *object_primitive(ObjectAllocator *a, Object_Primitive fn);

Object *object_closure(ObjectAllocator *a, Object *env, Object *args, Object *body);
