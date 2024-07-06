#pragma once

#include <stdlib.h>

#include "arena.h"

typedef struct StringBuilder StringBuilder;

StringBuilder *sb_new(void);

void sb_free(StringBuilder **sb);

char *sb_str_(StringBuilder *sb);

char const *sb_str_const_(StringBuilder const *sb);

#define sb_str(Builder) _Generic((Builder),             \
    StringBuilder const *   : sb_str_const_((Builder)), \
    StringBuilder *         : sb_str_((Builder))        \
)

size_t sb_length(StringBuilder const *sb);

void sb_clear(StringBuilder *sb);

char *sb_copy_str(StringBuilder const *sb);

void sb_sprintf(StringBuilder *sb, char const *format, ...);

#define sb_append_char(Builder, Char) sb_sprintf((Builder), "%c", (Char))

#define sb_append(Builder, Str) sb_sprintf((Builder), "%s", (Str))
