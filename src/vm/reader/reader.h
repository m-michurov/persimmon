#pragma once

#include <stdio.h>

#include "utility/arena.h"
#include "object/object.h"
#include "object/allocator.h"
#include "syntax_error.h"
#include "parser.h"
#include "named_file.h"

typedef struct Reader Reader;

typedef struct {
    Scanner_Config scanner_config;
    Parser_Config parser_config;
} Reader_Config;

struct VirtualMachine;

Reader *reader_new(struct VirtualMachine *vm, Reader_Config config);

void reader_free(Reader **r);

void reader_reset(Reader *r);

struct Parser_ExpressionsStack;

struct Parser_ExpressionsStack const *reader_parser_stack(Reader const *r);

Object *const *reader_parser_expr(Reader const *r);

[[nodiscard]]

bool reader_try_prompt(Reader *r, NamedFile file, Object **exprs);

[[nodiscard]]
bool reader_try_read_all(Reader *r, NamedFile file, Object **exprs);
