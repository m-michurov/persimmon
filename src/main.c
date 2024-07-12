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

static bool try_env_init_default(ObjectAllocator *a, Object **env) {
    return env_try_create(a, object_nil(), env)
           && try_define_primitives(a, *env);
}

static void print_error(Object *error) {
    printf("ERROR:\n");
    if (TYPE_CONS != error->type) {
        object_repr_print(error, stdout);
        printf("\n");
        return;
    }

    printf("(");
    object_repr_print(error->as_cons.first, stdout);
    printf("\n");

    printf("    ");
    object_repr_print(error->as_cons.rest->as_cons.first, stdout);

    object_list_for(it, error->as_cons.rest->as_cons.rest) {
        printf("\n    ");
        object_repr_print(it, stdout);
    }
    printf(")\n");

    Object *traceback;
    if (object_list_try_get_tagged(error, "traceback", &traceback)) {
        traceback_print(traceback, stdout);
    }
}

static bool try_eval_input(VirtualMachine *vm) {
    auto const named_stdin = (NamedFile) {.name = "<stdin>", .handle = stdin};

    slice_try_append(vm_expressions_stack(vm), object_nil());
    Object *error;
    if (false == reader_try_prompt(vm_reader(vm), named_stdin, slice_last(*vm_expressions_stack(vm)), &error)) {
        slice_try_pop(vm_expressions_stack(vm), nullptr);
        print_error(error);
        return true;
    }

    if (object_nil() == *slice_last(*vm_expressions_stack(vm))) {
        slice_try_pop(vm_expressions_stack(vm), nullptr);
        return false;
    }

    object_list_for(it, *slice_last(*vm_expressions_stack(vm))) {
        Object *value;
        if (try_eval(vm, *vm_globals(vm), it, &value, &error)) {
            object_repr_print(value, stdout);
            printf("\n");
            continue;
        }

        print_error(error);
        break;
    }

    slice_try_pop(vm_expressions_stack(vm), nullptr);
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
    Object *error;
    if (false == reader_try_read_all(vm_reader(vm), file, slice_last(*vm_expressions_stack(vm)), &error)) {
        slice_try_pop(vm_expressions_stack(vm), nullptr);
        print_error(error);
        return false;
    }

    if (object_nil() == *slice_last(*vm_expressions_stack(vm))) {
        slice_try_pop(vm_expressions_stack(vm), nullptr);
        return false;
    }

    object_list_for(it, *slice_last(*vm_expressions_stack(vm))) {
        Object *value;
        if (try_eval(vm, *vm_globals(vm), it, &value, &error)) {
            continue;
        }

        print_error(error);
        slice_try_pop(vm_expressions_stack(vm), nullptr);
        return false;
    }

    slice_try_pop(vm_expressions_stack(vm), nullptr);
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
                            .gc_mode = ALLOCATOR_SOFT_GC
                    }
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

//    Object *error;
//    create_syntax_error(vm, (SyntaxError) {
//            .code   = SYNTAX_ERROR_UNEXPECTED_CLOSE_PAREN,
//            .pos = {
//                    .lineno = 3,
//                    .col = 7,
//                    .end_col = 7
//            }
//    }, "main.lisp", "(a b c))", &error);
//    object_repr_print(error, stdout);
//    printf("\n");

    if (false == try_env_init_default(vm_allocator(vm), vm_globals(vm))) {
        printf("Not enough memory to define basic functions\n");
        vm_free(&vm);
        return EXIT_FAILURE;
    }

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