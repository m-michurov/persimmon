#pragma once

#include "object.h"

typedef enum {
    ALLOCATOR_SOFT_GC,
    ALLOCATOR_ALWAYS_GC,
    ALLOCATOR_NEVER_GC,
} ObjectAllocator_GarbageCollectionMode;

struct Stack;
struct Parser_ExpressionsStack;

typedef struct {
    struct Stack *stack;
    struct Parser_ExpressionsStack *parser_stack;
    Object **parser_expr;
    Object **globals;
    Object **value;
    Object **error;
    Object **exprs;
} ObjectAllocator_Roots;

typedef struct ObjectAllocator ObjectAllocator;

struct ObjectAllocator {
    Object *_objects;
    Object *_freed;
    ObjectAllocator_Roots _roots;
    ObjectAllocator_GarbageCollectionMode _gc_mode;
    bool _trace;
    bool _no_free;
    bool _gc_is_running;
    size_t _heap_size;
    size_t _hard_limit;
    size_t _soft_limit;
    double _grow_factor;
};

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

ObjectAllocator allocator_make(ObjectAllocator_Config config);

void allocator_free(ObjectAllocator *a);

void allocator_set_roots(ObjectAllocator *a, ObjectAllocator_Roots roots);

[[nodiscard]]
bool allocator_try_allocate(ObjectAllocator *a, size_t size, Object **obj);

void allocator_print_statistics(ObjectAllocator *a, FILE *file);
