#include "reader.h"

#include <ctype.h>

#include "utility/guards.h"
#include "utility/slice.h"
#include "utility/strings.h"
#include "utility/exchange.h"
#include "utility/arena_append.h"
#include "object/constructors.h"
#include "object/list.h"
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

bool object_reader_try_init(
        ObjectReader *r,
        struct VirtualMachine *vm,
        Reader_Config config,
        errno_t *error_code
) {
    guard_is_not_null(r);
    guard_is_not_null(vm);

    auto const a = &vm->allocator;

    *r = (ObjectReader) {._vm = vm};

    auto const ok =
            scanner_try_init(&r->_s, config.scanner_config, error_code)
            && parser_try_init(&r->_p, a, config.parser_config, error_code);
    if (false == ok) {
        object_reader_free(r);
        return false;
    }

    allocator_set_roots(a, (ObjectAllocator_Roots) {
            .parser_stack = &r->_p.exprs_stack,
            .parser_expr = &r->_p.expr
    });

    return true;
}

void object_reader_free(ObjectReader *r) {
    guard_is_not_null(r);

    scanner_free(&r->_s);
    parser_free(&r->_p);

    *r = (ObjectReader) {0};
}

void object_reader_reset(ObjectReader *r) {
    scanner_reset(&r->_s);
    parser_reset(&r->_p);
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
        if (false == scanner_try_accept(&r->_s, char_pos, *it, &syntax_error)) {
            auto const erroneous_line = slice_at(&lines, syntax_error.pos.lineno - 1)->data;
            syntax_error(r->_vm, syntax_error, file_name, erroneous_line);
        }

        if (false == r->_s.has_token) {
            continue;
        }

        Parser_Error parser_error;
        if (false == parser_try_accept(&r->_p, r->_s.token, &parser_error)) {
            switch (parser_error.type) {
                case PARSER_SYNTAX_ERROR: {
                    auto const erroneous_line = slice_at(&lines, parser_error.as_syntax_error.pos.lineno - 1)->data;
                    syntax_error(r->_vm, parser_error.as_syntax_error, file_name, erroneous_line);
                }
                case PARSER_ALLOCATION_ERROR: {
                    out_of_memory_error(r->_vm);
                }
            }

            guard_unreachable();
        }

        if (false == r->_p.has_expr) {
            continue;
        }

        if (object_try_make_list(&r->_vm->allocator, r->_p.expr, *exprs, exprs)) {
            continue;
        }

        out_of_memory_error(r->_vm);
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
    *exprs = OBJECT_NIL;

    auto line = (Line) {0};
    auto input_empty = true;
    auto lines = (Lines) {0};
    while (input_empty || parser_is_inside_expression(r->_p)) {
        printf("%s ", input_empty ? PROMPT_NEW : PROMPT_CONTINUE);

        errno_t error_code;
        if (false == line_reader_try_read(line_reader, lines_arena, &line, &error_code)) {
            os_error(r->_vm, error_code);
        }

        if (nullptr == line.data) {
            return true;
        }

        if (false == arena_try_append(lines_arena, &lines, line, &error_code)) {
            os_error(r->_vm, error_code);
        }

        if (string_is_blank(line.data)) {
            continue;
        }

        input_empty = false;
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
            os_error(r->_vm, error_code);
        }

        if (nullptr == line.data) {
            break;
        }

        if (false == arena_try_append(lines_arena, &lines, line, &error_code)) {
            os_error(r->_vm, error_code);
        }

        if (false == try_parse_line(r, file_name, lines, line, exprs)) {
            return false;
        }
    }

    Parser_Error parser_error;
    if (false == parser_try_accept(&r->_p, (Token) {.type = TOKEN_EOF}, &parser_error)) {
        switch (parser_error.type) {
            case PARSER_SYNTAX_ERROR: {
                auto const erroneous_line = slice_at(&lines, parser_error.as_syntax_error.pos.lineno - 1)->data;
                syntax_error(r->_vm, parser_error.as_syntax_error, file_name, erroneous_line);
            }
            case PARSER_ALLOCATION_ERROR: {
                out_of_memory_error(r->_vm);
            }
        }

        guard_unreachable();
    }

    return true;
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
    object_list_reverse_inplace(exprs);

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
