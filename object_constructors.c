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
    guard_is_greater_or_equal(obj->size, object_min_size_int());

    obj->type = TYPE_INT;
    obj->as_int = value;

    return obj;
}

Object *object_init_string(Object *obj, char const *s, size_t len) {
    guard_is_not_null(obj);
    guard_is_not_null(s);
    guard_is_greater_or_equal(obj->size, object_min_size_string(len));

    obj->type = TYPE_STRING;
    memcpy(obj->as_string, s, len + 1);

    return obj;
}

Object *object_init_atom(Object *obj, char const *s, size_t len) {
    guard_is_not_null(obj);
    guard_is_not_null(s);
    guard_is_greater_or_equal(obj->size, object_min_size_atom(len));

    obj->type = TYPE_ATOM;
    memcpy(obj->as_string, s, len + 1);

    return obj;
}

Object *object_init_cons(Object *obj, Object *first, Object *rest) {
    guard_is_not_null(obj);
    guard_is_not_null(first);
    guard_is_not_null(rest);
    guard_is_one_of(rest->type, TYPE_CONS, TYPE_NIL);
    guard_is_greater_or_equal(obj->size, object_min_size_cons());

    obj->type = TYPE_CONS;
    obj->as_cons = (Object_Cons) {.first = first, .rest = rest};

    return obj;
}

Object *object_init_primitive(Object *obj, Object_Primitive fn) {
    guard_is_not_null(obj);
    guard_is_greater_or_equal(obj->size, object_min_size_primitive());

    obj->type = TYPE_PRIMITIVE;
    obj->as_primitive = fn;

    return obj;
}

Object *object_init_closure(Object *obj, Object *env, Object *args, Object *body) {
    guard_is_not_null(obj);
    guard_is_not_null(env);
    guard_is_not_null(args);
    guard_is_not_null(body);
    guard_is_greater_or_equal(obj->size, object_min_size_closure());

    obj->type = TYPE_CLOSURE;
    obj->as_closure = (Object_Closure) {
            .env = env,
            .args = args,
            .body = body
    };

    return obj;
}

Object *object_int(Object_Allocator *a, int64_t value) {
    guard_is_not_null(a);

    return object_init_int(object_allocate(a, object_min_size_int()), value);
}

Object *object_string(Object_Allocator *a, char const *s) {
    guard_is_not_null(a);
    guard_is_not_null(s);

    auto const len = strlen(s);
    return object_init_string(object_allocate(a, object_min_size_string(len)), s, len);
}

Object *object_atom(Object_Allocator *a, char const *s) {
    guard_is_not_null(a);
    guard_is_not_null(s);

    auto const len = strlen(s);
    return object_init_atom(object_allocate(a, object_min_size_atom(len)), s, len);
}

Object *object_cons(Object_Allocator *a, Object *first, Object *rest) {
    guard_is_not_null(a);

    return object_init_cons(object_allocate(a, object_min_size_cons()), first, rest);
}

Object *object_primitive(Object_Allocator *a, Object_Primitive fn) {
    guard_is_not_null(a);

    return object_init_primitive(object_allocate(a, object_min_size_primitive()), fn);
}

Object *object_closure(Object_Allocator *a, Object *env, Object *args, Object *body) {
    guard_is_not_null(a);
    guard_is_not_null(env);
    guard_is_not_null(args);
    guard_is_not_null(body);

    return object_init_closure(object_allocate(a, object_min_size_closure()), env, args, body);
}
