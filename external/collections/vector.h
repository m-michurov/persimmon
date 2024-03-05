#pragma once

#include <stdlib.h>

#ifndef CallChecked
#error Please include call_checked.h before vector.h
#endif

#define VECTOR_CONCAT_(A, B)   A ## B
#define VECTOR_CONCAT(A, B)    VECTOR_CONCAT_(A, B)

#define Vector_Of(TValue) \
struct {                  \
    size_t Size;          \
    size_t Capacity;      \
    TValue *Items;        \
}

#define Vector_Free(VecPtr)                 \
do {                                        \
    free((void *) (VecPtr)->Items);         \
    *(VecPtr) = (typeof(*(VecPtr))) { 0 };  \
} while (0)

#define Vector_PushBack(VecPtr, Val)                            \
do {                                                            \
    if ((VecPtr)->Size >= (VecPtr)->Capacity) {                 \
        (VecPtr)->Capacity =                                    \
            3 * (VecPtr)->Capacity / 2 + 1;                     \
        __auto_type _items = CallChecked(realloc, (             \
            (VecPtr)->Items,                                    \
            (VecPtr)->Capacity * sizeof((VecPtr)->Items[0])     \
        ));                                                     \
        (VecPtr)->Items = _items;                               \
    }                                                           \
    (VecPtr)->Items[(VecPtr)->Size++] = (Val);                  \
} while (0)

#define Vector_TryPopBack(VecPtr, ValuePtr)         \
({                                                  \
    bool _ok = false;                               \
    if ((VecPtr)->Size > 0) {                       \
        *(ValuePtr) =                               \
            (VecPtr)->Items[--(VecPtr)->Size];      \
        _ok = true;                                 \
    }                                               \
    _ok;                                            \
})

#define Vector_At(Vec, Index)               \
({                                          \
    long long _idx = (Index);               \
    long long _sz = (Vec).Size;             \
    typeof((Vec).Items[0]) *_value = NULL;  \
    if (0 <= _idx && _idx < _sz) {          \
        _value = (Vec).Items + _idx;        \
    }                                       \
    if (-_sz <= _idx && _idx < 0) {         \
        _value = (Vec).Items + _sz + _idx;  \
    }                                       \
    _value;                                 \
})

#define Vector_ForEach(ValuePtr, Vec)                       \
size_t VECTOR_CONCAT(_it_, __LINE__) = 0;                   \
for (                                                       \
    typeof(*((Vec).Items)) *ValuePtr =                      \
        VECTOR_CONCAT(_it_, __LINE__) >= (Vec).Size         \
            ? NULL                                          \
            : &(Vec).Items[VECTOR_CONCAT(_it_, __LINE__)];  \
    VECTOR_CONCAT(_it_, __LINE__) < (Vec).Size;             \
    VECTOR_CONCAT(_it_, __LINE__)++,                        \
    ValuePtr =                                              \
        VECTOR_CONCAT(_it_, __LINE__) >= (Vec).Size         \
            ? NULL                                          \
            : &(Vec).Items[VECTOR_CONCAT(_it_, __LINE__)]   \
)

#define Vector_Empty(Vec)       (0 == (Vec).Size)

#define Vector_Clear(VecPtr)    do { (VecPtr)->Size = 0; } while (0)
