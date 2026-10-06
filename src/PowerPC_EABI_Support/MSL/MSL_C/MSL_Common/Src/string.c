#include "string.h"

#define K1 0x80808080
#define K2 0xFEFEFEFF

size_t strlen(const char* str) {
    size_t len = -1;
    unsigned char* p = (unsigned char*)str - 1;

    do {
        len++;
    } while (*++p);

    return len;
}

char* strcpy(char* dst, const char* src) {
    register unsigned char *destb, *fromb;
    register unsigned long w, t, align;

    fromb = (unsigned char*)src;
    destb = (unsigned char*)dst;

    if ((align = ((int)fromb & 3)) != ((int)destb & 3)) {
        goto bytecopy;
    }

    if (align) {
        if ((*destb = *fromb) == 0) {
            return dst;
        }

        for (align = 3 - align; align; align--) {
            if ((*(++destb) = *(++fromb)) == 0) {
                return dst;
            }
        }
        ++destb;
        ++fromb;
    }

    w = *((int*)(fromb));

    t = w + K2;

    t &= K1;
    if (t) {
        goto bytecopy;
    }
    --((int*)(destb));

    do {
        *(++((int*)(destb))) = w;
        w = *(++((int*)(fromb)));

        t = w + K2;
        t &= K1;
        if (t) {
            goto adjust;
        }
    } while (1);

adjust:
    ++((int*)(destb));

bytecopy:
    if ((*destb = *fromb) == 0) {
        return dst;
    }

    do {
        if ((*(++destb) = *(++fromb)) == 0) {
            return dst;
        }
    } while (1);

    return dst;
}

char* strncpy(char* dst, const char* src, size_t n) {
    const unsigned char* p = (const unsigned char*)src - 1;
    unsigned char* q = (unsigned char*)dst - 1;

    n++;
    while (--n) {
        if (!(*++q = *++p)) {
            while (--n) {
                *++q = 0;
            }
            break;
        }
    }

    return dst;
}

char* strcat(char* dst, const char* src) {
    const unsigned char* p = (unsigned char*)src - 1;
    unsigned char* q = (unsigned char*)dst - 1;

    while (*++q) {
    }

    q--;

    while (*++q = *++p) {
    }

    return dst;
}

char* strncat(char* dst, const char* src, size_t n) {
    const unsigned char* p = (unsigned char*)src - 1;
    unsigned char* q = (unsigned char*)dst - 1;

    while (*++q) {
    }

    q--;
    n++;

    while (--n) {
        if (!(*++q = *++p)) {
            q--;
            break;
        }
    }

    *++q = 0;

    return dst;
}

int strcmp(const char* str1, const char* str2) {
    register unsigned char* left = (unsigned char*)str1;
    register unsigned char* right = (unsigned char*)str2;
    unsigned long align, l1, r1, x;

    l1 = *left;
    r1 = *right;
    if (l1 - r1) {
        return l1 - r1;
    }

    if ((align = ((int)left & 3)) != ((int)right & 3)) {
        goto bytecopy;
    }

    if (align) {
        if (l1 == 0) {
            return 0;
        }
        for (align = 3 - align; align; align--) {
            l1 = *(++left);
            r1 = *(++right);
            if (l1 - r1) {
                return l1 - r1;
            }
            if (l1 == 0) {
                return 0;
            }
        }
        left++;
        right++;
    }

    l1 = *(int*)left;
    r1 = *(int*)right;
    x = l1 + K2;
    if (x & K1) {
        goto adjust;
    }

    while (l1 == r1) {
        l1 = *(++((int*)(left)));
        r1 = *(++((int*)(right)));
        x = l1 + K2;
        if (x & K1) {
            goto adjust;
        }
    }

    if (l1 > r1) {
        return 1;
    }
    return -1;

adjust:
    l1 = *left;
    r1 = *right;
    if (l1 - r1) {
        return l1 - r1;
    }

bytecopy:
    if (l1 == 0) {
        return 0;
    }

    do {
        l1 = *(++left);
        r1 = *(++right);
        if (l1 - r1) {
            return l1 - r1;
        }
        if (l1 == 0) {
            return 0;
        }
    } while (1);
}

int strncmp(const char* str1, const char* str2, size_t n) {
    const unsigned char* p1 = (unsigned char*)str1 - 1;
    const unsigned char* p2 = (unsigned char*)str2 - 1;
    unsigned long c1, c2;

    n++;
    while (--n) {
        if ((c1 = *++p1) != (c2 = *++p2)) {
            return c1 - c2;
        } else if (c1 == 0) {
            break;
        }
    }

    return 0;
}

char* strchr(const char* str, int c) {
    const unsigned char* p = (unsigned char*)str - 1;
    unsigned long chr = (c & 0xFF);

    unsigned long ch;
    while (ch = *++p) {
        if (ch == chr) {
            return (char*)p;
        }
    }

    return chr ? NULL : (char*)p;
}

char* strrchr(const char* str, int c) {
    const unsigned char* p = (unsigned char*)str - 1;
    const unsigned char* q = NULL;
    unsigned long chr = (c & 0xFF);

    unsigned long ch;
    while (ch = *++p) {
        if (ch == chr) {
            q = p;
        }
    }

    if (q != NULL) {
        return (char*)q;
    }

    return chr ? NULL : (char*)p;
}

