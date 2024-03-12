#include "token.h"

#include <stdio.h>
#include <inttypes.h>

#include "common/macros.h"

const char *TokenType_Name(TokenType tokenType) {
    switch (tokenType) {
        case TOKEN_INVALID:
            return "TOKEN_INVALID";
        case TOKEN_OPEN_PAREN:
            return "TOKEN_OPEN_PAREN";
        case TOKEN_CLOSE_PAREN:
            return "TOKEN_CLOSE_PAREN";
        case TOKEN_IDENTIFIER:
            return "TOKEN_IDENTIFIER";
        case TOKEN_INT_LITERAL:
            return "TOKEN_INT_LITERAL";
        case TOKEN_STRING_LITERAL:
            return "TOKEN_STRING_LITERAL";
        default:
            Unreachable("tokenType=%d", tokenType);
    }
}

void Token_Print(FILE file[static 1], Token token) {
    fprintf(
            file,
            "(Token) {.Type=%s, .Start=%ld, .End=%ld",
            TokenType_Name(token.Type),
            token.Start,
            token.End
    );

    switch (token.Type) {
        case TOKEN_IDENTIFIER: {
            fprintf(file, ", .AsIdentifier=`%s`}", token.Value.AsIdentifier);
            return;
        }
        case TOKEN_STRING_LITERAL: {
            fprintf(file, ", .AsString=\"%s\"}", token.Value.AsStringLiteral);
            return;
        }
        case TOKEN_INT_LITERAL: {
            fprintf(file, ", .AsInt=%" PRId64 "}", token.Value.AsIntLiteral);
            return;
        }
        case TOKEN_NONE:
        case TOKEN_INVALID:
        case TOKEN_OPEN_PAREN:
        case TOKEN_CLOSE_PAREN: {
            fprintf(file, "}");
            return;
        }
    }

    Unreachable("%d", token.Type);
}
