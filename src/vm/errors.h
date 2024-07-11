#pragma once

#include "vm/virtual_machine.h"

#define ERRORS__error(Fn, ...)  \
do {                            \
    Fn(__VA_ARGS__);            \
    return false;               \
} while (false)

void create_os_error(VirtualMachine *vm, errno_t error_code, Object **error);

#define os_error(VM, Errno, Error) ERRORS__error(create_os_error, (VM), (Errno), (Error))

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

#define type_error(VM, Error, Got, ...) ERRORS__error(create_type_error, (VM), (Error), (Got), __VA_ARGS__)

void create_syntax_error(
        VirtualMachine *vm,
        SyntaxError base,
        char const *file,
        char const *text,
        Object **error
);

#define syntax_error(VM, Base, File, Text, Error) \
    ERRORS__error(create_syntax_error, (VM), (Base), (File), (Text), (Error))

void create_call_error(VirtualMachine *vm, char const *name, size_t expected_args, size_t got_args, Object **error);

#define call_error(VM, Name, ExpectedArgs, GotArgs, Error) \
    ERRORS__error(create_call_error, (VM), (Name), (ExpectedArgs), (GotArgs), (Error))

void create_name_error(VirtualMachine *vm, char const *name, Object **error);

#define name_error(VM, Name, Error) ERRORS__error(create_name_error, (VM), (Name), (Error))

void create_zero_division_error(VirtualMachine *vm, Object **error);

#define zero_division_error(VM, Error) ERRORS__error(create_zero_division_error, (VM), (Error))

void create_out_of_memory_error(VirtualMachine *vm, Object **error);

#define out_of_memory_error(VM, Error) ERRORS__error(create_out_of_memory_error, (VM), (Error))

void create_stack_overflow_error(VirtualMachine *vm, Object **error);

#define stack_overflow_error(VM, Error) ERRORS__error(create_stack_overflow_error, (VM), (Error))

void create_import_nesting_too_deep_error(VirtualMachine *vm, Object **error);

#define import_nesting_too_deep_error(VM, Error) ERRORS__error(create_import_nesting_too_deep_error, (VM), (Error))

void create_if_too_few_args_error(VirtualMachine *vm, Object **error);

#define if_too_few_args_error(VM, Error) ERRORS__error(create_if_too_few_args_error, (VM), (Error))

void create_if_too_many_args_error(VirtualMachine *vm, Object **error);

#define if_too_many_args_error(VM, Error) ERRORS__error(create_if_too_many_args_error, (VM), (Error))

void create_fn_too_few_args_error(VirtualMachine *vm, Object **error);

#define fn_too_few_args_error(VM, Error) ERRORS__error(create_fn_too_few_args_error, (VM), (Error))

void create_fn_args_type_error(VirtualMachine *vm, Object **error);

#define fn_args_type_error(VM, Error) ERRORS__error(create_fn_args_type_error, (VM), (Error))

void create_import_args_error(VirtualMachine *vm, Object **error);

#define import_args_error(VM, Error) ERRORS__error(create_import_args_error, (VM), (Error))

void create_import_path_type_error(VirtualMachine *vm, Object **error);

#define import_path_type_error(VM, Error) ERRORS__error(create_import_path_type_error, (VM), (Error))

void create_define_args_error(VirtualMachine *vm, Object **error);

#define define_args_error(VM, Error) ERRORS__error(create_define_args_error, (VM), (Error))

void create_define_name_type_error(VirtualMachine *vm, Object **error);

#define define_name_type_error(VM, Error) ERRORS__error(create_define_name_type_error, (VM), (Error))