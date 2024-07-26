#include "line_reader.h"

#include "utility/guards.h"

struct LineReader {
    FILE *file;
    StringBuilder sb;
    size_t lineno;
};

LineReader *line_reader_new(FILE *file) {
    auto const r = (LineReader *) guard_succeeds(calloc, (1, sizeof(LineReader)));
    *r = (LineReader) {
            .file = file,
            .lineno = 1
    };
    return r;
}

void line_reader_free(LineReader **r) {
    guard_is_not_null(r);
    guard_is_not_null(*r);

    sb_free(&(*r)->sb);
    free(*r);
    *r = nullptr;
}

#define TAB_AS_SPACES "    "

bool line_reader_try_read(LineReader *r, Arena *a, Line *line, errno_t *error_code) {
    guard_is_not_null(r);

    if (feof(r->file)) {
        *line = (Line) {0};
        return true;
    }

    sb_clear(&r->sb);

    while (true) {
        errno = 0;
        auto const c = fgetc(r->file);
        if (errno) {
            *error_code = errno;
            return false;
        }

        if (EOF == c) {
            if (false == sb_try_printf_realloc(&r->sb, "%c", '\n')) {
                *error_code = ENOMEM;
                return false;
            }

            break;
        }

        if ('\t' == c) {
            if (false == sb_try_printf_realloc(&r->sb, "%s", TAB_AS_SPACES)) {
                *error_code = ENOMEM;
                return false;
            }

            continue;
        }

        if (false == sb_try_printf_realloc(&r->sb, "%c", c)) {
            *error_code = ENOMEM;
            return false;
        }

        if ('\n' == c) {
            break;
        }
    }

    *line = (Line) {
            .data = arena_copy_all(a, r->sb.str, r->sb.length + 1),
            .count = r->sb.length,
            .lineno = r->lineno
    };
    r->lineno++;

    return true;
}
