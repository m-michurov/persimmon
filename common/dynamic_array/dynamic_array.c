#include "dynamic_array.h"

#include <memory.h>
#include <stdint.h>

#include "common/macros.h"

static void *ReserveGeneric(
        void *data,
        size_t elementSize, size_t minCapacity,
        size_t size, size_t capacity[1]
) {
    size_t newCapacity = size;
    while (newCapacity < minCapacity) {
        newCapacity = newCapacity * 3 / 2 + 1;
    }

    void *newData = realloc(data, newCapacity * elementSize);
    if (NULL == newData) {
        CallFailed(realloc, "Failed to reallocate _buffer");
    }

    *capacity = newCapacity;
    return newData;
}

void *DynamicArray_ExtendGeneric(
        void *data,
        size_t size[1],
        size_t capacity[1],
        size_t elementSize,
        size_t count,
        const void *src
) {
    if (NULL == data || *size + count > *capacity) {
        data = ReserveGeneric(data, elementSize, *size + count, *size, capacity);
    }

    memcpy(((uint8_t *) data) + *size * elementSize, src, count * elementSize);
    *size += count;

    return data;
}
