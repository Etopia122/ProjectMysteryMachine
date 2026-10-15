#ifndef BINK_RAD_H
#define BINK_RAD_H

/* Common RAD Game Tools types for the Bink library (SN ProDG / GCC 2.95). */

typedef unsigned char U8;
typedef signed char S8;
typedef unsigned short U16;
typedef signed short S16;
typedef unsigned long U32;
typedef signed long S32;
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

#define radcntlzw(x)                                                                                   ({                                                                                                     U32 _r;                                                                                            __asm__("cntlzw %0,%1" : "=r"(_r) : "r"(x));                                                      _r;                                                                                            })

/* (hi:lo) = a * b */
#define radmul64(hi, lo, a, b) __asm__("mulhwu %0,%2,%3
	mullw %1,%2,%3" : "=&r"(hi), "=&r"(lo) : "r"(a), "r"(b))

/* (hi:lo) -= a * b */
#define radsubmul64(hi, lo, a, b)                                                                      {                                                                                                      U32 _mh, _ml;                                                                                      __asm__("mulhwu %2,%4,%5
	mullw %3,%4,%5
	subfc %1,%3,%1
	subfe %0,%2,%0"                            : "=r"(hi), "=r"(lo), "=&r"(_mh), "=&r"(_ml)                                                       : "r"(a), "r"(b), "0"(hi), "1"(lo));                                                   }

/* Inline sqrtf from the SDK math header: three Newton steps on frsqrte. */
static inline float radsqrtf(float x)
{
    if (x > 0.0f)
    {
        double guess;
        __asm__("frsqrte %0,%1" : "=f"(guess) : "f"(x));
        guess = 0.5 * guess * (3.0 - guess * guess * x);
        guess = 0.5 * guess * (3.0 - guess * guess * x);
        guess = 0.5 * guess * (3.0 - guess * guess * x);
        return (float)(x * guess);
    }
    return x;
}

/* Dolphin OS */
typedef S64 OSTime;
typedef struct OSAlarm OSAlarm;
typedef struct OSContext OSContext;
typedef void (*OSAlarmHandler)(OSAlarm* alarm, OSContext* context);
struct OSAlarm
{
    OSAlarmHandler handler;
    U32 tag;
    OSTime fire;
    OSAlarm* prev;
    OSAlarm* next;
    OSTime period;
    OSTime start;
};

#define OS_BUS_CLOCK (*(U32*)0x800000F8)
#define OS_TIMER_CLOCK (OS_BUS_CLOCK / 4)

extern volatile int __OSCurrHeap;
void* OSAllocFromHeap(int heap, U32 size);
void OSFreeToHeap(int heap, void* ptr);
OSTime OSGetTime(void);
int OSDisableInterrupts(void);
int OSEnableInterrupts(void);
int OSRestoreInterrupts(int level);
void OSSetPeriodicAlarm(OSAlarm* alarm, OSTime start, OSTime period, OSAlarmHandler handler);
void OSCancelAlarm(OSAlarm* alarm);
void OSYieldThread(void);

void* memset(void* dst, int c, U32 n);
void* memcpy(void* dst, const void* src, U32 n);

void* radmalloc(U32 bytes);
void radfree(void* ptr);

#endif
