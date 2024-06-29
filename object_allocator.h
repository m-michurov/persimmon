#pragma once

#include "object.h"

typedef struct Object_Allocator Object_Allocator;

typedef struct {
    void (*free)(Object_Allocator *);

    Object *(*allocate)(Object_Allocator *, size_t size);
} Object_Allocator_VMT;

struct Object_Allocator {
    Object_Allocator_VMT const *methods;
};

void object_allocator_free(Object_Allocator **a);

Object *object_allocate(Object_Allocator *a, size_t size);
