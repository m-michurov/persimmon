#include "lexer.h"

#include <ctype.h>

#include "call_checked.h"
#include "collections/vector.h"

#include "common/file_utils/file_utils.h"
#include "common/macros.h"

struct Lexer {
    FILE *File;
    Vector_Of(char) Buffer;
    long TokenStart;
    TokenType LastTokenType;
};

static bool IsSpace(int c) {
    return isspace(c) || ',' == c;
}

static bool IsIdentifierFirstChar(int c) {
    if (isalpha(c)) {
        return true;
    }

    const char otherChars[] = ".+-*/|=:";
    for (const char *it = otherChars; '\0' != *it; it++) {
        if (*it == c) {
            return true;
        }
    }

    return false;
}

static bool IsIdentifierChar(int c) { return IsIdentifierFirstChar(c) || isdigit(c); }

static bool IsTokenEndChar(int c) {
    return IsSpace(c) || '(' == c || ')' == c;
}

Lexer *Lexer_Init(FILE file[static 1]) {
    DiscardWhile(file, IsSpace, DISCARD_WHILE_TRUE);
    Lexer *lexer = CallChecked(calloc, (1, sizeof(*lexer)));
    *lexer = (Lexer) {
            .File = file,
            .LastTokenType = TOKEN_NONE,
            .Buffer = {0},
    };
    return lexer;
}

void Lexer_Free(Lexer *lexer) {
    Assert(NULL != lexer);

    Vector_Free(&lexer->Buffer);
    free(lexer);
}

#define ReturnError(lexer, error)           \
do {                                        \
    __auto_type _e = (error);               \
    (lexer)->LastTokenType = TOKEN_INVALID; \
    CallChecked(fseek, (                    \
        (lexer)->File,                      \
        _e.StreamPosition,                  \
        SEEK_SET                            \
    ));                                     \
    DiscardWhile(                           \
        (lexer)->File,                      \
        IsTokenEndChar,                     \
        DISCARD_WHILE_FALSE                 \
    );                                      \
    return (LexerResult) {                  \
        .Type = LEXER_ERROR,                \
        .Error = _e,                        \
    };                                      \
} while (0)

#define ReturnToken(lexer, token)           \
do {                                        \
    (lexer)->LastTokenType = (token).Type;  \
    return (LexerResult) {                  \
        .Type = LEXER_TOKEN,                \
        .Token = (token),                   \
    };                                      \
} while (0)


static LexerResult ParseStringLiteral(Lexer lexer[1]) {
    lexer->Buffer.Size = 0;

    int c;
    while (EOF != (c = CallChecked(fgetc, (lexer->File))) && '"' != c) {
        if ('\\' == c) {
            TODO("Escape sequences are not supported yet");
        }
        Vector_PushBack(&lexer->Buffer, c);
    }
    Vector_PushBack(&lexer->Buffer, '\0');

    if (EOF == c) {
        ReturnError(lexer, ((LexerError) {
                .StreamPosition = CallChecked(ftell, (lexer->File)),
                .BadChar = c,
                .Why = "Unexpected end of file",
        }));
    }

    ReturnToken(lexer, ((Token) {
            .Type = TOKEN_STRING_LITERAL,
            .Start = lexer->TokenStart,
            .End = CallChecked(ftell, (lexer->File)),
            .StringLiteral = lexer->Buffer.Items,
    }));
}

static LexerResult ParseIdentifier(Lexer lexer[1]) {
    int c;
    while (EOF != (c = CallChecked(fgetc, (lexer->File))) && IsIdentifierChar(c)) {
        Vector_PushBack(&lexer->Buffer, c);
    }
    Vector_PushBack(&lexer->Buffer, '\0');
    CallChecked(ungetc, (c, lexer->File));

    if (false == IsSpace(c) && ')' != c && '.' != c) {
        ReturnError(lexer, ((LexerError) {
                .StreamPosition = CallChecked(ftell, (lexer->File)),
                .BadChar = c,
                .Why = "Expected whitespace or ')' or '.'",
        }));
    }

    ReturnToken(lexer, ((Token) {
            .Type = TOKEN_IDENTIFIER,
            .Start = lexer->TokenStart,
            .End = CallChecked(ftell, (lexer->File)),
            .Identifier = lexer->Buffer.Items,
    }));
}

static LexerResult ParseIntLiteral(Lexer lexer[1]) {
    int c;
    while (EOF != (c = CallChecked(fgetc, (lexer->File))) && isdigit(c)) {
        Vector_PushBack(&lexer->Buffer, c);
    }
    Vector_PushBack(&lexer->Buffer, '\0');
    CallChecked(ungetc, (c, lexer->File));

    if (false == IsSpace(c) && ')' != c) {
        ReturnError(lexer, ((LexerError) {
                .StreamPosition = CallChecked(ftell, (lexer->File)) - 1,
                .BadChar = c,
                .Why = "Invalid int literal: expected digit, whitespace or ')'",
        }));
    }

    auto value = CallChecked(strtoll, (lexer->Buffer.Items, NULL, 10));

    ReturnToken(lexer, ((Token) {
            .Type = TOKEN_INT_LITERAL,
            .Start = lexer->TokenStart,
            .End = ftell(lexer->File),
            .IntLiteral = value
    }));
}

LexerResult Lexer_Next(Lexer *lexer) {
    Assert(NULL != lexer);

    lexer->Buffer.Size = 0;

    int c;
    while (EOF != (c = CallChecked(fgetc, (lexer->File)))) {
        Vector_PushBack(&lexer->Buffer, c);
        lexer->TokenStart = CallChecked(ftell, (lexer->File)) - 1;

        if ('"' == c) {
            return ParseStringLiteral(lexer);
        }

        if ('(' == c) {
            ReturnToken(lexer, ((Token) {
                    .Type = TOKEN_OPEN_PAREN,
                    .Start = lexer->TokenStart,
                    .End = lexer->TokenStart + 1
            }));
        }

        if (')' == c) {
            ReturnToken(lexer, ((Token) {
                    .Type = TOKEN_CLOSE_PAREN,
                    .Start = lexer->TokenStart,
                    .End = lexer->TokenStart + 1
            }));
        }

        if (IsIdentifierFirstChar(c)) {
            return ParseIdentifier(lexer);
        }

        if (isdigit(c)) {
            return ParseIntLiteral(lexer);
        }

        if (IsSpace(c)) {
            lexer->LastTokenType = TOKEN_NONE;
            lexer->Buffer.Size = 0;
            continue;
        }

        ReturnError(lexer, ((LexerError) {
                .StreamPosition=CallChecked(ftell, (lexer->File)) - 1,
                .BadChar=c,
                .Why="Expected whitespace"
        }));
    }

    ReturnError(lexer, ((LexerError) {
            .StreamPosition=CallChecked(ftell, (lexer->File)) - 1,
            .BadChar=c,
            .Why="Unexpected EOF"
    }));
}

bool Lexer_HasNext(Lexer *lexer) {
    Assert(NULL != lexer);

    auto const pos = CallChecked(ftell, (lexer->File));
    DiscardWhile(lexer->File, IsSpace, DISCARD_WHILE_TRUE);
    auto const hasNext = false == feof(lexer->File);
    CallChecked(fseek, (lexer->File, pos, SEEK_SET));

    return hasNext;
}
