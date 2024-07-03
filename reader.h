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

Reader *reader_new(NamedFile file, ObjectAllocator *a);

void reader_free(Reader **r);

bool reader_try_prompt(Reader *r, Objects *exprs);

bool reader_try_read_all(Reader *r, Objects *exprs);
