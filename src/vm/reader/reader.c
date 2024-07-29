#include "reader.h"

#include <ctype.h>

#include "utility/guards.h"
#include "utility/slice.h"
#include "utility/strings.h"
#include "utility/exchange.h"
#include "utility/arena_append.h"
#include "object/constructors.h"
#include "object/lists.h"
#include "vm/errors.h"
#include "vm/virtual_machine.h"
#include "line_reader.h"
#include "scanner.h"
#include "parser.h"

typedef struct {
    Line *data;
    size_t count;
    size_t capacity;
} Lines;

struct ObjectReader {
    VirtualMachine *vm;
    Scanner s;
    Parser p;
};

ObjectReader *object_reader_new(struct VirtualMachine *vm, Reader_Config config) {
    guard_is_not_null(vm);

    auto const r = (ObjectReader *) guard_succeeds(calloc, (1, sizeof(ObjectReader)));
    *r = (ObjectReader) {.vm = vm};
    errno_t error_code;
    guard_is_true(scanner_try_init(&r->s, config.scanner_config, &error_code));
    guard_is_true(parser_try_init(&r->p, vm_allocator(vm), config.parser_config, &error_code));
    return r;
}

void object_reader_free(ObjectReader **r) {
    guard_is_not_null(r);
    guard_is_not_null(*r);

    scanner_free(&(*r)->s);
    parser_free(&(*r)->p);

    free(*r);
    *r = nullptr;
}

void object_reader_reset(ObjectReader *r) {
    scanner_reset(&r->s);
    parser_reset(&r->p);
}

struct Parser_ExpressionsStack const *object_reader_parser_stack(ObjectReader const *r) {
    guard_is_not_null(r);

    return &r->p.exprs_stack;
}

Object *const *object_reader_parser_expr(ObjectReader const *r) {
    guard_is_not_null(r);

    return &r->p.expr;
}

static bool try_parse_line(
        ObjectReader *r,
        char const *file_name,
        Lines lines,
        Line line,
        Object **exprs
) {
    guard_is_not_null(r);
    guard_is_not_null(exprs);

    auto pos = (Position) {.lineno = line.lineno};
    string_for(it, line.data) {
        auto const char_pos = exchange(pos, ((Position) {
                .lineno = pos.lineno,
                .col = pos.col + 1,
                .end_col = pos.end_col + 1
        }));

        SyntaxError syntax_error;
        if (false == scanner_try_accept(&r->s, char_pos, *it, &syntax_error)) {
            auto const erroneous_line = slice_at(lines, syntax_error.pos.lineno - 1)->data;
            syntax_error(r->vm, syntax_error, file_name, erroneous_line);
        }

        if (false == r->s.has_token) {
            continue;
        }

        switch (parser_try_accept(&r->p, r->s.token, &syntax_error)) {
            case PARSER_OK: {
                break;
            }
            case PARSER_SYNTAX_ERROR: {
                auto const erroneous_line = slice_at(lines, syntax_error.pos.lineno - 1)->data;
                syntax_error(r->vm, syntax_error, file_name, erroneous_line);
            }
            case PARSER_ALLOCATION_ERROR: {
                out_of_memory_error(r->vm);
            }
        }

        if (false == r->p.has_expr) {
            continue;
        }

        if (object_try_make_cons(vm_allocator(r->vm), r->p.expr, *exprs, exprs)) {
            continue;
        }

        out_of_memory_error(r->vm);
    }

    return true;
}

#define PROMPT_NEW      ">>>"
#define PROMPT_CONTINUE "..."

static bool try_prompt(
        ObjectReader *r,
        LineReader *line_reader,
        Arena *lines_arena,
        char const *file_name,
        Object **exprs
) {
    guard_is_not_null(r);
    guard_is_not_null(exprs);

    object_reader_reset(r);
    *exprs = object_nil();

    auto line = (Line) {0};
    auto lines = (Lines) {0};
    while (slice_empty(line) || string_is_blank(line.data) || parser_is_inside_expression(r->p)) {
        printf("%s ", (line.lineno > 0 ? PROMPT_CONTINUE : PROMPT_NEW));

        errno_t error_code;
        if (false == line_reader_try_read(line_reader, lines_arena, &line, &error_code)) {
            os_error(r->vm, error_code);
        }

        if (nullptr == line.data) {
            return true;
        }

        if (false == arena_try_append(lines_arena, &lines, line, &error_code)) {
            os_error(r->vm, error_code);
        }

        if (string_is_blank(line.data)) {
            continue;
        }

        if (try_parse_line(r, file_name, lines, line, exprs)) {
            continue;
        }

        return false;
    }

    return true;
}

static bool try_read_all(
        ObjectReader *r,
        LineReader *line_reader,
        Arena *lines_arena,
        char const *file_name,
        Object **exprs
) {
    guard_is_not_null(r);

    object_reader_reset(r);

    auto line = (Line) {0};
    auto lines = (Lines) {0};
    while (true) {
        errno_t error_code;
        if (false == line_reader_try_read(line_reader, lines_arena, &line, &error_code)) {
            os_error(r->vm, error_code);
        }

        if (nullptr == line.data) {
            break;
        }

        if (false == arena_try_append(lines_arena, &lines, line, &error_code)) {
            os_error(r->vm, error_code);
        }

        if (false == try_parse_line(r, file_name, lines, line, exprs)) {
            return false;
        }
    }

    SyntaxError syntax_error;
    switch (parser_try_accept(&r->p, (Token) {.type = TOKEN_EOF}, &syntax_error)) {
        case PARSER_OK: {
            return true;
        }
        case PARSER_SYNTAX_ERROR: {
            auto const erroneous_line = slice_at(lines, syntax_error.pos.lineno - 1)->data;
            syntax_error(r->vm, syntax_error, file_name, erroneous_line);
        }
        case PARSER_ALLOCATION_ERROR: {
            out_of_memory_error(r->vm);
        }
    }

    guard_unreachable();
}

static bool reader_call(
        ObjectReader *r,
        NamedFile file,
        Object **exprs,
        bool (*fn)(ObjectReader *, LineReader *, Arena *, char const *, Object **)
) {
    auto lines_arena = (Arena) {0};
    auto line_reader = line_reader_make(file.handle);

    auto const ok = fn(r, &line_reader, &lines_arena, file.name, exprs);
    object_list_reverse(exprs);

    arena_free(&lines_arena);
    line_reader_free(&line_reader);

    return ok;
}

bool object_reader_try_prompt(ObjectReader *r, NamedFile file, Object **exprs) {
    return reader_call(r, file, exprs, try_prompt);
}

bool object_reader_try_read_all(ObjectReader *r, NamedFile file, Object **exprs) {
    return reader_call(r, file, exprs, try_read_all);
}
