#pragma once

#include "arena.h"
#include "tokenizer.h"
#include "object.h"
#include "object_allocator.h"

typedef struct Parser Parser;

Parser *parser_new(Arena *a, Object_Allocator *allocator);

void parser_reset(Parser *p);

bool parser_is_inside_expression(Parser const *p);

bool parser_try_accept(Parser *p, Token token, SyntaxError *error);

bool parser_try_get_expression(Parser *p, Object **expression);
