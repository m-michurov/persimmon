#include "reader.h"

#include <ctype.h>

#include "arena_append.h"
#include "guards.h"
#include "io.h"
#include "tokenizer.h"
#include "parser.h"
#include "slice.h"
#include "strings.h"

void reader_print_error(ReaderError error, FILE *file) {
    guard_is_not_null(file);

    printf("  File \"%s\", line %zu\n", error.file_name, error.base.pos.lineno);
    printf("    %s", error.text);

    printf("    ");
    for (size_t i = 0; i < error.base.pos.col; i++) {
        printf(" ");
    }
    for (size_t i = 0; i < error.base.pos.end_col - error.base.pos.col + 1; i++) {
        printf("^");
    }
    printf("\n");

    syntax_error_print(file, error.base);
}

typedef struct {
    Line *data;
    size_t count;
    size_t capacity;
} Lines;

struct Reader {
    char const *file_name;
    LineReader *line_reader;
    Tokenizer *t;
    Parser *p;
};

Reader *reader_open(Arena *a, NamedFile file, Object_Allocator *allocator) {
    guard_is_not_null(a);
    guard_is_not_null(file.handle);
    guard_is_not_null(allocator);

    return arena_emplace(a, ((Reader) {
            .file_name = file.name,
            .line_reader = line_reader_open(a, file.handle),
            .t = tokenizer_new(a),
            .p = parser_new(a, allocator)
    }));
}

static void reader_reset(Reader *r) {
    line_reader_reset(r->line_reader);
    tokenizer_reset(r->t);
    parser_reset(r->p);
}

static bool try_parse_line(
        Arena *a,
        Tokenizer *t,
        Parser *p,
        Line line,
        Objects *exprs,
        SyntaxError *error
) {
    guard_is_not_null(a);
    guard_is_not_null(t);
    guard_is_not_null(p);
    guard_is_not_null(exprs);

    auto pos = (Position) {.lineno = line.lineno};
    string_for(it, line.data) {
        auto const char_pos = exchange(pos, ((Position) {
                .lineno = pos.lineno,
                .col = pos.col + 1,
                .end_col = pos.end_col + 1
        }));

        if (false == tokenizer_try_accept(t, char_pos, *it, error)) {
            return false;
        }

        auto token = tokenizer_token(t);
        if (nullptr == token) {
            continue;
        }

        if (false == parser_try_accept(p, *token, error)) {
            return false;
        }

        Object *expr;
        if (false == parser_try_get_expression(p, &expr)) {
            continue;
        }

        arena_append(a, exprs, expr); // NOLINT(*-sizeof-expression)
    }

    return true;
}

#define PROMPT_NEW      ">>>"
#define PROMPT_CONTINUE "..."

static bool reader_try_prompt_(Arena *a, Arena *scratch, Reader *r, Objects *exprs, ReaderError *error) {
    guard_is_not_null(r);
    guard_is_not_null(exprs);

    reader_reset(r);

    slice_clear(exprs);

    auto line = (Line) {0};
    auto lines = (Lines) {0};
    while (slice_empty(line) || parser_is_inside_expression(r->p)) {
        printf("%s ", (line.lineno > 0 ? PROMPT_CONTINUE : PROMPT_NEW));

        if (false == line_try_read(scratch, r->line_reader, &line)) {
            return true;
        }
        arena_append(scratch, &lines, line);

        if (string_is_blank(line.data)) {
            continue;
        }

        SyntaxError syntax_error;
        if (try_parse_line(a, r->t, r->p, line, exprs, &syntax_error)) {
            continue;
        }

        *error = (ReaderError) {
                .base = syntax_error,
                .file_name = r->file_name,
                .text = string_copy(a, slice_at(lines, syntax_error.pos.lineno - 1)->data)
        };
        return false;
    }

    return true;
}

bool reader_try_prompt(Arena *a, Reader *r, Objects *exprs, ReaderError *error) {
    auto scratch = (Arena) {0};
    auto const result = reader_try_prompt_(a, &scratch, r, exprs, error);
    arena_free(&scratch);
    return result;
}

static bool reader_try_read_all_(Arena *a, Arena *scratch, Reader *r, Objects *exprs, ReaderError *error) {
    guard_is_not_null(r);

    reader_reset(r);

    slice_clear(exprs);

    SyntaxError syntax_error;
    auto lines = (Lines) {0};

    auto line = (Line) {0};
    while (line_try_read(scratch, r->line_reader, &line)) {
        arena_append(scratch, &lines, line);

        if (false == try_parse_line(a, r->t, r->p, line, exprs, &syntax_error)) {
            *error = (ReaderError) {
                    .base = syntax_error,
                    .file_name = r->file_name,
                    .text = string_copy(a, slice_at(lines, syntax_error.pos.lineno - 1)->data)
            };
            return false;
        }
    }

    if (false == parser_try_accept(r->p, (Token) {.type = TOKEN_EOF}, &syntax_error)) {
        *error = (ReaderError) {
                .base = syntax_error,
                .file_name = r->file_name,
                .text = string_copy(a, slice_at(lines, syntax_error.pos.lineno - 1)->data)
        };
        return false;
    }

    return true;
}

bool reader_try_read_all(Arena *a, Reader *r, Objects *exprs, ReaderError *error) {
    auto scratch = (Arena) {0};
    auto const result = reader_try_read_all_(a, &scratch, r, exprs, error);
    arena_free(&scratch);
    return result;
}
