#include "line_reader.h"

#include "guards.h"

struct LineReader {
    FILE *file;
    StringBuilder *sb;
    size_t lineno;
};

LineReader *line_reader_new(FILE *file) {
    auto const r = (LineReader *) guard_succeeds(calloc, (1, sizeof(LineReader)));
    *r = (LineReader) {
            .file = file,
            .sb = sb_new(),
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

void line_reader_reset(LineReader *r) {
    guard_is_not_null(r);

    r->lineno = 1;
    sb_clear(r->sb);
}

void line_reader_reset_file(LineReader *r, FILE *file) {
    guard_is_not_null(r);
    guard_is_not_null(file);

    line_reader_reset(r);
    r->file = file;
}

bool line_try_read(LineReader *r, Arena *a, Line *line) {
    guard_is_not_null(r);

    if (feof(r->file)) {
        return false;
    }

    sb_clear(r->sb);

    while (true) {
        auto const c = guard_succeeds(fgetc, (r->file));

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
        .data = arena_copy_all(a, sb_str(r->sb), sb_length(r->sb) + 1),
        .count = sb_length(r->sb),
        .lineno = r->lineno
    };
    r->lineno++;

    return true;
}
