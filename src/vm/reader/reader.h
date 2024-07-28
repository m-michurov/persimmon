#pragma once

#include <stdio.h>

#include "utility/arena.h"
#include "object/object.h"
#include "object/allocator.h"
#include "syntax_error.h"
#include "parser.h"
#include "named_file.h"

typedef struct ObjectReader ObjectReader;

typedef struct {
    Scanner_Config scanner_config;
    Parser_Config parser_config;
} Reader_Config;

struct VirtualMachine;

ObjectReader *object_reader_new(struct VirtualMachine *vm, Reader_Config config);

void object_reader_free(ObjectReader **r);

void object_reader_reset(ObjectReader *r);

struct Parser_ExpressionsStack;

struct Parser_ExpressionsStack const *object_reader_parser_stack(ObjectReader const *r);

Object *const *object_reader_parser_expr(ObjectReader const *r);

[[nodiscard]]
bool object_reader_try_prompt(ObjectReader *r, NamedFile file, Object **exprs);

[[nodiscard]]
bool object_reader_try_read_all(ObjectReader *r, NamedFile file, Object **exprs);
