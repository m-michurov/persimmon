#pragma once

#include <stdio.h>
#include <stdlib.h>

#if __STDC_VERSION__ <= 201710
#define auto __auto_type
#endif

#define LOG(format, ...)                    \
do {                                        \
    fprintf(                                \
        stderr,                             \
        "[%s:%d] " format "\n",             \
        __FILE__, __LINE__, ##__VA_ARGS__   \
    );                                      \
    exit(EXIT_FAILURE);                     \
} while (0)

#define Unreachable(why, ...) LOG("This code should never be reached: " why, ##__VA_ARGS__)

#define TODO(msg) do { LOG("TODO: " msg); exit(EXIT_FAILURE); } while (0)

#define VA_ARGS_IS_EMPTY(dummy, ...) ( sizeof( (char[]){#__VA_ARGS__} ) == 1 )
#define CallFailed(callee, ...)             \
do {                                        \
    perror(#callee);                        \
    if (VA_ARGS_IS_EMPTY(__VA_ARGS__)) {    \
        LOG(__VA_ARGS__);                   \
    }                                       \
    exit(EXIT_FAILURE);                     \
} while (0)

#define Assert(expression)          \
do {                                \
    __auto_type _e = (expression);  \
    if (!_e) {                      \
        LOG(                        \
            "Assertion `"           \
            #expression             \
            "` failed. "            \
        );                          \
        exit(EXIT_SUCCESS);         \
    }                               \
} while (0)
