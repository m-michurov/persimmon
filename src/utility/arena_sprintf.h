#pragma once

#include "arena.h"
#include "string_builder.h"

#define arena_sprintf(Arena_, Format, ...)      \
({                                              \
    auto _sb = sb_new((Arena_));                \
    sb_sprintf(_sb, (Format), ##__VA_ARGS__);   \
    sb_str_view(_sb);                           \
})
