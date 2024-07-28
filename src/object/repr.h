#pragma once

#include <stdio.h>

#include "object.h"
#include "utility/string_builder.h"

[[nodiscard]]
bool object_try_repr_sb(Object *obj, StringBuilder *sb, errno_t *error_code);

[[nodiscard]]
bool object_try_repr_file(Object *obj, FILE *file, errno_t *error_code);

#define object_try_repr(Obj, Dst, ErrorCode)    \
_Generic((Dst),                                 \
    StringBuilder * : object_try_repr_sb,       \
    FILE *          : object_try_repr_file      \
) ((Obj), (Dst), (ErrorCode))

#define object_repr(Obj, Dst)                           \
do {                                                    \
    errno_t _error_code;                                \
    (void) object_try_repr((Obj), (Dst), &_error_code); \
} while (false)

[[nodiscard]]
bool object_try_print_sb(Object *obj, StringBuilder *sb, errno_t *error_code);

[[nodiscard]]
bool object_try_print_file(Object *obj, FILE *file, errno_t *error_code);

#define object_try_print(Obj, Dst, ErrorCode)   \
_Generic((Dst),                                 \
    StringBuilder * : object_try_print_sb,      \
    FILE *          : object_try_print_file     \
) ((Obj), (Dst), (ErrorCode))

#define object_print(Obj, Dst)                              \
do {                                                        \
    errno_t _error_code;                                    \
    (void) object_try_print((Obj), (Dst), &_error_code);    \
} while (false)
