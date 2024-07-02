#pragma once

#include "object.h"

typedef enum {
    FRAME_CALL,
    FRAME_IF,
    FRAME_DO,
    FRAME_DEFINE
} Frame_Type;

typedef struct {
    Frame_Type type;
    Object *expr;
    Object *env;
    Object **result;
    Object *unevaluated;
    Object *evaluated;
    Object *error;
} Frame;

Frame frame_new(Frame_Type type, Object *expr, Object *env, Object **result, Object *unevaluated);

// TODO make opaque
typedef struct {
    Frame *frames;
    size_t count;
    size_t capacity;
} Stack;

Stack *stack_new(Arena *a, size_t max_depth);

Frame *stack_top(Stack *s);

void stack_pop(Stack *s);

bool stack_try_push(Stack *s, Frame frame);
