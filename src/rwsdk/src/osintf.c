/*
 * OS interface: file access goes through the engine's file function table,
 * which defaults to the C library.
 */

#include "rwsdk/rwcore.h"

void* fopen(const char* name, const char* mode);
int fclose(void* file);
unsigned long fread(void* ptr, unsigned long size, unsigned long count, void* file);
unsigned long fwrite(const void* ptr, unsigned long size, unsigned long count, void* file);
char* fgets(char* s, int n, void* file);
int fputs(const char* s, void* file);
int feof(void* file);
int fseek(void* file, long offset, int mode);
int fflush(void* file);
long ftell(void* file);

RwFileFunctions* RwOsGetFileInterface(void) {
    return &RWSRCGLOBAL(fileFuncs);
}

static RwBool rwfexist(const RwChar* name) {
    void* fp;
    RwBool result;

    fp = RWSRCGLOBAL(fileFuncs).rwfopen(name, "rb");
    result = (fp != NULL);
    if (fp) {
        RWSRCGLOBAL(fileFuncs).rwfclose(fp);
    }

    return result;
}

RwBool _rwFileSystemOpen(void) {
    RWSRCGLOBAL(fileFuncs).rwfexist = rwfexist;
    RWSRCGLOBAL(fileFuncs).rwfopen = fopen;
    RWSRCGLOBAL(fileFuncs).rwfclose = fclose;
    RWSRCGLOBAL(fileFuncs).rwfread = (RwUInt32(*)(void*, RwUInt32, RwUInt32, void*))fread;
    RWSRCGLOBAL(fileFuncs).rwfwrite = (RwUInt32(*)(const void*, RwUInt32, RwUInt32, void*))fwrite;
    RWSRCGLOBAL(fileFuncs).rwfgets = fgets;
    RWSRCGLOBAL(fileFuncs).rwfputs = fputs;
    RWSRCGLOBAL(fileFuncs).rwfeof = feof;
    RWSRCGLOBAL(fileFuncs).rwfseek = fseek;
    RWSRCGLOBAL(fileFuncs).rwfflush = fflush;
    RWSRCGLOBAL(fileFuncs).rwftell = (int (*)(void*))ftell;

    return TRUE;
}

void _rwFileSystemClose(void) {
}
