#include "object/object.h"
#include "vm/errors.h"

Object *const OBJECT_NIL = &(Object) {.type = TYPE_NIL};

Object *const OBJECT_TRUE = &(Object) {.type = TYPE_SYMBOL, .as_symbol = "true"};

Object *const OBJECT_ERROR_OUT_OF_MEMORY = &(Object) {
        .type = TYPE_DICT,
        .as_dict = (Object_Dict) {
                .key = &(Object) {.type = TYPE_SYMBOL, .as_symbol = ERROR_KEY_NAME_TYPE},
                .value = &(Object) {.type = TYPE_SYMBOL, .as_symbol = "OutOfMemoryError"},
                .size = 1,
                .height = 1,
                .left = OBJECT_NIL,
                .right = OBJECT_NIL
        }
};
