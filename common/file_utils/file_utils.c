#include "file_utils.h"

#include "common/macros.h"

void CopyLine(FILE src[static 1], FILE dst[static 1]) {
    const size_t count = 1024;
    char buf[count];

    size_t readCount;
    while ((readCount = fread(buf, sizeof(*buf), count, src)) > 0) {
        for (size_t i = 0; i < readCount && '\r' != buf[i] && '\n' != buf[i]; i++) {
            fputc(buf[i], dst);
        }
    }
}

bool SeekLineStart(
        FILE file[static 1],
        long charPos,
        long lineStart[static 1],
        long lineIndex[static 1]
) {
    rewind(file);

    long lineNo = 0;
    long curLineStart = 0;
    long curLineEnd;

    while (true) {
        auto const c = CallChecked(fgetc, (file));
        if ('\n' == c || EOF == c) {
            curLineEnd = CallChecked(ftell, (file));
            lineNo++;

            if ((charPos >= curLineStart && charPos < curLineEnd) || (EOF == c && charPos == curLineEnd)) {
                CallChecked(fseek, (file, curLineStart, SEEK_SET));

                *lineStart = curLineStart;
                *lineIndex = lineNo;

                return true;
            }

            curLineStart = curLineEnd;
        }

        if (EOF == c) {
            break;
        }
    }

    return false;
}

void DiscardWhile(
        FILE file[static 1],
        bool (predicate)(int),
        DiscardMode mode
) {
    Assert(DISCARD_WHILE_TRUE == mode || DISCARD_WHILE_FALSE == mode);

    int c;
    while (EOF != (c = CallChecked(fgetc, (file)))) {
        auto const r = predicate(c);
        if (DISCARD_WHILE_TRUE == mode) {
            if (true == r) continue;
            break;
        }

        if (DISCARD_WHILE_FALSE == mode) {
            if (false == r) continue;
            break;
        }

        Unreachable("Invalid mode");
    }

    CallChecked(ungetc, (c, file));
}
