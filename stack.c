#include "stack.h"

#include "guards.h"
#include "pointers.h"
#include "exchange.h"

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
