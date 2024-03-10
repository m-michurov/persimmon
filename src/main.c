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

RuntimeObject *SumInts(RuntimeObjectsSlice args) {
    RuntimeInt acc = 0;
    Vector_ForEach(argPtr, args) {
        auto const arg = *(argPtr);
        if (RUNTIME_TYPE_INT != arg->Type) {
            return RuntimeObject_Undefined();
        }

        acc += arg->AsInt;
    }

    return RuntimeObject_NewInt(acc);
}

RuntimeObject *MulInts(RuntimeObjectsSlice args) {
    RuntimeInt acc = 1;
    Vector_ForEach(argPtr, args) {
        auto const arg = *(argPtr);
        if (RUNTIME_TYPE_INT != arg->Type) {
            return RuntimeObject_Undefined();
        }

        acc *= arg->AsInt;
    }

    return RuntimeObject_NewInt(acc);
}

RuntimeObject *ConcatStrings(RuntimeObjectsSlice args) {
    size_t totalLength = 0;
    Vector_ForEach(argPtr, args) {
        auto const arg = *(argPtr);
        if (RUNTIME_TYPE_STRING != arg->Type) {
            return RuntimeObject_Undefined();
        }

        totalLength += strlen(arg->AsString);
    }

    auto buffer = (char *) calloc(totalLength + 1, sizeof(char));
    Vector_ForEach(argPtr, args) {
        auto const arg = *(argPtr);
        strcat(buffer, arg->AsString);
    }

    auto result = RuntimeObject_NewString(buffer);
    free(buffer);

    return result;
}

RuntimeObject *Add(RuntimeObjectsSlice args) {
    if (Vector_Empty(args)) {
        return RuntimeObject_Undefined();
    }

    if (RUNTIME_TYPE_INT == args.Items[0]->Type) {
        return SumInts(args);
    }

    if (RUNTIME_TYPE_STRING == args.Items[0]->Type) {
        return ConcatStrings(args);
    }

    return RuntimeObject_Undefined();
}

RuntimeObject *Mul(RuntimeObjectsSlice args) {
    if (Vector_Empty(args)) {
        return RuntimeObject_Undefined();
    }

    if (RUNTIME_TYPE_INT == args.Items[0]->Type) {
        return MulInts(args);
    }

    return RuntimeObject_Undefined();
}

RuntimeObject *Print(RuntimeObjectsSlice args) {
    if (Vector_Empty(args)) {
        return RuntimeObject_Undefined();
    }

    size_t i = 0;
    Vector_ForEach(argPtr, args) {
        RuntimeObject_Print(stdout, **argPtr);
        if (i + 1 < args.Size) {
            printf(" ");
        }
        i++;
    }

    printf("\n");

    return RuntimeObject_Undefined();
}

RuntimeObject *Equals(RuntimeObjectsSlice args) {
    auto equals = true;

    for (size_t i = 0; i + 1 < args.Size; i++) {
        equals = equals && RuntimeObject_Equals(*args.Items[i], *args.Items[i + 1]);
    }

    return
            equals
            ? RuntimeObject_NewInt(1)
            : RuntimeObject_NewInt(0);
}

RuntimeObject *NotEquals(RuntimeObjectsSlice args) {
    for (size_t i = 0; i + 1 < args.Size; i++) {
        if (false == RuntimeObject_Equals(*args.Items[i], *args.Items[i + 1])) {
            return RuntimeObject_NewInt(1);
        }
    }

    return RuntimeObject_NewInt(0);
}

int main() {
    auto const path = "../demo/fib.pmn";
    auto const in = fopen(path, "rb");
    if (NULL == in) {
        fprintf(stderr, "Could not open \"%s\": %s\n", path, strerror(errno));
        return EXIT_FAILURE;
    }

    auto globalScope = Scope_Empty();
    Scope_Put(&globalScope, "+", RuntimeObject_NewNativeFunction(Add));
    Scope_Put(&globalScope, "*", RuntimeObject_NewNativeFunction(Mul));
    Scope_Put(&globalScope, "print", RuntimeObject_NewNativeFunction(Print));
    Scope_Put(&globalScope, "==", RuntimeObject_NewNativeFunction(Equals));
    Scope_Put(&globalScope, "!=", RuntimeObject_NewNativeFunction(NotEquals));

    auto const lexer = Lexer_Init(in);
    auto const parser = Parser_Init(lexer);

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
                /*
                AstNode_PrettyPrint(stdout, node);
                fprintf(stdout, "\n");
                 */
                auto value = Evaluate(&globalScope, node);
                RuntimeObject_ReferenceCreated(value);
                AstNode_Free(&node);
                /*
                RuntimeObject_Repr(stdout, value);
                fprintf(stdout, "\n");
                RuntimeObject_Print(stdout, value);
                fprintf(stdout, "\n");
                 */

                RuntimeObject_ReferenceDeleted(value);
                break;
            }
        }
    }

    Scope_Free(&globalScope);

    printf("Leaked Objects: %" PRId64 "\n", RuntimeObject_DanglingPointers);

    Parser_Free(parser);
    Lexer_Free(lexer);
    CallChecked(fclose, (in));
    return EXIT_SUCCESS;
}
