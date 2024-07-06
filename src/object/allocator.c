#include "allocator.h"

#include "utility/guards.h"
#include "vm/reader/parser.h"
#include "utility/slice.h"
#include "utility/dynamic_array.h"
#include "utility/exchange.h"
#include "utility/pointers.h"

struct ObjectAllocator {
    Object *objects;

    ObjectAllocator_Roots roots;

    bool gc_is_running;
    size_t heap_size;
    size_t hard_limit;
    size_t soft_limit;
    double grow_factor;
};

ObjectAllocator *allocator_new(ObjectAllocator_Config config) {
    auto const allocator = (ObjectAllocator *) guard_succeeds(calloc, (1, sizeof(ObjectAllocator)));
    *allocator = (ObjectAllocator) {
            .soft_limit = config.soft_limit_initial,
            .hard_limit = config.hard_limit,
            .grow_factor = config.soft_limit_grow_factor
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

void allocator_set_roots(ObjectAllocator *a, ObjectAllocator_Roots roots) {
    guard_is_not_null(a);

    a->roots.stack = pointer_first_nonnull(a->roots.stack, roots.stack);
    a->roots.parser_stack = pointer_first_nonnull(a->roots.parser_stack, roots.parser_stack);
    a->roots.parser_expr = pointer_first_nonnull(a->roots.parser_expr, roots.parser_expr);
    a->roots.constants = pointer_first_nonnull(a->roots.constants, roots.constants);
    a->roots.temporaries = pointer_first_nonnull(a->roots.temporaries, roots.temporaries);
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

static void adjust_soft_limit(ObjectAllocator *a, size_t size) {
    guard_is_not_null(a);

    a->soft_limit = min(size + a->soft_limit * a->grow_factor, a->hard_limit);
}

static bool all_roots_set(ObjectAllocator const *a) {
    return nullptr != a->roots.stack
           && nullptr != a->roots.parser_stack
           && nullptr != a->roots.parser_expr
           && nullptr != a->roots.temporaries;
}

bool allocator_try_allocate(ObjectAllocator *a, size_t size, Object **obj) {
    guard_is_not_null(a);
    guard_is_greater(size, 0);
    guard_is_true(all_roots_set(a));

    if (a->heap_size + size >= a->soft_limit) {
        collect_garbage(a);
        adjust_soft_limit(a, size);
    }

    if (a->heap_size + size >= a->hard_limit) {
        return false;
    }

    auto const new_obj = (Object *) guard_succeeds(calloc, (size, 1));
    new_obj->next = exchange(a->objects, new_obj);
    a->heap_size += size;
    *obj = new_obj;

    return true;
}

void allocator_print_statistics(ObjectAllocator const *a) {
    size_t objects = 0;
    for (auto it = a->objects; nullptr != it; it = it->next) {
        objects++;
    }

    printf("Heap usage:\n");
    printf("          Objects: %zu\n", objects);
    printf("        Heap size: %zu bytes\n", a->heap_size);
    printf("  Heap size limit: %zu bytes\n", a->hard_limit);
}
