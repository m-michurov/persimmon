#pragma once

#include "object.h"

typedef struct ObjectAllocator ObjectAllocator;

typedef struct {
    size_t hard_limit;
    size_t soft_limit_initial;
    double soft_limit_grow_factor;
} ObjectAllocator_Config;

ObjectAllocator *allocator_new(ObjectAllocator_Config config);

void allocator_free(ObjectAllocator **a);

struct Stack;
struct Parser_Stack;

typedef struct {
    struct Stack const *stack;
    struct Parser_Stack const *parser_stack;
    Object *const *parser_expr;
    Objects *temporaries;
} ObjectAllocator_Roots;

void allocator_set_roots(ObjectAllocator *a, ObjectAllocator_Roots roots);

bool allocator_try_allocate(ObjectAllocator *a, size_t size, Object **obj);
