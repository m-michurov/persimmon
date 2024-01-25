#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "common/macros.h"
#include "common/file_utils/file_utils.h"
#include "lexer/lexer.h"

int main() {
    auto token = (Token) {};
    auto path = "../demo/test.p";
    auto in = fopen(path, "rb");
    if (NULL == in) {
        fprintf(stderr, "Could not open \"%s\": %s\n", path, strerror(errno));
        return EXIT_FAILURE;
    }

    auto lexer = Lexer_New(in);

    while (Lexer_NextToken(&lexer, &token)) {
        long p = ftell(in);
        Token_Print(stdout, token);
        printf("\n");
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
        fseek(in, p, SEEK_SET);
    }

    Lexer_Free(&lexer);
    CallChecked(fclose, (in));
    return EXIT_SUCCESS;
}
