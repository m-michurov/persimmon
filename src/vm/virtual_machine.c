#include "virtual_machine.h"

#include "reader/reader.h"
#include "utility/guards.h"
#include "utility/dynamic_array.h"
#include "utility/slice.h"
#include "object/constructors_unchecked.h"
#include "stack.h"

struct VirtualMachine {
    Reader *reader;
    ObjectAllocator *allocator;
    Stack *stack;
    Objects temporaries;

    Objects constants;
};

//    STATIC_NIL = 0,
//    STATIC_TRUE,
//    STATIC_FALSE,
//    STATIC_IO_ERROR,
//    STATIC_TYPE_ERROR,
//    STATIC_CALL_ERROR,
//    STATIC_NAME_ERROR,
//    STATIC_ZERO_DIVISION_ERROR,
//    STATIC_OUT_OF_MEMORY_ERROR,
//    STATIC_STACK_OVERFLOW_ERROR,
//    STATIC_SYNTAX_ERROR,

static void init_constants(ObjectAllocator *a, Objects *constants) {
    guard_is_not_null(a);
    guard_is_not_null(constants);
    guard_is_greater_or_equal(constants->count, STATIC_CONSTANTS_COUNT);

    *slice_at(*constants, STATIC_NIL) = object_nil();
    *slice_at(*constants, STATIC_TRUE) = object_atom(a, "true");
    *slice_at(*constants, STATIC_FALSE) = object_atom(a, "false");
    *slice_at(*constants, STATIC_IO_ERROR) = object_atom(a, "");
    *slice_at(*constants, STATIC_TYPE_ERROR) = object_atom(a, "TypeError");
    *slice_at(*constants, STATIC_CALL_ERROR) = object_atom(a, "CallError");
    *slice_at(*constants, STATIC_NAME_ERROR) = object_atom(a, "NameError");
    *slice_at(*constants, STATIC_ZERO_DIVISION_ERROR) = object_atom(a, "ZeroDivisionError");
    *slice_at(*constants, STATIC_OUT_OF_MEMORY_ERROR) = object_atom(a, "OutOfMemoryError");
    *slice_at(*constants, STATIC_STACK_OVERFLOW_ERROR) = object_atom(a, "StackOverflowError");
    *slice_at(*constants, STATIC_SYNTAX_ERROR) = object_atom(a, "SyntaxError");
}

VirtualMachine *vm_new(VirtualMachine_Config config) {
    auto const vm = (VirtualMachine *) guard_succeeds(calloc, (1, sizeof(VirtualMachine)));

    auto const allocator = allocator_new(config.allocator_config);
    auto const reader = reader_new(allocator);
    auto const stack = stack_new(config.stack_size);

    auto const constants = (Objects) {
            .data = (Object **) guard_succeeds(calloc, (STATIC_CONSTANTS_COUNT, sizeof(Object *))),
            .count = STATIC_CONSTANTS_COUNT,
    };

    *vm = (VirtualMachine) {
            .allocator = allocator,
            .reader = reader,
            .stack = stack,
            .constants = constants
    };

    allocator_set_roots(allocator, (ObjectAllocator_Roots) {
            .temporaries = &vm->temporaries,
            .stack = vm->stack,
            .parser_stack = reader_parser_stack(vm->reader),
            .parser_expr = reader_parser_expr(vm->reader)
    });

    init_constants(allocator, &vm->constants);

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

Object *vm_get(VirtualMachine *vm, StaticConstantName name) {
    guard_is_not_null(vm);

    return *slice_at(vm->constants, name);
}
