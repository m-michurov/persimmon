#include "stack.h"

#include "guards.h"
#include "pointers.h"

struct Stack {
    Frame *top;
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

Frame frame_new(Frame_Type type, Object *expr, Object *env, Object **result, Object *unevaluated) {
    guard_is_not_null(env);
    guard_assert(nullptr == result || nullptr != *result);
    guard_is_not_null(unevaluated);

    return (Frame) {
            .type = type,
            .expr = expr,
            .env = env,
            .result = result,
            .unevaluated = unevaluated,
            .evaluated = object_nil()
    };
}

Frame *stack_top(Stack *s) {
    guard_is_not_null(s);
    guard_is_not_null(s->top);

    return s->top;
}

void stack_pop(Stack *s) {
    guard_is_not_null(s);
    guard_is_not_null(s->top);

    s->top = s->top->prev;
}

bool stack_try_push(Stack *s, Frame frame) {
    guard_is_not_null(s);

    auto const new_top =
            nullptr == s->top
            ? s->data
            : pointer_roundup(s->top->locals_end, _Alignof(Frame));
    if ((uint8_t *) new_top + sizeof(Frame) >= s->end) {
        printf("used = %zu\n", (size_t) ((uintptr_t) s->top->locals_end - (uintptr_t) s->data));
        return false;
    }

    *((Frame *) new_top) = frame;
    ((Frame *) new_top)->prev = s->top;
    ((Frame *) new_top)->locals_end = ((Frame *) new_top)->locals;
    s->top = new_top;

    return true;
}
