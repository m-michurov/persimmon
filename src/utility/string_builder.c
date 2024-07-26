#include "string_builder.h"

#include <stdarg.h>
#include <stdio.h>
#include <memory.h>

#include "guards.h"

void sb_free(StringBuilder *sb) {
    guard_is_not_null(sb);

    free(sb->str);
    *sb = (StringBuilder) {0};
}

void sb_clear(StringBuilder *sb) {
    guard_is_not_null(sb);
    sb->length = 0;
    memset(sb->str, 0, sb->_max_length);
}

bool sb_try_reserve(StringBuilder *sb, size_t max_length) {
    guard_is_not_null(sb);

    if (max_length <= sb->_max_length) {
        return true;
    }

    auto const str = realloc(sb->str, max_length + 1);
    if (nullptr == str) {
        return false;
    }

    sb->str = str;
    sb->_max_length = max_length;

    return true;
}

static void sb_format(StringBuilder *sb, char const *format, va_list args) {
    guard_is_not_null(sb);
    guard_is_not_null(format);

    auto const dst = sb->str + sb->length;
    auto const available = sb->_max_length - sb->length + 1;
    memset(dst, 0, available);

    auto const written = vsnprintf(dst, available, format, args);

    guard_is_greater_or_equal(written, 0);
    guard_is_less((size_t) written, available);

    sb->length += written;
    guard_is_equal(sb->str[sb->length], '\0');
}

bool sb_try_printf(StringBuilder *sb, char const *format, ...) {
    guard_is_not_null(sb);
    guard_is_not_null(format);

    va_list args;

    va_start(args, format);
    auto const to_be_written = vsnprintf(nullptr, 0, format, args);
    va_end(args);
    guard_is_greater_or_equal(to_be_written, 0);

    auto const new_length = sb->length + to_be_written;
    if (new_length > sb->_max_length) {
        return false;
    }

    va_start(args, format);
    sb_format(sb, format, args);
    va_end(args);

    return true;
}

bool sb_try_printf_realloc(StringBuilder *sb, char const *format, ...) {
    guard_is_not_null(sb);
    guard_is_not_null(format);

    va_list args;

    va_start(args, format);
    auto const to_be_written = vsnprintf(nullptr, 0, format, args);
    va_end(args);
    guard_is_greater_or_equal(to_be_written, 0);

    auto const new_length = sb->length + to_be_written;
    if (new_length > sb->_max_length) {
        if (false == sb_try_reserve(sb, new_length)) {
            return false;
        }
    }

    va_start(args, format);
    sb_format(sb, format, args);
    va_end(args);

    return true;
}
