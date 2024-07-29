#pragma once

#include <stdio.h>

#include "utility/arena.h"
#include "object/object.h"
#include "object/allocator.h"
#include "syntax_error.h"
#include "parser.h"
#include "named_file.h"

struct VirtualMachine;

typedef struct ObjectReader {
    struct VirtualMachine *_vm;
    Scanner _s;
    Parser _p;
} ObjectReader;

typedef struct {
    Scanner_Config scanner_config;
    Parser_Config parser_config;
} Reader_Config;

bool object_reader_try_init(
        ObjectReader *r,
        struct VirtualMachine *vm,
        Reader_Config config,
        errno_t *error_code
);

void object_reader_free(ObjectReader *r);

void object_reader_reset(ObjectReader *r);

[[nodiscard]]
bool object_reader_try_prompt(ObjectReader *r, NamedFile file, Object **exprs);

[[nodiscard]]
bool object_reader_try_read_all(ObjectReader *r, NamedFile file, Object **exprs);
