#include "object_allocator.h"

#include "guards.h"
#include "stack.h"
#include "parser.h"
#include "slice.h"
#include "dynamic_array.h"
#include "exchange.h"

struct ObjectAllocator {
    bool gc_is_running;
    Object *objects;
    Parser_Stack *parser_stack;
    Stack *stack;
    size_t hard_limit;
    size_t soft_limit;
    size_t heap_size;
};

ObjectAllocator *allocator_new(void) {
    auto const allocator = (ObjectAllocator *) guard_succeeds(calloc, (1, sizeof(ObjectAllocator)));
    *allocator = (ObjectAllocator) {
            .soft_limit = 1024,
            .hard_limit = 1024 * 1024 * 1024
    };
    return allocator;
}

void allocator_free(ObjectAllocator **a) {
    guard_is_not_null(a);
    guard_is_not_null(*a);

    for (auto it = (*a)->objects; nullptr != it;) {
        auto const next = it->next;
        free(it);
        it = next;
    }

    free(*a);
    *a = nullptr;
}

static void mark_gray(Objects *gray, Object *obj) {
    guard_is_equal(obj->color, OBJECT_WHITE);

    da_append(gray, obj);
    obj->color = OBJECT_GRAY;
}

static void mark_gray_if_white(Objects *gray, Object *obj) {
    if (OBJECT_WHITE != obj->color) {
        return;
    }

    mark_gray(gray, obj);
}

static void mark_black(Object *obj) {
    guard_is_equal(obj->color, OBJECT_GRAY);
    obj->color = OBJECT_BLACK;
}

static void mark_children(Objects *gray, Object *obj) {
    guard_is_not_null(gray);
    guard_is_not_null(obj);
    guard_is_equal(obj->color, OBJECT_GRAY);

    switch (obj->type) {
        case TYPE_INT:
        case TYPE_STRING:
        case TYPE_ATOM:
        case TYPE_NIL:
        case TYPE_PRIMITIVE: {
            mark_black(obj);
            return;
        }
        case TYPE_CONS: {
            mark_gray_if_white(gray, obj->as_cons.first);
            mark_gray_if_white(gray, obj->as_cons.rest);

            mark_black(obj);
            return;
        }
        case TYPE_CLOSURE: {
            mark_gray_if_white(gray, obj->as_closure.args);
            mark_gray_if_white(gray, obj->as_closure.env);
            mark_gray_if_white(gray, obj->as_closure.body);

            mark_black(obj);
            return;
        }
    }

    guard_unreachable();
}

static void mark(ObjectAllocator *a) {
    guard_is_not_null(a);

    auto gray = (Objects) {0};
    // TODO mark data stack roots
    // TODO mark parser stack roots

    for (auto it = a->objects; nullptr != it; it = it->next) {
        guard_is_equal(it->color, OBJECT_WHITE);
        mark_gray(&gray, it);
    }


    Object *obj;
    while (false == slice_empty(gray) && slice_try_pop(&gray, &obj)) {
        mark_children(&gray, obj);
    }

    guard_is_true(slice_empty(gray));
    for (auto it = a->objects; nullptr != it; it = it->next) {
        guard_is_equal(it->color, OBJECT_BLACK);
    }

    da_free(&gray);
}

static void sweep(ObjectAllocator *a) {
    guard_is_not_null(a);

    Object *prev = nullptr;
    for (auto it = a->objects; nullptr != it;) {
        if (OBJECT_BLACK == it->color) {
            it->color = OBJECT_WHITE;
            prev = exchange(it, it->next);
            continue;
        }

        guard_unreachable();

        auto const unreached = exchange(it, it->next);
        guard_is_equal(unreached->color, OBJECT_WHITE);

        if (nullptr == prev) {
            a->objects = it;
        } else {
            prev->next = it;
        }

        free(unreached);
    }
}

static void collect_garbage(ObjectAllocator *a) {
    guard_is_not_null(a);
    guard_is_false(a->gc_is_running);

    a->gc_is_running = true;
    mark(a);
    sweep(a);
    a->gc_is_running = false;
}

#define min(A, B) ({ typeof(A) const _a = (A); typeof(A) const _b = (B); _a < _b ? _a : _b; })

static void adjust_soft_limit(ObjectAllocator *a, size_t size) {
    guard_is_not_null(a);

    a->soft_limit = min(size + a->soft_limit * 4 / 3, a->hard_limit);
}

bool allocator_try_allocate(ObjectAllocator *a, size_t size, Object **obj) {
    guard_is_not_null(a);
    guard_is_greater(size, 0);

    if (a->heap_size + size >= a->soft_limit) {
        collect_garbage(a);
        adjust_soft_limit(a, size);
    }

    if (a->heap_size + size >= a->hard_limit) {
        guard_unreachable();
        return false;
    }

    auto const new_obj = (Object *) guard_succeeds(calloc, (size, sizeof(uint8_t)));
    new_obj->next = exchange(a->objects, new_obj);
    a->heap_size += size;
    *obj = new_obj;

    return true;
}
