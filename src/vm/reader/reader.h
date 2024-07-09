#pragma once

#include <stdio.h>

#include "utility/arena.h"
#include "object/object.h"
#include "object/allocator.h"
#include "syntax_error.h"
#include "parser.h"

typedef struct Reader Reader;

typedef struct {
    char const *name;
    FILE *handle;
} NamedFile;

typedef struct {
    Parser_Config parser_config;
} Reader_Config;

Reader *reader_new(ObjectAllocator *a, Reader_Config config);

void reader_free(Reader **r);

void reader_reset(Reader *r);

struct Parser_ExpressionsStack;

struct Parser_ExpressionsStack const *reader_parser_stack(Reader const *r);

Object *const *reader_parser_expr(Reader const *r);

bool reader_try_prompt(Reader *r, NamedFile file, Object **exprs);

bool reader_try_read_all(Reader *r, NamedFile file, Object **exprs);
