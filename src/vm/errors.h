#pragma once

#include "virtual_machine.h"
#include "bindings.h"

#define ERROR_KEY_NAME_TYPE      "type"
#define ERROR_KEY_NAME_MESSAGE   "message"
#define ERROR_KEY_NAME_TRACEBACK "traceback"

bool error_try_unpack(Object *error, Object **type, Object **message, Object **traceback);

bool error_try_unpack_type(Object *error, Object **type);

#define ERRORS__error(Fn, ...)  \
do {                            \
    Fn(__VA_ARGS__);            \
    return false;               \
} while (false)

void set_os_error(VirtualMachine *vm, errno_t error_code);

#define os_error(VM, Errno) ERRORS__error(set_os_error, (VM), (Errno))

void set_type_error_(
        VirtualMachine *vm,
        Object_Type got,
        size_t expected_count,
        Object_Type *expected
);

#define set_type_error(VM, Got, ...)                                 \
set_type_error_(                                                        \
    (VM), (Got),                                                        \
    (sizeof(((Object_Type[]) {__VA_ARGS__})) / sizeof(Object_Type)),    \
    ((Object_Type[]) {__VA_ARGS__})                                     \
)

#define type_error(VM, Got, ...) ERRORS__error(set_type_error, (VM), (Got), __VA_ARGS__)

void set_syntax_error(VirtualMachine *vm, SyntaxError error, char const *file, char const *text);

#define syntax_error(VM, Error, File, Text) \
    ERRORS__error(set_syntax_error, (VM), (Error), (File), (Text))

void set_variadic_syntax_error(VirtualMachine *vm);

#define variadic_syntax_error(VM) ERRORS__error(set_variadic_syntax_error, (VM))

void set_special_syntax_error_(
        VirtualMachine *vm,
        char const *name,
        size_t signatures_count,
        char const **signatures
);

#define set_special_syntax_error(VM, SpecialName, _0, ...)                  \
set_special_syntax_error_(                                                  \
    (VM), (SpecialName),                                                    \
    (sizeof(((char const *[]) {_0, __VA_ARGS__})) / sizeof(char const *)),  \
    ((char const *[]) {_0, __VA_ARGS__})                                    \
)

#define special_syntax_error(VM, SpecialName, ...) ERRORS__error(set_special_syntax_error, (VM), (SpecialName), __VA_ARGS__)

void set_call_args_count_error(VirtualMachine *vm, char const *name, size_t expected, size_t got);

#define call_args_count_error(VM, Name, Expected, Got) \
    ERRORS__error(set_call_args_count_error, (VM), (Name), (Expected), (Got))

void set_name_error(VirtualMachine *vm, char const *name);

#define name_error(VM, Name) ERRORS__error(set_name_error, (VM), (Name))

void set_zero_division_error(VirtualMachine *vm);

#define zero_division_error(VM) ERRORS__error(set_zero_division_error, (VM))

void set_out_of_memory_error(VirtualMachine *vm);

#define out_of_memory_error(VM) ERRORS__error(set_out_of_memory_error, (VM))

void set_stack_overflow_error(VirtualMachine *vm);

#define stack_overflow_error(VM) ERRORS__error(set_stack_overflow_error, (VM))

void set_binding_error(VirtualMachine *vm, BindingError error);

#define binding_error(VM, Error) ERRORS__error(set_binding_error, (VM), (Error))

void set_key_error(VirtualMachine *vm, Object *key);

#define key_error(VM, Key) ERRORS__error(set_key_error, (VM), (Key))
