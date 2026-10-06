#include "buffer_io.h"

inline void __convert_from_newlines(unsigned char* buf, size_t* n) {}

inline void __convert_to_newlines(unsigned char* buf, size_t* n) {}

void __prep_buffer(FILE* file) {
    file->buffer_ptr = file->buffer;
    file->buffer_length = file->buffer_size;
    file->buffer_length -= file->position & file->buffer_alignment;
    file->buffer_position = file->position;
}

int __load_buffer(FILE* file, size_t* bytes_loaded, int alignment) {
    int ioresult;

    __prep_buffer(file);

    if (alignment == __dont_align_buffer)
        file->buffer_length = file->buffer_size;

    ioresult = (*file->read_fn)(file->handle, file->buffer, (size_t*)&file->buffer_length,
                                file->idle_fn);

    if (ioresult == __io_EOF)
        file->buffer_length = 0;

    if (bytes_loaded)
        *bytes_loaded = file->buffer_length;

    if (ioresult)
        return ioresult;

    file->position += file->buffer_length;

    return 0;
}

int __flush_buffer(FILE* file, size_t* bytes_flushed) {
    size_t buffer_len;
    int ioresult;

    buffer_len = file->buffer_ptr - file->buffer;

    if (buffer_len) {
        file->buffer_length = buffer_len;

        if (!file->file_mode.binary_io)
            __convert_from_newlines(file->buffer, (size_t*)&file->buffer_length);

        ioresult = (*file->write_fn)(file->handle, file->buffer, (size_t*)&file->buffer_length,
                                     file->idle_fn);

        if (bytes_flushed)
            *bytes_flushed = file->buffer_length;

        if (ioresult)
            return (ioresult);

        file->position += file->buffer_length;
    }

    __prep_buffer(file);

    return 0;
}
