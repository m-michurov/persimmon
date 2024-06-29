#include "arena.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "guards.h"

struct Arena_Region {
    size_t capacity;
    uint8_t *begin;
    uint8_t *top;
    Arena_Region *next;
    uint8_t data[];
};

#define REGION_DEFAULT_CAPACITY ((size_t) 4 * 1024)

static Arena_Region *region_create(size_t capacity_bytes) {
    auto const size_bytes = sizeof(Arena_Region) + capacity_bytes;

    errno = 0;
    auto region = (Arena_Region *) calloc(size_bytes, 1);
    if (nullptr == region) {
        fprintf(stderr, "[region_create] failed to allocate %zu bytes: %s\n", size_bytes, strerror(errno));
        exit(EXIT_FAILURE);
    }

    region->capacity = capacity_bytes;
    region->begin = region->data;
    region->top = region->begin + capacity_bytes;
    region->next = nullptr;

    return region;
}

static void arena_append_region(Arena *a, size_t capacity_bytes) {
    guard_is_not_null(a);

    auto region = region_create(capacity_bytes);
    if (nullptr == a->last) {
        a->first = a->last = region;
        return;
    }

    a->last->next = region;
    a->last = region;
}

static uint8_t *arena_align(uint8_t *top, size_t alignment) {
    while ((ptrdiff_t) top % alignment) {
        top--;
    }
    return top;
}

void arena_free(Arena *a) {
    guard_is_not_null(a);

    auto region = a->first;
    while (nullptr != region) {
        auto next = region->next;
        free(region);
        region = next;
    }

    *a = (Arena) {0};
}

void *arena_allocate(Arena *a, size_t alignment, size_t size) {
    guard_is_not_null(a);
    guard_is_greater(alignment, 0);

    if (nullptr == a->last || arena_align(a->last->top - size, alignment) < a->last->begin) {
        arena_append_region(a, REGION_DEFAULT_CAPACITY > size ? REGION_DEFAULT_CAPACITY : size);
    }

    a->last->top = arena_align(a->last->top - size, alignment);
    guard_is_greater_or_equal(a->last->top, a->last->begin);
    return a->last->top;
}

// TODO check if src is equal to top of a region, try reusing that region
void *arena_allocate_copy(Arena *a, void const *src, size_t src_size, size_t alignment, size_t total_size) {
    guard_is_not_null(a);

    if (nullptr == src) {
        src_size = 0;
    }

    if (src_size > total_size) {
        src_size = total_size;
    }

    auto p = arena_allocate(a, alignment, total_size);
    memcpy(p, src, src_size);

    return p;
}

Arena_Statistics arena_statistics(Arena const *a) {
    guard_is_not_null(a);

    auto stats = (Arena_Statistics) {0};
    for (auto it = a->first; nullptr != it; it = it->next) {
        stats.regions++;
        stats.memory_used_bytes += it->capacity;
        auto const wasted = (size_t) (it->top - it->begin);
        stats.allocated_bytes += it->capacity - wasted;
        stats.wasted_bytes += wasted;
    }

    return stats;
}
