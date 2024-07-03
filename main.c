#include <stdlib.h>

#include "reader.h"
#include "guards.h"
#include "object_env.h"
#include "primitives.h"
#include "stack.h"
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

static Object *env_default(ObjectAllocator *a) {
    auto env = env_new(a, object_nil());
    define_primitives(a, env);
    return env;
}

static bool try_eval_input(Reader *r, ObjectAllocator *allocator, Stack *stack, Object *env) {
    auto exprs = (Objects) {0};

    if (false == reader_try_prompt(r, &exprs)) {
        return true;
    }

    if (slice_empty(exprs)) {
        return false;
    }

    slice_for(it, exprs) {
        Object *value;
        if (try_eval(allocator, stack, env, *it, &value)) {
            object_repr_print(value, stdout);
            printf("\n");
            continue;
        }
    }

    return true;
}

static void run_repl(Reader *r, ObjectAllocator *allocator, Stack *stack, Object *env) {
    printf("env: ");
    object_repr_print(env, stdout);
    printf("\n");

    auto stream_is_open = true;
    while (stream_is_open) {
        stream_is_open = try_eval_input(r, allocator, stack, env);
    }

    allocator_free(&allocator);
}

static bool try_eval_file(Reader *r, ObjectAllocator *allocator, Stack *stack, Object *env) {
    auto exprs = (Objects) {0};

    if (false == reader_try_read_all(r, &exprs)) {
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
    auto allocator = allocator_new();
    auto env = env_default(allocator);

    char *file_name;
    bool ok = true;
    if (try_shift_args(&argc, &argv, &file_name)) {
        auto handle = fopen(file_name, "rb");
        if (nullptr == file_name) {
            fprintf(OUTPUT_STREAM, "Could not open \"%s\": %s\n", file_name, strerror(errno));

            allocator_free(&allocator);
            arena_free(a);
            return EXIT_FAILURE;
        }

        auto r = reader_new((NamedFile) {.name = file_name, .handle=handle}, allocator);
        ok = try_eval_file(r, allocator, stack, env);
        fclose(handle);
        reader_free(&r);
    } else {
        auto r = reader_new((NamedFile) {.name = "<stdin>", .handle = stdin}, allocator);
        run_repl(r, allocator, stack, env);
        reader_free(&r);
    }

    allocator_free(&allocator);
    arena_free(a);
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}