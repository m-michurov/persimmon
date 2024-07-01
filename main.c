#include <stdlib.h>

#include "reader.h"
#include "guards.h"
#include "object_env.h"
#include "primitives.h"
#include "stack.h"
#include "object_arena_allocator.h"
#include "slice.h"
#include "eval.h"

#define OUTPUT_STREAM stdout
#define MAX_STACK_DEPTH 10

static bool try_shift_args(int *argc, char ***argv, char **arg) {
    if (*argc <= 0) {
        return false;
    }

    (*argc)--;
    if (nullptr != arg) {
        *arg = (*argv)[0];
    }
    (*argv)++;

    return true;
}

static Object *env_default(Object_Allocator *a) {
    auto env = env_new(a, object_nil());
    define_primitives(a, env);
    return env;
}

static bool try_eval_input(Arena *a, Reader *r, Object_Allocator *allocator, Stack *stack, Object *env) {
    auto exprs = (Objects) {0};

    if (false == reader_try_prompt(a, r, &exprs)) {
        return true;
    }

    if (slice_empty(exprs)) {
        return false;
    }

    slice_for(it, exprs) {
        Object *value;
        if (try_eval(allocator, stack, env, *it, &value)) {
            fprintf(OUTPUT_STREAM, "%s\n", object_repr(a, value));
            continue;
        }
    }

    return true;
}

static void run_repl(Arena *a, Reader *r, Object_Allocator *allocator, Stack *stack, Object *env) {
    fprintf(OUTPUT_STREAM, "env: %s\n", object_repr(a, env));

    auto stream_is_open = true;
    while (stream_is_open) {
        auto scratch = &(Arena) {0};
        stream_is_open = try_eval_input(scratch, r, allocator, stack, env);
        arena_free(scratch);
    }

    object_allocator_free(&allocator);
}

static bool try_eval_file(Arena *a, Reader *r, Object_Allocator *allocator, Stack *stack, Object *env) {
    auto exprs = (Objects) {0};

    if (false == reader_try_read_all(a, r, &exprs)) {
        return false;
    }

    if (slice_empty(exprs)) {
        return true;
    }

    slice_for(it, exprs) {
        Object *value;
        if (try_eval(allocator, stack, env, *it, &value)) {
            continue;
        }
        return false;
    }

    return true;
}

int main(int argc, char **argv) {
    try_shift_args(&argc, &argv, nullptr);

    auto a = &(Arena) {0};
    auto stack = stack_new(a, MAX_STACK_DEPTH);
    auto allocator = object_arena_allocator_new(a);
    auto env = env_default(allocator);

    char *file_name;
    bool ok = true;
    if (try_shift_args(&argc, &argv, &file_name)) {
        auto handle = fopen(file_name, "rb");
        if (nullptr == file_name) {
            fprintf(OUTPUT_STREAM, "Could not open \"%s\": %s\n", file_name, strerror(errno));

            object_allocator_free(&allocator);
            arena_free(a);
            return EXIT_FAILURE;
        }

        auto const r = reader_open(a, (NamedFile) {.name = file_name, .handle=handle}, allocator);
        ok = try_eval_file(a, r, allocator, stack, env);
        fclose(handle);
    } else {
        auto const r = reader_open(a, (NamedFile) {.name = "<stdin>", .handle = stdin}, allocator);
        run_repl(a, r, allocator, stack, env);
    }

    object_allocator_free(&allocator);
    arena_free(a);
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}