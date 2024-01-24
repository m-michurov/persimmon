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

#define UNREACHABLE(why, ...) LOG("This code should never be reached: " why, ##__VA_ARGS__)

#define TODO(msg) do { fprintf(stderr, "[%s:%d] TODO: " msg "\n", __FILE__, __LINE__); exit(EXIT_FAILURE); } while (0)

#define CALL_FAILED(callee, msg, ...)   \
do {                                    \
    perror(#callee);                    \
    LOG(msg, ##__VA_ARGS__);            \
    exit(EXIT_FAILURE);                 \
} while (0)
