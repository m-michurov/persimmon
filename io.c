#include "io.h"

#include "guards.h"

struct LineReader {
    FILE *file;
    StringBuilder *sb;
    size_t lineno;
};

LineReader *line_reader_open(Arena *a, FILE *file) {
    return arena_emplace(a, ((LineReader) {
            .file = file,
            .sb = sb_new(a),
            .lineno = 1
    }));
}

void line_reader_reset(LineReader *r) {
    r->lineno = 1;
    sb_clear(r->sb);
}

void line_reader_reopen(LineReader *r, FILE *file) {
    guard_is_not_null(r);
    guard_is_not_null(file);

    r->file = file;
    line_reader_reset(r);
}

bool line_try_read(Arena *a, LineReader *r, Line *line) {
    guard_is_not_null(r);
    guard_is_not_null(line);

    if (feof(r->file)) {
        return false;
    }

    sb_clear(r->sb);

    while (true) {
        errno = 0;
        auto const c = fgetc(r->file);
        guard_errno_is_not_set();

        if (EOF == c) {
            sb_append_char(r->sb, '\n');
            break;
        }

        if ('\t' == c) {
            sb_append(r->sb, "    ");
            continue;
        }

        sb_append_char(r->sb, c);
        if ('\n' == c) {
            break;
        }
    }

    *line = (Line) {
            .data = sb_copy_str(a, r->sb),
            .count = sb_length(r->sb),
            .lineno = r->lineno
    };

    r->lineno++;

    return true;
}
