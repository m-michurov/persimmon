#pragma once

#include <stdint.h>

typedef struct Arena_Region Arena_Region;

typedef struct {
    Arena_Region *_regions;
} Arena;

typedef struct {
    size_t regions;
    size_t system_memory_used_bytes;
    size_t allocated_bytes;
    size_t wasted_bytes;
} Arena_Statistics;

[[maybe_unused]]
Arena_Statistics arena_statistics(Arena const *a);

void arena_free(Arena *a);

[[nodiscard]]
bool arena_try_allocate(Arena *a, size_t alignment, size_t size, void **p, errno_t *error_code);

[[nodiscard]]
bool arena_try_allocate_copy(
        Arena *a,
        void const *src,
        size_t src_size,
        size_t alignment,
        size_t total_size,
        void **p,
        errno_t *error_code
);

#define arena_try_copy(Arena_, Src, SrcCount, TotalCount, Ptr, ErrorCode)   \
((typeof_unqual(*(Src)) *) arena_try_allocate_copy(                         \
    (Arena_),                                                               \
    (Src),                                                                  \
    sizeof(*(Src)) * (SrcCount),                                            \
    _Alignof(typeof(*(Src))),                                               \
    sizeof(*(Src)) * (TotalCount),                                          \
    (void **) (Ptr),                                                        \
    (ErrorCode)                                                             \
))

#define arena_try_copy_all(Arena_, Src, SrcCount, Ptr, ErrorCode) \
    arena_try_copy((Arena_), (Src), (SrcCount), (SrcCount), (Ptr), (ErrorCode))
