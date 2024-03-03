#include "parser.h"

#include "common/macros.h"

struct Parser {
    Lexer *Lexer;
};

Parser *Parser_Init(Lexer *lexer) {
    auto parser = (Parser *) calloc(1, sizeof(Parser));
    *parser = (Parser) {.Lexer = lexer};
    return parser;
}

void Parser_Free(Parser *parser) {
    free(parser);
}

static ParserResult ParseToken(Parser *parser, Token token);

static ParserResult ParseExpression(Parser *parser) {
    auto items = (Objects) {0};
    while (true) {
        auto const lexerResult = Lexer_Next(parser->Lexer);
        Token token;
        switch (lexerResult.Type) {
            case LEXER_ERROR:
                return (ParserResult) {.Type = PARSER_LEXER_ERROR, .AsLexerError = lexerResult.Error};
            case LEXER_TOKEN:
                token = lexerResult.Token;
                break;
            default:
                Unreachable();
        }

        if (TOKEN_CLOSE_PAREN == token.Type) {
            break;
        }

        auto const parserResult = ParseToken(parser, token);
        switch (parserResult.Type) {
            case PARSER_LEXER_ERROR:
            case PARSER_PARSER_ERROR:
                return parserResult;
            case PARSER_OBJECT:
                Vector_PushBack(&items, parserResult.AsObject);
                break;
            default:
                Unreachable();
        }
    }

    return (ParserResult) {
            .Type = PARSER_OBJECT,
            .AsObject = (Object) {
                    .Type = OBJECT_EXPRESSION,
                    .AsExpression = {items}
            }
    };
}

static ParserResult ParseToken(Parser *parser, Token token) {
    switch (token.Type) {
        case TOKEN_NONE:
        case TOKEN_INVALID:
            Unreachable();
        case TOKEN_OPEN_PAREN:
            return ParseExpression(parser);
        case TOKEN_CLOSE_PAREN:
            Unreachable("Must be handled in ParseExpressions");
        case TOKEN_IDENTIFIER:
            return (ParserResult) {
                    .Type = PARSER_OBJECT,
                    .AsObject = (Object) {
                            .Type = OBJECT_IDENTIFIER,
                            .AsIdentifier = {token.Identifier}
                    }
            };
        case TOKEN_INT_LITERAL:
            return (ParserResult) {
                    .Type = PARSER_OBJECT,
                    .AsObject = (Object) {
                            .Type = OBJECT_INT,
                            .AsInt = {token.IntLiteral}
                    }
            };
        case TOKEN_STRING_LITERAL:
            return (ParserResult) {
                    .Type = PARSER_OBJECT,
                    .AsObject = (Object) {
                            .Type = OBJECT_STRING,
                            .AsString = {token.StringLiteral}
                    }
            };
        default:
            Unreachable();
    }
}

ParserResult Parser_Next(Parser *parser) {
    auto const lexerResult = Lexer_Next(parser->Lexer);
    switch (lexerResult.Type) {
        case LEXER_ERROR:
            return (ParserResult) {
                    .Type = PARSER_LEXER_ERROR,
                    .AsLexerError = lexerResult.Error
            };
        case LEXER_TOKEN:
            return ParseToken(parser, lexerResult.Token);
    }

    Unreachable();
}

bool Parser_HasNext(Parser *parser) {
    return Lexer_HasNext(parser->Lexer);
}
