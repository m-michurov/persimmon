#include "named_file.h"

#include "utility/guards.h"

bool named_file_try_open(char const *path, char const *mode, NamedFile *file) {
    guard_is_not_null(path);
    guard_is_not_null(file);

    errno = 0;
    auto handle = fopen(path, mode);
    if (errno) {
        return false;
    }

    *file = (NamedFile) {
            .name = path,
            .handle = handle
    };
    return true;
}

void named_file_close(NamedFile *file) {
    guard_is_not_null(file);

    fclose(file->handle);
    *file = (NamedFile) {0};
}