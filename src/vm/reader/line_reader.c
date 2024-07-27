#include "line_reader.h"

#include "utility/guards.h"

void line_reader_free(LineReader *r) {
    guard_is_not_null(r);

    sb_free(&r->_sb);
    *r = (LineReader) {0};
}

#define TAB_AS_SPACES "    "

bool line_reader_try_read(LineReader *r, Arena *a, Line *line, errno_t *error_code) {
    guard_is_not_null(r);
    guard_is_not_null(r->_file);

    if (feof(r->_file)) {
        *line = (Line) {0};
        return true;
    }

    sb_clear(&r->_sb);

    while (true) {
        errno = 0;
        auto const c = fgetc(r->_file);
        if (errno) {
            *error_code = errno;
            return false;
        }

        if (EOF == c) {
            if (false == sb_try_printf_realloc(&r->_sb, error_code, "%c", '\n')) {
                return false;
            }

            break;
        }

        if ('\t' == c) {
            if (false == sb_try_printf_realloc(&r->_sb, error_code, "%s", TAB_AS_SPACES)) {
                return false;
            }

            continue;
        }

        if (false == sb_try_printf_realloc(&r->_sb, error_code, "%c", c)) {
            return false;
        }

        if ('\n' == c) {
            break;
        }
    }

    *line = (Line) {
            .data = arena_copy_all(a, r->_sb.str, r->_sb.length + 1),
            .count = r->_sb.length,
            .lineno = r->_lineno
    };
    r->_lineno++;

    return true;
}
