#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>

#include "call_checked.h"

#include "common/macros.h"
#include "common/file_utils/file_utils.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "runtime/scope.h"
#include "runtime/object.h"
#include "runtime/eval.h"
#include "runtime/builtin/functions.h"

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

void PrintParserError(ParserError error, FILE in[static 1]) {
    auto const pos = CallChecked(ftell, (in));

    printf("\nLexerError: %s\n", error.Why);
    PrintTokenInfo(error.BadToken, in);

    CallChecked(fseek, (in, pos, SEEK_SET));
}

int RunFile(Scope globalScope[static 1], char const *path) {
    auto const in = fopen(path, "rb");
    if (NULL == in) {
        fprintf(stderr, "Could not open \"%s\": %s\n", path, strerror(errno));
        return EXIT_FAILURE;
    }

    auto const lexer = Lexer_Init(in);
    auto const parser = Parser_Init(lexer);

    auto tmpRefs = TemporaryReferences_Empty();

    auto ok = true;
    while (ok && Parser_HasNext(parser)) {
        auto const result = Parser_Next(parser);
        switch (result.Type) {
            case PARSER_LEXER_ERROR: {
                PrintLexerError(result.AsLexerError, in);
                ok = false;
                break;
            }
            case PARSER_PARSER_ERROR: {
                PrintParserError(result.AsParserError, in);
                ok = false;
                break;
            }
            case PARSER_AST_NODE: {
                auto node = result.AsAstNode;
                TemporaryReferences_Add(&tmpRefs, Evaluate(globalScope, node));
                AstNode_Free(&node);
                TemporaryReferences_Free(&tmpRefs);
                break;
            }
        }
    }

    Parser_Free(parser);
    Lexer_Free(lexer);
    CallChecked(fclose, (in));

    return EXIT_SUCCESS;
}

int main(int argc, char *argv[static argc]) {
    auto globalScope = Scope_Empty();
    DefineBuiltinFunctions(&globalScope);

    for (int i = 1; i < argc; i++) {
        auto const ret = RunFile(&globalScope, argv[i]);
        if (EXIT_SUCCESS != ret) {
            break;
        }
    }

    Scope_Free(&globalScope);
    printf("Leaked Objects: %" PRId64 "\n", RuntimeObject_DanglingPointers);
    return EXIT_SUCCESS;
}
