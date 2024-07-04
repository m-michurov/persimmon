#pragma once

#include "object.h"

typedef enum {
    FRAME_CALL,
    FRAME_IF,
    FRAME_DO,
    FRAME_DEFINE
} Frame_Type;

typedef struct Frame Frame;
struct Frame {
    void *prev;
    void *locals_end;
    Frame_Type type;
    Object *expr;
    Object *env;
    Object **result;
    Object *unevaluated;
    Object *evaluated;
    Object *locals[];
};

Frame frame_new(Frame_Type type, Object *expr, Object *env, Object **result, Object *unevaluated);

typedef struct Stack Stack;

Stack *stack_new(size_t size_bytes);

bool stack_is_empty(Stack const *s);

Frame *stack_top(Stack *s);

void stack_pop(Stack *s);

bool stack_try_push(Stack *s, Frame frame);
