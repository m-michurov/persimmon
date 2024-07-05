#pragma once

#include "guards.h"
#include "math.h"

#define slice_at(Slice, Index)                      \
({                                                  \
    auto const _slice = (Slice);                    \
    long long _index = (Index);                     \
    if (_index < 0) {                               \
        _index += _slice.count;                     \
    }                                               \
    guard_is_in_range(_index + 1, 1, _slice.count); \
    &_slice.data[_index];                           \
})

#define slice_clear(Slice) do { (Slice)->count = 0; } while (false)

#define slice_take_from(T, Slice, StartIdx)             \
({                                                      \
    auto const _slice = (Slice);                        \
    auto const _start = min((StartIdx), _slice.count);  \
    (T) {                                               \
        .data = _slice.data + _start,                   \
        .count = _slice.count - _start                  \
    };                                                  \
})

#define slice_trim(Slice, Count)                    \
do {                                                \
    auto const _slice = (Slice);                    \
    _slice->count = min((Count), _slice->count);    \
} while (false)

#define slice_try_append(Slice, Item)                                                           \
({                                                                                              \
    auto const _slice = (Slice);                                                                \
    guard_is_not_null(_slice);                                                                  \
    auto _ok = false;                                                                           \
    if (_slice->count + 1 <= _slice->capacity) {                                                \
        _slice->data[_slice->count++] = (Item);                                                 \
        _ok = true;                                                                             \
    }                                                                                           \
    _ok;                                                                                        \
})

#define slice_try_pop(Slice, Item)                      \
({                                                      \
    auto const _slice = (Slice);                        \
    typeof(_slice->data) _item = (Item);                \
    guard_is_not_null(_slice);                          \
    auto _ok = false;                                   \
    if (_slice->count > 0) {                            \
        if (nullptr != _item) {                         \
            *_item = _slice->data[_slice->count - 1];   \
        }                                               \
        _slice->count--;                                \
        _ok = true;                                     \
    }                                                   \
    _ok;                                                \
})

#define slice_empty(Slice) (0 == (Slice).count)

#define slice_first(Slice)              \
({                                      \
    auto const _slice = (Slice);        \
    guard_is_greater(_slice.count, 0);  \
    &_slice.data[0];                    \
})

#define slice_last(Slice)               \
({                                      \
    auto const _slice = (Slice);        \
    guard_is_greater(_slice.count, 0);  \
    &_slice.data[_slice.count - 1];     \
})

#define SLICE__concat_(A, B) A ## B
#define SLICE__concat(A, B) SLICE__concat_(A, B)

#define slice_for(It, Slice)                                                        \
auto SLICE__concat(_s_, __LINE__) = (Slice);                                        \
for (                                                                               \
    auto It = SLICE__concat(_s_, __LINE__).data;                                    \
    It != SLICE__concat(_s_, __LINE__).data + SLICE__concat(_s_, __LINE__).count;   \
    It++                                                                            \
)
