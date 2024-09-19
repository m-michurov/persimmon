#pragma once

#include <stdlib.h>

#include "object/allocator.h"
#include "reader/reader.h"
#include "stack.h"

typedef struct VirtualMachine VirtualMachine;

struct VirtualMachine {
    Stack stack;
    ObjectReader reader;
    ObjectAllocator allocator;

    Object *globals;
    Object *value;
    Object *error;
    Object *exprs;
};

typedef struct {
    ObjectAllocator_Config allocator_config;
    Reader_Config reader_config;
    Stack_Config stack_config;
} VirtualMachine_Config;

bool vm_try_init(VirtualMachine *vm, VirtualMachine_Config config);

void vm_free(VirtualMachine *vm);
