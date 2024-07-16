#include "virtual_machine.h"

#include "utility/guards.h"
#include "utility/slice.h"
#include "object/constructors.h"
#include "reader/reader.h"
#include "env.h"
#include "stack.h"
#include "constants.h"
#include "primitives.h"

struct VirtualMachine {
    Reader *reader;
    ObjectAllocator *allocator;
    Stack *stack;

    Object *globals;
    Object *value;
    Object *error;
    VM_ExpressionsStack expressions_stack;

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

static bool try_wrap_atom(ObjectAllocator *a, char const *name, Object **value) {
    guard_is_not_null(a);
    guard_is_not_null(name);
    guard_is_not_null(value);

    return object_try_make_cons(a, object_nil(), object_nil(), value)
           && object_try_make_atom(a, name, &(*value)->as_cons.first);
}

static bool try_init_static_constants(ObjectAllocator *a, Objects *constants) {
    guard_is_not_null(a);
    guard_is_not_null(constants);
    guard_is_greater_or_equal(constants->count, STATIC_CONSTANTS_COUNT);

    return object_try_make_atom(a, "true", slice_at(*constants, STATIC_TRUE))
           && (*slice_at(*constants, STATIC_FALSE) = object_nil())
           && object_try_make_atom(a, "do", slice_at(*constants, STATIC_ATOM_DO))
           && try_wrap_atom(a, "OSError", slice_at(*constants, STATIC_OS_ERROR_DEFAULT))
           && try_wrap_atom(a, "TypeError", slice_at(*constants, STATIC_TYPE_ERROR_DEFAULT))
           && try_wrap_atom(a, "CallError", slice_at(*constants, STATIC_CALL_ERROR_DEFAULT))
           && try_wrap_atom(a, "NameError", slice_at(*constants, STATIC_NAME_ERROR_DEFAULT))
           && try_wrap_atom(a, "ZeroDivisionError", slice_at(*constants, STATIC_ZERO_DIVISION_ERROR_DEFAULT))
           && try_wrap_atom(a, "OutOfMemoryError", slice_at(*constants, STATIC_OUT_OF_MEMORY_ERROR_DEFAULT))
           && try_wrap_atom(a, "StackOverflowError", slice_at(*constants, STATIC_STACK_OVERFLOW_ERROR_DEFAULT))
           && try_wrap_atom(a, "SyntaxError", slice_at(*constants, STATIC_SYNTAX_ERROR_DEFAULT))
           && try_wrap_atom(a, "SpecialFormError", slice_at(*constants, STATIC_SPECIAL_ERROR_DEFAULT))
           && try_wrap_atom(a, "ImportError", slice_at(*constants, STATIC_TOO_MANY_DEFAULT))
           && try_wrap_atom(a, "BindError", slice_at(*constants, STATIC_BIND_DEFAULT));
}

VirtualMachine *vm_new(VirtualMachine_Config config) {
    guard_is_greater(config.import_stack_size, 0);

    auto const vm = (VirtualMachine *) guard_succeeds(calloc, (1, sizeof(VirtualMachine)));

    auto const allocator = allocator_new(config.allocator_config);
    vm->allocator = allocator;

    auto const reader = reader_new(vm, config.reader_config);
    auto const stack = stack_new(config.stack_config);

    auto const constants = (Objects) {
            .data = (Object **) guard_succeeds(calloc, (STATIC_CONSTANTS_COUNT, sizeof(Object *))),
            .count = STATIC_CONSTANTS_COUNT,
    };
    slice_for(it, constants) {
        *it = object_nil();
    }

    memcpy(vm, &(VirtualMachine) {
            .allocator = allocator,
            .reader = reader,
            .stack = stack,
            .globals = object_nil(),
            .value = object_nil(),
            .error = object_nil(),
            .expressions_stack = {
                    .data = guard_succeeds(calloc, (config.import_stack_size, sizeof(Object *))),
                    .capacity = config.import_stack_size
            },
            .constants = constants
    }, sizeof(VirtualMachine));

    allocator_set_roots(allocator, (ObjectAllocator_Roots) {
            .stack = vm->stack,
            .parser_stack = reader_parser_stack(vm->reader),
            .parser_expr = reader_parser_expr(vm->reader),
            .globals = &vm->globals,
            .value = &vm->value,
            .error = &vm->error,
            .vm_expressions_stack = &vm->expressions_stack,
            .constants = &vm->constants
    });

    guard_is_true(try_init_static_constants(allocator, &vm->constants));

    guard_is_true(env_try_create(vm->allocator, object_nil(), &vm->globals));
    guard_is_true(try_define_constants(vm, vm->globals));
    guard_is_true(try_define_primitives(vm->allocator, vm->globals));

    return vm;
}

void vm_free(VirtualMachine **vm) {
    guard_is_not_null(vm);

    reader_free(&(*vm)->reader);
    allocator_free(&(*vm)->allocator);
    stack_free(&(*vm)->stack);

    free(*vm);
    *vm = nullptr;
}

ObjectAllocator *vm_allocator(VirtualMachine *vm) {
    guard_is_not_null(vm);

    return vm->allocator;
}

Object **vm_value(VirtualMachine *vm) {
    guard_is_not_null(vm);

    return &vm->value;
}

Object **vm_error(VirtualMachine *vm) {
    guard_is_not_null(vm);

    return &vm->error;
}

Stack *vm_stack(VirtualMachine *vm) {
    guard_is_not_null(vm);

    return vm->stack;
}

Reader *vm_reader(VirtualMachine *vm) {
    guard_is_not_null(vm);

    return vm->reader;
}

Object **vm_globals(VirtualMachine *vm) {
    guard_is_not_null(vm);

    return &vm->globals;
}

VM_ExpressionsStack *vm_expressions_stack(VirtualMachine *vm) {
    guard_is_not_null(vm);

    return &vm->expressions_stack;
}

Object *vm_get(VirtualMachine const *vm, StaticConstantName name) {
    guard_is_not_null(vm);

    auto const value = *slice_at(vm->constants, name);
    guard_is_not_null(value);
    return value;
}
