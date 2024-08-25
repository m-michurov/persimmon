#pragma once

#include "virtual_machine.h"
#include "bindings.h"

#define ERROR_FIELD_MESSAGE   "message"
#define ERROR_FIELD_TRACEBACK "traceback"

#define ERRORS__error(Fn, ...)  \
do {                            \
    Fn(__VA_ARGS__);            \
    return false;               \
} while (false)

void create_os_error(VirtualMachine *vm, errno_t error_code);

#define os_error(VM, Errno) ERRORS__error(create_os_error, (VM), (Errno))

void create_type_error_(
        VirtualMachine *vm,
        Object_Type got,
        size_t expected_count,
        Object_Type *expected
);

#define create_type_error(VM, Got, ...)                          \
create_type_error_(                                                     \
    (VM), (Got),                                               \
    (sizeof(((Object_Type[]) {__VA_ARGS__})) / sizeof(Object_Type)),    \
    ((Object_Type[]) {__VA_ARGS__})                                     \
)

#define type_error(VM, Got, ...) ERRORS__error(create_type_error, (VM), (Got), __VA_ARGS__)

void create_syntax_error(VirtualMachine *vm, SyntaxError error, char const *file, char const *text);

#define syntax_error(VM, Error, File, Text) \
    ERRORS__error(create_syntax_error, (VM), (Error), (File), (Text))

void create_call_error(VirtualMachine *vm, char const *name, size_t expected, bool is_variadic, size_t got);

#define call_error(VM, Name, Expected, IsVariadic, Got) \
    ERRORS__error(create_call_error, (VM), (Name), (Expected), (IsVariadic), (Got))

void create_call_ampersand_before_error(VirtualMachine *vm);

#define call_ampersand_before_error(VM) ERRORS__error(create_call_ampersand_before_error, (VM))

void create_call_ampersand_after_error(VirtualMachine *vm);

#define call_ampersand_after_error(VM) ERRORS__error(create_call_ampersand_after_error, (VM))

void create_call_extra_args_error(VirtualMachine *vm, Object_Type extras_type);

#define call_extra_args_error(VM, ExtrasType) ERRORS__error(create_call_extra_args_error, (VM), (ExtrasType))

void create_name_error(VirtualMachine *vm, char const *name);

#define name_error(VM, Name) ERRORS__error(create_name_error, (VM), (Name))

void create_zero_division_error(VirtualMachine *vm);

#define zero_division_error(VM) ERRORS__error(create_zero_division_error, (VM))

void create_out_of_memory_error(VirtualMachine *vm);

#define out_of_memory_error(VM) ERRORS__error(create_out_of_memory_error, (VM))

void create_stack_overflow_error(VirtualMachine *vm);

#define stack_overflow_error(VM) ERRORS__error(create_stack_overflow_error, (VM))

void create_too_few_args_error(VirtualMachine *vm, char const *name);

#define too_few_args_error(VM, Name) ERRORS__error(create_too_few_args_error, (VM), (Name))

void create_too_many_args_error(VirtualMachine *vm, char const *name);

#define too_many_args_error(VM, Name) ERRORS__error(create_too_many_args_error, (VM), (Name))

void create_args_count_error(VirtualMachine *vm, char const *name, size_t expected);

#define args_count_error(VM, Name, Expected) \
    ERRORS__error(create_args_count_error, (VM), (Name), (Expected))

void create_parameters_declaration_error(VirtualMachine *vm, BindingTargetError error);

#define parameters_declaration_error(VM, Error) ERRORS__error(create_parameters_declaration_error, (VM), (Error))

void create_import_path_type_error(VirtualMachine *vm);

#define import_path_type_error(VM) ERRORS__error(create_import_path_type_error, (VM))

void create_binding_error(VirtualMachine *vm, BindingError error);

#define binding_error(VM, Error) ERRORS__error(create_binding_error, (VM), (Error))

void create_key_error(VirtualMachine *vm, Object *key);

#define key_error(VM, Key) ERRORS__error(create_key_error, (VM), (Key))
