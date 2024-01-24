#include "token.h"

#include <stdio.h>
#include <inttypes.h>

#include "common/macros.h"

const char *TokenType_Name(TokenType tokenType) {
    switch (tokenType) {
        case TOKEN_DOT:
            return "TOKEN_DOT";
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
            UNREACHABLE("tokenType=%d", tokenType);
    }
}

void Token_Print(FILE file[static 1], Token token) {
    fprintf(file, "Token{ .Type=%s", TokenType_Name(token.Type));
    switch (token.Type) {
        case TOKEN_IDENTIFIER: {
            fprintf(file, ", .Identifier=`%s`", token.Identifier);
            break;
        }
        case TOKEN_STRING_LITERAL: {
            fprintf(file, ", .StringLiteral=\"%s\"", token.StringLiteral);
            break;
        }
        case TOKEN_INT_LITERAL: {
            fprintf(file, ", .IntLiteral=%" PRId64, token.IntLiteral);
            break;
        }
        default:
            break;
    }
    fprintf(file, " }");
}
