#pragma once

#include <stdio.h>

#include "utility/string_builder.h"
#include "position.h"

typedef struct {
    char const *data;
    size_t count;
    size_t lineno;
} Line;

typedef struct {
    FILE *_file;
    size_t _lineno;
    StringBuilder _sb;
} LineReader;

#define line_reader_make(File) ((LineReader) {._file = (File)})

void line_reader_free(LineReader *r);

[[nodiscard]]
bool line_reader_try_read(LineReader *r, Arena *a, Line *line, errno_t *error_code);
