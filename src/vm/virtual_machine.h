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
    STATIC_NIL = 0,
    STATIC_TRUE,
    STATIC_FALSE,
    STATIC_IO_ERROR,
    STATIC_TYPE_ERROR,
    STATIC_CALL_ERROR,
    STATIC_NAME_ERROR,
    STATIC_ZERO_DIVISION_ERROR,
    STATIC_OUT_OF_MEMORY_ERROR,
    STATIC_STACK_OVERFLOW_ERROR,
    STATIC_SYNTAX_ERROR,

    STATIC_CONSTANTS_COUNT
} StaticConstantName;

Object *vm_get(VirtualMachine *vm, StaticConstantName name);
