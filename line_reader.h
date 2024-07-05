#pragma once

#include <stdio.h>

#include "string_builder.h"
#include "position.h"

typedef struct {
    char const *data;
    size_t count;
    size_t lineno;
} Line;

typedef struct LineReader LineReader;

LineReader *line_reader_new(FILE *file);

void line_reader_free(LineReader **r);

void line_reader_reset(LineReader *r);

void line_reader_reset_file(LineReader *r, FILE *file);

bool line_try_read(LineReader *r, Arena *a, Line *line);
