#pragma once

#include "object/object.h"

typedef enum {
    FRAME_CALL,
    FRAME_FN,
    FRAME_MACRO,
    FRAME_IF,
    FRAME_DO,
    FRAME_DEFINE,
    FRAME_IMPORT,
    FRAME_QUOTE,
    FRAME_CATCH,
    FRAME_AND,
    FRAME_OR
} Stack_FrameType;

typedef struct {
    Stack_FrameType type;
    Object *expr;
    Object *env;
    Object *unevaluated;
    Object *evaluated;
    Object **results_list;
} Stack_Frame;

Stack_Frame frame_make(
        Stack_FrameType type,
        Object *expr,
        Object *env,
        Object **results_list,
        Object *unevaluated
);

Objects frame_locals(Stack_Frame const *frame);

typedef struct Stack_WrappedFrame Stack_WrappedFrame;

typedef struct Stack Stack;
struct Stack {
    Stack_WrappedFrame *_top;
    uint8_t *_end;
    uint8_t *_data;
};

typedef struct {
    size_t size_bytes;
} Stack_Config;

bool stack_try_init(Stack *s, Stack_Config config, errno_t *error_code);

void stack_free(Stack *s);

bool stack_is_empty(Stack *s);

Stack_Frame *stack_top(Stack *s);

void stack_pop(Stack *s);

[[nodiscard]]
bool stack_try_push_frame(Stack *s, Stack_Frame frame);

void stack_swap_top(Stack *s, Stack_Frame frame);

// TODO pass to primitives and error constructors
typedef struct {
    uint8_t *_end;
    Object ***_top;
} Stack_Locals;

[[nodiscard]]
Stack_Locals stack_locals(Stack *s);

[[nodiscard]]
bool stack_try_create_local(Stack_Locals locals, Object ***obj);

[[nodiscard]]
bool STACK__try_get_prev_frame(Stack *s, Stack_Frame *frame, Stack_Frame **prev);

#define stack_for_reversed(It, Stack_)              \
for (                                               \
    Stack_Frame *It =                               \
        stack_is_empty((Stack_))                    \
            ? nullptr                               \
            : stack_top((Stack_));                  \
    nullptr != It;                                  \
    STACK__try_get_prev_frame((Stack_), It, &It)    \
        ? nullptr                                   \
        : (It = nullptr)                            \
)
