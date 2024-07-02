#pragma once

#include "object.h"
#include "object_allocator.h"

size_t object_min_size_int(void);

size_t object_min_size_string(size_t len);

size_t object_min_size_atom(size_t len);

size_t object_min_size_cons(void);

size_t object_min_size_primitive(void);

size_t object_min_size_closure(void);

Object *object_init_int(Object *obj, int64_t value);

Object *object_init_string(Object *obj, char const *s, size_t len);

Object *object_init_atom(Object *obj, char const *s, size_t len);

Object *object_init_cons(Object *obj, Object *first, Object *rest);

Object *object_init_primitive(Object *obj, Object_Primitive fn);

Object *object_init_closure(Object *obj, Object *env, Object *args, Object *body);

Object *object_int(ObjectAllocator *a, int64_t value);

Object *object_string(ObjectAllocator *a, char const *s);

Object *object_atom(ObjectAllocator *a, char const *s);

Object *object_cons(ObjectAllocator *a, Object *first, Object *rest);

Object *object_primitive(ObjectAllocator *a, Object_Primitive fn);

Object *object_closure(ObjectAllocator *a, Object *env, Object *args, Object *body);
