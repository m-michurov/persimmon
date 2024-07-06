#pragma once

#include <stdlib.h>

#include "object/allocator.h"
#include "reader/reader.h"
#include "stack.h"

typedef struct VirtualMachine VirtualMachine;

typedef struct {
    ObjectAllocator_Config allocator_config;
    size_t stack_size;
} VirtualMachine_Config;

VirtualMachine *vm_new(VirtualMachine_Config config);

void vm_free(VirtualMachine **vm);

ObjectAllocator *vm_allocator(VirtualMachine *vm);

Stack *vm_stack(VirtualMachine *vm);

Reader *vm_reader(VirtualMachine *vm);

Objects *vm_temporaries(VirtualMachine *vm);
