#pragma once

#include <stdio.h>

#include "arena.h"
#include "object.h"
#include "object_allocator.h"
#include "syntax_error.h"

typedef struct Reader Reader;

typedef struct {
    SyntaxError base;
    char const *file_name;
    char const *text;
} ReaderError;

void reader_print_error(ReaderError error, FILE *file);

typedef struct {
    char const *name;
    FILE *handle;
} NamedFile;

Reader *reader_open(Arena *a, NamedFile file, Object_Allocator *allocator);

bool reader_try_prompt(Arena *a, Reader *r, Objects *exprs, ReaderError *error);

bool reader_try_read_all(Arena *a, Reader *r, Objects *exprs, ReaderError *error);
