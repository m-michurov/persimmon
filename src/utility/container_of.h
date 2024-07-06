#pragma once

#include <stdint.h>

#define container_of(Ptr, Type, Member)                     \
({                                                          \
    const typeof(((Type *) nullptr)->Member) *_p = (Ptr);   \
    (Type *) ((uint8_t *) _p - offsetof(Type, Member));     \
})
