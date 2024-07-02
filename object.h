#pragma once

#include <stdint.h>
#include <stdio.h>

#include "arena.h"
#include "exchange.h"

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

struct ObjectAllocator;

typedef bool (*Object_Primitive)(struct ObjectAllocator *, Object *args, Object **value);

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

char *object_repr(Arena *a, Object *object);

void object_repr_print(FILE *file, Object *object);
