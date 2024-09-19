#include "allocator.h"

#include "utility/guards.h"
#include "utility/slice.h"
#include "utility/dynamic_array.h"
#include "utility/exchange.h"
#include "utility/pointers.h"
#include "vm/stack.h"
#include "vm/reader/parser.h"

ObjectAllocator allocator_make(ObjectAllocator_Config config) {
    return (ObjectAllocator) {
            ._soft_limit = config.soft_limit_initial,
            ._hard_limit = config.hard_limit,
            ._grow_factor = config.soft_limit_grow_factor,
            ._gc_mode = config.debug.gc_mode,
            ._trace = config.debug.trace,
            ._no_free = config.debug.no_free
    };
}

void allocator_free(ObjectAllocator *a) {
    guard_is_not_null(a);

    for (auto it = a->_objects; nullptr != it;) {
        auto const next = it->next;
        free(it);
        it = next;
    }

    *a = (ObjectAllocator) {0};
}

#define update_root(Dst, Src, Member) ((Dst).Member = pointer_first_nonnull((Dst).Member, (Src).Member))

void allocator_set_roots(ObjectAllocator *a, ObjectAllocator_Roots roots) {
    guard_is_not_null(a);

    update_root(a->_roots, roots, stack); // NOLINT(*-sizeof-expression)
    update_root(a->_roots, roots, parser_stack); // NOLINT(*-sizeof-expression)
    update_root(a->_roots, roots, parser_expr);
    update_root(a->_roots, roots, globals);
    update_root(a->_roots, roots, value);
    update_root(a->_roots, roots, error);
    update_root(a->_roots, roots, exprs);
}

[[nodiscard]]
static bool try_mark_gray(Objects *gray, Object *obj) {
    guard_is_not_null(gray);
    guard_is_not_null(obj);
    guard_is_equal(obj->color, OBJECT_WHITE);

    if (false == da_try_append(gray, obj)) { // NOLINT(*-sizeof-expression)
        return false;
    }

    obj->color = OBJECT_GRAY;
    return true;
}

#define TYPE_FREED 12345

[[nodiscard]]
static bool try_mark_gray_if_white(Objects *gray, Object *obj) {
    guard_is_not_null(gray);
    guard_is_not_null(obj);

    guard_is_not_equal((int) obj->type, TYPE_FREED);

    if (OBJECT_WHITE != obj->color) {
        return true;
    }

    return try_mark_gray(gray, obj);
}

static void mark_black(Object *obj) {
    guard_is_equal(obj->color, OBJECT_GRAY);
    obj->color = OBJECT_BLACK;
}

[[nodiscard]]
static bool try_mark_children(Objects *gray, Object *obj) {
    guard_is_not_null(gray);
    guard_is_not_null(obj);
    guard_is_equal(obj->color, OBJECT_GRAY);

    switch (obj->type) {
        case TYPE_INT:
        case TYPE_STRING:
        case TYPE_SYMBOL:
        case TYPE_NIL:
        case TYPE_PRIMITIVE: {
            mark_black(obj);
            return true;
        }
        case TYPE_LIST: {
            mark_black(obj);

            return try_mark_gray_if_white(gray, obj->as_list.first)
                   && try_mark_gray_if_white(gray, obj->as_list.rest);
        }
        case TYPE_CLOSURE:
        case TYPE_MACRO: {
            mark_black(obj);

            return try_mark_gray_if_white(gray, obj->as_closure.args)
                   && try_mark_gray_if_white(gray, obj->as_closure.env)
                   && try_mark_gray_if_white(gray, obj->as_closure.body);
        }
        case TYPE_DICT: {
            mark_black(obj);

            return try_mark_gray_if_white(gray, obj->as_dict.key)
                   && try_mark_gray_if_white(gray, obj->as_dict.value)
                   && try_mark_gray_if_white(gray, obj->as_dict.left)
                   && try_mark_gray_if_white(gray, obj->as_dict.right);
        }
    }

    guard_unreachable();
}

