#include "reader.h"

#include <ctype.h>

#include "guards.h"
#include "line_reader.h"
#include "scanner.h"
#include "parser.h"
#include "slice.h"
#include "dynamic_array.h"
#include "strings.h"
#include "exchange.h"
#include "arena_append.h"

static void pad(size_t count) {
    for (size_t i = 0; i < count; i++) {
        printf(" ");
    }
}

static void reader_print_error(
        SyntaxError error,
        char const *file_name,
        char const *text
) {
    guard_is_not_null(file_name);
    guard_is_not_null(text);

    syntax_error_print(error);

    auto const line_number_chars = snprintf(nullptr, 0, "%zu", error.pos.lineno);
    pad(line_number_chars);
    printf(" --> %s\n", file_name);
    pad(line_number_chars + 2);
    printf("|\n");
    pad(1);
    printf("%zu | %s", error.pos.lineno, text);
    pad(line_number_chars + 2);
    printf("| ");

    for (size_t i = 0; i < error.pos.col; i++) {
        printf(" ");
    }
    for (size_t i = 0; i < error.pos.end_col - error.pos.col + 1; i++) {
        printf("^");
    }
    printf("\n");

}

typedef struct {
    Line *data;
    size_t count;
    size_t capacity;
} Lines;

struct Reader {
    Scanner *s;
    Parser *p;
};

Reader *reader_new(ObjectAllocator *a) {
    guard_is_not_null(a);

    auto const r = (Reader *) guard_succeeds(calloc, (1, sizeof(Reader)));
    *r = (Reader) {
            .s = scanner_new(),
            .p = parser_new(a)
    };
    return r;
}

void reader_free(Reader **r) {
    guard_is_not_null(r);
    guard_is_not_null(*r);

    scanner_free(&(*r)->s);
    parser_free(&(*r)->p);

    free(*r);
    *r = nullptr;
}

void reader_reset(Reader *r) {
    scanner_reset(r->s);
    parser_reset(r->p);
}

struct Parser_Stack const *reader_parser_stack(Reader const *r) {
    guard_is_not_null(r);

    return parser_stack(r->p);
}

Object *const *reader_parser_expr(Reader const *r) {
    guard_is_not_null(r);

    return parser_expression(r->p);
}

static bool try_parse_line(
        Scanner *t,
        Parser *p,
        Line line,
        Objects *exprs,
        SyntaxError *error
) {
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

        if (false == scanner_try_accept(t, char_pos, *it, error)) {
            return false;
        }

        auto token = scanner_peek(t);
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

        da_append(exprs, expr); // NOLINT(*-sizeof-expression)
    }

    return true;
}

#define PROMPT_NEW      ">>>"
#define PROMPT_CONTINUE "..."

static bool try_prompt(
        Reader *r,
        LineReader *line_reader,
        Arena *lines_arena,
        char const *file_name,
        Objects *exprs
) {
    guard_is_not_null(r);
    guard_is_not_null(exprs);

    reader_reset(r);

    auto line = (Line) {0};
    auto lines = (Lines) {0};
    while (slice_empty(line) || string_is_blank(line.data) || parser_is_inside_expression(r->p)) {
        printf("%s ", (line.lineno > 0 ? PROMPT_CONTINUE : PROMPT_NEW));

        if (false == line_try_read(line_reader, lines_arena, &line)) {
            return true;
        }
        arena_append(lines_arena, &lines, line);

        if (string_is_blank(line.data)) {
            continue;
        }

        SyntaxError error;
        if (try_parse_line(r->s, r->p, line, exprs, &error)) {
            continue;
        }

        reader_print_error(error, file_name, slice_at(lines, error.pos.lineno - 1)->data);
        return false;
    }

    return true;
}

static bool try_read_all(
        Reader *r,
        LineReader *line_reader,
        Arena *lines_arena,
        char const *file_name,
        Objects *exprs
) {
    guard_is_not_null(r);

    reader_reset(r);

    auto line = (Line) {0};
    auto lines = (Lines) {0};
    while (line_try_read(line_reader, lines_arena, &line)) {
        arena_append(lines_arena, &lines, line);

        SyntaxError error;
        if (false == try_parse_line(r->s, r->p, line, exprs, &error)) {
            reader_print_error(error, file_name, slice_at(lines, error.pos.lineno - 1)->data);
            return false;
        }
    }

    SyntaxError error;
    if (false == parser_try_accept(r->p, (Token) {.type = TOKEN_EOF}, &error)) {
        reader_print_error(error, file_name, slice_at(lines, error.pos.lineno - 1)->data);
        return false;
    }

    return true;
}

static bool reader_call(
        Reader *r,
        NamedFile file,
        Objects *exprs,
        bool (*fn)(Reader *, LineReader *, Arena *, char const *, Objects *)
) {
    auto lines_arena = (Arena) {0};
    auto line_reader = line_reader_new(file.handle);

    auto const ok = fn(r, line_reader, &lines_arena, file.name, exprs);

    arena_free(&lines_arena);
    line_reader_free(&line_reader);

    return ok;
}

bool reader_try_prompt(Reader *r, NamedFile file, Objects *exprs) {
    return reader_call(r, file, exprs, try_prompt);
}

bool reader_try_read_all(Reader *r, NamedFile file, Objects *exprs) {
    return reader_call(r, file, exprs, try_read_all);
}
