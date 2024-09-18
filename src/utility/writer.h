#pragma once

#include <errno.h>
#include <stdio.h>

#include "string_builder.h"

typedef enum {
    WRITER_FILE,
    WRITER_SB
} Writer_Type;

typedef struct {
    Writer_Type type;
    union {
        FILE *as_file;
        StringBuilder *as_sb;
    };
} Writer;

Writer writer_make_from_file_(FILE *file);

Writer writer_make_from_sb_(StringBuilder *sb);

#define writer_make(FileOrBuilder)              \
(_Generic((FileOrBuilder),                      \
    FILE *          : writer_make_from_file_,   \
    StringBuilder * : writer_make_from_sb_      \
)((FileOrBuilder)))

#define writer_try_printf(Writer_, ErrorCode, Format, ...)  \
({                                                          \
    auto const _w = (Writer_);                              \
    errno_t *const _error_code = (ErrorCode);               \
    auto _ok = true;                                        \
    switch (_w.type) {                                      \
        case WRITER_FILE: {                                 \
            errno = 0;                                      \
            fprintf(_w.as_file, (Format), ##__VA_ARGS__);   \
            if (0 != errno) {                               \
                _ok = false;                                \
                 *_error_code = errno;                      \
            }                                               \
            break;                                          \
        }                                                   \
        case WRITER_SB: {                                   \
            _ok = sb_try_printf(                            \
                _w.as_sb,                                   \
                _error_code,                                \
                (Format),                                   \
                ##__VA_ARGS__                               \
            );                                              \
            break;                                          \
        }                                                   \
    }                                                       \
    _ok;                                                    \
})
