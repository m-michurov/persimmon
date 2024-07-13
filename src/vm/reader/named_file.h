#pragma once

#include <stdio.h>
#include <errno.h>

typedef struct {
    char const *name;
    FILE *handle;
} NamedFile;

bool named_file_try_open(char const *path, char const *mode, NamedFile *file);

void named_file_close(NamedFile *file);
