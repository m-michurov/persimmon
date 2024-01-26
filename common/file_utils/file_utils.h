#pragma once

#include <stdio.h>
#include <stdbool.h>

void CopyLine(FILE src[static 1], FILE dst[static 1]);

bool SeekLineStart(
        FILE file[static 1],
        long charPos,
        long lineStart[static 1],
        long lineIndex[static 1]
);

typedef enum DiscardMode {
    DISCARD_WHILE_TRUE,
    DISCARD_WHILE_FALSE
} DiscardMode;

void DiscardWhile(
        FILE file[static 1],
        bool (predicate)(int),
        DiscardMode mode
);
