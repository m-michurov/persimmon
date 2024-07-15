#pragma once

#include <stdlib.h>

#include "object/allocator.h"
#include "reader/reader.h"
#include "stack.h"

typedef struct VirtualMachine VirtualMachine;

typedef struct {
    ObjectAllocator_Config allocator_config;
    Reader_Config reader_config;
    Stack_Config stack_config;
    size_t import_stack_size;
} VirtualMachine_Config;

VirtualMachine *vm_new(VirtualMachine_Config config);

void vm_free(VirtualMachine **vm);

ObjectAllocator *vm_allocator(VirtualMachine *vm);

Object **vm_value(VirtualMachine *vm);

Object **vm_error(VirtualMachine *vm);

Stack *vm_stack(VirtualMachine *vm);

Reader *vm_reader(VirtualMachine *vm);

Object **vm_globals(VirtualMachine *vm);

typedef struct VM_ExpressionsStack VM_ExpressionsStack;
struct VM_ExpressionsStack {
    Object **const data;
    size_t count;
    size_t const capacity;
};

VM_ExpressionsStack *vm_expressions_stack(VirtualMachine *vm);

typedef enum {
    STATIC_TRUE,
    STATIC_FALSE,

    STATIC_OS_ERROR_DEFAULT,
    STATIC_TYPE_ERROR_DEFAULT,
    STATIC_CALL_ERROR_DEFAULT,
    STATIC_NAME_ERROR_DEFAULT,
    STATIC_ZERO_DIVISION_ERROR_DEFAULT,
    STATIC_OUT_OF_MEMORY_ERROR_DEFAULT,
    STATIC_STACK_OVERFLOW_ERROR_DEFAULT,
    STATIC_SYNTAX_ERROR_DEFAULT,
    STATIC_SPECIAL_ERROR_DEFAULT,
    STATIC_TOO_MANY_DEFAULT,
    STATIC_BIND_DEFAULT,

    STATIC_CONSTANTS_COUNT
} StaticConstantName;

Object *vm_get(VirtualMachine const *vm, StaticConstantName name);
