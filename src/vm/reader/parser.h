#pragma once

#include "scanner.h"
#include "utility/arena.h"
#include "object/object.h"
#include "object/allocator.h"

typedef enum {
    PARSER_EXPRESSION,
    PARSER_QUOTE,
    PARSER_DOT
} Parser_ExpressionType;

typedef struct {
    Parser_ExpressionType type;
    Object *last;
    Position begin;
} Parser_Expression;

typedef struct Parser_ExpressionsStack Parser_ExpressionsStack;

struct Parser_ExpressionsStack {
    Parser_Expression *data;
    size_t count;
    size_t capacity;
};

typedef struct {
    bool has_expr;
    Object *expr;
    Parser_ExpressionsStack exprs_stack;

    ObjectAllocator *_a;
} Parser;

typedef struct {
    size_t max_nesting_depth;
} Parser_Config;

bool parser_try_init(Parser *p, ObjectAllocator *a, Parser_Config config, errno_t *error_code);

void parser_free(Parser *p);

void parser_reset(Parser *p);

bool parser_is_inside_expression(Parser p);

typedef enum {
    PARSER_SYNTAX_ERROR,
    PARSER_ALLOCATION_ERROR
} Parser_ErrorType;

typedef struct {
    Parser_ErrorType type;
    SyntaxError as_syntax_error;
} Parser_Error;

[[nodiscard]]
bool parser_try_accept(Parser *p, Token token, Parser_Error *error);
