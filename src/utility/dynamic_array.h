#pragma once

#include "guards.h"

#define da_free(DynArray)           \
do {                                \
    auto const _da = (DynArray);    \
    free(_da->data);                \
    *_da = (typeof(*_da)) {0};      \
    }                               \
while (false)

#define DA__try_reserve(DynArray, Capacity)     \
({                                              \
    size_t const _capacity = (Capacity);        \
    guard_is_not_null((DynArray));              \
    errno = 0;                                  \
    if (_capacity > (DynArray)->capacity) {     \
        auto const _new_data = realloc(         \
            (DynArray)->data,                   \
            sizeof(*DynArray->data) * _capacity \
        );                                      \
        if (0 == errno) {                       \
            (DynArray)->data = _new_data;       \
            (DynArray)->capacity = _capacity;   \
        }                                       \
    }                                           \
    0 == errno;                                 \
})

#define da_try_reserve(DynArray, Capacity)  \
({                                          \
    auto const _da = (DynArray);            \
    DA__try_reserve(_da, (Capacity));       \
})

#define da_try_append(DynArray, Item)                           \
({                                                              \
    auto const _da = (DynArray);                                \
    guard_is_not_null(_da);                                     \
    auto _ok = true;                                            \
    if (_da->count + 1 > _da->capacity) {                       \
        auto const _new_capacity = 1 + _da->capacity * 3 / 2;   \
        _ok = DA__try_reserve(_da, _new_capacity);              \
    }                                                           \
    if (_ok) {                                                  \
        _da->data[_da->count++] = (Item);                       \
    }                                                           \
    _ok;                                                        \
})
