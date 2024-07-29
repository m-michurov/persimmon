#pragma once

#include "object/allocator.h"
#include "vm/stack.h"

[[nodiscard]]
bool traceback_try_get(ObjectAllocator *a, Stack s, Object **traceback);

void traceback_print(Object *traceback, FILE *file);

void traceback_print_from_stack(Stack s, FILE *file);

