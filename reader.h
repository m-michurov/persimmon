#pragma once

#include <stdio.h>

#include "arena.h"
#include "object.h"
#include "object_allocator.h"
#include "syntax_error.h"

typedef struct Reader Reader;

typedef struct {
    char const *name;
    FILE *handle;
} NamedFile;

Reader *reader_open(Arena *a, NamedFile file, ObjectAllocator *allocator);

bool reader_try_prompt(Arena *a, Reader *r, Objects *exprs);

bool reader_try_read_all(Arena *a, Reader *r, Objects *exprs);
