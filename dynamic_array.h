#pragma once

#include "guards.h"

#define da_free(Slice)                  \
do {                                    \
    auto const _slice = (Slice);        \
    free(_slice->data);                 \
    *_slice = (typeof(*_slice)) {0};    \
    }                                   \
while (false)

#define da_append(Slice, Item)                                                                  \
do {                                                                                            \
    auto const _slice = (Slice);                                                                \
    guard_is_not_null(_slice);                                                                  \
    if (_slice->count + 1 > _slice->capacity) {                                                 \
        auto const _new_capacity = 1 + _slice->capacity * 3 / 2;                                \
        errno = 0;                                                                              \
        auto const _new_data = realloc(_slice->data, sizeof(*_slice->data) * _new_capacity);    \
        guard_errno_is_not_set();                                                               \
        _slice->data = _new_data;                                                               \
        _slice->capacity = _new_capacity;                                                       \
    }                                                                                           \
    _slice->data[_slice->count++] = (Item);                                                     \
} while (false)
