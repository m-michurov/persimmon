#include "object_constructors.h"

#include <string.h>
#include <stddef.h>

#include "guards.h"

size_t object_min_size_int(void) {
    return offsetof(Object, as_int) + sizeof(int64_t);
}

size_t object_min_size_string(size_t len) {
    return offsetof(Object, as_string) + len + 1;
}

size_t object_min_size_atom(size_t len) {
    return offsetof(Object, as_atom) + len + 1;
}

size_t object_min_size_cons(void) {
    return offsetof(Object, as_cons) + sizeof(Object_Cons);
}

size_t object_min_size_primitive(void) {
    return offsetof(Object, as_primitive) + sizeof(Object_Primitive);
}

size_t object_min_size_closure(void) {
    return offsetof(Object, as_closure) + sizeof(Object_Closure);
}

Object *object_init_int(Object *obj, int64_t value) {
    guard_is_not_null(obj);

    obj->type = TYPE_INT;
    obj->as_int = value;

    return obj;
}

Object *object_init_string(Object *obj, char const *s, size_t len) {
    guard_is_not_null(obj);
    guard_is_not_null(s);

    obj->type = TYPE_STRING;
    memcpy(obj->as_string, s, len + 1);

    return obj;
}

Object *object_init_atom(Object *obj, char const *s, size_t len) {
    guard_is_not_null(obj);
    guard_is_not_null(s);

    obj->type = TYPE_ATOM;
    memcpy(obj->as_string, s, len + 1);

    return obj;
}

Object *object_init_cons(Object *obj, Object *first, Object *rest) {
    guard_is_not_null(obj);
    guard_is_not_null(first);
    guard_is_not_null(rest);
    guard_is_one_of(rest->type, TYPE_CONS, TYPE_NIL);

    obj->type = TYPE_CONS;
    obj->as_cons = (Object_Cons) {.first = first, .rest = rest};

    return obj;
}

Object *object_init_primitive(Object *obj, Object_Primitive fn) {
    guard_is_not_null(obj);

    obj->type = TYPE_PRIMITIVE;
    obj->as_primitive = fn;

    return obj;
}

Object *object_init_closure(Object *obj, Object *env, Object *args, Object *body) {
    guard_is_not_null(obj);
    guard_is_not_null(env);
    guard_is_not_null(args);
    guard_is_not_null(body);

    obj->type = TYPE_CLOSURE;
    obj->as_closure = (Object_Closure) {
            .env = env,
            .args = args,
            .body = body
    };

    return obj;
}

Object *object_int(ObjectAllocator *a, int64_t value) {
    guard_is_not_null(a);

    Object *obj;
    return allocator_try_allocate(a, object_min_size_int(), &obj)
           ? object_init_int(obj, value)
           : nullptr;
}

Object *object_string(ObjectAllocator *a, char const *s) {
    guard_is_not_null(a);
    guard_is_not_null(s);

    auto const len = strlen(s);
    Object *obj;
    return allocator_try_allocate(a, object_min_size_string(len), &obj)
           ? object_init_string(obj, s, len)
           : nullptr;
}

Object *object_atom(ObjectAllocator *a, char const *s) {
    guard_is_not_null(a);
    guard_is_not_null(s);

    auto const len = strlen(s);
    Object *obj;
    return allocator_try_allocate(a, object_min_size_atom(len), &obj)
           ? object_init_atom(obj, s, len)
           : nullptr;
}

Object *object_cons(ObjectAllocator *a, Object *first, Object *rest) {
    guard_is_not_null(a);

    Object *obj;
    return allocator_try_allocate(a, object_min_size_cons(), &obj)
           ? object_init_cons(obj, first, rest)
           : nullptr;
}

Object *object_primitive(ObjectAllocator *a, Object_Primitive fn) {
    guard_is_not_null(a);

    Object *obj;
    return allocator_try_allocate(a, object_min_size_primitive(), &obj)
           ? object_init_primitive(obj, fn)
           : nullptr;
}

Object *object_closure(ObjectAllocator *a, Object *env, Object *args, Object *body) {
    guard_is_not_null(a);
    guard_is_not_null(env);
    guard_is_not_null(args);
    guard_is_not_null(body);

    Object *obj;
    return allocator_try_allocate(a, object_min_size_closure(), &obj)
           ? object_init_closure(obj, env, args, body)
           : nullptr;
}
