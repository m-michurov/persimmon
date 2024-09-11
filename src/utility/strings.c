#include "strings.h"

#include <ctype.h>

#include "guards.h"

bool string_is_blank(char const *str) {
    guard_is_not_null(str);

    string_for(it, str) {
        if (false == isblank(*it) && false == isspace(*it)) {
            return false;
        }
    }

    return true;
}

typedef struct {
    char value;
    char const *repr;
} EscapeSequence;

static EscapeSequence const ESCAPE_SEQUENCES[] = {
        {.value = '\n', .repr = "\\n"},
        {.value = '\r', .repr = "\\region"},
        {.value = '\t', .repr = "\\t"},
        {.value = '\\', .repr = "\\\\"},
        {.value = '"', .repr = "\\\""},
        {0}
};

bool string_try_repr_escape_seq(char value, char const **escape_seq) {
    guard_is_not_null(escape_seq);

    for (auto it = ESCAPE_SEQUENCES; nullptr != it->repr; it++) {
        if (it->value != value) {
            continue;
        }

        *escape_seq = it->repr;
        return true;
    }

    return false;
}

bool string_try_get_escape_seq_value(char escaped, char *value) {
    guard_is_not_null(value);

    for (auto it = ESCAPE_SEQUENCES; nullptr != it->repr; it++) {
        if (it->repr[1] != escaped) {
            continue;
        }

        *value = it->value;
        return true;
    }

    return false;
}
