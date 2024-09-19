#include <stdlib.h>

#include "utility/guards.h"
#include "object/list.h"
#include "object/repr.h"
#include "vm/reader/reader.h"
#include "vm/eval.h"
#include "vm/virtual_machine.h"
#include "vm/traceback.h"
#include "vm/errors.h"

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

static void print_any_as_error(Object *error) {
    printf("Error: ");
    object_repr(error, stdout);
    printf("\n");
}

static void print_error(Object *error) {
    guard_is_not_null(error);

    Object *type, *message, *traceback;
    if (error_try_unpack(error, &type, &message, &traceback)) {
        printf("%s: %s\n", type->as_symbol, message->as_string);
        traceback_print(traceback, stdout);
        return;
    }

    if (error_try_unpack_type(error, &type)) {
        printf("%s\n", type->as_symbol);
        return;
    }

    print_any_as_error(error);
}

static bool try_eval_input(VirtualMachine *vm) {
    if (false == object_reader_try_prompt(&vm->reader, named_file_stdin, &vm->exprs)) {
        print_error(vm->error);
        return true;
    }

    if (OBJECT_NIL == vm->exprs) {
        return false;
    }

    object_list_for(it, vm->exprs) {
        if (try_eval(vm, vm->globals, it)) {
            object_repr(vm->value, stdout);
            printf("\n");
            continue;
        }

        print_error(vm->error);
        break;
    }

    return true;
}

static void run_repl(VirtualMachine *vm) {
    printf("env: ");
    object_repr(vm->globals, stdout);
    printf("\n");

    auto stream_is_open = true;
    while (stream_is_open) {
        stream_is_open = try_eval_input(vm);
    }
}

static bool try_eval_file(VirtualMachine *vm, NamedFile file) {
    if (false == object_reader_try_read_all(&vm->reader, file, &vm->exprs)) {
        print_error(vm->error);
        return true;
    }

    if (OBJECT_NIL == vm->exprs) {
        return true;
    }

    object_list_for(it, vm->exprs) {
        if (try_eval(vm, vm->globals, it)) {
            continue;
        }

        print_error(vm->error);
        return false;
    }

    return true;
}

int main(int argc, char **argv) {
    try_shift_args(&argc, &argv, nullptr);

    VirtualMachine vm;
    auto const config = (VirtualMachine_Config) {
            .allocator_config = {
                    .hard_limit = 1024 * 1024,
                    .soft_limit_initial = 1024,
                    .soft_limit_grow_factor = 1.25,
                    .debug = {
                            .no_free = true,
                            .trace = false,
                            .gc_mode = ALLOCATOR_ALWAYS_GC
                    }
            },
            .reader_config = {
                    .scanner_config = {.max_token_length = 2 * 1024},
                    .parser_config = {.max_nesting_depth = 50}
            },
            .stack_config = {.size_bytes = 2048}
    };
    if (false == vm_try_init(&vm, config)) {
        printf("ERROR: Failed to initialize VM\n");
        return EXIT_FAILURE;
    }

    char *file_name;
    bool ok = true;
    if (try_shift_args(&argc, &argv, &file_name)) {
        NamedFile file;
        if (false == named_file_try_open(file_name, "rb", &file)) {
            printf("ERROR: Could not open \"%s\": %s\n", file_name, strerror(errno));
            vm_free(&vm);
            return EXIT_FAILURE;
        }

        ok = try_eval_file(&vm, file);
        named_file_close(&file);
    } else {
        run_repl(&vm);
    }

    vm_free(&vm);
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}