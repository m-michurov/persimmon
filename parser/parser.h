#pragma once

#include <stdint.h>

#include "call_checked.h"
#include "collections/vector.h"

#include "lexer/lexer.h"

typedef enum ObjectType {
    OBJECT_INT,
    OBJECT_STRING,
    OBJECT_IDENTIFIER,
    OBJECT_EXPRESSION,
} ObjectType;

typedef struct Object Object;
typedef Vector_Of(Object) Objects;

struct Object {
    ObjectType Type;
    union {
        struct {
            int64_t Value;
        } AsInt;
        struct {
            char const *Value;
        } AsString;
        struct {
            char const *Name;
        } AsIdentifier;
        struct {
            Objects Items;
        } AsExpression;
    };
};

typedef struct ParserError {
    Token BadToken;
    char const *Why;
} ParserError;

typedef struct Parser Parser;

Parser *Parser_Init(Lexer *);

void Parser_Free(Parser *);

typedef enum ParserResultType {
    PARSER_LEXER_ERROR,
    PARSER_PARSER_ERROR,
    PARSER_OBJECT
} ParserResultType;

typedef struct ParserResult {
    ParserResultType Type;
    union {
        LexerError AsLexerError;
        ParserError AsParserError;
        Object AsObject;
    };
} ParserResult;

ParserResult Parser_Next(Parser *);

bool Parser_HasNext(Parser *);
