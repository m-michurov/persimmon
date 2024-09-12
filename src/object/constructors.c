#include "constructors.h"

#include <string.h>
#include <stddef.h>

#include "utility/guards.h"
#include "utility/pointers.h"
#include "utility/math.h"
#include "dict.h"

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

static size_t size_dict(void) {
    return offsetof(Object, as_dict) + sizeof(Object_Dict);
}

bool object_try_make_int(ObjectAllocator *a, int64_t value, Object **obj) {
    guard_is_not_null(a);
    guard_is_not_null(obj);

    if (false == allocator_try_allocate(a, size_int(), obj)) {
        return false;
    }

    (*obj)->type = TYPE_INT;
    (*obj)->as_int = value;
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

    (*obj)->type = TYPE_STRING;
    memcpy((void *) (*obj)->as_string, s, len + 1);
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

    (*obj)->type = TYPE_ATOM;
    memcpy((void *) (*obj)->as_atom, s, len + 1);
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

    (*obj)->type = TYPE_CONS;
    (*obj)->as_cons = ((Object_Cons) {
            .first = first,
            .rest = rest
    });
    return true;
}

bool object_try_make_primitive(ObjectAllocator *a, Object_Primitive fn, Object **obj) {
    guard_is_not_null(a);
    guard_is_not_null(obj);

    if (false == allocator_try_allocate(a, size_primitive(), obj)) {
        return false;
    }

    (*obj)->type = TYPE_PRIMITIVE;
    (*obj)->as_primitive = fn;
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

    (*obj)->type = TYPE_CLOSURE;
    (*obj)->as_closure = ((Object_Closure) {
            .env = env,
            .args = args,
            .body = body
    });
    return true;
}

bool object_try_make_macro(ObjectAllocator *a, Object *env, Object *args, Object *body, Object **obj) {
    guard_is_not_null(a);
    guard_is_not_null(env);
    guard_is_not_null(args);
    guard_is_not_null(body);
    guard_is_not_null(obj);

    if (false == allocator_try_allocate(a, size_closure(), obj)) {
        return false;
    }

    (*obj)->type = TYPE_MACRO;
    (*obj)->as_closure = ((Object_Closure) {
            .env = env,
            .args = args,
            .body = body
    });
    return true;
}

bool object_try_make_dict(ObjectAllocator *a, Object *key, Object *value, Object *left, Object *right, Object **obj) {
    guard_is_not_null(a);
    guard_is_not_null(key);
    guard_is_not_null(value);
    guard_is_not_null(left);
    guard_is_not_null(right);
    guard_is_not_null(obj);
    guard_is_one_of(left->type, TYPE_NIL, TYPE_DICT);
    guard_is_one_of(right->type, TYPE_NIL, TYPE_DICT);

    if (false == allocator_try_allocate(a, size_dict(), obj)) {
        return false;
    }

    (*obj)->type = TYPE_DICT;
    (*obj)->as_dict = ((Object_Dict) {
            .key = key,
            .value = value,
            .height = 1 + max(object_dict_height(left), object_dict_height(right)),
            .left = left,
            .right = right
    });
    return true;
}

static bool try_deep_copy_in_place(ObjectAllocator *a, Object *const *dst) { // NOLINT(*-no-recursion)
    guard_is_not_null(a);
    guard_is_not_null(dst);
    guard_is_not_null(*dst);

    return object_try_deep_copy(a, *dst, (Object **) dst);
}

bool object_try_deep_copy(ObjectAllocator *a, Object *obj, Object **copy) { // NOLINT(*-no-recursion)
    guard_is_not_null(a);
    guard_is_not_null(obj);
    guard_is_not_null(copy);

    switch (obj->type) {
        case TYPE_INT:
        case TYPE_STRING:
        case TYPE_ATOM:
        case TYPE_PRIMITIVE:
        case TYPE_NIL: {
            return object_try_shallow_copy(a, obj, copy);
        }
        case TYPE_CONS: {
            return object_try_shallow_copy(a, obj, copy)
                   && try_deep_copy_in_place(a, &(*copy)->as_cons.first)
                   && try_deep_copy_in_place(a, &(*copy)->as_cons.rest);
        }
        case TYPE_DICT: {
            return object_try_shallow_copy(a, obj, copy)
                   && try_deep_copy_in_place(a, &(*copy)->as_dict.left)
                   && try_deep_copy_in_place(a, &(*copy)->as_dict.right);
        }
        case TYPE_CLOSURE:
        case TYPE_MACRO: {
            return object_try_shallow_copy(a, obj, copy)
                   && try_deep_copy_in_place(a, &(*copy)->as_closure.args)
                   && try_deep_copy_in_place(a, &(*copy)->as_closure.body);
        }
    }

    guard_unreachable();
}

bool object_try_shallow_copy(ObjectAllocator *a, Object *obj, Object **copy) {
    guard_is_not_null(a);
    guard_is_not_null(obj);
    guard_is_not_null(copy);

    switch (obj->type) {
        case TYPE_INT: {
            return object_try_make_int(a, obj->as_int, copy);
        }
        case TYPE_STRING: {
            return object_try_make_string(a, obj->as_string, copy);
        }
        case TYPE_ATOM: {
            return object_try_make_atom(a, obj->as_atom, copy);
        }
        case TYPE_CONS: {
            return object_try_make_cons(a, obj->as_cons.first, obj->as_cons.rest, copy);
        }
        case TYPE_DICT: {
            return object_try_make_dict(
                    a,
                    obj->as_dict.key, obj->as_dict.value,
                    obj->as_dict.left, obj->as_dict.right,
                    copy
            );
        }
        case TYPE_PRIMITIVE: {
            return object_try_make_primitive(a, obj->as_primitive, copy);
        }
        case TYPE_CLOSURE: {
            return object_try_make_closure(a, obj->as_closure.env, obj->as_closure.args, obj->as_closure.body, copy);
        }
        case TYPE_MACRO: {
            return object_try_make_macro(a, obj->as_closure.env, obj->as_closure.args, obj->as_closure.body, copy);
        }
        case TYPE_NIL: {
            *copy = obj;
            return true;
        }
    }

    guard_unreachable();
}
