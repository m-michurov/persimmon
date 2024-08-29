#pragma once

#include <stdint.h>
#include <stdio.h>

#include "utility/string_builder.h"

typedef enum {
    TYPE_INT,
    TYPE_STRING,
    TYPE_ATOM,
    TYPE_CONS,
    TYPE_DICT_ENTRIES,
    TYPE_DICT,
    TYPE_PRIMITIVE,
    TYPE_CLOSURE,
    TYPE_MACRO,
    TYPE_NIL,
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

typedef bool (*Object_Primitive)(struct VirtualMachine *, Object *args, Object **value);

typedef struct {
    Object *env;
    Object *args;
    Object *body;
} Object_Closure;

typedef struct {
    bool used;
    Object *key;
    Object *value;
} Object_DictEntry;

typedef struct {
    size_t used;
    size_t count;
    Object_DictEntry data[];
} Object_DictEntries;

typedef struct {
    Object *entries;
    Object *new_entries;
} Object_Dict;

typedef enum {
    OBJECT_WHITE,
    OBJECT_GRAY,
    OBJECT_BLACK
} Object_Color;

struct Object {
    Object_Type type;
    size_t size;
    Object_Color color;
    Object *next;

    union {
        int64_t as_int;
        char as_string[1]; // actual size may vary
        char as_atom[1]; // actual size may vary
        Object_Cons as_cons;
        Object_Primitive as_primitive;
        Object_Closure as_closure;
        Object_DictEntries as_dict_entries;
        Object_Dict as_dict;
    };
};

Object *object_nil();
