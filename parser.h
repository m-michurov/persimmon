#pragma once

#include "arena.h"
#include "scanner.h"
#include "object.h"
#include "object_allocator.h"

typedef struct {
    Object *last;
    Position begin;
} Parser_Expression;

typedef struct Parser_Stack {
    Parser_Expression *data;
    size_t count;
    size_t capacity;
} Parser_Stack;

typedef struct Parser Parser;

Parser *parser_new(ObjectAllocator *a);

void parser_free(Parser **p);

void parser_reset(Parser *p);

bool parser_is_inside_expression(Parser const *p);

bool parser_try_accept(Parser *p, Token token, SyntaxError *error);

bool parser_try_get_expression(Parser *p, Object **expression);

Parser_Stack const *parser_stack(Parser const *p);

Object *const *parser_expression(Parser const *p);
