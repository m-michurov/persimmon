#pragma once

#include <stdlib.h>

#ifndef CallChecked
#error Please include call_checked.h before map.h
#endif

#define MAP_CONCAT_(A, B)   A ## B
#define MAP_CONCAT(A, B)    MAP_CONCAT_(A, B)

#define Map_Of(TKey, TValue)        \
struct {                            \
    size_t Size;                    \
    size_t Capacity;                \
    size_t (*Hash)(TKey);           \
    bool (*KeyEquals)(TKey, TKey);  \
    struct {                        \
        TKey Key;                   \
        TValue Value;               \
        bool Used;                  \
    } *Entries;                     \
}

#define Map_Free(MapPtr)                    \
do {                                        \
    free((MapPtr)->Entries);                \
    *(MapPtr) = (typeof(*(MapPtr))) { 0 };  \
} while (0)

#define Map_FindSlot(Map, Key_)                                                 \
({                                                                              \
    typeof(*((Map).Entries)) *_found_slot = NULL;                               \
    if ((Map).Capacity > 0) {                                                   \
        size_t _index = (Map).Hash(Key_) % (Map).Capacity;                      \
        for (size_t _i_fs = 0; _i_fs < (Map).Capacity; _i_fs++) {               \
            __auto_type _item = (Map).Entries[_index];                          \
            if (false == _item.Used || (Map).KeyEquals((Key_), _item.Key)) {    \
                break;                                                          \
            }                                                                   \
            _index = (_index + 1) % (Map).Capacity;                             \
        }                                                                       \
        _found_slot = &((Map).Entries[_index]);                                 \
    }                                                                           \
    _found_slot;                                                                \
})

#define Map_Reserve(MapPtr, NewCapacity)                        \
do {                                                            \
    size_t const _oldCapacity = (MapPtr)->Capacity;             \
    size_t const _newCapacity = (NewCapacity);                  \
    if (_oldCapacity >= _newCapacity) { break; }                \
                                                                \
    __auto_type _oldEntries = (MapPtr)->Entries;                \
    (MapPtr)->Entries = CallChecked(                            \
        calloc, (_newCapacity, sizeof(*((MapPtr)->Entries)))    \
    );                                                          \
    (MapPtr)->Capacity = _newCapacity;                          \
                                                                \
    for (size_t _i = 0; _i < _oldCapacity; _i++) {              \
        __auto_type _e = _oldEntries[_i];                       \
        if (false == _e.Used) {                                 \
            continue;                                           \
        }                                                       \
        __auto_type _slot = Map_FindSlot(*(MapPtr), _e.Key);    \
        *_slot = _e;                                            \
    }                                                           \
                                                                \
    (MapPtr)->Capacity = _newCapacity;                          \
    free(_oldEntries);                                          \
} while (0)

#define Map_Put(MapPtr, Key_, Value_)                               \
({                                                                  \
    if (3 * ((MapPtr)->Size + 1) >= 2 * (MapPtr)->Capacity) {       \
        Map_Reserve((MapPtr), 3 * (MapPtr)->Capacity / 2 + 3);      \
    }                                                               \
    __auto_type _slot_put = Map_FindSlot(*(MapPtr), (Key_));        \
    _slot_put->Value = (Value_);                                    \
    if (false == _slot_put->Used) {                                 \
        _slot_put->Key = (Key_);                                    \
        _slot_put->Used = true;                                     \
        (MapPtr)->Size += 1;                                        \
    }                                                               \
    &(_slot_put->Value);                                            \
})

#define Map_At(Map, Key_)                                   \
({                                                          \
    __auto_type _slot_at = Map_FindSlot((Map), (Key_));     \
    (NULL == _slot_at || false == _slot_at->Used            \
        ? NULL :                                            \
        &_slot_at->Value);                                  \
})

#define Map_TryGet(Map, Key_, ValuePtr)                     \
({                                                          \
    __auto_type _value_try_get = Map_At((Map), (Key_));     \
    if (NULL != _value_try_get) {                           \
        *ValuePtr = *_value_try_get;                        \
    }                                                       \
    NULL != _value_try_get;                                 \
})

#define Map_GetOrDefault(Map, Key_, DefaultExpr)                    \
({                                                                  \
    typeof((Map).Entries->Value) _value_or_default;                 \
    if (false == Map_TryGet((Map), (Key_), &_value_or_default)) {   \
        _value_or_default = (DefaultExpr);                          \
    }                                                               \
    _value_or_default;                                              \
})

#define Map_TryFindNextUsedIndex(Map, BaseIndex, NextIndexPtr)  \
({                                                              \
    size_t _i_next = (BaseIndex);                               \
    bool _ok = false;                                           \
    for (; _i_next < (Map).Capacity; _i_next++) {               \
        if (false == (Map).Entries[_i_next].Used) {             \
            continue;                                           \
        }                                                       \
        *(NextIndexPtr) = _i_next;                              \
        _ok = true;                                             \
        break;                                                  \
    }                                                           \
     _ok;                                                       \
})

#define Map_ForEach(EntryPtr, Map)                                                                  \
size_t MAP_CONCAT(_i_, __LINE__) = 0;                                                               \
for (                                                                                               \
    typeof(*((Map).Entries)) *EntryPtr =                                                            \
        Map_TryFindNextUsedIndex((Map), MAP_CONCAT(_i_, __LINE__), &MAP_CONCAT(_i_, __LINE__))      \
            ? &((Map).Entries[MAP_CONCAT(_i_, __LINE__)])                                           \
            : NULL;                                                                                 \
    NULL != EntryPtr;                                                                               \
    EntryPtr =                                                                                      \
        Map_TryFindNextUsedIndex((Map), MAP_CONCAT(_i_, __LINE__) + 1, &MAP_CONCAT(_i_, __LINE__))  \
            ? &((Map).Entries[MAP_CONCAT(_i_, __LINE__)])                                           \
            : NULL                                                                                  \
)

#define Map_Empty(Map) (0 == (Map).Size)
