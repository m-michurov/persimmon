#pragma once

#include <stdint.h>

typedef struct Arena_Region Arena_Region;

typedef struct {
    Arena_Region *first;
} Arena;

typedef struct {
    size_t regions;
    size_t system_memory_used_bytes;
    size_t allocated_bytes;
    size_t wasted_bytes;
} Arena_Statistics;

Arena_Statistics arena_statistics(Arena const *a);

void arena_free(Arena *a);

void *arena_allocate(Arena *a, size_t alignment, size_t size);

void *arena_allocate_copy(Arena *a, void const *src, size_t src_size, size_t alignment, size_t total_size);

#define arena_new_array(Arena_, T, Count) ((T *) arena_allocate((Arena_), _Alignof(T), sizeof(T) * (Count)))

#define arena_new(Arena_, T) arena_new_array((Arena_), T, 1)

#define arena_copy(Arena_, Src, SrcCount, TotalCount)   \
((typeof_unqual(*(Src)) *) arena_allocate_copy(         \
    (Arena_),                                           \
    (Src),                                              \
    sizeof(*(Src)) * (SrcCount),                        \
    _Alignof(typeof(*(Src))),                           \
    sizeof(*(Src)) * (TotalCount)                       \
))

#define arena_copy_all(Arena_, Src, SrcCount) arena_copy((Arena_), (Src), (SrcCount), (SrcCount))

#define arena_emplace(Arena_, Init) ((typeof(Init) *) arena_copy((Arena_), (typeof(Init)[]) {(Init)}, 1, 1))
