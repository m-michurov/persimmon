#include "stack.h"

#include "utility/guards.h"
#include "utility/pointers.h"
#include "utility/exchange.h"
#include "utility/container_of.h"

typedef struct Stack_WrappedFrame Stack_WrappedFrame;
struct Stack_WrappedFrame {
    Stack_WrappedFrame *prev;
    Stack_Frame frame;
    Object **locals_end;
    Object *locals[];
};

bool stack_try_init(Stack *s, Stack_Config config, errno_t *error_code) {
    guard_is_not_null(s);
    guard_is_greater(config.size_bytes, 0);

    errno = 0;
    auto const data = calloc(config.size_bytes, 1);
    if (nullptr == data) {
        *error_code = errno;
        return false;
    }

    *s = (Stack) {
            ._data = data,
            ._end = data + config.size_bytes
    };
    return true;
}

void stack_free(Stack *s) {
    guard_is_not_null(s);

    free(s->_data);
    *s = (Stack) {0};
}

bool stack_is_empty(Stack *s) {
    guard_is_not_null(s);

    return nullptr == s->_top;
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
            .evaluated = OBJECT_NIL
    };
}

Objects frame_locals(Stack_Frame const *frame) {
    auto const wrapped_frame = container_of(frame, Stack_WrappedFrame, frame);
    return (Objects) {
            .data = wrapped_frame->locals,
            .count = wrapped_frame->locals_end - wrapped_frame->locals
    };
}

Stack_Frame *stack_top(Stack *s) {
    guard_is_not_null(s);
    guard_is_not_null(s->_top);

    return &s->_top->frame;
}

void stack_pop(Stack *s) {
    guard_is_not_null(s);
    guard_is_not_null(s->_top);

    s->_top = s->_top->prev;
}

bool STACK__try_get_prev_frame(Stack *s, Stack_Frame *frame, Stack_Frame **prev) {
    guard_is_not_null(s);
    guard_is_not_null(frame);
    guard_is_not_null(prev);

    auto const wrapped_frame = container_of(frame, Stack_WrappedFrame, frame);
    guard_is_in_range(wrapped_frame, s->_data, s->_end - sizeof(Stack_WrappedFrame));

    if (nullptr == wrapped_frame->prev) {
        return false;
    }
    *prev = &wrapped_frame->prev->frame;
    return true;
}

bool stack_try_push_frame(Stack *s, Stack_Frame frame) {
    guard_is_not_null(s);

    auto const new_top =
            nullptr == s->_top
            ? s->_data
            : pointer_roundup(s->_top->locals_end, _Alignof(Stack_WrappedFrame));
    if ((uint8_t *) new_top + sizeof(Stack_WrappedFrame) > s->_end) {
        return false;
    }

    auto const wrapped_frame = (Stack_WrappedFrame *) new_top;
    *wrapped_frame = (Stack_WrappedFrame) {
            .prev = s->_top,
            .frame = frame,
            .locals_end = wrapped_frame->locals
    };
    s->_top = wrapped_frame;

    return true;
}

void stack_swap_top(Stack *s, Stack_Frame frame) {
    guard_is_not_null(s);
    guard_is_false(stack_is_empty(s));

    s->_top->locals_end = s->_top->locals;
    s->_top->frame = frame;
}

Stack_Locals stack_locals(Stack *s) {
    guard_is_not_null(s);
    guard_is_false(stack_is_empty(s));

    return (Stack_Locals) {
            ._end = s->_end,
            ._top = &s->_top->locals_end
    };
}

bool stack_try_create_local(Stack_Locals locals, Object ***obj) {
    guard_is_not_null(obj);
    guard_is_not_null(locals._top);

    auto new_top = (*locals._top) + 1;
    if ((uint8_t *) new_top > locals._end) {
        return false;
    }

    *obj = exchange(*locals._top, new_top);
    **obj = OBJECT_NIL;
    return true;
}
