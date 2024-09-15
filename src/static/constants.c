#include "object/object.h"

Object *const OBJECT_NIL = &(Object) {.type = TYPE_NIL};

Object *const OBJECT_ERROR_OUT_OF_MEMORY = &(Object) {
        .type = TYPE_DICT,
        .as_dict = (Object_Dict) {
                .key = &(Object) {.type = TYPE_SYMBOL, .as_symbol = "type"},
                .value = &(Object) {.type = TYPE_SYMBOL, .as_symbol = "OutOfMemoryError"},
                .size = 1,
                .height = 1,
                .left = OBJECT_NIL,
                .right = OBJECT_NIL
        }
};
