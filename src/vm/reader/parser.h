#pragma once

#include "scanner.h"
#include "utility/arena.h"
#include "object/object.h"
#include "object/allocator.h"

typedef struct {
    Object *last;
    Position begin;
} Parser_Expression;

typedef struct Parser_ExpressionsStack Parser_ExpressionsStack;
struct Parser_ExpressionsStack {
    Parser_Expression *const data;
    size_t count;
    size_t const capacity;
};

typedef struct Parser Parser;

typedef struct {
    size_t max_nesting_depth;
} Parser_Config;

Parser *parser_new(ObjectAllocator *a, Parser_Config config);

void parser_free(Parser **p);

void parser_reset(Parser *p);

bool parser_is_inside_expression(Parser const *p);

bool parser_try_accept(Parser *p, Token token, SyntaxError *error);

bool parser_try_get_expression(Parser *p, Object **expression);

Parser_ExpressionsStack const *parser_stack(Parser const *p);

Object *const *parser_expression(Parser const *p);
