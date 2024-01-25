#include "lexer.h"

#include <ctype.h>

#include "common/dynamic_array/dynamic_array.h"
#include "common/file_utils/file_utils.h"
#include "common/macros.h"

static bool IsSpace(int c) {
    return isspace(c) || ',' == c;
}

static bool IsIdentifierChar(int c) {
    if (isalpha(c)) {
        return true;
    }

    const char otherChars[] = "+-*/|";
    for (const char *it = otherChars; '\0' != *it; it++) {
        if (*it == c) {
            return true;
        }
    }

    return false;
}

static bool IsSpecialChar(int c, const char *seq[static 1]) {
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

    if (EOF == c) {
        *seq = "EOF";
        return true;
    }

    return false;
}

Lexer Lexer_New(FILE file[static 1]) {
    return (Lexer) {
            .file = file,
            .lastTokenType = TOKEN_NONE,
            .buffer = {0}
    };
}

void Lexer_Free(Lexer lexer[static 1]) {
    DynamicArray_Free(&lexer->buffer);
}

bool Lexer_SkipToken(Lexer lexer[static 1]) {
    int c;
    while (EOF != (c = fgetc(lexer->file))) {
        if (IsSpace(c)) {
            break;
        }
    }

    return EOF != c;
}

bool Lexer_SyntaxError(Lexer lexer[static 1], const char *expected, int got, Token token[static 1]) {
    auto const charPos = CallChecked(ftell, (lexer->file)) - 1;

    long lineStart, lineNumber;
    if (false == SeekLineStart(lexer->file, charPos, &lineStart, &lineNumber)) {
        Unreachable("SeekLineStart must succeed for position within file");
    }

    long charLineOffset = charPos - lineStart;
    printf("Line %ld, offset %ld:\n", lineNumber, charLineOffset);

    CopyLine(lexer->file, stdout);
    printf("\n");

    for (long i = 0; i < charLineOffset; i++) {
        printf(" ");
    }
    printf("^\n");

    char const *charRepr;
    if (IsSpecialChar(got, &charRepr)) {
        printf("SyntaxError: Expected %s, got '%s'\n", expected, charRepr);
    } else {
        printf("SyntaxError: Expected %s, got '%c'\n", expected, got);
    }

    CallChecked(fseek, (lexer->file, charPos, SEEK_SET));

    lexer->lastTokenType = TOKEN_INVALID;
    *token = (Token) {
            .Type = lexer->lastTokenType,
            .Start = lexer->tokenStart,
            .End = CallChecked(ftell, (lexer->file))
    };

    return Lexer_SkipToken(lexer);
}

bool Lexer_ParseStringLiteral(Lexer lexer[static 1], Token token[static 1]) {
    DynamicArray_Clear(&lexer->buffer);

    int c;
    while (EOF != (c = fgetc(lexer->file)) && '"' != c) {
        if ('\\' == c) {
            Lexer_SyntaxError(lexer, "valid string literal character", c, token);
            TODO("Escape sequences are not supported yet");
        }
        DynamicArray_Append(&lexer->buffer, c);
    }

    if (feof(lexer->file)) {
        return Lexer_SyntaxError(lexer, "'\"'", EOF, token);
    }

    DynamicArray_Append(&lexer->buffer, '\0');

    lexer->lastTokenType = TOKEN_STRING_LITERAL;
    *token = (Token) {
            .Type = lexer->lastTokenType,
            .Start = lexer->tokenStart,
            .End = ftell(lexer->file),
            .StringLiteral = lexer->buffer.Data,
    };

    return EOF != c;
}

bool Lexer_ParseIdentifier(Lexer lexer[static 1], Token token[static 1]) {
    int c;
    while (EOF != (c = fgetc(lexer->file)) && IsIdentifierChar(c)) {
        DynamicArray_Append(&lexer->buffer, c);
    }
    DynamicArray_Append(&lexer->buffer, '\0');

    lexer->lastTokenType = TOKEN_IDENTIFIER;
    *token = (Token) {
            .Type = lexer->lastTokenType,
            .Start = lexer->tokenStart,
            .End = ftell(lexer->file) - 1,
            .Identifier = lexer->buffer.Data,
    };

    ungetc(c, lexer->file);
    return EOF != c;
}

bool Lexer_ParseIntLiteral(Lexer lexer[static 1], Token token[static 1]) {
    int c;
    while (EOF != (c = fgetc(lexer->file)) && isdigit(c)) {
        DynamicArray_Append(&lexer->buffer, c);
    }
    DynamicArray_Append(&lexer->buffer, '\0');

    errno = 0;
    auto value = CallChecked(strtoll, (lexer->buffer.Data, NULL, 10));

    lexer->lastTokenType = TOKEN_INT_LITERAL;
    *token = (Token) {
            .Type = lexer->lastTokenType,
            .Start = lexer->tokenStart,
            .End = ftell(lexer->file) - 1,
            .IntLiteral = value
    };

    return EOF != c;
}

bool Lexer_NextToken(Lexer lexer[static 1], Token token[static 1]) {
    DynamicArray_Clear(&lexer->buffer);

    int c;
    while (EOF != (c = fgetc(lexer->file))) {
        lexer->tokenStart = CallChecked(ftell, (lexer->file)) - 1;
        DynamicArray_Append(&lexer->buffer, c);

        if (TOKEN_DOT == lexer->lastTokenType) {
            if (IsIdentifierChar(c)) {
                return Lexer_ParseIdentifier(lexer, token);
            }
            return Lexer_SyntaxError(lexer, "identifier", c, token);
        }

        if (TOKEN_IDENTIFIER == lexer->lastTokenType) {
            if ('.' == c) {
                lexer->lastTokenType = TOKEN_DOT;
                *token = (Token) {
                        .Type = lexer->lastTokenType,
                        .Start = lexer->tokenStart,
                        .End = lexer->tokenStart + 1
                };
                break;
            }
        }

        if ('"' == c) {
            return Lexer_ParseStringLiteral(lexer, token);
        }

        if ('(' == c) {
            lexer->lastTokenType = TOKEN_OPEN_PAREN;
            *token = (Token) {
                    .Type = lexer->lastTokenType,
                    .Start = lexer->tokenStart,
                    .End = lexer->tokenStart + 1
            };
            break;
        }

        if (')' == c) {
            lexer->lastTokenType = TOKEN_CLOSE_PAREN;
            *token = (Token) {
                    .Type = lexer->lastTokenType,
                    .Start = lexer->tokenStart,
                    .End = lexer->tokenStart + 1
            };
            break;
        }

        if ('-' == c || '+' == c || isdigit(c)) {
            return Lexer_ParseIntLiteral(lexer, token);
        }

        if (IsIdentifierChar(c)) {
            return Lexer_ParseIdentifier(lexer, token);
        }

        if (IsSpace(c)) {
            lexer->lastTokenType = TOKEN_NONE;
            DynamicArray_Clear(&lexer->buffer);
            continue;
        }

        return Lexer_SyntaxError(lexer, "whitespace or ','", c, token);
    }

    return EOF != c;
}
