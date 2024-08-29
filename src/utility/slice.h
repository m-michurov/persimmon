#pragma once

#include "guards.h"
#include "math.h"

#define SLICE__concat_(A, B) A ## B
#define SLICE__concat(A, B) SLICE__concat_(A, B)

// TODO make pointer versions default or rename
#define slice_at(Slice, Index)                      \
({                                                  \
    auto _slice = (Slice);                          \
    typeof((Slice).data) _data = _slice.data;       \
    long long _index = (Index);                     \
    if (_index < 0) {                               \
        _index += _slice.count;                     \
    }                                               \
    guard_is_in_range(_index + 1, 1, _slice.count); \
    &_data[_index];                                 \
})

#define slice_ptr_at(SlicePtr, Index)                   \
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

#define slice_last(Slice)                       \
({                                              \
    auto _slice = (Slice);                      \
    typeof((Slice).data) _data = _slice.data;   \
    guard_is_greater(_slice.count, 0);          \
    &_data[_slice.count - 1];                   \
})

#define slice_for(It, Slice)                                                        \
auto SLICE__concat(_s_, __LINE__) = (Slice);                                        \
typeof((Slice).data) SLICE__concat(_s_data, __LINE__) =                             \
    SLICE__concat(_s_, __LINE__).data;                                              \
for (                                                                               \
    auto It = SLICE__concat(_s_data, __LINE__);                                     \
    It != SLICE__concat(_s_data, __LINE__) + SLICE__concat(_s_, __LINE__).count;    \
    It++                                                                            \
)

#define slice_ptr_for(It, Slice)                                                    \
auto SLICE__concat(_s_, __LINE__) = (Slice);                                        \
for (                                                                               \
    auto It = SLICE__concat(_s_, __LINE__)->data;                                   \
    It != SLICE__concat(_s_, __LINE__)->data + SLICE__concat(_s_, __LINE__)->count; \
    It++                                                                            \
)
