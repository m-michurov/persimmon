#pragma once

#include "vm/virtual_machine.h"

void create_os_error(VirtualMachine *vm, errno_t error_code, Object **error);

void create_type_error_(
        VirtualMachine *vm,
        Object **error,
        Object_Type got,
        size_t expected_count,
        Object_Type *expected
);

#define create_type_error(VM, Error, Got, ...)                          \
create_type_error_(                                                     \
    (VM), (Error), (Got),                                               \
    (sizeof(((Object_Type[]) {__VA_ARGS__})) / sizeof(Object_Type)),    \
    ((Object_Type[]) {__VA_ARGS__})                                     \
)

void create_syntax_error(
        VirtualMachine *vm,
        SyntaxError base,
        char const *file,
        char const *text,
        Object **error
);

void create_call_error(VirtualMachine *vm, char const *name, size_t expected_args, size_t got_args, Object **error);

void create_name_error(VirtualMachine *vm, char const *name, Object **error);

void create_zero_division_error(VirtualMachine *vm, Object **error);

void create_out_of_memory_error(VirtualMachine *vm, Object **error);

void create_stack_overflow_error(VirtualMachine *vm, Object **error);

void create_import_nesting_too_deep_error(VirtualMachine *vm, Object **error);

void create_if_too_few_args_error(VirtualMachine *vm, Object **error);

void create_if_too_many_args_error(VirtualMachine *vm, Object **error);

void create_fn_too_few_args_error(VirtualMachine *vm, Object **error);

void create_fn_args_type_error(VirtualMachine *vm, Object **error);

void create_import_args_error(VirtualMachine *vm, Object **error);

void create_import_path_type_error(VirtualMachine *vm, Object **error);

void create_define_args_error(VirtualMachine *vm, Object **error);

void create_define_name_type_error(VirtualMachine *vm, Object **error);