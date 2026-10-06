#include "ansi_files.h"
#include "buffer_io.h"
#include "FILE_POS.h"
#include "misc_io.h"
#include "wchar_io.h"

// Older MSL character I/O (fgets/fputs and the buffer refill/flush helpers).

#define EOF -1

#define __getc(file)                                                                               \
    ((fwide(file, -1) >= 0)                                                                        \
         ? EOF                                                                                     \
         : ((file)->buffer_length-- ? (int)*(file)->buffer_ptr++ : __get_char(file)))

#define __putc(c, file)                                                                            \
    ((fwide(file, -1) >= 0)                                                                        \
         ? EOF                                                                                     \
         : ((file)->buffer_length-- ? (int)(*(file)->buffer_ptr++ = (unsigned char)(c))          \
                                    : __put_char(c, file)))

int __get_char(FILE* file);
int __put_char(int c, FILE* file);

int __get_char(FILE* file) {
    int ioresult;
    int state;

    file->buffer_length = 0;

    if (file->file_state.error || file->file_mode.file_kind == __closed_file) {
        return EOF;
    }

    state = file->file_state.io_state;

    if (state == __writing || !(file->file_mode.io_mode & __read)) {
        set_error(file);
        return EOF;
    }

    if (state >= __rereading) {
        file->file_state.io_state = state - 1;

        if (state == __rereading) {
            file->buffer_length = file->save_buffer_length;
        }

        return (unsigned char)file->ungetc_buffer[state - __rereading];
    }

    file->file_state.io_state = __reading;

    ioresult = __load_buffer(file, NULL, __align_buffer);

    if (ioresult || file->buffer_length == 0) {
        if (ioresult == __io_error) {
            set_error(file);
        } else {
            file->file_state.io_state = __neutral;
            file->file_state.eof = 1;
            file->buffer_length = 0;
        }
        return EOF;
    }

    file->buffer_length--;
    return *file->buffer_ptr++;
}

char* fgets(char* s, int n, FILE* file) {
    char* p = s;
    int c;

    if (--n < 0) {
        return NULL;
    }

    if (n) {
        do {
            c = __getc(file);

            if (c == EOF) {
                if (file->file_state.eof && p != s) {
                    break;
                }
                return NULL;
            }

            *p++ = c;
        } while (c != '\n' && --n);
    }

    *p = 0;
    return s;
}

int __put_char(int c, FILE* file) {
    int kind = file->file_mode.file_kind;

    file->buffer_length = 0;

    if (file->file_state.error || kind == __closed_file) {
        return EOF;
    }

    if (kind == __console_file) {
        __stdio_atexit();
    }

    if (file->file_state.io_state == __neutral && (file->file_mode.io_mode & __write)) {
        if ((file->file_mode.io_mode & __append) && fseek(file, 0, SEEK_END)) {
            return 0;
        }

        file->file_state.io_state = __writing;
        __prep_buffer(file);
    }

    if (file->file_state.io_state != __writing) {
        set_error(file);
        return EOF;
    }

    if (file->file_mode.buffer_mode == _IOFBF ||
        file->buffer_size == (file->buffer_ptr - file->buffer)) {
        if (__flush_buffer(file, NULL)) {
            set_error(file);
            return EOF;
        }
    }

    file->buffer_length--;
    *file->buffer_ptr++ = c;

    if (file->file_mode.buffer_mode != _IOFBF) {
        if (file->file_mode.buffer_mode == _IONBF || c == '\n') {
            if (__flush_buffer(file, NULL)) {
                set_error(file);
                return EOF;
            }
        }

        file->buffer_length = 0;
    }

    return (unsigned char)c;
}

int fputs(const char* s, FILE* file) {
    int c;
    int retval = 0;

    while ((c = *s++) != 0) {
        if (__putc(c, file) == EOF) {
            retval = EOF;
            break;
        }
    }

    return retval;
}
