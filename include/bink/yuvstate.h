#ifndef BINK_YUVSTATE_H
#define BINK_YUVSTATE_H

/* Shared state of the Bink YUV -> RGB blitters (yuv.cpp, yuv4x2.c). */

#include "bink/rad.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Per-format function table, indexed by the blit setup. */
typedef struct BLITS BLITS;
struct BLITS
{
    U32 bpp;     /* 0x00: bytes per output pixel */
    U32 fn[31];  /* 0x04: alignment shifts and row/column workers */
};

typedef void (*BLOCKFN)(S32 n);
typedef void (*ROWFN)(U32 x, U32 n);
typedef U32 (*COLFN)(U32 n, S32 x);

/* Current source and destination pointers for the two rows being blitted. */
typedef struct YUVSTATE YUVSTATE;
struct YUVSTATE
{
    U8* d0;      /* 0x00 */
    U8* d1;      /* 0x04 */
    U8* y0;      /* 0x08 */
    U8* y1;      /* 0x0C */
    U8* u;       /* 0x10 */
    U8* v;       /* 0x14 */
    U8* a0;      /* 0x18 */
    U8* a1;      /* 0x1C */
    S32 cg;      /* 0x20: green index (from U and V) */
    S32 cb;      /* 0x24: blue index (from U) */
    S32 cr;      /* 0x28: red index (from V) */
    U32 pad[6];  /* 0x2C */
    S32 pitch;   /* 0x44 */
    U8* base;    /* 0x48 */
    U32 pad2[5]; /* 0x4C */
};

extern YUVSTATE yuvs;
extern U32 yuv_ytab[256];   /* Y -> grey 32-bit pixel */
extern U32* yuv_yclip[256]; /* Y -> clamp table entry for that Y */
/* Chroma contributions, as indexes into the clamp tables */
typedef struct YUVTAB YUVTAB;
struct YUVTAB
{
    S32 t[256];
};
typedef struct YUVTABS YUVTABS;
struct YUVTABS
{
    YUVTAB ub; /* 0x000: blue from U */
    YUVTAB vg; /* 0x400: green from V */
    YUVTAB ug; /* 0x800: green from U */
    YUVTAB vr; /* 0xC00: red from V */
};
extern YUVTABS yuvt;
extern YUVTABS yuvsave; /* unswapped copy */
extern S32 yuv_swapped;
extern U32 yuv_shift;
extern U32 yuv_f64c;
extern U32 yuv_y16[256];    /* Y -> grey 16-bit pixel (low half) */
extern U32 yuv_a16[256];    /* alpha -> 4-bit alpha bits (low half) */
extern S32 yuv_yprime[256]; /* Y -> scaled luma */
extern U32 yuv_r16[776];    /* clamped red/green/blue bits, centred at 256 */
extern U32 yuv_g16[776];
extern U32 yuv_b16[776];
extern BLITS blits32;
extern BLITS blits32a;
extern BLITS blits16;
extern BLITS blits16a4;
extern BLITS blitsYUY2;

#define YUVCOLOR()                                                                                     yuvs.cb = yuvt.ub.t[*yuvs.u];                                                                      yuvs.cg = yuvt.ug.t[*yuvs.u] + yuvt.vg.t[*yuvs.v];                                                 yuvs.cr = yuvt.vr.t[*yuvs.v];

#define RGB32(c) ((c)[yuvs.cb] | ((c)[yuvs.cg] << 8) | ((c)[yuvs.cr] << 16))

#define RGB16(c)                                                                                       ((U16)yuv_b16[(c) + yuvs.cb + 256] | (U16)yuv_g16[(c) + yuvs.cg + 256] |                            (U16)yuv_r16[(c) + yuvs.cr + 256])

#ifdef __cplusplus
}
#endif

#endif
