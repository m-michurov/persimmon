#include "stack.h"

#include "guards.h"

Stack *stack_new(Arena *a, size_t max_depth) {
    return arena_emplace(a, ((Stack) {
        .frames = arena_new_array(a, Frame, max_depth),
        .capacity = max_depth
    }));
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
            .evaluated = object_nil(),
            .error = object_nil()
    };
}

Frame *stack_top(Stack *s) {
    guard_is_not_null(s);
    guard_is_greater(s->count, 0);

    return &s->frames[s->count - 1];
}

void stack_pop(Stack *s) {
    guard_is_not_null(s);
    guard_is_greater(s->count, 0);

    s->count--;
}

bool stack_try_push(Stack *s, Frame frame) {
    guard_is_not_null(s);

    if (s->count >= s->capacity) {
        return false;
    }

    s->frames[s->count++] = frame;
    return true;
}
