#pragma once

#include <stdint.h>
#include <stdio.h>

#include "utility/string_builder.h"

typedef enum {
    TYPE_INT,
    TYPE_STRING,
    TYPE_ATOM,
    TYPE_CONS,
    TYPE_PRIMITIVE,
    TYPE_CLOSURE,
    TYPE_NIL
} Object_Type;

char const *object_type_str(Object_Type type);

typedef struct Object Object;

typedef struct {
    Object **data;
    size_t count;
    size_t capacity;
} Objects;

typedef struct {
    Object *first;
    Object *rest;
} Object_Cons;

struct VirtualMachine;

typedef bool (*Object_Primitive)(struct VirtualMachine *, Object *args, Object **value, Object **error);

typedef struct Object_Closure {
    Object *env;
    Object *args;
    Object *body;
} Object_Closure;

typedef enum {
    OBJECT_WHITE,
    OBJECT_GRAY,
    OBJECT_BLACK
} Object_Color;

struct Object {
    Object_Type type;
    Object_Color color;
    Object *next;

    union {
        int64_t as_int;
        char as_string[1]; // actual size may vary
        char as_atom[1]; // actual size may vary
        Object_Cons as_cons;
        Object_Primitive as_primitive;
        Object_Closure as_closure;
    };
};

Object *object_nil();

bool object_equals(Object *a, Object *b);

void object_repr_sb(Object *object, StringBuilder *sb);

void object_repr_print(Object *object, FILE *file);
