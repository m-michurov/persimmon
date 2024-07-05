#include <stdlib.h>

#include "reader.h"
#include "guards.h"
#include "object_env.h"
#include "primitives.h"
#include "stack.h"
#include "slice.h"
#include "eval.h"
#include "virtual_machine.h"
#include "dynamic_array.h"

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
    Object *env;
    guard_is_true(env_try_create(a, object_nil(), &env));
    define_primitives(a, env);
    return env;
}

static bool try_eval_input(VirtualMachine *vm, Object *env) {
    auto const named_stdin = (NamedFile) {.name = "<stdin>", .handle = stdin};

    da_append(vm_temporaries(vm), env);
    auto const exprs_begin = vm_temporaries(vm)->count;

    if (false == reader_try_prompt(vm_reader(vm), named_stdin, vm_temporaries(vm))) {
        return true;
    }

    auto const exprs = slice_take_from(Objects, *vm_temporaries(vm), exprs_begin);
    if (slice_empty(exprs)) {
        return false;
    }

    slice_for(it, exprs) {
        Object *value;
        if (try_eval(vm, env, *it, &value)) {
            object_repr_print(value, stdout);
            printf("\n");
            continue;
        }
    }

    return true;
}

static void run_repl(VirtualMachine *vm, Object *env) {
    printf("env: ");
    object_repr_print(env, stdout);
    printf("\n");

    auto stream_is_open = true;
    while (stream_is_open) {
        stream_is_open = try_eval_input(vm, env);
    }
}

static bool try_eval_file(VirtualMachine *vm, NamedFile file, Object *env) {
    da_append(vm_temporaries(vm), env);
    auto const exprs_begin = vm_temporaries(vm)->count;

    if (false == reader_try_read_all(vm_reader(vm), file, vm_temporaries(vm))) {
        return false;
    }

    auto const exprs = slice_take_from(Objects, *vm_temporaries(vm), exprs_begin);
    if (slice_empty(exprs)) {
        return false;
    }

    slice_for(it, exprs) {
        Object *value;
        if (try_eval(vm, env, *it, &value)) {
            continue;
        }
        return false;
    }

    return true;
}

int main(int argc, char **argv) {
    try_shift_args(&argc, &argv, nullptr);

    auto vm = vm_new((VirtualMachine_Config) {
            .allocator_config = {
                    .hard_limit = 1024 * 1024,
                    .soft_limit_initial = 1024,
                    .soft_limit_grow_factor = 1.25
            },
            .stack_size = 384
    });
    auto env = env_default(vm_allocator(vm));

    char *file_name;
    bool ok = true;
    if (try_shift_args(&argc, &argv, &file_name)) {
        auto handle = fopen(file_name, "rb");
        if (nullptr == file_name) {
            printf("Could not open \"%s\": %s\n", file_name, strerror(errno));
            vm_free(&vm);
            return EXIT_FAILURE;
        }

        ok = try_eval_file(vm, (NamedFile) {.name = file_name, .handle=handle}, env);
        fclose(handle);
    } else {
        run_repl(vm, env);
    }

    vm_free(&vm);
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}