#pragma once

#include <stdint.h>

#include "arena.h"
#include "exchange.h"

typedef enum {
    TYPE_INT,
    TYPE_STRING,
    TYPE_ATOM,
    TYPE_CONS,
    TYPE_PRIMITIVE,
    TYPE_CLOSURE,
    TYPE_NIL,
    TYPE_MOVED
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

struct Object_Allocator;

typedef bool (*Object_Primitive)(struct Object_Allocator *, Object *args, Object **value, Object **error);

typedef struct Object_Closure {
    Object *env;
    Object *args;
    Object *body;
} Object_Closure;

struct Object {
    Object_Type type;
    [[maybe_unused]] size_t size;

    union {
        int64_t as_int;
        char as_string[1]; // actual size may vary
        char as_atom[1]; // actual size may vary
        Object_Cons as_cons;
        Object_Primitive as_primitive;
        Object_Closure as_closure;
        Object *as_moved;
    };
};

Object *object_nil();

bool object_equals(Object *a, Object *b);

char *object_repr(Arena *a, Object *object);
