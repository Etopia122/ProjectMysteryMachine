/*
 * String functions: the engine routes string handling through a table so a
 * platform can substitute its own (here the MSL C library plus local
 * case-insensitive helpers).
 */

#include "rwsdk/rwcore.h"

int sprintf(char* s, const char* format, ...);
int vsprintf(char* s, const char* format, void* arg);
char* strcpy(char* dst, const char* src);
char* strncpy(char* dst, const char* src, unsigned long n);
char* strcat(char* dst, const char* src);
char* strncat(char* dst, const char* src, unsigned long n);
char* strstr(const char* str, const char* pat);
int strcmp(const char* s1, const char* s2);
unsigned long strlen(const char* s);
char* strtok(char* str, const char* delim);
int sscanf(const char* s, const char* format, ...);

static int StrICmp(const RwChar* s1, const RwChar* s2) {
    if (s1 && s2) {
        RwChar c1;
        RwChar c2;

        do {
            c1 = *s1;
            c2 = *s2;

            if (c1 >= 'A' && c1 <= 'Z') {
                c1 += 'a' - 'A';
            }
            if (c2 >= 'A' && c2 <= 'Z') {
                c2 += 'a' - 'A';
            }

            if (c1 != c2) {
                return c1 - c2;
            }

            s1++;
            s2++;
        } while (c1 && c2);

        if (c1 != c2) {
            return c1 - c2;
        }
    }

    return 0;
}

static RwChar* StrUpr(RwChar* str) {
    if (str) {
        RwChar* s = str;

        while (*s) {
            RwChar c = *s;

            if (c >= 'a' && c <= 'z') {
                *s = c - ('a' - 'A');
            }
            s++;
        }
    }

    return str;
}

static RwChar* StrLwr(RwChar* str) {
    if (str) {
        RwChar* s = str;

        while (*s) {
            RwChar c = *s;

            if (c >= 'A' && c <= 'Z') {
                *s = c + ('a' - 'A');
            }
            s++;
        }
    }

    return str;
}

static RwChar* StrChr(const RwChar* s, RwInt32 c) {
    RwChar* result = NULL;
    RwChar ch = (RwChar)c;

    do {
        RwChar cur = *s;

        if (cur == ch) {
            result = (RwChar*)s;
            break;
        }
        s++;
        if (!cur) break;
    } while (1);

    return result;
}

static RwChar* StrRChr(const RwChar* s, RwInt32 c) {
    RwChar* result = NULL;
    RwChar ch = (RwChar)c;

    do {
        RwChar cur = *s;

        if (cur == ch) {
            result = (RwChar*)s;
        }
        s++;
        if (!cur) break;
    } while (1);

    return result;
}

RwBool _rwStringOpen(void) {
    RWSRCGLOBAL(stringFuncs).vecSprintf = (int (*)(RwChar*, const RwChar*, ...))sprintf;
    RWSRCGLOBAL(stringFuncs).vecVsprintf = (int (*)(RwChar*, const RwChar*, void*))vsprintf;
    RWSRCGLOBAL(stringFuncs).rwstrcpy = strcpy;
    RWSRCGLOBAL(stringFuncs).rwstrncpy = (RwChar * (*)(RwChar*, const RwChar*, RwUInt32)) strncpy;
    RWSRCGLOBAL(stringFuncs).rwstrcat = strcat;
    RWSRCGLOBAL(stringFuncs).rwstrncat = (RwChar * (*)(RwChar*, const RwChar*, RwUInt32)) strncat;
    RWSRCGLOBAL(stringFuncs).rwstrrchr = StrRChr;
    RWSRCGLOBAL(stringFuncs).rwstrchr = StrChr;
    RWSRCGLOBAL(stringFuncs).rwstrstr = strstr;
    RWSRCGLOBAL(stringFuncs).rwstrcmp = strcmp;
    RWSRCGLOBAL(stringFuncs).rwstricmp = StrICmp;
    RWSRCGLOBAL(stringFuncs).rwstrlen = (RwUInt32(*)(const RwChar*))strlen;
    RWSRCGLOBAL(stringFuncs).rwstrupr = StrUpr;
    RWSRCGLOBAL(stringFuncs).rwstrlwr = StrLwr;
    RWSRCGLOBAL(stringFuncs).rwstrtok = strtok;
    RWSRCGLOBAL(stringFuncs).rwsscanf = (int (*)(const RwChar*, const RwChar*, ...))sscanf;

    return TRUE;
}

void _rwStringClose(void) {
}
