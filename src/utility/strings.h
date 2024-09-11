#pragma once

#include "macros.h"

bool string_is_blank(char const *str);

[[nodiscard]]
bool string_try_repr_escape_seq(char value, char const **escape_seq);

[[nodiscard]]
bool string_try_get_escape_seq_value(char escaped, char *value);

#define string_for(It, Str)                         \
auto concat_identifiers(_s_, __LINE__) = (Str);     \
for (                                               \
    auto It = concat_identifiers(_s_, __LINE__);    \
    nullptr != It && '\0' != *It;                   \
    It++                                            \
)
