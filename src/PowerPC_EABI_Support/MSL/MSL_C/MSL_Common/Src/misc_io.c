#include "misc_io.h"
#include "abort_exit.h"
#include "ansi_files.h"

void clearerr(FILE* stream) {
    stream->file_state.eof = 0;
    stream->file_state.error = 0;
}

int feof(FILE* stream) {
    return stream->file_state.eof;
}

void __stdio_atexit(void) {
    __stdio_exit = __close_all;
}
