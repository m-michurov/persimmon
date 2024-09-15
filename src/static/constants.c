#include "object/object.h"

Object *const OBJECT_NIL = &(Object) {.type = TYPE_NIL};

Object *const OBJECT_ERROR_OUT_OF_MEMORY = &(Object) {
        .type = TYPE_DICT,
        .as_dict = (Object_Dict) {
                .key = &(Object) {.type = TYPE_ATOM, .as_atom = "type"},
                .value = &(Object) {.type = TYPE_ATOM, .as_atom = "OutOfMemoryError"},
                .size = 1,
                .height = 1,
                .left = OBJECT_NIL,
                .right = OBJECT_NIL
        }
};
