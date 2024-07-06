#pragma once

#include "arena.h"

#define arena_append(Arena_, DynArrayPtr, Element)                      \
do {                                                                    \
    auto const _da = (DynArrayPtr);                                     \
    if (_da->count >= _da->capacity) {                                  \
        auto const _cap = 1 + 2 * _da->capacity;                        \
        _da->data = arena_copy((Arena_), _da->data, _da->count, _cap);  \
        _da->capacity = _cap;                                           \
    }                                                                   \
    _da->data[_da->count++] = (Element);                                \
} while (false)
