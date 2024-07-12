#include "constructors.h"

#include <string.h>
#include <stddef.h>

#include "utility/guards.h"

static size_t size_int(void) {
    return offsetof(Object, as_int) + sizeof(int64_t);
}

static size_t size_string(size_t len) {
    return offsetof(Object, as_string) + len + 1;
}

static size_t size_atom(size_t len) {
    return offsetof(Object, as_atom) + len + 1;
}

static size_t size_cons(void) {
    return offsetof(Object, as_cons) + sizeof(Object_Cons);
}

static size_t size_primitive(void) {
    return offsetof(Object, as_primitive) + sizeof(Object_Primitive);
}

static size_t size_closure(void) {
    return offsetof(Object, as_closure) + sizeof(Object_Closure);
}

static Object *init_int(Object *obj, int64_t value) {
    guard_is_not_null(obj);

    obj->type = TYPE_INT;
    obj->as_int = value;

    return obj;
}

static Object *init_string(Object *obj, char const *s, size_t len) {
    guard_is_not_null(obj);
    guard_is_not_null(s);

    obj->type = TYPE_STRING;
    memcpy(obj->as_string, s, len + 1);

    return obj;
}

static Object *init_atom(Object *obj, char const *s, size_t len) {
    guard_is_not_null(obj);
    guard_is_not_null(s);

    obj->type = TYPE_ATOM;
    memcpy(obj->as_string, s, len + 1);

    return obj;
}

static Object *init_cons(Object *obj, Object *first, Object *rest) {
    guard_is_not_null(obj);
    guard_is_not_null(first);
    guard_is_not_null(rest);
    guard_is_one_of(rest->type, TYPE_CONS, TYPE_NIL);

    obj->type = TYPE_CONS;
    obj->as_cons = (Object_Cons) {.first = first, .rest = rest};

    return obj;
}

static Object *init_primitive(Object *obj, Object_Primitive fn) {
    guard_is_not_null(obj);

    obj->type = TYPE_PRIMITIVE;
    obj->as_primitive = fn;

    return obj;
}

static Object *init_closure(Object *obj, Object *env, Object *args, Object *body) {
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

bool object_try_make_int(ObjectAllocator *a, int64_t value, Object **obj) {
    guard_is_not_null(a);
    guard_is_not_null(obj);

    if (false == allocator_try_allocate(a, size_int(), obj)) {
        return false;
    }

    init_int(*obj, value);
    return true;
}

bool object_try_make_string(ObjectAllocator *a, char const *s, Object **obj) {
    guard_is_not_null(a);
    guard_is_not_null(s);
    guard_is_not_null(obj);

    auto const len = strlen(s);
    if (false == allocator_try_allocate(a, size_string(len), obj)) {
        return false;
    }

    init_string(*obj, s, len);
    return true;
}

bool object_try_make_atom(ObjectAllocator *a, char const *s, Object **obj) {
    guard_is_not_null(a);
    guard_is_not_null(s);
    guard_is_not_null(obj);

    auto const len = strlen(s);
    if (false == allocator_try_allocate(a, size_atom(len), obj)) {
        return false;
    }

    init_atom(*obj, s, len);
    return true;
}

bool object_try_make_cons(ObjectAllocator *a, Object *first, Object *rest, Object **obj) {
    guard_is_not_null(a);
    guard_is_not_null(first);
    guard_is_not_null(rest);
    guard_is_not_null(obj);

    if (false == allocator_try_allocate(a, size_cons(), obj)) {
        return false;
    }

    init_cons(*obj, first, rest);
    return true;
}

bool object_try_make_primitive(ObjectAllocator *a, Object_Primitive fn, Object **obj) {
    guard_is_not_null(a);
    guard_is_not_null(obj);

    if (false == allocator_try_allocate(a, size_primitive(), obj)) {
        return false;
    }

    init_primitive(*obj, fn);
    return true;
}

bool object_try_make_closure(ObjectAllocator *a, Object *env, Object *args, Object *body, Object **obj) {
    guard_is_not_null(a);
    guard_is_not_null(env);
    guard_is_not_null(args);
    guard_is_not_null(body);
    guard_is_not_null(obj);

    if (false == allocator_try_allocate(a, size_closure(), obj)) {
        return false;
    }

    init_closure(*obj, env, args, body);
    return true;
}
