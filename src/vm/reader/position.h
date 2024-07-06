#pragma once

#include <stdint.h>

typedef struct {
    size_t lineno;
    size_t col;
    size_t end_col;
} Position;
