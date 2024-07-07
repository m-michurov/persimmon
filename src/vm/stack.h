#pragma once

#include "object/object.h"

typedef enum {
    FRAME_CALL,
    FRAME_IF,
    FRAME_DO,
    FRAME_DEFINE
} Stack_FrameType;

typedef struct {
    Stack_FrameType type;
    Object *expr;
    Object *env;
    Object *unevaluated;
    Object *evaluated;
    Object **results_list;
    Object *error;
} Stack_Frame;

Stack_Frame frame_make(
        Stack_FrameType type,
        Object *expr,
        Object *env,
        Object **results_list,
        Object *unevaluated
);

typedef struct Stack Stack;

Stack *stack_new(size_t size_bytes);

void stack_free(Stack **s);

bool stack_is_empty(Stack const *s);

Stack_Frame *stack_top(Stack const*s);

void stack_pop(Stack *s);

bool stack_try_push_frame(Stack *s, Stack_Frame frame);

bool stack_try_create_local(Stack *s, Object ***obj);

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
