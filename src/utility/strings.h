#pragma once

bool string_is_blank(char const *str);

bool string_try_repr_escape_seq(char value, char const **escape_seq);

bool string_try_get_escape_seq_value(char escaped, char *value);

#define STRINGS__concat_(A, B) A ## B
#define STRINGS__concat(A, B) STRINGS__concat_(A, B)

#define string_for(It, Str)                     \
auto STRINGS__concat(_s_, __LINE__) = (Str);    \
for (                                           \
    auto It = STRINGS__concat(_s_, __LINE__);   \
    nullptr != It && '\0' != *It;               \
    It++                                        \
)
