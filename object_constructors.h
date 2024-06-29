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

Object *object_int(Object_Allocator *a, int64_t value);

Object *object_string(Object_Allocator *a, char const *s);

Object *object_atom(Object_Allocator *a, char const *s);

Object *object_cons(Object_Allocator *a, Object *first, Object *rest);

Object *object_primitive(Object_Allocator *a, Object_Primitive fn);

Object *object_closure(Object_Allocator *a, Object *env, Object *args, Object *body);
