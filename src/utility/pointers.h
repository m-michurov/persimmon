#pragma once

#include <inttypes.h>

void *pointer_roundup(void *p, size_t alignment);

#define pointer_first_nonnull(_0, ...)              \
({                                                  \
    typeof(_0) _ptrs[] = {_0, ##__VA_ARGS__};       \
    auto const _size = sizeof(_ptrs) / sizeof(_0);  \
    typeof(_0) _ptr = nullptr;                      \
    for (size_t _i = 0; _i < _size; _i++) {         \
        if (nullptr == _ptrs[_i]) {                 \
            continue;                               \
        }                                           \
        _ptr = _ptrs[_i];                           \
        break;                                      \
    }                                               \
    _ptr;                                           \
})
