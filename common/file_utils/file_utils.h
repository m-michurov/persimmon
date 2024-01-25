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
