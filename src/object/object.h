#pragma once

#include <stdint.h>
#include <stdio.h>

#include "utility/string_builder.h"

typedef enum {
    TYPE_NIL,
    TYPE_INT,
    TYPE_STRING,
    TYPE_ATOM,
    TYPE_CONS,
    TYPE_DICT,
    TYPE_PRIMITIVE,
    TYPE_CLOSURE,
    TYPE_MACRO,
} Object_Type;

char const *object_type_str(Object_Type type);

typedef struct Object Object;

typedef struct {
    Object *first;
    Object *rest;
} Object_Cons;

struct VirtualMachine;

typedef bool (*Object_Primitive)(struct VirtualMachine *, Object *args, Object **value);

typedef struct {
    Object *env;
    Object *args;
    Object *body;
} Object_Closure;

typedef struct {
    Object *key;
    Object *value;
    int64_t height;
    size_t size;
    Object *left;
    Object *right;
} Object_Dict;

typedef enum {
    OBJECT_WHITE,
    OBJECT_GRAY,
    OBJECT_BLACK
} Object_Color;

struct Object {
    size_t size;
    Object_Color color;
    Object *next;

    Object_Type type;

    union {
        int64_t as_int;
        char const *as_string;
        char const *as_atom;
        Object_Cons as_cons;
        Object_Primitive as_primitive;
        Object_Closure as_closure;
        Object_Dict as_dict;
    };
};

typedef struct {
    Object **data;
    size_t count;
    size_t capacity;
} Objects;

typedef struct {
    bool has_value;
    Object *value;
} ObjectOption;
