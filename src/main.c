#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "call_checked.h"
#include "collections/map.h"

#include "common/macros.h"
#include "common/file_utils/file_utils.h"
#include "lexer/lexer.h"
#include "parser/parser.h"

size_t StrHash(char const *s) {
    unsigned long hash = 5381;
    int c;

    while ('\0' != (c = (unsigned char) *s++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }

    return hash;
}

bool StrEquals(char const *s1, char const *s2) {
    return 0 == strcmp(s1, s2);
}

static bool IsSpecialChar(int c, char const *seq[static 1]) {
    if ('\n' == c) {
        *seq = "\\n";
        return true;
    }

    if ('\t' == c) {
        *seq = "\\t";
        return true;
    }

    if ('\r' == c) {
        *seq = "\\r";
        return true;
    }

    if ('\f' == c) {
        *seq = "\\f";
        return true;
    }

    if ('\v' == c) {
        *seq = "\\v";
        return true;
    }

    if ('\0' == c) {
        *seq = "\\0";
        return true;
    }

    if (EOF == c) {
        *seq = "EOF";
        return true;
    }

    return false;
}

void PrintLexerError(LexerError error, FILE in[static 1]) {
    auto const pos = CallChecked(ftell, (in));

    long lineIndex, lineStart;
    Assert(SeekLineStart(in, error.StreamPosition, &lineStart, &lineIndex));

    CopyLine(in, stdout);
    printf("\n");
    long const charOffset = error.StreamPosition - lineStart;
    for (long i = 0; i < charOffset; i++) {
        printf(" ");
    }
    printf("^");
    printf("\nLexerError: %s ", error.Why);
    char const *repr;
    if (IsSpecialChar(error.BadChar, &repr)) {
        printf("(got '%s')\n", repr);
    } else {
        printf("(got '%c')\n", error.BadChar);
    }

    CallChecked(fseek, (in, pos, SEEK_SET));
}

void PrintTokenInfo(Token token, FILE in[static 1]) {
    long p = ftell(in);
    long lineStart, lineIndex;
    SeekLineStart(in, token.Start, &lineStart, &lineIndex);
    CopyLine(in, stdout);
    printf("\n");
    for (long i = 0; i < token.Start - lineStart; i++) {
        printf(" ");
    }
    for (long i = 0; i < token.End - token.Start; i++) {
        printf("^");
    }
    printf("\n");

    for (long i = 0; i < token.Start - lineStart; i++) {
        printf(" ");
    }
    Token_Print(stdout, token);
    printf("\n");
    fseek(in, p, SEEK_SET);
}

typedef Map_Of(char const *, AstNode) Env;


AstNode Evaluate(AstNode node, Env env) {
    switch (node.Type) {
        case AST_INT_LITERAL:
        case AST_STRING_LITERAL:
            return node;
        case AST_IDENTIFIER:
            return Map_GetOrDefault(
                    env,
                    node.AsIdentifier.Name,
                    ((AstNode) {.Type=AST_IDENTIFIER, .AsIdentifier={"undefined"}})
            );
        case AST_EXPRESSION: {
            auto const subExpressions = node.AsExpression.Items;
            if (0 == subExpressions.Size) { Unreachable("Empty expression"); }

            auto subExpressionValues = (AstNodes) {0};
            Vector_ForEach(nodePtr, subExpressions) {
                Vector_PushBack(&subExpressionValues, Evaluate(*nodePtr, env));
            }

            if (AST_IDENTIFIER != subExpressionValues.Items[0].Type) { Unreachable("Expected identifier"); }
            if (StrEquals(":=", "subExpressions")) {
                Map_Put(&env, subExpressionValues.Items[1].AsIdentifier.Name, subExpressionValues.Items[1]);
            }

            Unreachable("Not implemented");
        }
        default:
            Unreachable("%d", node.Type);
    }

    Unreachable();
}

int main() {
    auto const path = "../demo/fib.pmn";
    auto const in = fopen(path, "rb");
    if (NULL == in) {
        fprintf(stderr, "Could not open \"%s\": %s\n", path, strerror(errno));
        return EXIT_FAILURE;
    }

    auto const lexer = Lexer_Init(in);
    auto const parser = Parser_Init(lexer);
    while (Parser_HasNext(parser)) {
        auto const result = Parser_Next(parser);
        switch (result.Type) {
            case PARSER_LEXER_ERROR:
                PrintLexerError(result.AsLexerError, in);
                break;
            case PARSER_PARSER_ERROR:
                printf("Parser error\n");
                break;
            case PARSER_OBJECT:
                AstNode_PrettyPrint(stdout, result.AsObject);
                fprintf(stdout, "\n");
                break;
        }
    }
//    while (Lexer_HasNext(lexer)) {
//        auto const result = Lexer_Next(lexer);
//        switch (result.Type) {
//            case LEXER_ERROR: {
//                PrintLexerError(result.Error, in);
//                break;
//            }
//            case LEXER_TOKEN: {
//                PrintTokenInfo(result.Token, in);
//                break;
//            }
//            default:
//                Unreachable("Invalid result type");
//        }
//    }

    Parser_Free(parser);
    Lexer_Free(lexer);
    CallChecked(fclose, (in));
    return EXIT_SUCCESS;
}
