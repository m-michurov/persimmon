#include <stdlib.h>

#include "utility/guards.h"
#include "utility/slice.h"
#include "object/env.h"
#include "object/lists.h"
#include "vm/reader/reader.h"
#include "vm/primitives.h"
#include "vm/eval.h"
#include "vm/virtual_machine.h"
#include "vm/traceback.h"
#include "vm/eval_errors.h"

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

static void env_init_default(ObjectAllocator *a, Object **env) {
    guard_is_true(env_try_create(a, object_nil(), env));
    define_primitives(a, *env);
}

static bool try_eval_input(VirtualMachine *vm) {
    auto const named_stdin = (NamedFile) {.name = "<stdin>", .handle = stdin};

    slice_try_append(vm_expressions_stack(vm), object_nil());
    if (false == reader_try_prompt(vm_reader(vm), named_stdin, slice_last(*vm_expressions_stack(vm)))) {
        return true;
    }

    if (object_nil() == *slice_last(*vm_expressions_stack(vm))) {
        return false;
    }

    object_list_for(it, *slice_last(*vm_expressions_stack(vm))) {
        Object *value, *error;
        if (try_eval(vm, *vm_globals(vm), it, &value, &error)) {
            object_repr_print(value, stdout);
            printf("\n");
            continue;
        }

        traceback_print_from_stack(vm_stack(vm), stdout);
        while (false == stack_is_empty(vm_stack(vm))) {
            stack_pop(vm_stack(vm));
        }
        break;
    }

    return true;
}

static void run_repl(VirtualMachine *vm) {
    printf("env: ");
    object_repr_print(*vm_globals(vm), stdout);
    printf("\n");

    auto stream_is_open = true;
    while (stream_is_open) {
        stream_is_open = try_eval_input(vm);
    }
}

static bool try_eval_file(VirtualMachine *vm, NamedFile file) {
    slice_try_append(vm_expressions_stack(vm), object_nil());
    if (false == reader_try_read_all(vm_reader(vm), file, slice_last(*vm_expressions_stack(vm)))) {
        return false;
    }

    if (object_nil() == *slice_last(*vm_expressions_stack(vm))) {
        return false;
    }

    object_list_for(it, *slice_last(*vm_expressions_stack(vm))) {
        Object *value, *error;
        if (try_eval(vm, *vm_globals(vm), it, &value, &error)) {
            continue;
        }

        traceback_print_from_stack(vm_stack(vm), stdout);
        while (false == stack_is_empty(vm_stack(vm))) { stack_pop(vm_stack(vm)); }
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
            .reader_config = {
                    .parser_config = {
                            .max_nesting_depth = 50
                    }
            },
            .stack_config = {
                    .size_bytes = 512
            },
            .import_stack_size = 2
    });
    env_init_default(vm_allocator(vm), vm_globals(vm));

    char *file_name;
    bool ok = true;
    if (try_shift_args(&argc, &argv, &file_name)) {
        auto handle = fopen(file_name, "rb");
        if (nullptr == file_name) {
            printf("Could not open \"%s\": %s\n", file_name, strerror(errno));
            vm_free(&vm);
            return EXIT_FAILURE;
        }

        ok = try_eval_file(vm, (NamedFile) {.name = file_name, .handle=handle});
        fclose(handle);
    } else {
        run_repl(vm);
    }

    vm_free(&vm);
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}