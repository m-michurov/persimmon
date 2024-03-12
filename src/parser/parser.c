#include "parser.h"

#include "common/macros.h"

#define ParserResult_LexerError(Error)  ((ParserResult) {.Type=PARSER_LEXER_ERROR, .AsLexerError=Error})
#define ParserResult_ParserError(BadToken, Why) \
((ParserResult) {                               \
    .Type=PARSER_PARSER_ERROR,                  \
    .AsParserError={(BadToken), (Why)}          \
})
#define ParserResult_AstNode(Node)      ((ParserResult) {.Type=PARSER_AST_NODE, .AsAstNode=Node})

struct Parser {
    Lexer *Lexer;
    Vector_Of(AstExpression) ExpressionsStack;
};

Parser *Parser_Init(Lexer *lexer) {
    auto parser = (Parser *) calloc(1, sizeof(Parser));
    *parser = (Parser) {.Lexer = lexer};
    return parser;
}

void Parser_Free(Parser *parser) {
    Vector_Free(&parser->ExpressionsStack);
    free(parser);
}

static bool HandleToken(Parser parser[static 1], Token token, ParserResult result[static 1]) {
    auto const stack = &parser->ExpressionsStack;

    switch (token.Type) {
        case TOKEN_OPEN_PAREN: {
            Vector_PushBack(stack, ((AstExpression) {0}));
            break;
        }
        case TOKEN_CLOSE_PAREN: {
            AstExpression items;
            if (false == Vector_TryPopBack(stack, &items)) {
                *result = ParserResult_ParserError(token, "unexpected closing parenthesis");
                return true;
            }

            auto const node = AstNode_Expression(items);
            if (0 == stack->Size) {
                *result = ParserResult_AstNode(node);
                return true;
            }

            Vector_PushBack(Vector_At(*stack, -1), node);
            break;
        }
        case TOKEN_INT_LITERAL: {
            auto const node = AstNode_Int(token.Value.AsIntLiteral);
            if (0 == stack->Size) {
                *result = ParserResult_AstNode(node);
                return true;
            }

            Vector_PushBack(Vector_At(*stack, -1), node);
            break;
        }
        case TOKEN_STRING_LITERAL: {
            auto const node = AstNode_String(strdup(token.Value.AsStringLiteral));
            if (0 == stack->Size) {
                *result = ParserResult_AstNode(node);
                return true;
            }

            Vector_PushBack(Vector_At(*stack, -1), node);
            break;
        }
        case TOKEN_IDENTIFIER: {
            auto const node = AstNode_Identifier(strdup(token.Value.AsIdentifier));
            if (0 == stack->Size) {
                *result = ParserResult_AstNode(node);
                return true;
            }

            Vector_PushBack(Vector_At(*stack, -1), node);
            break;
        }
        case TOKEN_NONE:
        case TOKEN_INVALID:
            *result = ParserResult_ParserError(token, "unexpected token");
            return true;
        default:
            Unreachable("%d", token.Type);
    }

    return false;
}

ParserResult Parser_Next(Parser *parser) {
    while (Lexer_HasNext(parser->Lexer)) {
        auto const r = Lexer_Next(parser->Lexer);
        switch (r.Type) {
            case LEXER_ERROR:
                return ParserResult_LexerError(r.AsError);
            case LEXER_TOKEN:
                break;
            default:
                Unreachable("%d", r.Type);
        }

        ParserResult result;
        if (HandleToken(parser, r.AsToken, &result)) {
            return result;
        }
    }

    auto const r = Lexer_Next(parser->Lexer);
    if (LEXER_ERROR == r.Type) {
        return ParserResult_LexerError(r.AsError);
    }

    Unreachable();
}

bool Parser_HasNext(Parser *parser) {
    return Lexer_HasNext(parser->Lexer);
}
