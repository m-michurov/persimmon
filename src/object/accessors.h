#pragma once

#include "object.h"

int64_t object_as_int(Object *obj);

char const *object_as_string(Object *obj);

char const *object_as_atom(Object *obj);

Object_Cons object_as_cons(Object *obj);

Object_Primitive object_as_primitive(Object *obj);

Object_Closure object_as_closure(Object *obj);
