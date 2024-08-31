#pragma once

#include "guards.h"
#include "math.h"

#define SLICE__concat_(A, B) A ## B
#define SLICE__concat(A, B) SLICE__concat_(A, B)

#define slice_at(SlicePtr, Index)                       \
({                                                      \
    auto _slice = (SlicePtr);                           \
    long long _index = (Index);                         \
    if (_index < 0) {                                   \
        _index += _slice->count;                        \
    }                                                   \
    guard_is_in_range(_index + 1, 1, _slice->count);    \
    &_slice->data[_index];                              \
})

#define slice_clear(SlicePtr) do { (SlicePtr)->count = 0; } while (false)

#define slice_try_append(SlicePtr, Item)            \
({                                                  \
    auto const _slice = (SlicePtr);                 \
    guard_is_not_null(_slice);                      \
    auto _ok = false;                               \
    if (_slice->count + 1 <= _slice->capacity) {    \
        _slice->data[_slice->count++] = (Item);     \
        _ok = true;                                 \
    }                                               \
    _ok;                                            \
})

#define slice_try_pop(SlicePtr, Item)                   \
({                                                      \
    auto const _slice = (SlicePtr);                     \
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

#define slice_last(SlicePtr)            \
({                                      \
    auto const _slice = (SlicePtr);     \
    guard_is_greater(_slice->count, 0); \
    &_slice->data[_slice->count - 1];   \
})

#define slice_for_v(It, Slice)                                                      \
auto SLICE__concat(_s_, __LINE__) = (Slice);                                        \
typeof((Slice).data) SLICE__concat(_s_data, __LINE__) =                             \
    SLICE__concat(_s_, __LINE__).data;                                              \
for (                                                                               \
    auto It = SLICE__concat(_s_data, __LINE__);                                     \
    It != SLICE__concat(_s_data, __LINE__) + SLICE__concat(_s_, __LINE__).count;    \
    It++                                                                            \
)

#define slice_for(It, SlicePtr)                                                     \
auto SLICE__concat(_s_, __LINE__) = (SlicePtr);                                     \
for (                                                                               \
    auto It = SLICE__concat(_s_, __LINE__)->data;                                   \
    It != SLICE__concat(_s_, __LINE__)->data + SLICE__concat(_s_, __LINE__)->count; \
    It++                                                                            \
)
