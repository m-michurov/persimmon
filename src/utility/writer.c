#include "writer.h"

Writer writer_make_from_file_(FILE *file) {
    return (Writer) {
            .type = WRITER_FILE,
            .as_file = file
    };
}

Writer writer_make_from_sb_(StringBuilder *sb) {
    return (Writer) {
            .type = WRITER_SB,
            .as_sb = sb
    };
}
