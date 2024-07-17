#pragma once

#include "object.h"

typedef struct ObjectAllocator ObjectAllocator;

typedef enum {
    ALLOCATOR_SOFT_GC,
    ALLOCATOR_ALWAYS_GC,
    ALLOCATOR_NEVER_GC,
} ObjectAllocator_GarbageCollectionMode;

typedef struct {
    size_t hard_limit;
    size_t soft_limit_initial;
    double soft_limit_grow_factor;

    struct {
        ObjectAllocator_GarbageCollectionMode gc_mode;
        bool no_free;
        bool trace;
    } debug;
} ObjectAllocator_Config;

ObjectAllocator *allocator_new(ObjectAllocator_Config config);

void allocator_free(ObjectAllocator **a);

struct Stack;
struct Parser_ExpressionsStack;

typedef struct {
    struct Stack const *stack;
    struct Parser_ExpressionsStack const *parser_stack;
    Object *const *parser_expr;
    Object *const *globals;
    Object *const *value;
    Object *const *error;
    Object *const *exprs;
    Objects const *constants;
} ObjectAllocator_Roots;

void allocator_set_roots(ObjectAllocator *a, ObjectAllocator_Roots roots);

bool allocator_try_allocate(ObjectAllocator *a, size_t size, Object **obj);

void allocator_print_statistics(ObjectAllocator const *a, FILE *file);
