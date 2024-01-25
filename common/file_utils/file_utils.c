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
        auto c = fgetc(file);
        if ('\n' == c || feof(file)) {
            curLineEnd = CallChecked(ftell, (file));
            lineNo++;

            if (charPos >= curLineStart && charPos < curLineEnd) {
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
