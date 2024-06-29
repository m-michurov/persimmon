#include "object_arena_allocator.h"

#include <string.h>

#include "guards.h"

typedef struct {
    Object_Allocator super;
    Arena *arena;
} Object_ArenaAllocator;

static void free_(Object_Allocator *a) {
    (void) a;
}

static Object *allocate(Object_Allocator *a, size_t size) {
    guard_is_not_null(a);

    auto const self = (Object_ArenaAllocator *) a;
    auto const obj = (Object *) arena_allocate(self->arena, _Alignof(Object), size);
    obj->size = size;
    return obj;
}

static auto const VMT = (Object_Allocator_VMT) {
        .free = free_,
        .allocate = allocate
};

Object_Allocator *object_arena_allocator_new(Arena *a) {
    auto const allocator = arena_emplace(a, ((Object_ArenaAllocator) {
            .super = (Object_Allocator) {.methods = &VMT},
            .arena = a
    }));
    return &allocator->super;
}