char* strtok(char* str, const char* set) {
    unsigned char* p;
    unsigned char* q;
    unsigned char set_map[32] = {0};
    static unsigned char* empty = (unsigned char*)"";
    static unsigned char* s = (unsigned char*)"";
    int c;

    if (str)
        s = (unsigned char*)str;

    p = (unsigned char*)set - 1;
    while (c = *++p)
        set_map[(c & 0xff) >> 3] |= (1 << (c & 7));

    p = s - 1;
    while ((c = *++p) != 0)
        if (!(set_map[(c & 0xff) >> 3] & (1 << (c & 7))))
            break;

    if (!c) {
        s = empty;
        return NULL;
    }

    q = p;
    while ((c = *++p) != 0)
        if (set_map[(c & 0xff) >> 3] & (1 << (c & 7)))
            break;

    if (!c)
        s = empty;
    else {
        s = p + 1;
        *p = 0;
    }

    return (char*)q;
}

char* strstr(const char* str, const char* pat) {
    unsigned char* s1 = (unsigned char*)str - 1;
    unsigned char* p1 = (unsigned char*)pat;
    unsigned long firstc, c1, c2;

    if ((pat == NULL) || (!(firstc = *p1)))
        return (char*)str;

    while (c1 = *++s1) {
        if (c1 == firstc) {
            const unsigned char* s2 = s1 - 1;
            const unsigned char* p2 = p1 - 1;

            while ((c1 = *++s2) == (c2 = *++p2) && c1)
                ;

            if (!c2)
                return (char*)s1;
        }
    }

    return NULL;
}

// Older MSL keeps strerror here. It is dead-stripped from the game, but its
// message strings stay in this unit's string pool.
extern int sprintf(char* s, const char* format, ...);

char* __strerror(int errnum, char* str) {
    switch (errnum) {
    case 7: /* E2BIG */
        strcpy(str, "Argument list too long");
        break;
    case 13: /* EACCES */
        strcpy(str, "Permission denied");
        break;
    case 35: /* EAGAIN */
        strcpy(str, "Resource temporarily unavailable");
        break;
    case 9: /* EBADF */
        strcpy(str, "Bad file descriptor");
        break;
    case 16: /* EBUSY */
        strcpy(str, "Device busy");
        break;
    case 10: /* ECHILD */
        strcpy(str, "No child processes");
        break;
    case 11: /* EDEADLK */
        strcpy(str, "Resource deadlock avoided");
        break;
    case 33: /* EDOM */
        strcpy(str, "Numerical argument out of domain");
        break;
    case 17: /* EEXIST */
        strcpy(str, "File exists");
        break;
    case 14: /* EFAULT */
        strcpy(str, "Bad address");
        break;
    case 27: /* EFBIG */
        strcpy(str, "File too large");
        break;
    case 40: /* EFPOS */
        strcpy(str, "File Position Error");
        break;
    case 88: /* EILSEQ */
        strcpy(str, "Wide character encoding error");
        break;
    case 4: /* EINTR */
        strcpy(str, "Interrupted system call");
        break;
    case 22: /* EINVAL */
        strcpy(str, "Invalid argument");
        break;
    case 5: /* EIO */
        strcpy(str, "Input/output error");
        break;
    case 21: /* EISDIR */
        strcpy(str, "Is a directory");
        break;
    case 24: /* EMFILE */
        strcpy(str, "Too many open files");
        break;
    case 31: /* EMLINK */
        strcpy(str, "Too many links");
        break;
    case 63: /* ENAMETOOLONG */
        strcpy(str, "File name too long");
        break;
    case 23: /* ENFILE */
        strcpy(str, "Too many open files in system");
        break;
    case 19: /* ENODEV */
        strcpy(str, "Operation not supported by device");
        break;
    case 2: /* ENOENT */
        strcpy(str, "No such file or directory");
        break;
    case 0: /* ENOERR */
        strcpy(str, "No error detected");
        break;
    case 8: /* ENOEXEC */
        strcpy(str, "Exec format error");
        break;
    case 77: /* ENOLCK */
        strcpy(str, "No locks available");
        break;
    case 12: /* ENOMEM */
        strcpy(str, "Cannot allocate memory");
        break;
    case 28: /* ENOSPC */
        strcpy(str, "No space left on device");
        break;
    case 78: /* ENOSYS */
        strcpy(str, "Function not implemented");
        break;
    case 20: /* ENOTDIR */
        strcpy(str, "Not a directory");
        break;
    case 66: /* ENOTEMPTY */
        strcpy(str, "Directory not empty");
        break;
    case 25: /* ENOTTY */
        strcpy(str, "Inappropriate ioctl for device");
        break;
    case 6: /* ENXIO */
        strcpy(str, "Device not configured");
        break;
    case 1: /* EPERM */
        strcpy(str, "Operation not permitted");
        break;
    case 32: /* EPIPE */
        strcpy(str, "Broken pipe");
        break;
    case 34: /* ERANGE */
        strcpy(str, "Result too large");
        break;
    case 30: /* EROFS */
        strcpy(str, "Read-only file system");
        break;
    case 36: /* ESIGPARM */
        strcpy(str, "Signal error");
        break;
    case 29: /* ESPIPE */
        strcpy(str, "Illegal seek");
        break;
    case 3: /* ESRCH */
        strcpy(str, "No such process");
        break;
    case 99: /* EUNKNOWN */
        strcpy(str, "Unknown error");
        break;
    case 18: /* EXDEV */
        strcpy(str, "Cross-device link");
        break;
    default:
        sprintf(str, "Unknown Error (%d)", errnum);
        break;
    }

    return str;
}

char* strerror(int errnum) {
    static char errstr[100];
    return __strerror(errnum, errstr);
}
