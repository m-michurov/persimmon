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

Stack *vm_stack(VirtualMachine *vm);

ObjectReader *vm_reader(VirtualMachine *vm);

Object **vm_value(VirtualMachine *vm);

Object **vm_error(VirtualMachine *vm);

Object **vm_globals(VirtualMachine *vm);

Object **vm_exprs(VirtualMachine *vm);

typedef enum {
    STATIC_TRUE,
    STATIC_FALSE,

    STATIC_ATOM_DO,

    STATIC_OS_ERROR_DEFAULT,
    STATIC_TYPE_ERROR_DEFAULT,
    STATIC_CALL_ERROR_DEFAULT,
    STATIC_NAME_ERROR_DEFAULT,
    STATIC_ZERO_DIVISION_ERROR_DEFAULT,
    STATIC_OUT_OF_MEMORY_ERROR_DEFAULT,
    STATIC_STACK_OVERFLOW_ERROR_DEFAULT,
    STATIC_SYNTAX_ERROR_DEFAULT,
    STATIC_SPECIAL_ERROR_DEFAULT,
    STATIC_BINDING_ERROR_DEFAULT,

    STATIC_CONSTANTS_COUNT
} VirtualMachine_StaticConstantName;

Object *vm_get(VirtualMachine const *vm, VirtualMachine_StaticConstantName name);
