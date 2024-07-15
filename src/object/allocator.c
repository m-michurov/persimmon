#include "allocator.h"

#include "utility/guards.h"
#include "utility/slice.h"
#include "utility/dynamic_array.h"
#include "utility/exchange.h"
#include "utility/pointers.h"
#include "vm/virtual_machine.h"
#include "vm/stack.h"
#include "vm/reader/parser.h"

struct ObjectAllocator {
    Object *objects;
    Object *freed;

    ObjectAllocator_Roots roots;

    ObjectAllocator_GarbageCollectionMode gc_mode;
    bool trace;
    bool no_free;
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
            .grow_factor = config.soft_limit_grow_factor,
            .gc_mode = config.debug.gc_mode,
            .trace = config.debug.trace,
            .no_free = config.debug.no_free
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

#define update_root(Dst, Src, Member) ((Dst).Member = pointer_first_nonnull((Dst).Member, (Src).Member))

void allocator_set_roots(ObjectAllocator *a, ObjectAllocator_Roots roots) {
    guard_is_not_null(a);

    update_root(a->roots, roots, stack);
    update_root(a->roots, roots, parser_stack);
    update_root(a->roots, roots, parser_expr);
    update_root(a->roots, roots, globals);
    update_root(a->roots, roots, value);
    update_root(a->roots, roots, error);
    update_root(a->roots, roots, vm_expressions_stack);
    update_root(a->roots, roots, constants);
}

static void mark_gray(Objects *gray, Object *obj) {
    guard_is_not_null(gray);
    guard_is_not_null(obj);
    guard_is_equal(obj->color, OBJECT_WHITE);

    guard_is_true(da_try_append(gray, obj));
    obj->color = OBJECT_GRAY;
}

static void mark_gray_if_white(Objects *gray, Object *obj) {
    guard_is_not_null(gray);
    guard_is_not_null(obj);

    guard_is_not_equal((int) obj->type, 12345);

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
        case TYPE_CLOSURE:
        case TYPE_MACRO: {
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

    stack_for_reversed(frame, a->roots.stack) {
        mark_gray_if_white(&gray, frame->expr);
        mark_gray_if_white(&gray, frame->env);
        mark_gray_if_white(&gray, frame->unevaluated);
        mark_gray_if_white(&gray, frame->evaluated);
        if (nullptr != frame->results_list) {
            mark_gray_if_white(&gray, *frame->results_list);
        }

        slice_for(it, frame_locals(frame)) {
            mark_gray_if_white(&gray, *it);
        }
    }

    slice_for(it, *a->roots.parser_stack) {
        mark_gray_if_white(&gray, it->last);
    }

    mark_gray_if_white(&gray, *a->roots.parser_expr);
    mark_gray_if_white(&gray, *a->roots.globals);
    mark_gray_if_white(&gray, *a->roots.value);
    mark_gray_if_white(&gray, *a->roots.error);

    slice_for(it, *a->roots.vm_expressions_stack) {
        mark_gray_if_white(&gray, *it);
    }

    slice_for(it, *a->roots.constants) {
        mark_gray_if_white(&gray, *it);
    }

    Object *obj;
    while (slice_try_pop(&gray, &obj)) {
        mark_children(&gray, obj);
    }
    guard_is_true(slice_empty(gray));

    da_free(&gray);
}

#define TYPE_FREED 12345

static void sweep(ObjectAllocator *a) {
    guard_is_not_null(a);

    Object *prev = nullptr;
    for (auto it = a->objects; nullptr != it;) {
        if (OBJECT_BLACK == it->color) {
            it->color = OBJECT_WHITE;
            prev = exchange(it, it->next);
            continue;
        }

        auto const unreached = exchange(it, it->next);
        guard_is_equal(unreached->color, OBJECT_WHITE);

        if (nullptr == prev) {
            a->objects = it;
        } else {
            prev->next = it;
        }

        unreached->next = exchange(a->freed, unreached);
        a->heap_size -= unreached->size;

        if (a->no_free) {
            unreached->type = TYPE_FREED;
        } else {
            free(unreached);
        }
    }
}

static size_t count_objects(ObjectAllocator *a) {
    size_t count = 0;
    for (auto it = a->objects; nullptr != it; it = it->next) {
        count++;
    }

    return count;
}

static void collect_garbage(ObjectAllocator *a) {
    guard_is_not_null(a);
    guard_is_false(a->gc_is_running);

    a->gc_is_running = true;


    auto const heap_size_initial = a->heap_size;
    size_t count_initial, count_final;

    if (a->trace) {
        count_initial = count_objects(a);
    }

    mark(a);
    sweep(a);

    if (a->trace && (count_final = count_objects(a)) < count_initial) {
        printf(
                "GC: freed %zu objects (%zu bytes total)\n",
                count_initial - count_final, heap_size_initial - a->heap_size
        );
    }

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
           && nullptr != a->roots.globals
           && nullptr != a->roots.value
           && nullptr != a->roots.error
           && nullptr != a->roots.vm_expressions_stack
           && nullptr != a->roots.constants;
}

bool allocator_try_allocate(ObjectAllocator *a, size_t size, Object **obj) {
    guard_is_not_null(a);
    guard_is_greater(size, 0);
    guard_is_true(all_roots_set(a));

    auto const should_collect =
            ALLOCATOR_NEVER_GC != a->gc_mode
            && (ALLOCATOR_ALWAYS_GC == a->gc_mode || a->heap_size + size >= a->soft_limit);
    if (should_collect) {
        collect_garbage(a);
        adjust_soft_limit(a, size);
    }

    if (a->heap_size + size >= a->hard_limit) {
        return false;
    }

    auto const new_obj = (Object *) guard_succeeds(calloc, (size, 1));
    new_obj->next = exchange(a->objects, new_obj);
    new_obj->size = size;
    a->heap_size += size;
    *obj = new_obj;

    return true;
}

void allocator_print_statistics(ObjectAllocator const *a, FILE *file) {
    size_t objects = 0;
    for (auto it = a->objects; nullptr != it; it = it->next) {
        objects++;
    }

    fprintf(file, "Heap usage:\n");
    fprintf(file, "          Objects: %zu\n", objects);
    fprintf(file, "        Heap size: %zu bytes\n", a->heap_size);
    fprintf(file, "  Heap size limit: %zu bytes\n", a->hard_limit);
}
