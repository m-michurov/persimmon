#pragma once

#include "object.h"

typedef struct ObjectAllocator ObjectAllocator;

ObjectAllocator *allocator_new(void);

void allocator_free(ObjectAllocator **a);

bool allocator_try_allocate(ObjectAllocator *a, size_t size, Object **obj);
