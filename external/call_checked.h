#pragma once

#include <string.h>
#include <errno.h>
#include <stdio.h>

#define CallChecked(Callee, ArgsList)   \
({                                      \
    errno = 0;                          \
    __auto_type _r = Callee ArgsList;   \
    if (errno) {                        \
        fprintf(                        \
            stderr, "[%s:%d] %s: %s\n", \
            __FILE_NAME__, __LINE__,    \
            #Callee, strerror(errno)    \
        );                              \
        exit(EXIT_FAILURE);             \
    }                                   \
    _r;                                 \
})
