#ifndef XSTRING_H
#define XSTRING_H

// Buffer for xStrTokBuffer: the token is copied after the next pointer.
struct xStrTokBuf
{
    unsigned char* next;
    char token[1];
};

unsigned int xStrHash(char* str);
char* xStrTok(char* string, char* control, char** nextoken);
char* xStrTokBuffer(char* string, char* control, void* buffer);
signed int xStricmp(char* string1, char* string2);
char* xStrupr(char* string);
signed int xStrParseFloatList(float* dest, char* strbuf, signed int max);

#endif
