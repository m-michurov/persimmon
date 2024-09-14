#include "constants.h"

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

Object *const OBJECT_ATOM_GET = make_atom("get");

Object *const OBJECT_ATOM_QUOTE = make_atom("quote");