[[nodiscard]]
static bool try_mark_(ObjectAllocator *a, Objects *gray) {
    guard_is_not_null(a);
    guard_is_not_null(gray);

    slice_clear(gray);

    stack_for_reversed(frame, a->_roots.stack) {
        auto const ok =
                try_mark_gray_if_white(gray, frame->expr)
                && try_mark_gray_if_white(gray, frame->env)
                && try_mark_gray_if_white(gray, frame->unevaluated)
                && try_mark_gray_if_white(gray, frame->evaluated);
        if (false == ok) {
            return false;
        }
        if (nullptr != frame->results_list) {
            if (false == try_mark_gray_if_white(gray, *frame->results_list)) {
                return false;
            }
        }

        slice_for_v(it, frame_locals(frame)) {
            if (false == try_mark_gray_if_white(gray, *it)) {
                return false;
            }
        }
    }

    slice_for(it, a->_roots.parser_stack) {
        if (false == try_mark_gray_if_white(gray, it->last)) {
            return false;
        }
    }

    auto const ok =
            try_mark_gray_if_white(gray, *a->_roots.parser_expr)
            && try_mark_gray_if_white(gray, *a->_roots.globals)
            && try_mark_gray_if_white(gray, *a->_roots.value)
            && try_mark_gray_if_white(gray, *a->_roots.error)
            && try_mark_gray_if_white(gray, *a->_roots.exprs);
    if (false == ok) {
        return false;
    }

    Object *obj;
    while (slice_try_pop(gray, &obj)) {
        if (false == try_mark_children(gray, obj)) {
            return false;
        }
    }
    guard_is_true(slice_empty(*gray));

    return true;
}

[[nodiscard]]
static bool try_mark(ObjectAllocator *a) {
    auto gray = (Objects) {0};
    auto const ok = try_mark_(a, &gray);
    da_free(&gray);
    return ok;
}

static void sweep(ObjectAllocator *a) {
    guard_is_not_null(a);

    Object *prev = nullptr;
    for (auto it = a->_objects; nullptr != it;) {
        if (OBJECT_BLACK == it->color) {
            it->color = OBJECT_WHITE;
            prev = exchange(it, it->next);
            continue;
        }

        auto const unreached = exchange(it, it->next);
        guard_is_equal(unreached->color, OBJECT_WHITE);

        if (nullptr == prev) {
            a->_objects = it;
        } else {
            prev->next = it;
        }

        unreached->next = exchange(a->_freed, unreached);
        a->_heap_size -= unreached->size;

        if (a->_no_free) {
            unreached->type = TYPE_FREED;
        } else {
            free(unreached);
        }
    }
}

static size_t count_objects(ObjectAllocator *a) {
    size_t count = 0;
    for (auto it = a->_objects; nullptr != it; it = it->next) {
        count++;
    }

    return count;
}

[[nodiscard]]
static bool try_collect_garbage(ObjectAllocator *a) {
    guard_is_not_null(a);
    guard_is_false(a->_gc_is_running);

    a->_gc_is_running = true;

    auto const heap_size_initial = a->_heap_size;
    size_t count_initial, count_final;

    if (a->_trace) {
        count_initial = count_objects(a);
    }

    if (false == try_mark(a)) {
        return false;
    }
    sweep(a);

    if (a->_trace && (count_final = count_objects(a)) < count_initial) {
        printf(
                "GC: freed %zu objects (%zu bytes total)\n",
                count_initial - count_final, heap_size_initial - a->_heap_size
        );
    }

    a->_gc_is_running = false;
    return true;
}

static void adjust_soft_limit(ObjectAllocator *a, size_t size) {
    guard_is_not_null(a);

    a->_soft_limit = min(size + a->_soft_limit * a->_grow_factor, a->_hard_limit);
}

static bool all_roots_set(ObjectAllocator const *a) {
    return nullptr != a->_roots.stack
           && nullptr != a->_roots.parser_stack
           && nullptr != a->_roots.parser_expr
           && nullptr != a->_roots.globals
           && nullptr != a->_roots.value
           && nullptr != a->_roots.error
           && nullptr != a->_roots.exprs;
}

bool allocator_try_allocate(ObjectAllocator *a, size_t size, Object **obj) {
    guard_is_not_null(a);
    guard_is_greater(size, 0);
    guard_is_true(all_roots_set(a));

    auto const should_collect =
            ALLOCATOR_NEVER_GC != a->_gc_mode
            && (ALLOCATOR_ALWAYS_GC == a->_gc_mode || a->_heap_size + size >= a->_soft_limit);
    if (should_collect) {
        if (false == try_collect_garbage(a)) {
            return false;
        }
        adjust_soft_limit(a, size);
    }

    if (a->_heap_size + size >= a->_hard_limit) {
        return false;
    }

    auto const new_obj = (Object *) calloc(size, 1);
    if (nullptr == new_obj) {
        return false;
    }

    new_obj->next = exchange(a->_objects, new_obj);
    new_obj->size = size;
    a->_heap_size += size;

    *obj = new_obj;

    return true;
}

void allocator_print_statistics(ObjectAllocator *a, FILE *file) {
    size_t objects = 0;
    for (auto it = a->_objects; nullptr != it; it = it->next) {
        objects++;
    }

    fprintf(file, "Heap usage:\n");
    fprintf(file, "          Objects: %zu\n", objects);
    fprintf(file, "        Heap size: %zu bytes\n", a->_heap_size);
    fprintf(file, "  Heap size limit: %zu bytes\n", a->_hard_limit);
}
