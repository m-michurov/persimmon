#include "constructors.h"

#include <string.h>
#include <stddef.h>

#include "utility/guards.h"
#include "utility/pointers.h"
#include "utility/slice.h"

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

static size_t size_dict_entries(size_t count) {
    guard_is_greater(count, 0);

    return offsetof(Object, as_dict_entries)
           + sizeof(Object_DictEntries)
           + count * sizeof(Object_DictEntry);
}

static size_t size_dict(void) {
    return offsetof(Object, as_dict) + sizeof(Object_Dict);
}

static void init_int(Object *obj, int64_t value) {
    guard_is_not_null(obj);

    obj->type = TYPE_INT;
    obj->as_int = value;
}

static void init_string(Object *obj, char const *s, size_t len) {
    guard_is_not_null(obj);
    guard_is_not_null(s);

    obj->type = TYPE_STRING;
    memcpy(obj->as_string, s, len + 1);
}

static void init_atom(Object *obj, char const *s, size_t len) {
    guard_is_not_null(obj);
    guard_is_not_null(s);

    obj->type = TYPE_ATOM;
    memcpy(obj->as_string, s, len + 1);
}

static void init_cons(Object *obj, Object *first, Object *rest) {
    guard_is_not_null(obj);
    guard_is_not_null(first);
    guard_is_not_null(rest);
    if (TYPE_CONS != rest->type && TYPE_NIL != rest->type) {
        printf("%d\n", rest->type);
    }
    guard_is_one_of(rest->type, TYPE_CONS, TYPE_NIL);

    obj->type = TYPE_CONS;
    obj->as_cons = (Object_Cons) {.first = first, .rest = rest};
}

static void init_primitive(Object *obj, Object_Primitive fn) {
    guard_is_not_null(obj);

    obj->type = TYPE_PRIMITIVE;
    obj->as_primitive = fn;
}

static void init_closure(Object *obj, Object *env, Object *args, Object *body) {
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
}

static void init_macro(Object *obj, Object *env, Object *args, Object *body) {
    guard_is_not_null(obj);
    guard_is_not_null(env);
    guard_is_not_null(args);
    guard_is_not_null(body);

    obj->type = TYPE_MACRO;
    obj->as_closure = (Object_Closure) {
            .env = env,
            .args = args,
            .body = body
    };
}

static void init_dict_entries(Object *obj, size_t count) {
    guard_is_not_null(obj);
    guard_is_greater(count, 0);

    obj->type = TYPE_DICT_ENTRIES;
    obj->as_dict_entries = (Object_DictEntries) {
            .count = count,
//            .data = obj->as_dict_entries._data
    };

    slice_ptr_for(entry, &obj->as_dict_entries) {
        *entry = (Object_DictEntry) {
                .key = object_nil(),
                .value = object_nil()
        };
    }
}

static void init_dict(Object *obj, Object *entries) {
    guard_is_not_null(obj);
    guard_is_not_null(entries);
    guard_is_equal(entries->type, TYPE_DICT_ENTRIES);

    obj->type = TYPE_DICT;
    obj->as_dict = (Object_Dict) {
            .entries = entries,
            .new_entries = object_nil()
    };
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

bool object_try_make_macro(ObjectAllocator *a, Object *env, Object *args, Object *body, Object **obj) {
    guard_is_not_null(a);
    guard_is_not_null(env);
    guard_is_not_null(args);
    guard_is_not_null(body);
    guard_is_not_null(obj);

    if (false == allocator_try_allocate(a, size_closure(), obj)) {
        return false;
    }

    init_macro(*obj, env, args, body);
    return true;
}

bool object_try_make_dict_entries(ObjectAllocator *a, size_t count, Object **obj) {
    guard_is_not_null(a);
    guard_is_not_null(obj);
    guard_is_greater(count, 0);

    if (false == allocator_try_allocate(a, size_dict_entries(count), obj)) {
        return false;
    }

    init_dict_entries(*obj, count);
    return true;
}

bool object_try_make_dict(ObjectAllocator *a, Object *entries, Object **obj) {
    guard_is_not_null(a);
    guard_is_not_null(entries);
    guard_is_not_null(obj);
    guard_is_equal(entries->type, TYPE_DICT_ENTRIES);

    if (false == allocator_try_allocate(a, size_dict(), obj)) {
        return false;
    }

    init_dict(*obj, entries);
    return true;
}

bool object_try_copy(ObjectAllocator *a, Object *obj, Object **copy) { // NOLINT(*-no-recursion)
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
            return object_try_make_cons(a, object_nil(), object_nil(), copy)
                   && object_try_copy(a, obj->as_cons.first, &(*copy)->as_cons.first)
                   && object_try_copy(a, obj->as_cons.rest, &(*copy)->as_cons.rest);
        }
        case TYPE_DICT_ENTRIES: {
            if (false == object_try_make_dict_entries(a, obj->as_dict_entries.count, copy)) {
                return false;
            }

            memcpy(*copy, obj, size_dict_entries(obj->as_dict_entries.count));
            return true;
        }
        case TYPE_DICT: {
            return object_try_make_dict(a, obj->as_dict.entries, copy)
                   && object_try_copy(a, obj->as_dict.entries, &(*copy)->as_dict.entries);
        }
        case TYPE_PRIMITIVE: {
            return object_try_make_primitive(a, obj->as_primitive, copy);
        }
        case TYPE_CLOSURE: {
            return object_try_make_closure(a, obj->as_closure.env, object_nil(), object_nil(), copy)
                   && object_try_copy(a, obj->as_closure.args, &(*copy)->as_closure.args)
                   && object_try_copy(a, obj->as_closure.body, &(*copy)->as_closure.body);
        }
        case TYPE_MACRO: {
            return object_try_make_macro(a, obj->as_closure.env, object_nil(), object_nil(), copy)
                   && object_try_copy(a, obj->as_closure.args, &(*copy)->as_closure.args)
                   && object_try_copy(a, obj->as_closure.body, &(*copy)->as_closure.body);
        }
        case TYPE_NIL: {
            *copy = obj;
            return true;
        }
    }

    guard_unreachable();
}
