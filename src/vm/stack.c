#include "stack.h"

#include "utility/guards.h"
#include "utility/pointers.h"
#include "utility/exchange.h"
#include "utility/container_of.h"

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

Stack *stack_new(Stack_Config config) {
    guard_is_greater(config.size_bytes, 0);

    auto const s = (Stack *) guard_succeeds(calloc, (sizeof(Stack) + config.size_bytes, 1));
    s->end = s->data + config.size_bytes;
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

Stack_Frame *stack_top(Stack const*s) {
    guard_is_not_null(s);
    guard_is_not_null(s->top);

    return &s->top->frame;
}

void stack_pop(Stack *s) {
    guard_is_not_null(s);
    guard_is_not_null(s->top);

    s->top = s->top->prev;
}

bool stack_try_get_prev(Stack const *s, Stack_Frame *frame, Stack_Frame **prev) {
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

void stack_swap_top(Stack *s, Stack_Frame frame) {
    guard_is_not_null(s);
    guard_is_false(stack_is_empty(s));

    s->top->locals_end = s->top->locals;
    s->top->frame = frame;
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
