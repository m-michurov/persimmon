#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdint.h>

#include "common/macros.h"
#include "lexer/token.h"
#include "common/dynamic_array/dynamic_array.h"

typedef struct CharBuffer {
    char *Data;
    size_t Size;
    size_t Capacity;
} CharBuffer;

bool IsSpace(int c) {
    return isspace(c) || ',' == c;
}

bool IsIdentifierChar(int c) {
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

bool IsSpecialChar(int c, const char *seq[static 1]) {
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

void CopyLine(FILE src[static 1], FILE dst[static 1]) {
    const size_t count = 1024;
    char buf[count];

    size_t readCount;
    while ((readCount = fread(buf, sizeof(*buf), count, src)) > 0) {
        for (size_t i = 0; i < readCount && '\r' != buf[i] && '\n' != buf[i]; i++) {
            fputc(buf[i], dst);
        }
    }
}

typedef struct Lexer {
    FILE *file;
    TokenType lastTokenType;
    CharBuffer buffer;
} Lexer;

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

bool SeekLineStart(
        FILE file[static 1],
        long charPos,
        long lineStart[static 1],
        long lineNumber[static 1]
) {
    rewind(file);

    long lineNo = 0;
    long curLineStart = 0;
    long curLineEnd;

    while (true) {
        auto c = fgetc(file);
        if ('\n' == c || feof(file)) {
            curLineEnd = ftell(file);
            if (-1 == curLineEnd) {
                CALL_FAILED(ftell, "Failed to get position in file");
            }

            lineNo++;

            if (charPos >= curLineStart && charPos < curLineEnd) {
                if (-1 == fseek(file, curLineStart, SEEK_SET)) {
                    CALL_FAILED(ftell, "Failed to set position in file");
                }

                *lineStart = curLineStart;
                *lineNumber = lineNo;

                return true;
            }

            curLineStart = curLineEnd;
        }

        if (EOF == c) {
            break;
        }
    }

    return false;
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
    auto const charPos = ftell(lexer->file) - 1;
    if (-1 == charPos) {
        CALL_FAILED(ftell, "Failed to get position in file");
    }

    long lineStart, lineNumber;
    if (false == SeekLineStart(lexer->file, charPos, &lineStart, &lineNumber)) {
        UNREACHABLE("SeekLineStart must succeed for position within file");
    }

    long charLineOffset = charPos - lineStart;
    printf("Line %ld, offset %ld:\n", lineNumber, charLineOffset);

    CopyLine(lexer->file, stdout);
    printf("\n");

    for (long i = 0; i < charLineOffset; i++) {
        printf(" ");
    }
    printf("^\n");

    const char *charRepr;
    if (IsSpecialChar(got, &charRepr)) {
        printf("SyntaxError: Expected %s, got '%s'\n", expected, charRepr);
    } else {
        printf("SyntaxError: Expected %s, got '%c'\n", expected, got);
    }

    if (-1 == fseek(lexer->file, charPos, SEEK_SET)) {
        CALL_FAILED(ftell, "Failed to set position in file");
    }

    lexer->lastTokenType = TOKEN_INVALID;
    *token = (Token) {.Type = lexer->lastTokenType};

    return Lexer_SkipToken(lexer);
}

bool Lexer_ParseStringLiteral(Lexer lexer[static 1], Token token[static 1]) {
    DynamicArray_Clear(&lexer->buffer);

    int c;
    while (EOF != (c = fgetc(lexer->file)) && '"' != c) {
        DynamicArray_Append(&lexer->buffer, c);
    }

    if (feof(lexer->file)) {
        return Lexer_SyntaxError(lexer, "'\"'", EOF, token);
    }

    DynamicArray_Append(&lexer->buffer, '\0');

    *token = (Token) {
            .Type = TOKEN_STRING_LITERAL,
            .StringLiteral = lexer->buffer.Data,
    };
    lexer->lastTokenType = TOKEN_STRING_LITERAL;

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
    auto value = strtoll(lexer->buffer.Data, NULL, 10);
    if (errno) {
        CALL_FAILED(strtoll, "Failed to parse int literal");
    }

    lexer->lastTokenType = TOKEN_INT_LITERAL;
    *token = (Token) {.Type = lexer->lastTokenType, .IntLiteral = value};

    return EOF != c;
}

/*
(list.append x "hello world")

line
(list. append x "hello world")

(b.)
*/

bool Lexer_NextToken(Lexer lexer[static 1], Token token[static 1]) {
    DynamicArray_Clear(&lexer->buffer);

    int c;
    while (EOF != (c = fgetc(lexer->file))) {
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
                *token = (Token) {.Type = lexer->lastTokenType};
                break;
            }
        }

        if ('"' == c) {
            return Lexer_ParseStringLiteral(lexer, token);
        }

        if ('(' == c) {
            lexer->lastTokenType = TOKEN_OPEN_PAREN;
            *token = (Token) {.Type = lexer->lastTokenType};
            break;
        }

        if (')' == c) {
            lexer->lastTokenType = TOKEN_CLOSE_PAREN;
            *token = (Token) {.Type = lexer->lastTokenType};
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

int main() {
    auto token = (Token) {};
    auto path = "../demo/test.p";
    auto in = fopen(path, "rb");
    if (NULL == in) {
        CALL_FAILED(fopen, "%s", path);
    }

    auto lexer = Lexer_New(in);

    while (Lexer_NextToken(&lexer, &token)) {
        Token_Print(stdout, token);
        printf("\n");
    }

    Lexer_Free(&lexer);
    fclose(in);
    return EXIT_SUCCESS;
}
