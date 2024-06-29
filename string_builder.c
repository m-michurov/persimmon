#include "string_builder.h"

#include <stdarg.h>
#include <stdio.h>
#include <memory.h>

#include "guards.h"

struct StringBuilder {
    Arena *a;
    char *str;
    size_t length;
    size_t capacity;
};

StringBuilder *sb_new(Arena *a) {
    return arena_emplace(a, ((StringBuilder) {.a = a}));
}

char *sb_str_(StringBuilder *sb) {
    return sb->str;
}

char const *sb_str_const_(StringBuilder const *sb) {
    return sb->str;
}

size_t sb_length(StringBuilder const *sb) {
    return sb->length;
}

void sb_clear(StringBuilder *sb) {
    guard_is_not_null(sb);
    sb->length = 0;
}

char *sb_copy_str(Arena *a, StringBuilder const *sb) {
    return arena_copy_all(a, sb->str, sb->length + 1);
}

void sb_sprintf(StringBuilder *sb, char const *format, ...) {
    guard_is_not_null(sb);

    va_list args;

    va_start(args, format);
    auto const to_be_written = vsnprintf(nullptr, 0, format, args);
    va_end(args);
    guard_is_greater_or_equal(to_be_written, 0);

    auto const min_capacity = sb->length + to_be_written + 1;
    if (min_capacity > sb->capacity) {
        sb->str = arena_copy(sb->a, sb->str, sb->capacity, min_capacity);
        sb->capacity = min_capacity;
    }

    auto const dst = sb->str + sb->length;
    auto const available = sb->capacity - sb->length;
    memset(dst, 0xFF, available);

    va_start(args, format);
    auto const written = vsnprintf(dst, available, format, args);
    va_end(args);

    guard_is_greater_or_equal(written, 0);
    guard_is_less((size_t) written, available);

    sb->length += written;
    guard_is_equal(sb->str[sb->length], '\0');
}
