#include "eval_errors.h"

#include <stdio.h>

#include "utility/guards.h"
#include "object/allocator.h"

void print_type_error_(Object_Type got, size_t expected_count, Object_Type *expected) {
    guard_is_greater(expected_count, 0);

    if (1 == expected_count) {
        printf("TypeError: expected %s but got %s\n", object_type_str(expected[0]), object_type_str(got));
        return;
    }

    printf("TypeError: expected %s", object_type_str(expected[0]));
    for (size_t i = 1; i + 1 < expected_count; i++) {
        printf(", %s", object_type_str(expected[i]));
    }
    printf(" or %s but got %s\n", object_type_str(expected[expected_count - 1]), object_type_str(got));
}

void print_args_error(char const *name, size_t got, size_t expected) {
    printf("TypeError: %s takes %zu arguments but %zu were given\n", name, expected, got);
}

void print_name_error(char const *name) {
    printf("NameError: %s\n", name);
}

void print_stack_overflow_error(void) {
    printf("StackOverflowError: stack capacity exceeded\n");
}

void print_zero_division_error(void) {
    printf("ZeroDivisionError: division by zero\n");
}

void print_out_of_memory_error(ObjectAllocator *a) {
    printf("OutOfMemoryError: cannot allocate more dynamic memory\n");
    allocator_print_statistics(a);
}
