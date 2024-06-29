#include "object_allocator.h"

#include "guards.h"

void object_allocator_free(Object_Allocator **a) {
    guard_is_not_null(a);
    guard_is_not_null(*a);

    (*a)->methods->free(*a);
    *a = nullptr;
}

Object *object_allocate(Object_Allocator *a, size_t size) {
    guard_is_not_null(a);
    guard_is_greater(size, 0);

    auto const obj = a->methods->allocate(a, size);
    // TODO maybe these checks are not needed; you don't check the return value of an implemented method
    guard_is_not_null(obj);
    guard_is_equal(obj->size, size);

    return obj;
}
