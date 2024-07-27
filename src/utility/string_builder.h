#pragma once

#include <stdlib.h>

#include "arena.h"

typedef struct {
    char *str;
    size_t length;

    size_t _max_length;
} StringBuilder;

void sb_free(StringBuilder *sb);

void sb_clear(StringBuilder *sb);

bool sb_try_reserve(StringBuilder *sb, size_t max_length, errno_t *error_code);

bool sb_try_printf(StringBuilder *sb, char const *format, ...);

bool sb_try_printf_realloc(StringBuilder *sb, errno_t *error_code, char const *format, ...);
