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

typedef struct Stack Stack;

typedef struct {
    size_t size_bytes;
} Stack_Config;

Stack *stack_new(Stack_Config config);

void stack_free(Stack **s);

bool stack_is_empty(Stack const *s);

Stack_Frame *stack_top(Stack const *s);

void stack_pop(Stack *s);

[[nodiscard]]

bool stack_try_push_frame(Stack *s, Stack_Frame frame);

void stack_swap_top(Stack *s, Stack_Frame frame);

[[nodiscard]]

bool stack_try_create_local(Stack *s, Object ***obj);

[[nodiscard]]

bool stack_try_get_prev(Stack const *s, Stack_Frame *frame, Stack_Frame **prev);

#define stack_for_reversed(It, Stack_)      \
for (                                       \
    Stack_Frame *It =                       \
        stack_is_empty((Stack_))            \
            ? nullptr                       \
            : stack_top((Stack_));          \
    nullptr != It;                          \
    stack_try_get_prev((Stack_), It, &It)   \
        ? nullptr                           \
        : (It = nullptr)                    \
)
