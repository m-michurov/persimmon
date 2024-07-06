#include "arena.h"

#include <stdio.h>
#include <string.h>

#include "guards.h"
#include "pointers.h"
#include "math.h"
#include "exchange.h"

struct Arena_Region {
    Arena_Region *next;
    size_t capacity;
    uint8_t *top;
    uint8_t *end;
    uint8_t data[];
};

#define REGION_DEFAULT_CAPACITY ((size_t) 4 * 1024)

static bool region_try_create(size_t capacity_bytes, Arena_Region **r) {
    auto const size_bytes = sizeof(Arena_Region) + capacity_bytes;
    *r = (Arena_Region *) calloc(size_bytes, 1);
    if (nullptr == *r) {
        return false;
    }

    (*r)->capacity = capacity_bytes;
    (*r)->end = (*r)->data + (*r)->capacity;
    (*r)->top = (*r)->data;
    (*r)->next = nullptr;

    return true;
}

static bool region_try_allocate(Arena_Region *r, size_t alignment, size_t size, void **p) {
    guard_is_not_null(r);
    guard_is_not_null(p);
    guard_is_greater(alignment, 0);

    auto const ptr = (uint8_t *) pointer_roundup(r->top, alignment);
    if (ptr + size > r->end) {
        return false;
    }

    *p = ptr;
    r->top = ptr + size;
    return true;
}

static void arena_append_region(Arena *a, size_t capacity_bytes) {
    guard_is_not_null(a);

    Arena_Region *r;
    if (false == region_try_create(capacity_bytes, &r)) {
        auto const stats = arena_statistics(a);
        fprintf(stderr, "Arena info:\n");
        fprintf(stderr, "            Regions: %zu\n", stats.regions);
        fprintf(stderr, "  Total memory used: %zu bytes\n", stats.system_memory_used_bytes);
        fprintf(stderr, "          Allocated: %zu bytes\n", stats.allocated_bytes);
        fprintf(stderr, "             Wasted: %zu bytes\n", stats.wasted_bytes);
        exit(EXIT_FAILURE);
    }

    r->next = exchange(a->first, r);
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

    for (auto r = a->first; nullptr != r; r = r->next) {
        void *p;
        if (region_try_allocate(r, alignment, size, &p)) {
            return p;
        }
    }

    arena_append_region(a, max(size, REGION_DEFAULT_CAPACITY));
    void *p;
    guard_is_true(region_try_allocate(a->first, alignment, size, &p));
    return p;
}

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
        stats.system_memory_used_bytes += it->capacity;
        auto const wasted = (size_t) (it->end - it->top);
        stats.allocated_bytes += it->capacity - wasted;
        stats.wasted_bytes += wasted;
    }

    return stats;
}
