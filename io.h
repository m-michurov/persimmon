#pragma once

#include <stdio.h>

#include "arena.h"
#include "string_builder.h"
#include "position.h"

typedef struct {
    char const *data;
    size_t count;
    size_t lineno;
} Line;

typedef struct LineReader LineReader;

LineReader *line_reader_open(Arena *a, FILE *file);

void line_reader_reset(LineReader *r);

void line_reader_reopen(LineReader *r, FILE *file);

bool line_try_read(Arena *a, LineReader *r, Line *line);
