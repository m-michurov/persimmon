#pragma once

#define option_some(Type, Value) ((Type) {.has_value = true, .value = (Value)})

#define option_none(Type) ((Type) {.has_value = false})

#define option_or_some(Option, Default)             \
({                                                  \
    auto const _opt = (Option);                     \
    (_opt.has_value                                 \
        ? _opt                                      \
        : option_some(typeof(Option), Default));    \
})
