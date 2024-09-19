#include "virtual_machine.h"

#include "utility/guards.h"
#include "object/constructors.h"
#include "reader/reader.h"
#include "env.h"
#include "stack.h"
#include "constants.h"
#include "primitives.h"

bool vm_try_init(VirtualMachine *vm, VirtualMachine_Config config) {
    *vm = (VirtualMachine) {
            .allocator = allocator_make(config.allocator_config),
            .globals = OBJECT_NIL,
            .value = OBJECT_NIL,
            .error = OBJECT_NIL,
            .exprs = OBJECT_NIL,
    };

    allocator_set_roots(&vm->allocator, (ObjectAllocator_Roots) {
            .stack = &vm->stack,
            .globals = &vm->globals,
            .value = &vm->value,
            .error = &vm->error,
            .exprs = &vm->exprs,
    });

    errno_t error_code;
    auto const ok = stack_try_init(&vm->stack, config.stack_config, &error_code)
           && object_reader_try_init(&vm->reader, vm, config.reader_config, &error_code)
           && env_try_create(&vm->allocator, OBJECT_NIL, &vm->globals)
           && try_define_constants(&vm->allocator, vm->globals)
           && try_define_primitives(&vm->allocator, vm->globals);
    if (false == ok) {
        vm_free(vm);
    }

    return ok;
}

void vm_free(VirtualMachine *vm) {
    guard_is_not_null(vm);

    object_reader_free(&vm->reader);
    allocator_free(&vm->allocator);
    stack_free(&vm->stack);

    *vm = (VirtualMachine) {0};
}

ObjectAllocator *vm_allocator(VirtualMachine *vm) {
    guard_is_not_null(vm);

    return &vm->allocator;
}

Stack *vm_stack(VirtualMachine *vm) {
    guard_is_not_null(vm);

    return &vm->stack;
}

ObjectReader *vm_reader(VirtualMachine *vm) {
    guard_is_not_null(vm);

    return &vm->reader;
}

Object **vm_value(VirtualMachine *vm) {
    guard_is_not_null(vm);

    return &vm->value;
}

Object **vm_error(VirtualMachine *vm) {
    guard_is_not_null(vm);

    return &vm->error;
}

Object **vm_globals(VirtualMachine *vm) {
    guard_is_not_null(vm);

    return &vm->globals;
}

Object **vm_exprs(VirtualMachine *vm) {
    guard_is_not_null(vm);

    return &vm->exprs;
}
