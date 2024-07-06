#pragma once

#include "object/object.h"

void print_type_error_(Object_Type got, size_t expected_count, Object_Type *expected);

#define print_type_error(Got, ...) print_type_error_(                   \
    (Got),                                                              \
    (sizeof(((Object_Type[]) {__VA_ARGS__})) / sizeof(Object_Type)),    \
    ((Object_Type[]) {__VA_ARGS__})                                     \
)

void print_args_error(char const *name, size_t got, size_t expected);

void print_name_error(char const *name);

void print_stack_overflow_error(void);

void print_zero_division_error(void);

void print_out_of_memory_error(struct ObjectAllocator *a);
