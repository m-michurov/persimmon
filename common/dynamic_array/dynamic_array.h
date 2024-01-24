#pragma once

#include <stdlib.h>

void *DynamicArray_ReserveGeneric(
        void *data,
        size_t elementSize, size_t minCapacity,
        size_t size, size_t capacity[static 1]
);

void *DynamicArray_ExtendGeneric(
        void *data,
        size_t size[static 1],
        size_t capacity[static 1],
        size_t elementSize,
        size_t count,
        const void *src
);

#define DynamicArray_Extend(array, count, data) \
((array)->Data = DynamicArray_ExtendGeneric(    \
    (array)->Data,                              \
    &(array)->Size,                             \
    &(array)->Capacity,                         \
    sizeof(*(array)->Data),                     \
    count,                                      \
    data                                        \
))

#define DynamicArray_Append(array, element) DynamicArray_Extend(array, 1, (typeof(*(array)->Data)[]) {element})

#define DynamicArray_Free(array)        \
do {                                    \
    free((array)->Data);                \
    *(array) = (typeof(*(array))) {};   \
} while (0)

#define DynamicArray_Clear(array) do { (array)->Size = 0; } while (0)
