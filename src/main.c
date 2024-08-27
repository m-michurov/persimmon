#include <stdlib.h>

#include "utility/guards.h"
#include "object/lists.h"
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
    char const *error_type;
    if (false == object_list_is_tagged(error, &error_type)) {
        print_any_as_error(error);
        return;
    }

    printf("%s: ", error_type);

    Object *message = nullptr;
    if (object_list_try_get_tagged_field(error, ERROR_FIELD_MESSAGE, &message)) {
        object_print(message, stdout);
    }
    printf("\n");

    Object *traceback;
    if (object_list_try_get_tagged_field(error, ERROR_FIELD_TRACEBACK, &traceback)) {
        traceback_print(traceback, stdout);
    }
}

static bool try_eval_input(VirtualMachine *vm) {
    if (false == object_reader_try_prompt(vm_reader(vm), named_file_stdin, vm_exprs(vm))) {
        print_error(*vm_error(vm));
        return true;
    }

    if (object_nil() == *vm_exprs(vm)) {
        return false;
    }

    object_list_for(it, *vm_exprs(vm)) {
        if (try_eval(vm, *vm_globals(vm), it)) {
            object_repr(*vm_value(vm), stdout);
            printf("\n");
            continue;
        }

        print_error(*vm_error(vm));
        break;
    }

    return true;
}

static void run_repl(VirtualMachine *vm) {
    printf("env: ");
    object_repr(*vm_globals(vm), stdout);
    printf("\n");

    auto stream_is_open = true;
    while (stream_is_open) {
        stream_is_open = try_eval_input(vm);
    }
}

static bool try_eval_file(VirtualMachine *vm, NamedFile file) {
    if (false == object_reader_try_read_all(vm_reader(vm), file, vm_exprs(vm))) {
        print_error(*vm_error(vm));
        return true;
    }

    if (object_nil() == *vm_exprs(vm)) {
        return true;
    }

    object_list_for(it, *vm_exprs(vm)) {
        if (try_eval(vm, *vm_globals(vm), it)) {
            continue;
        }

        print_error(*vm_error(vm));
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
                    .soft_limit_grow_factor = 1.25,
                    .debug = {
                            .no_free = true,
                            .trace = false,
                            .gc_mode = ALLOCATOR_ALWAYS_GC
                    }
            },
            .reader_config = {
                    .scanner_config = {
                            .max_token_length = 2 * 1024
                    },
                    .parser_config = {
                            .max_nesting_depth = 50
                    }
            },
            .stack_config = {
                    .size_bytes = 2048
            }
    });

    char *file_name;
    bool ok = true;
    if (try_shift_args(&argc, &argv, &file_name)) {
        NamedFile file;
        if (false == named_file_try_open(file_name, "rb", &file)) {
            printf("ERROR: Could not open \"%s\": %s\n", file_name, strerror(errno));
            vm_free(&vm);
            return EXIT_FAILURE;
        }

        ok = try_eval_file(vm, file);
        named_file_close(&file);
    } else {
        run_repl(vm);
    }

    vm_free(&vm);
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}