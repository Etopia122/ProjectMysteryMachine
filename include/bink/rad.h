#ifndef BINK_RAD_H
#define BINK_RAD_H

/* Common RAD Game Tools types for the Bink library (SN ProDG / GCC 2.95). */

typedef unsigned char U8;
typedef signed char S8;
typedef unsigned short U16;
typedef signed short S16;
typedef unsigned int U32;
typedef signed int S32;
typedef unsigned long long U64;
typedef signed long long S64;
typedef float F32;
typedef double F64;

typedef void* (*RADMEMALLOC)(U32 bytes);
typedef void (*RADMEMFREE)(void* ptr);

/* Reads the 64-bit time base (upper, lower, upper again until stable). */
#define RAD_READ_TIMEBASE(v)                                                                       \
    do                                                                                             \
    {                                                                                              \
        U32 _hi, _lo, _hi2;                                                                        \
        do                                                                                         \
        {                                                                                          \
            __asm__ __volatile__("mftbu %0" : "=r"(_hi));                                          \
            __asm__ __volatile__("mftb %0" : "=r"(_lo));                                           \
            __asm__ __volatile__("mftbu %0" : "=r"(_hi2));                                         \
        } while (_hi != _hi2);                                                                     \
        ((U32*)&(v))[0] = _hi;                                                                     \
        ((U32*)&(v))[1] = _lo;                                                                     \
    } while (0)

#define radlwbrx(p, o)                                                                             \
    ({                                                                                             \
        U32 _v;                                                                                    \
        __asm__("lwbrx %0,%1,%2" : "=r"(_v) : "b"(o), "r"(p));                                     \
        _v;                                                                                        \
    })

/* Dolphin OS */
typedef struct OSHeapHandle_s* OSHeapHandleDummy;
extern volatile int __OSCurrHeap;
void* OSAllocFromHeap(int heap, U32 size);
void OSFreeToHeap(int heap, void* ptr);

#endif
