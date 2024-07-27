#pragma once

#include "arena.h"

#define arena_try_append(Arena_, DynArrayPtr, Element, ErrorCode)       \
({                                                                      \
    auto const _da = (DynArrayPtr);                                     \
    auto _ok = true;                                                    \
    if (_da->count >= _da->capacity) {                                  \
        auto const _cap = 1 + 2 * _da->capacity;                        \
        _ok = arena_try_copy(                                           \
            (Arena_),                                                   \
            _da->data,                                                  \
            _da->count,                                                 \
            _cap,                                                       \
            &_da->data,                                                 \
            (ErrorCode)                                                 \
        );                                                              \
        if (_ok) {                                                      \
            _da->capacity = _cap;                                       \
        }                                                               \
    }                                                                   \
    _da->data[_da->count++] = (Element);                                \
    _ok;                                                                \
})
