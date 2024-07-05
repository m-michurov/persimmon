#include "virtual_machine.h"

#include "reader.h"
#include "stack.h"
#include "object_allocator.h"
#include "guards.h"
#include "dynamic_array.h"

struct VirtualMachine {
    Reader *reader;
    ObjectAllocator *allocator;
    Stack *stack;
    Objects temporaries;
};

VirtualMachine *vm_new(VirtualMachine_Config config) {
    auto const vm = (VirtualMachine *) guard_succeeds(calloc, (1, sizeof(VirtualMachine)));

    auto const allocator = allocator_new(config.allocator_config);
    auto const reader = reader_new(allocator);
    auto const stack = stack_new(config.stack_size);

    *vm = (VirtualMachine) {
            .allocator = allocator,
            .reader = reader,
            .stack = stack,
    };

    allocator_set_roots(allocator, (ObjectAllocator_Roots) {
            .temporaries = &vm->temporaries,
            .stack = vm->stack,
            .parser_stack = reader_parser_stack(vm->reader),
            .parser_expr = reader_parser_expr(vm->reader)
    });

    return vm;
}

void vm_free(VirtualMachine **vm) {
    guard_is_not_null(vm);

    reader_free(&(*vm)->reader);
    allocator_free(&(*vm)->allocator);
    stack_free(&(*vm)->stack);
    da_free(&(*vm)->temporaries);

    free(*vm);
    *vm = nullptr;
}

ObjectAllocator *vm_allocator(VirtualMachine *vm) {
    guard_is_not_null(vm);

    return vm->allocator;
}

Stack *vm_stack(VirtualMachine *vm) {
    guard_is_not_null(vm);

    return vm->stack;
}

Reader *vm_reader(VirtualMachine *vm) {
    guard_is_not_null(vm);

    return vm->reader;
}

Objects *vm_temporaries(VirtualMachine *vm) {
    guard_is_not_null(vm);

    return &vm->temporaries;
}
