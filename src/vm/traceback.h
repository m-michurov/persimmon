#pragma once

#include "object/allocator.h"
#include "vm/stack.h"

bool traceback_try_get(ObjectAllocator *a, Stack const *s, Object **traceback);

void traceback_print(Object *traceback, FILE *file);

void traceback_print_from_stack(Stack const *s, FILE *file);

