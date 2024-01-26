#include "lexer.h"

#include <ctype.h>

#include "common/dynamic_array/dynamic_array.h"
#include "common/file_utils/file_utils.h"
#include "common/macros.h"

typedef struct CharBuffer {
    char *Data;
    size_t Size;
    size_t Capacity;
} CharBuffer;

struct Lexer {
    FILE *File;
    CharBuffer Buffer;
    long TokenStart;
    TokenType LastTokenType;
};

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

Lexer *Lexer_Init(FILE file[1]) {
    DiscardWhile(file, IsSpace, DISCARD_WHILE_TRUE);
    Lexer *lexer = CallChecked(calloc, (1, sizeof(*lexer)));
    *lexer = (Lexer) {
            .File = file,
            .LastTokenType = TOKEN_NONE,
            .Buffer = {0},
//            .HasMoreTokens = false == CallChecked(feof, (file))
    };
    return lexer;
}

void Lexer_Free(Lexer *lexer) {
    Assert(NULL != lexer);

    DynamicArray_Free(&lexer->Buffer);
    free(lexer);
}

//static void SetError(Lexer lexer[static 1], int badChar, char const *why) {
//    lexer->Token = (Token) {.Type = TOKEN_INVALID};
//    lexer->Error = (LexerError) {
//            .StreamPosition = CallChecked(ftell, (lexer->File)) - 1,
//            .BadChar = badChar,
//            .Why = why
//    };
//}

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
        IsSpace,                            \
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
} while (0)\


static LexerResult ParseStringLiteral(Lexer lexer[1]) {
    DynamicArray_Clear(&lexer->Buffer);

    int c;
    while (EOF != (c = CallChecked(fgetc, (lexer->File))) && '"' != c) {
        if ('\\' == c) {
            TODO("Escape sequences are not supported yet");
        }
        DynamicArray_Append(&lexer->Buffer, c);
    }
    DynamicArray_Append(&lexer->Buffer, '\0');

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
            .StringLiteral = lexer->Buffer.Data,
    }));
}

static LexerResult ParseIdentifier(Lexer lexer[1]) {
    int c;
    while (EOF != (c = CallChecked(fgetc, (lexer->File))) && (IsIdentifierChar(c) || isdigit(c))) {
        DynamicArray_Append(&lexer->Buffer, c);
    }
    DynamicArray_Append(&lexer->Buffer, '\0');
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
            .Identifier = lexer->Buffer.Data,
    }));
}

static LexerResult ParseIntLiteral(Lexer lexer[1]) {
    int c;
    while (EOF != (c = CallChecked(fgetc, (lexer->File))) && isdigit(c)) {
        DynamicArray_Append(&lexer->Buffer, c);
    }
    DynamicArray_Append(&lexer->Buffer, '\0');

    if (false == IsSpace(c) && ')' != c) {
        ReturnError(lexer, ((LexerError) {
                .StreamPosition = CallChecked(ftell, (lexer->File)) - 1,
                .BadChar = c,
                .Why = "Invalid int literal: expected digit, whitespace or ')'",
        }));
    }

    auto value = CallChecked(strtoll, (lexer->Buffer.Data, NULL, 10));

    ReturnToken(lexer, ((Token) {
            .Type = TOKEN_INT_LITERAL,
            .Start = lexer->TokenStart,
            .End = ftell(lexer->File) - 1,
            .IntLiteral = value
    }));
}

LexerResult Lexer_Next(Lexer *lexer) {
    Assert(NULL != lexer);

    DynamicArray_Clear(&lexer->Buffer);

    int c;
    while (EOF != (c = CallChecked(fgetc, (lexer->File)))) {
        DynamicArray_Append(&lexer->Buffer, c);
        lexer->TokenStart = CallChecked(ftell, (lexer->File)) - 1;

        if (TOKEN_DOT == lexer->LastTokenType && false == IsIdentifierChar(c)) {
            ReturnError(lexer, ((LexerError) {
                    .StreamPosition=CallChecked(ftell, (lexer->File)) - 1,
                    .BadChar=c,
                    .Why="Expected identifier"
            }));
        }

        if (TOKEN_IDENTIFIER == lexer->LastTokenType && '.' == c) {
            ReturnToken(lexer, ((Token) {
                    .Type = TOKEN_DOT,
                    .Start = lexer->TokenStart,
                    .End = lexer->TokenStart + 1
            }));
        }

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

        if ('-' == c || '+' == c || isdigit(c)) {
            return ParseIntLiteral(lexer);
        }

        if (IsIdentifierChar(c)) {
            return ParseIdentifier(lexer);
        }

        if (IsSpace(c)) {
            lexer->LastTokenType = TOKEN_NONE;
            DynamicArray_Clear(&lexer->Buffer);
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
