#include "pointers.h"

void *pointer_roundup(void *p, size_t alignment) {
    return (void *) (((((uintptr_t) p) + alignment - 1) / alignment) * alignment);
}
