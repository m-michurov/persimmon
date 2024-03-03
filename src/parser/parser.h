#pragma once

#include <stdint.h>

#include "call_checked.h"
#include "collections/vector.h"

#include "ast/ast.h"
#include "lexer/lexer.h"

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
    PARSER_AST_NODE
} ParserResultType;

typedef struct ParserResult {
    ParserResultType Type;
    union {
        LexerError AsLexerError;
        ParserError AsParserError;
        AstNode AsAstNode;
    };
} ParserResult;

ParserResult Parser_Next(Parser *);

bool Parser_HasNext(Parser *);
