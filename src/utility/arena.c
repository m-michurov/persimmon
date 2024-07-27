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

[[nodiscard]]
static bool region_try_create(size_t capacity_bytes, Arena_Region **r, errno_t *error_code) {
    guard_is_not_null(r);
    guard_is_not_null(error_code);

    auto const size_bytes = sizeof(Arena_Region) + capacity_bytes;

    errno = 0;
    *r = (Arena_Region *) calloc(size_bytes, 1);
    if (nullptr == *r) {
        *error_code = errno;
        return false;
    }

    (*r)->capacity = capacity_bytes;
    (*r)->end = (*r)->data + (*r)->capacity;
    (*r)->top = (*r)->data;
    (*r)->next = nullptr;

    return true;
}

[[nodiscard]]
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

[[nodiscard]]
static bool arena_try_append_region(Arena *a, size_t capacity_bytes, errno_t *error_code) {
    guard_is_not_null(a);

    Arena_Region *r;
    if (false == region_try_create(capacity_bytes, &r, error_code)) {
        return false;
    }

    r->next = exchange(a->_regions, r);
    return true;
}

void arena_free(Arena *a) {
    guard_is_not_null(a);

    auto region = a->_regions;
    while (nullptr != region) {
        auto next = region->next;
        free(region);
        region = next;
    }

    *a = (Arena) {0};
}

bool arena_try_allocate(Arena *a, size_t alignment, size_t size, void **p, errno_t *error_code) {
    guard_is_not_null(a);
    guard_is_not_null(p);
    guard_is_not_null(error_code);
    guard_is_greater(alignment, 0);

    for (auto r = a->_regions; nullptr != r; r = r->next) {
        if (region_try_allocate(r, alignment, size, p)) {
            return true;
        }
    }

    if (false == arena_try_append_region(a, alignment + max(size, REGION_DEFAULT_CAPACITY), error_code)) {
        return false;
    }

    guard_is_true(region_try_allocate(a->_regions, alignment, size, p));
    return true;
}

bool arena_try_allocate_copy(
        Arena *a,
        void const *src,
        size_t src_size,
        size_t alignment,
        size_t total_size,
        void **p,
        errno_t *error_code
) {
    guard_is_not_null(a);
    guard_is_not_null(p);
    guard_is_not_null(error_code);

    if (nullptr == src) {
        src_size = 0;
    }

    if (src_size > total_size) {
        src_size = total_size;
    }

    void *ptr;
    if (false == arena_try_allocate(a, alignment, total_size, &ptr, error_code)) {
        return false;
    }

    memcpy(ptr, src, src_size);
    *p = ptr;

    return true;
}

Arena_Statistics arena_statistics(Arena const *a) {
    guard_is_not_null(a);

    auto stats = (Arena_Statistics) {0};
    for (auto it = a->_regions; nullptr != it; it = it->next) {
        stats.regions++;
        stats.system_memory_used_bytes += it->capacity;
        auto const wasted = (size_t) (it->end - it->top);
        stats.allocated_bytes += it->capacity - wasted;
        stats.wasted_bytes += wasted;
    }

    return stats;
}
