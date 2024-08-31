#pragma once

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define GUARD__print_error(Format, ...)                                                                 \
do {                                                                                                    \
    fprintf(stderr, "[%s:%d] %s - " Format "\n", __FILE_NAME__, __LINE__, __FUNCTION__, ##__VA_ARGS__); \
} while (false)

#define guard_assert(Cond, ...)             \
do {                                        \
    if (!(Cond)) {                          \
        GUARD__print_error(__VA_ARGS__);    \
        exit(EXIT_FAILURE);                 \
    }                                       \
} while (false)

#define guard_is_true(Val) guard_assert((Val), "expected %s to be true", #Val)

#define guard_is_false(Val) guard_assert(!(Val), "expected %s to be false", #Val)

#define guard_is_not_null(Ptr) guard_assert((nullptr != (Ptr)), "expected %s to not be null", #Ptr)

#define guard_is_equal(It, Value) guard_assert(((It) == (Value)), "expected %s to be equal to %s", #It, #Value)

#define GUARD__varargs_to_str(...) "{" #__VA_ARGS__ "}"

#define guard_is_one_of(It, _0, ...)                                \
do {                                                                \
    auto const _it = (It);                                          \
    typeof(It) _variants[] = {_0, ##__VA_ARGS__};                   \
    auto const _count = sizeof(_variants) / sizeof(_variants[0]);   \
    auto _any = false;                                              \
    for (auto _p = _variants; _p < _variants + _count; _p++) {      \
        if (_it != *_p) {                                           \
            continue;                                               \
        }                                                           \
                                                                    \
        _any = true;                                                \
        break;                                                      \
    }                                                               \
    guard_assert(                                                   \
        _any,                                                       \
        "expected %s to be one of %s",                              \
        #It, GUARD__varargs_to_str(_0, ##__VA_ARGS__)               \
    );                                                              \
} while (false)

#define guard_is_not_equal(It, Value)       \
guard_assert(                               \
    ((It) != (Value)),                      \
    "expected %s to not be equal to %s",    \
    #It, #Value                             \
)

#define guard_is_less(It, Value)                \
guard_assert(                                   \
    ((It) < (Value)),                           \
    "expected " #It " to be less than " #Value  \
)

#define guard_is_less_or_equal(It, Value)           \
guard_assert(                                       \
    ((It) <= (Value)),                              \
    "expected %s to be less than or equal to %s",   \
    #It, #Value                                     \
)

#define guard_is_greater(It, Value)         \
guard_assert(                               \
    ((It) > (Value)),                       \
    "expected %s to be greater than %s",    \
    #It, #Value                             \
)

#define guard_is_greater_or_equal(It, Value)            \
guard_assert(                                           \
    ((It) >= (Value)),                                  \
    "expected %s to be greater than or equal to %s",    \
    #It, #Value                                         \
)

#define guard_is_in_range(It, StartInclusive, EndInclusive) \
do {                                                        \
    auto const _it = (It);                                  \
    guard_assert(                                           \
        ((_it) >= (typeof(_it)) (StartInclusive)            \
            && (_it) <= (typeof(_it)) (EndInclusive)),      \
        "expected %s to be in range %s..%s",                \
        #It, #StartInclusive, #EndInclusive                 \
    );                                                      \
} while (false)

#define guard_errno_is_not_set()        \
do {                                    \
    auto const __errno = errno;         \
    guard_assert(                       \
        0 == __errno,                   \
        "expected errno to be 0: %s",   \
        strerror(__errno)               \
    );                                  \
} while (false)

#define guard_succeeds(Callee, ArgsList)    \
({                                          \
    errno = 0;                              \
    auto const _r = Callee ArgsList;        \
    guard_assert(                           \
        0 == errno,                         \
        "%s(%s) failed: %s",                \
        #Callee, #ArgsList, strerror(errno) \
    );                                      \
    _r;                                     \
})

#define guard_unreachable(...) guard_assert(false, "this code must never be reached: " __VA_ARGS__)