#include "object/object.h"

#define make_atom(Name) \
(&(Object) {            \
    .type = TYPE_ATOM,  \
    .as_atom = (Name)   \
})

#define make_dict_of(Key, Value)    \
(&(Object) {                        \
    .type = TYPE_DICT,              \
    .as_dict = (Object_Dict) {      \
            .key = (Key),           \
            .value = (Value),       \
            .size = 1,              \
            .height = 1,            \
            .left = OBJECT_NIL,     \
            .right = OBJECT_NIL     \
    }                               \
})

Object *const OBJECT_NIL = &(Object) {.type = TYPE_NIL};

Object *const OBJECT_ERROR_OUT_OF_MEMORY = make_dict_of(make_atom("type"), make_atom("OutOfMemoryError"));
