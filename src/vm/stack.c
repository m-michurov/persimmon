#include "stack.h"

#include "utility/guards.h"
#include "utility/pointers.h"
#include "utility/exchange.h"
#include "utility/strings.h"
#include "utility/container_of.h"
#include "object/lists.h"
#include "object/accessors.h"

typedef struct WrappedFrame WrappedFrame;
struct WrappedFrame {
    WrappedFrame *prev;
    Stack_Frame frame;
    Object **locals_end;
    Object *locals[];
};

struct Stack {
    WrappedFrame *top;
    uint8_t *end;
    uint8_t data[];
};

Stack *stack_new(size_t size_bytes) {
    auto const s = (Stack *) guard_succeeds(calloc, (sizeof(Stack) + size_bytes, 1));
    s->end = s->data + size_bytes;
    return s;
}

void stack_free(Stack **s) {
    guard_is_not_null(s);
    guard_is_not_null(*s);

    free(*s);
    *s = nullptr;
}

bool stack_is_empty(Stack const *s) {
    return nullptr == s->top;
}

Stack_Frame frame_make(
        Stack_FrameType type,
        Object *expr,
        Object *env,
        Object **results_list,
        Object *unevaluated
) {
    guard_is_not_null(env);
    guard_assert(nullptr == results_list || nullptr != *results_list);
    guard_is_not_null(unevaluated);

    return (Stack_Frame) {
            .type = type,
            .expr = expr,
            .env = env,
            .results_list = results_list,
            .unevaluated = unevaluated,
            .evaluated = object_nil()
    };
}

Stack_Frame *stack_top(Stack *s) {
    guard_is_not_null(s);
    guard_is_not_null(s->top);

    return &s->top->frame;
}

void stack_pop(Stack *s) {
    guard_is_not_null(s);
    guard_is_not_null(s->top);

    s->top = s->top->prev;
}

bool stack_try_get_prev(Stack *s, Stack_Frame *frame, Stack_Frame **prev) {
    auto const wrapped_frame = container_of(frame, WrappedFrame, frame);
    guard_is_in_range(wrapped_frame, s->data, s->end - sizeof(WrappedFrame));

    if (nullptr == wrapped_frame->prev) {
        return false;
    }
    *prev = &wrapped_frame->prev->frame;
    return true;
}

bool stack_try_push_frame(Stack *s, Stack_Frame frame) {
    guard_is_not_null(s);

    auto const new_top =
            nullptr == s->top
            ? s->data
            : pointer_roundup(s->top->locals_end, _Alignof(WrappedFrame));
    if ((uint8_t *) new_top + sizeof(WrappedFrame) > s->end) {
        printf("used = %zu\n", (size_t) ((uintptr_t) s->top->locals_end - (uintptr_t) s->data));
        return false;
    }

    auto const wrapped_frame = (WrappedFrame *) new_top;
    *wrapped_frame = (WrappedFrame) {
            .prev = s->top,
            .frame = frame,
            .locals_end = wrapped_frame->locals
    };
    s->top = wrapped_frame;

    return true;
}

bool stack_try_create_local(Stack *s, Object ***obj) {
    guard_is_not_null(s);
    guard_is_not_null(obj);
    guard_is_not_null(s->top);

    auto new_locals_end = s->top->locals_end + sizeof(Object *);
    if ((uint8_t *) new_locals_end > s->end) {
        return false;
    }

    *obj = exchange(s->top->locals_end, new_locals_end);
    return true;
}

static void print_env(Object *env) {
    printf("      Environment: {\n");
    object_list_for(it, object_as_cons(env).first) {
        Object *obj_name, *obj_value;
        guard_is_true(object_list_try_unpack_2(&obj_name, &obj_value, it));
        printf("        ");
        object_repr_print(obj_name, stdout);
        printf(": ");
        if (TYPE_CONS == obj_value->type) {
            size_t i = 1;

            printf("(");
            object_repr_print(obj_value->as_cons.first, stdout);
            object_list_for(elem, obj_value->as_cons.rest) {
                i++;
                printf(", ");
                object_repr_print(elem, stdout);
                if (i > 5) { break; }
            }
            printf(", ...)");
        } else if (TYPE_STRING == obj_value->type) {
            size_t i = 0;

            printf("\"");
            string_for(c, obj_value->as_atom)
            {
                i++;
                printf("%c", *c);
                if (i > 10) { break; }
            }

            printf("\"");
        } else {
            object_repr_print(obj_value, stdout);
        }
        printf("\n");
    }
    printf("      }\n");
}

void stack_print_traceback(Stack *s, FILE *file) {
    fprintf(file, "Traceback (most recent call last):\n");
    guard_is_false(stack_is_empty(s));

    auto frame = stack_top(s);
    while (true) {
        printf("    ");
        object_repr_print(frame->expr, file);
        printf("\n");
        print_env(frame->env);

        if (false == stack_try_get_prev(s, frame, &frame)) {
            break;
        }
    }
    fprintf(file, "Some calls may be missing due to tail call optimization.\n");
}
