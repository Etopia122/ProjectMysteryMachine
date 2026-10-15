/*
 * Bink YUV -> RGB blitters (32/32a/16/16a4/YUY2 output, optional 2x zoom).
 * C++ file (mangled helpers), compiled with SN ProDG (GCC 2.95).
 */

extern "C" {
#include "bink/rad.h"
}

/* Per-format function table, indexed by the blit setup. */
struct BLITS
{
    U32 bpp;     /* 0x00: bytes per output pixel */
    U32 fn[31];  /* 0x04: alignment shifts and row/column workers */
};

typedef void (*BLOCKFN)(S32 n);
typedef void (*ROWFN)(U32 x, U32 n);
typedef U32 (*COLFN)(U32 n, S32 x);

/* Current source and destination pointers for the two rows being blitted. */
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
struct YUVTAB
{
    S32 t[256];
};
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

extern U32 zoombufsize;
extern U8* zoombuf;
extern U8* zoombuf2;

extern BLOCKFN yuv_evenfn;
extern BLOCKFN yuv_oddfn;
extern BLOCKFN yuv_zevenfn;
extern BLOCKFN yuv_zoddfn;
extern ROWFN yuv_rowfn;
extern COLFN yuv_colfn;
extern U32 yuv_align;
extern U32 yuv_alignmask;

void checkzoombufs(U32 size)
{
    U32 half = (size + 31) & ~31;
    U32 total = half * 2;

    if (total > zoombufsize)
    {
        if (zoombuf)
        {
            radfree(zoombuf);
            zoombuf = 0;
        }
        zoombuf = (U8*)radmalloc(total);
        zoombufsize = total;
    }
    zoombuf2 = zoombuf + half;
}

void zoom2heven(S32 n)
{
    U8* d0 = yuvs.d0;
    U8* d1 = yuvs.d1;
    U8* y0 = yuvs.y0;
    U8* y1 = yuvs.y1;
    U8* a0 = yuvs.a0;
    U8* a1 = yuvs.a1;
    U8* u = yuvs.u;
    U8* v = yuvs.v;
    U32 adv;

    yuv_zevenfn(n);
    yuvs.d0 = d0 + yuvs.pitch;
    adv = yuvs.d1 - d1;
    yuvs.d1 = d1 + yuvs.pitch;
    yuvs.y0 = y0;
    yuvs.y1 = y1;
    yuvs.a0 = a0;
    yuvs.a1 = a1;
    yuvs.u = u;
    yuvs.v = v;
    yuv_zevenfn(n);
    yuvs.d0 = d0 + adv;
    yuvs.d1 = d1 + adv;
}

void zoom2hodd(S32 n)
{
    U8* d0 = yuvs.d0;
    U8* d1 = yuvs.d1;
    U8* y0 = yuvs.y0;
    U8* y1 = yuvs.y1;
    U8* a0 = yuvs.a0;
    U8* a1 = yuvs.a1;
    U8* u = yuvs.u;
    U8* v = yuvs.v;
    U32 adv;

    yuv_zoddfn(n);
    yuvs.d0 = d0 + yuvs.pitch;
    adv = yuvs.d1 - d1;
    yuvs.d1 = d1 + yuvs.pitch;
    yuvs.y0 = y0;
    yuvs.y1 = y1;
    yuvs.a0 = a0;
    yuvs.a1 = a1;
    yuvs.u = u;
    yuvs.v = v;
    yuv_zoddfn(n);
    yuvs.d0 = d0 + adv;
    yuvs.d1 = d1 + adv;
}

void dounaligned32rowm2w(U32 x, U32 n)
{
    while (n--)
    {
        U32 c = yuv_ytab[*yuvs.y0++];
        ((U32*)yuvs.d0)[0] = c;
        ((U32*)yuvs.d0)[1] = c;
        yuvs.d0 += 8;
    }
}

U32 dounaligned32colm2w(U32 n, S32 x)
{
    U32 i = n;
    do
    {
        U32 c = yuv_ytab[*yuvs.y0++];
        ((U32*)yuvs.d0)[0] = c;
        ((U32*)yuvs.d0)[1] = c;
        c = yuv_ytab[*yuvs.y1++];
        ((U32*)yuvs.d1)[0] = c;
        ((U32*)yuvs.d1)[1] = c;
        yuvs.d0 += 8;
        yuvs.d1 += 8;
    } while (--i);
    return x + n;
}

void dounaligned32rowm2h(U32 x, U32 n)
{
    while (n--)
    {
        U32 c = yuv_ytab[*yuvs.y0++];
        ((U32*)yuvs.d0)[0] = c;
        *(U32*)(yuvs.d0 + yuvs.pitch) = c;
        yuvs.d0 += 4;
    }
}

U32 dounaligned32colm2h(U32 n, S32 x)
{
    U32 i = n;
    do
    {
        U32 c = yuv_ytab[*yuvs.y0++];
        ((U32*)yuvs.d0)[0] = c;
        *(U32*)(yuvs.d0 + yuvs.pitch) = c;
        c = yuv_ytab[*yuvs.y1++];
        ((U32*)yuvs.d1)[0] = c;
        *(U32*)(yuvs.d1 + yuvs.pitch) = c;
        yuvs.d0 += 4;
        yuvs.d1 += 4;
    } while (--i);
    return x + n;
}

void dounaligned32rowm2wh(U32 x, U32 n)
{
    while (n--)
    {
        U32 c = yuv_ytab[*yuvs.y0++];
        ((U32*)yuvs.d0)[0] = c;
        ((U32*)yuvs.d0)[1] = c;
        ((U32*)(yuvs.d0 + yuvs.pitch))[0] = c;
        ((U32*)(yuvs.d0 + yuvs.pitch))[1] = c;
        yuvs.d0 += 8;
    }
}

U32 dounaligned32colm2wh(U32 n, S32 x)
{
    U32 i = n;
    do
    {
        U32 c = yuv_ytab[*yuvs.y0++];
        ((U32*)yuvs.d0)[0] = c;
        ((U32*)yuvs.d0)[1] = c;
        ((U32*)(yuvs.d0 + yuvs.pitch))[0] = c;
        ((U32*)(yuvs.d0 + yuvs.pitch))[1] = c;
        c = yuv_ytab[*yuvs.y1++];
        ((U32*)yuvs.d1)[0] = c;
        ((U32*)yuvs.d1)[1] = c;
        ((U32*)(yuvs.d1 + yuvs.pitch))[0] = c;
        ((U32*)(yuvs.d1 + yuvs.pitch))[1] = c;
        yuvs.d0 += 8;
        yuvs.d1 += 8;
    } while (--i);
    return x + n;
}

void dounaligned32row2w(U32 x, U32 n)
{
    while (n--)
    {
        U32* c;
        U32 p;
        x++;
        YUVCOLOR();
        c = yuv_yclip[*yuvs.y0++];
        p = RGB32(c);
        ((U32*)yuvs.d0)[0] = p;
        ((U32*)yuvs.d0)[1] = p;
        yuvs.d0 += 8;
        if (!(x & 1))
        {
            yuvs.u++;
            yuvs.v++;
        }
    }
}

U32 dounaligned32col2w(U32 n, S32 x)
{
    U32 i = n;
    do
    {
        U32* c;
        U32 p;
        x++;
        YUVCOLOR();
        c = yuv_yclip[*yuvs.y0++];
        p = RGB32(c);
        ((U32*)yuvs.d0)[0] = p;
        ((U32*)yuvs.d0)[1] = p;
        c = yuv_yclip[*yuvs.y1++];
        p = RGB32(c);
        ((U32*)yuvs.d1)[0] = p;
        ((U32*)yuvs.d1)[1] = p;
        yuvs.d0 += 8;
        yuvs.d1 += 8;
        if (!(x & 1))
        {
            yuvs.u++;
            yuvs.v++;
        }
    } while (--i);
    return x;
}

void dounaligned32row2h(U32 x, U32 n)
{
    while (n--)
    {
        U32* c;
        U32 p;
        x++;
        YUVCOLOR();
        c = yuv_yclip[*yuvs.y0++];
        p = RGB32(c);
        ((U32*)yuvs.d0)[0] = p;
        *(U32*)(yuvs.d0 + yuvs.pitch) = p;
        yuvs.d0 += 4;
        if (!(x & 1))
        {
            yuvs.u++;
            yuvs.v++;
        }
    }
}

U32 dounaligned32col2h(U32 n, S32 x)
{
    U32 i = n;
    do
    {
        U32* c;
        U32 p;
        x++;
        YUVCOLOR();
        c = yuv_yclip[*yuvs.y0++];
        p = RGB32(c);
        ((U32*)yuvs.d0)[0] = p;
        *(U32*)(yuvs.d0 + yuvs.pitch) = p;
        c = yuv_yclip[*yuvs.y1++];
        p = RGB32(c);
        ((U32*)yuvs.d1)[0] = p;
        *(U32*)(yuvs.d1 + yuvs.pitch) = p;
        yuvs.d0 += 4;
        yuvs.d1 += 4;
        if (!(x & 1))
        {
            yuvs.u++;
            yuvs.v++;
        }
    } while (--i);
    return x;
}

void dounaligned32row2wh(U32 x, U32 n)
{
    while (n--)
    {
        U32* c;
        U32 p;
        x++;
        YUVCOLOR();
        c = yuv_yclip[*yuvs.y0++];
        p = RGB32(c);
        ((U32*)yuvs.d0)[0] = p;
        ((U32*)yuvs.d0)[1] = p;
        ((U32*)(yuvs.d0 + yuvs.pitch))[0] = p;
        ((U32*)(yuvs.d0 + yuvs.pitch))[1] = p;
        yuvs.d0 += 8;
        if (!(x & 1))
        {
            yuvs.u++;
            yuvs.v++;
        }
    }
}

U32 dounaligned32col2wh(U32 n, S32 x)
{
    U32 i = n;
    do
    {
        U32* c;
        U32 p;
        x++;
        YUVCOLOR();
        c = yuv_yclip[*yuvs.y0++];
        p = RGB32(c);
        ((U32*)yuvs.d0)[0] = p;
        ((U32*)yuvs.d0)[1] = p;
        ((U32*)(yuvs.d0 + yuvs.pitch))[0] = p;
        ((U32*)(yuvs.d0 + yuvs.pitch))[1] = p;
        c = yuv_yclip[*yuvs.y1++];
        p = RGB32(c);
        ((U32*)yuvs.d1)[0] = p;
        ((U32*)yuvs.d1)[1] = p;
        ((U32*)(yuvs.d1 + yuvs.pitch))[0] = p;
        ((U32*)(yuvs.d1 + yuvs.pitch))[1] = p;
        yuvs.d0 += 8;
        yuvs.d1 += 8;
        if (!(x & 1))
        {
            yuvs.u++;
            yuvs.v++;
        }
    } while (--i);
    return x;
}

void dounaligned32rowm(U32 x, U32 n)
{
    while (n--)
    {
        *(U32*)yuvs.d0 = yuv_ytab[*yuvs.y0++];
        yuvs.d0 += 4;
    }
}

U32 dounaligned32colm(U32 n, S32 x)
{
    U32 i = n;
    do
    {
        *(U32*)yuvs.d0 = yuv_ytab[*yuvs.y0++];
        *(U32*)yuvs.d1 = yuv_ytab[*yuvs.y1++];
        yuvs.d0 += 4;
        yuvs.d1 += 4;
    } while (--i);
    return x + n;
}

void dounaligned32row(U32 x, U32 n)
{
    while (n--)
    {
        U32* c;
        x++;
        YUVCOLOR();
        c = yuv_yclip[*yuvs.y0++];
        ((U32*)yuvs.d0)[0] = RGB32(c);
        yuvs.d0 += 4;
        if (!(x & 1))
        {
            yuvs.u++;
            yuvs.v++;
        }
    }
}

U32 dounaligned32col(U32 n, S32 x)
{
    U32 i = n;
    do
    {
        U32* c;
        x++;
        YUVCOLOR();
        c = yuv_yclip[*yuvs.y0++];
        ((U32*)yuvs.d0)[0] = RGB32(c);
        c = yuv_yclip[*yuvs.y1++];
        ((U32*)yuvs.d1)[0] = RGB32(c);
        yuvs.d0 += 4;
        yuvs.d1 += 4;
        if (!(x & 1))
        {
            yuvs.u++;
            yuvs.v++;
        }
    } while (--i);
    return x;
}


void YUV_blit_mask(void* dst, U32 dx, U32 dy, U32 pitch, U8* mask, U32 maskpitch, void* src, U32 sx,
                              U32 sy, U32 w, U32 h, U32 stride, U32 uvy, U32 flags, void* alpha, BLITS* b);
void YUV_blit(void* dst, U32 dx, U32 dy, U32 pitch, void* src, U32 sx, U32 sy, U32 w, U32 h, U32 stride,
                         U32 uvy, U32 flags, void* alpha, BLITS* b);

#define BF(k) (b->fn[(k) - 1])

void setevenodd(U32 flags, U32* dpitch, U32 w, U32 h, BLITS* b, U32* adv)
{
    U32 m, a, sh;

    yuv_f64c = 0;
    yuvs.pitch = *dpitch;
    if (flags & 0x10000)
    {
        if (yuv_swapped != 1)
        {
            yuv_swapped = 1;
            yuvt.vr = yuvsave.ub;
            yuvt.vg = yuvsave.ug;
            yuvt.ug = yuvsave.vg;
            yuvt.ub = yuvsave.vr;
        }
    }
    else if (yuv_swapped != 0)
    {
        yuv_swapped = 0;
        yuvt = yuvsave;
    }

    m = flags & 0x70000000;
    if (m == 0x10000000)
    {
        checkzoombufs(w * b->bpp);
        *dpitch *= 2;
        *adv = *dpitch - w * b->bpp;
        if (flags & 0x20000)
        {
            yuv_evenfn = zoom2heven;
            yuv_oddfn = zoom2hodd;
            yuv_rowfn = (ROWFN)BF(0x17);
            yuv_colfn = (COLFN)BF(0x18);
            yuv_zoddfn = (BLOCKFN)BF(6);
            yuv_shift = BF(5);
            yuv_zevenfn = (BLOCKFN)BF(6);
        }
        else
        {
            sh = BF(3);
            a = BF(1);
            yuv_evenfn = zoom2heven;
            yuv_oddfn = zoom2hodd;
            yuv_rowfn = (ROWFN)BF(0x15);
            yuv_colfn = (COLFN)BF(0x16);
            yuv_zevenfn = (BLOCKFN)BF(2);
            yuv_zoddfn = (BLOCKFN)BF(4);
            if (sh < a)
                sh = a;
            yuv_shift = sh;
        }
    }
    else if (m == 0x30000000 || m == 0x50000000)
    {
        *adv = *dpitch - w * b->bpp * 2;
        if (flags & 0x20000)
        {
            yuv_rowfn = (ROWFN)BF(0x13);
            yuv_colfn = (COLFN)BF(0x14);
            yuv_oddfn = (BLOCKFN)BF(0xC);
            yuv_shift = BF(0xB);
            yuv_evenfn = (BLOCKFN)BF(0xC);
        }
        else
        {
            sh = BF(9);
            a = BF(7);
            yuv_rowfn = (ROWFN)BF(0x11);
            yuv_colfn = (COLFN)BF(0x12);
            yuv_evenfn = (BLOCKFN)BF(8);
            yuv_oddfn = (BLOCKFN)BF(10);
            if (sh < a)
                sh = a;
            yuv_shift = sh;
        }
    }
    else if (m == 0x40000000)
    {
        checkzoombufs(w * b->bpp * 2);
        *dpitch *= 2;
        *adv = *dpitch - w * b->bpp * 2;
        if (flags & 0x20000)
        {
            yuv_evenfn = zoom2heven;
            yuv_oddfn = zoom2hodd;
            yuv_rowfn = (ROWFN)BF(0x1B);
            yuv_colfn = (COLFN)BF(0x1C);
            yuv_zoddfn = (BLOCKFN)BF(0xC);
            yuv_shift = BF(0xB);
            yuv_zevenfn = (BLOCKFN)BF(0xC);
        }
        else
        {
            sh = BF(9);
            a = BF(7);
            yuv_evenfn = zoom2heven;
            yuv_oddfn = zoom2hodd;
            yuv_rowfn = (ROWFN)BF(0x19);
            yuv_colfn = (COLFN)BF(0x1A);
            yuv_zevenfn = (BLOCKFN)BF(8);
            yuv_zoddfn = (BLOCKFN)BF(10);
            if (sh < a)
                sh = a;
            yuv_shift = sh;
        }
    }
    else
    {
        *adv = *dpitch - w * b->bpp;
        if (flags & 0x20000)
        {
            yuv_rowfn = (ROWFN)BF(0xF);
            yuv_colfn = (COLFN)BF(0x10);
            yuv_oddfn = (BLOCKFN)BF(6);
            yuv_shift = BF(5);
            yuv_evenfn = (BLOCKFN)BF(6);
        }
        else
        {
            sh = BF(3);
            a = BF(1);
            yuv_rowfn = (ROWFN)BF(0xD);
            yuv_colfn = (COLFN)BF(0xE);
            yuv_evenfn = (BLOCKFN)BF(2);
            yuv_oddfn = (BLOCKFN)BF(4);
            if (sh < a)
                sh = a;
            yuv_shift = sh;
        }
    }
    yuv_align = 1 << yuv_shift;
    yuv_alignmask = yuv_align - 1;
}

void YUV_blit(void* dst, U32 dx, U32 dy, U32 pitch, void* src, U32 sx, U32 sy, U32 w, U32 h, U32 stride, U32 uvy,
              U32 flags, void* alpha, BLITS* b)
{
    U32 dpitch = pitch;
    U32 adv;
    U32 half, last, lead;
    S32 uvadv, yadv, t;

    if ((flags & 0x70000000) == 0x60000000)
    {
        dpitch = pitch * 2;
        sy >>= 1;
        h >>= 1;
        uvy >>= 1;
        stride *= 2;
    }
    yuvs.base = (U8*)dst;

    if (b->bpp == 2)
    {
        if (((U32)dst & 3) == 2)
        {
            dx++;
            dst = (void*)((U32)dst & ~3);
        }
    }
    else if (b->bpp == 3)
    {
        if ((((U32)dst - 3) & 3) == 0)
        {
            dx++;
            dst = (U8*)dst - 3;
        }
        else if ((((U32)dst - 6) & 3) == 0)
        {
            dx += 2;
            dst = (U8*)dst - 6;
        }
        else if ((((U32)dst - 9) & 3) == 0)
        {
            dx += 3;
            dst = (U8*)dst - 9;
        }
    }

    yuvs.d0 = (U8*)dst + dy * pitch + dx * b->bpp;
    if ((flags & 0x70000000) == 0x20000000 || (flags & 0x70000000) == 0x50000000)
        dpitch *= 2;
    setevenodd(flags, &dpitch, w, h, b, &adv);

    yuvs.a0 = (U8*)alpha + sx + sy * stride;
    yuvs.y0 = (U8*)src + sx + sy * stride;
    yuvs.d1 = yuvs.d0 + dpitch;
    half = stride >> 1;
    yuvs.y1 = yuvs.y0 + stride;
    yuvs.a1 = yuvs.a0 + stride;
    uvadv = half - (w >> 1);
    yadv = stride - w;
    if ((w & 1) && (sx & 1))
        uvadv--;

    if (flags & 0x10000)
    {
        yuvs.u = (U8*)src + stride * uvy;
        yuvs.v = yuvs.u + half * (uvy >> 1);
    }
    else
    {
        yuvs.v = (U8*)src + stride * uvy;
        yuvs.u = yuvs.v + half * (uvy >> 1);
    }
    t = (sy >> 1) * half;
    yuvs.u += (sx >> 1) + t;
    yuvs.v += (sx >> 1) + t;

    last = sy + h - 1;
    if ((sy & 1) && (S32)sy <= (S32)last)
    {
        sy++;
        yuv_rowfn(sx, w);
        yuvs.d0 += adv;
        yuvs.y0 += yadv;
        yuvs.a0 += yadv;
        yuvs.y1 = yuvs.y0 + stride;
        yuvs.a1 = yuvs.a0 + stride;
        yuvs.u += uvadv;
        yuvs.v += uvadv;
        yuvs.d1 = yuvs.d0 + dpitch;
    }

    lead = (4 - (dx & 3)) & 3;
    if (lead > w)
        lead = w;

    while ((S32)sy < (S32)last)
    {
        U32 x = sx;
        U32 n = w;
        U32 cnt, rem;

        if (lead)
        {
            n = w - lead;
            x = yuv_colfn(lead, sx);
        }
        cnt = (n & ~yuv_alignmask) >> 2;
        rem = n - cnt * 4;
        if (cnt)
        {
            if (x & 1)
                yuv_oddfn(cnt);
            else
                yuv_evenfn(cnt);
        }
        x += cnt * 4;
        if (rem)
            yuv_colfn(rem, x);

        sy += 2;
        yuvs.y0 = yuvs.y1 + yadv;
        yuvs.a0 = yuvs.a1 + yadv;
        yuvs.d0 = yuvs.d1 + adv;
        yuvs.y1 = yuvs.y0 + stride;
        yuvs.a1 = yuvs.a0 + stride;
        yuvs.u += uvadv;
        yuvs.v += uvadv;
        yuvs.d1 = yuvs.d0 + dpitch;
    }
    if ((S32)sy <= (S32)last)
        yuv_rowfn(sx, w);
}

extern "C" void YUV_blit_32bpp(void* dst, U32 dx, U32 dy, U32 pitch, void* src, U32 sx, U32 sy, U32 w, U32 h, U32 stride, U32 uvy, U32 flags)
{
    YUV_blit(dst, dx, dy, pitch, src, sx, sy, w, h, stride, uvy, flags, 0, &blits32);
}

void dounaligned32arowm2w(U32 x, U32 n)
{
    while (n--)
    {
        U32 c = yuv_ytab[*yuvs.y0++] | (*yuvs.a0++ << 24);
        ((U32*)yuvs.d0)[0] = c;
        ((U32*)yuvs.d0)[1] = c;
        yuvs.d0 += 8;
    }
}

U32 dounaligned32acolm2w(U32 n, S32 x)
{
    U32 i = n;
    do
    {
        U32 c = yuv_ytab[*yuvs.y0++] | (*yuvs.a0++ << 24);
        ((U32*)yuvs.d0)[0] = c;
        ((U32*)yuvs.d0)[1] = c;
        c = yuv_ytab[*yuvs.y1++] | (*yuvs.a1++ << 24);
        ((U32*)yuvs.d1)[0] = c;
        ((U32*)yuvs.d1)[1] = c;
        yuvs.d0 += 8;
        yuvs.d1 += 8;
    } while (--i);
    return x + n;
}

void dounaligned32arowm2h(U32 x, U32 n)
{
    while (n--)
    {
        U32 c = yuv_ytab[*yuvs.y0++] | (*yuvs.a0++ << 24);
        ((U32*)yuvs.d0)[0] = c;
        *(U32*)(yuvs.d0 + yuvs.pitch) = c;
        yuvs.d0 += 4;
    }
}

U32 dounaligned32acolm2h(U32 n, S32 x)
{
    U32 i = n;
    do
    {
        U32 c = yuv_ytab[*yuvs.y0++] | (*yuvs.a0++ << 24);
        ((U32*)yuvs.d0)[0] = c;
        *(U32*)(yuvs.d0 + yuvs.pitch) = c;
        c = yuv_ytab[*yuvs.y1++] | (*yuvs.a1++ << 24);
        ((U32*)yuvs.d1)[0] = c;
        *(U32*)(yuvs.d1 + yuvs.pitch) = c;
        yuvs.d0 += 4;
        yuvs.d1 += 4;
    } while (--i);
    return x + n;
}

void dounaligned32arowm2wh(U32 x, U32 n)
{
    while (n--)
    {
        U32 c = yuv_ytab[*yuvs.y0++] | (*yuvs.a0++ << 24);
        ((U32*)yuvs.d0)[0] = c;
        ((U32*)yuvs.d0)[1] = c;
        ((U32*)(yuvs.d0 + yuvs.pitch))[0] = c;
        ((U32*)(yuvs.d0 + yuvs.pitch))[1] = c;
        yuvs.d0 += 8;
    }
}

U32 dounaligned32acolm2wh(U32 n, S32 x)
{
    U32 i = n;
    do
    {
        U32 c = yuv_ytab[*yuvs.y0++] | (*yuvs.a0++ << 24);
        ((U32*)yuvs.d0)[0] = c;
        ((U32*)yuvs.d0)[1] = c;
        ((U32*)(yuvs.d0 + yuvs.pitch))[0] = c;
        ((U32*)(yuvs.d0 + yuvs.pitch))[1] = c;
        c = yuv_ytab[*yuvs.y1++] | (*yuvs.a1++ << 24);
        ((U32*)yuvs.d1)[0] = c;
        ((U32*)yuvs.d1)[1] = c;
        ((U32*)(yuvs.d1 + yuvs.pitch))[0] = c;
        ((U32*)(yuvs.d1 + yuvs.pitch))[1] = c;
        yuvs.d0 += 8;
        yuvs.d1 += 8;
    } while (--i);
    return x + n;
}

void dounaligned32arow2w(U32 x, U32 n)
{
    while (n--)
    {
        U32* c;
        U32 p;
        x++;
        YUVCOLOR();
        c = yuv_yclip[*yuvs.y0++];
        p = RGB32(c) | (*yuvs.a0++ << 24);
        ((U32*)yuvs.d0)[0] = p;
        ((U32*)yuvs.d0)[1] = p;
        yuvs.d0 += 8;
        if (!(x & 1))
        {
            yuvs.u++;
            yuvs.v++;
        }
    }
}

U32 dounaligned32acol2w(U32 n, S32 x)
{
    U32 i = n;
    do
    {
        U32* c;
        U32 p;
        x++;
        YUVCOLOR();
        c = yuv_yclip[*yuvs.y0++];
        p = RGB32(c) | (*yuvs.a0++ << 24);
        ((U32*)yuvs.d0)[0] = p;
        ((U32*)yuvs.d0)[1] = p;
        c = yuv_yclip[*yuvs.y1++];
        p = RGB32(c) | (*yuvs.a1++ << 24);
        ((U32*)yuvs.d1)[0] = p;
        ((U32*)yuvs.d1)[1] = p;
        yuvs.d0 += 8;
        yuvs.d1 += 8;
        if (!(x & 1))
        {
            yuvs.u++;
            yuvs.v++;
        }
    } while (--i);
    return x;
}

void dounaligned32arow2h(U32 x, U32 n)
{
    while (n--)
    {
        U32* c;
        U32 p;
        x++;
        YUVCOLOR();
        c = yuv_yclip[*yuvs.y0++];
        p = RGB32(c) | (*yuvs.a0++ << 24);
        ((U32*)yuvs.d0)[0] = p;
        *(U32*)(yuvs.d0 + yuvs.pitch) = p;
        yuvs.d0 += 4;
        if (!(x & 1))
        {
            yuvs.u++;
            yuvs.v++;
        }
    }
}

U32 dounaligned32acol2h(U32 n, S32 x)
{
    U32 i = n;
    do
    {
        U32* c;
        U32 p;
        x++;
        YUVCOLOR();
        c = yuv_yclip[*yuvs.y0++];
        p = RGB32(c) | (*yuvs.a0++ << 24);
        ((U32*)yuvs.d0)[0] = p;
        *(U32*)(yuvs.d0 + yuvs.pitch) = p;
        c = yuv_yclip[*yuvs.y1++];
        p = RGB32(c) | (*yuvs.a1++ << 24);
        ((U32*)yuvs.d1)[0] = p;
        *(U32*)(yuvs.d1 + yuvs.pitch) = p;
        yuvs.d0 += 4;
        yuvs.d1 += 4;
        if (!(x & 1))
        {
            yuvs.u++;
            yuvs.v++;
        }
    } while (--i);
    return x;
}

void dounaligned32arow2wh(U32 x, U32 n)
{
    while (n--)
    {
        U32* c;
        U32 p;
        x++;
        YUVCOLOR();
        c = yuv_yclip[*yuvs.y0++];
        p = RGB32(c) | (*yuvs.a0++ << 24);
        ((U32*)yuvs.d0)[0] = p;
        ((U32*)yuvs.d0)[1] = p;
        ((U32*)(yuvs.d0 + yuvs.pitch))[0] = p;
        ((U32*)(yuvs.d0 + yuvs.pitch))[1] = p;
        yuvs.d0 += 8;
        if (!(x & 1))
        {
            yuvs.u++;
            yuvs.v++;
        }
    }
}

U32 dounaligned32acol2wh(U32 n, S32 x)
{
    U32 i = n;
    do
    {
        U32* c;
        U32 p;
        x++;
        YUVCOLOR();
        c = yuv_yclip[*yuvs.y0++];
        p = RGB32(c) | (*yuvs.a0++ << 24);
        ((U32*)yuvs.d0)[0] = p;
        ((U32*)yuvs.d0)[1] = p;
        ((U32*)(yuvs.d0 + yuvs.pitch))[0] = p;
        ((U32*)(yuvs.d0 + yuvs.pitch))[1] = p;
        c = yuv_yclip[*yuvs.y1++];
        p = RGB32(c) | (*yuvs.a1++ << 24);
        ((U32*)yuvs.d1)[0] = p;
        ((U32*)yuvs.d1)[1] = p;
        ((U32*)(yuvs.d1 + yuvs.pitch))[0] = p;
        ((U32*)(yuvs.d1 + yuvs.pitch))[1] = p;
        yuvs.d0 += 8;
        yuvs.d1 += 8;
        if (!(x & 1))
        {
            yuvs.u++;
            yuvs.v++;
        }
    } while (--i);
    return x;
}

void dounaligned32arowm(U32 x, U32 n)
{
    while (n--)
    {
        *(U32*)yuvs.d0 = yuv_ytab[*yuvs.y0++] | (*yuvs.a0++ << 24);
        yuvs.d0 += 4;
    }
}

U32 dounaligned32acolm(U32 n, S32 x)
{
    U32 i = n;
    do
    {
        *(U32*)yuvs.d0 = yuv_ytab[*yuvs.y0++] | (*yuvs.a0++ << 24);
        *(U32*)yuvs.d1 = yuv_ytab[*yuvs.y1++] | (*yuvs.a1++ << 24);
        yuvs.d0 += 4;
        yuvs.d1 += 4;
    } while (--i);
    return x + n;
}

void dounaligned32arow(U32 x, U32 n)
{
    while (n--)
    {
        U32* c;
        x++;
        YUVCOLOR();
        c = yuv_yclip[*yuvs.y0++];
        *(U32*)yuvs.d0 = RGB32(c) | (*yuvs.a0++ << 24);
        yuvs.d0 += 4;
        if (!(x & 1))
        {
            yuvs.u++;
            yuvs.v++;
        }
    }
}

U32 dounaligned32acol(U32 n, S32 x)
{
    U32 i = n;
    do
    {
        U32* c;
        x++;
        YUVCOLOR();
        c = yuv_yclip[*yuvs.y0++];
        *(U32*)yuvs.d0 = RGB32(c) | (*yuvs.a0++ << 24);
        c = yuv_yclip[*yuvs.y1++];
        *(U32*)yuvs.d1 = RGB32(c) | (*yuvs.a1++ << 24);
        yuvs.d0 += 4;
        yuvs.d1 += 4;
        if (!(x & 1))
        {
            yuvs.u++;
            yuvs.v++;
        }
    } while (--i);
    return x;
}


extern "C" void YUV_blit_32abpp(void* dst, U32 dx, U32 dy, U32 pitch, void* src, U32 sx, U32 sy, U32 w, U32 h, U32 stride,
                                U32 uvy, void* alpha, U32 flags)
{
    YUV_blit(dst, dx, dy, pitch, src, sx, sy, w, h, stride, uvy, flags, alpha, &blits32a);
}

void dounaligned16rowm2h(U32 x, U32 n)
{
    while (n--)
    {
        U16 c = (U16)yuv_y16[*yuvs.y0++];
        ((U16*)yuvs.d0)[0] = c;
        *(U16*)(yuvs.d0 + yuvs.pitch) = c;
        yuvs.d0 += 2;
    }
}

U32 dounaligned16colm2h(U32 n, S32 x)
{
    U32 i = n;
    do
    {
        U16 c = (U16)yuv_y16[*yuvs.y0++];
        ((U16*)yuvs.d0)[0] = c;
        *(U16*)(yuvs.d0 + yuvs.pitch) = c;
        c = (U16)yuv_y16[*yuvs.y1++];
        ((U16*)yuvs.d1)[0] = c;
        *(U16*)(yuvs.d1 + yuvs.pitch) = c;
        yuvs.d0 += 2;
        yuvs.d1 += 2;
    } while (--i);
    return x + n;
}

void dounaligned16rowm2w(U32 x, U32 n)
{
    while (n--)
    {
        U16 c = (U16)yuv_y16[*yuvs.y0++];
        ((U16*)yuvs.d0)[0] = c;
        ((U16*)yuvs.d0)[1] = c;
        yuvs.d0 += 4;
    }
}

U32 dounaligned16colm2w(U32 n, S32 x)
{
    U32 i = n;
    do
    {
        U16 c = (U16)yuv_y16[*yuvs.y0++];
        ((U16*)yuvs.d0)[0] = c;
        ((U16*)yuvs.d0)[1] = c;
        c = (U16)yuv_y16[*yuvs.y1++];
        ((U16*)yuvs.d1)[0] = c;
        ((U16*)yuvs.d1)[1] = c;
        yuvs.d0 += 4;
        yuvs.d1 += 4;
    } while (--i);
    return x + n;
}

void dounaligned16rowm2wh(U32 x, U32 n)
{
    while (n--)
    {
        U16 c = (U16)yuv_y16[*yuvs.y0++];
        ((U16*)yuvs.d0)[0] = c;
        ((U16*)yuvs.d0)[1] = c;
        ((U16*)(yuvs.d0 + yuvs.pitch))[0] = c;
        ((U16*)(yuvs.d0 + yuvs.pitch))[1] = c;
        yuvs.d0 += 4;
    }
}

U32 dounaligned16colm2wh(U32 n, S32 x)
{
    U32 i = n;
    do
    {
        U16 c = (U16)yuv_y16[*yuvs.y0++];
        ((U16*)yuvs.d0)[0] = c;
        ((U16*)yuvs.d0)[1] = c;
        ((U16*)(yuvs.d0 + yuvs.pitch))[0] = c;
        ((U16*)(yuvs.d0 + yuvs.pitch))[1] = c;
        c = (U16)yuv_y16[*yuvs.y1++];
        ((U16*)yuvs.d1)[0] = c;
        ((U16*)yuvs.d1)[1] = c;
        ((U16*)(yuvs.d1 + yuvs.pitch))[0] = c;
        ((U16*)(yuvs.d1 + yuvs.pitch))[1] = c;
        yuvs.d0 += 4;
        yuvs.d1 += 4;
    } while (--i);
    return x + n;
}

void dounaligned16row2h(U32 x, U32 n)
{
    while (n--)
    {
        S32 c;
        U16 p;
        x++;
        YUVCOLOR();
        c = yuv_yprime[*yuvs.y0++];
        p = RGB16(c);
        ((U16*)yuvs.d0)[0] = p;
        *(U16*)(yuvs.d0 + yuvs.pitch) = p;
        yuvs.d0 += 2;
        if (!(x & 1))
        {
            yuvs.u++;
            yuvs.v++;
        }
    }
}

U32 dounaligned16col2h(U32 n, S32 x)
{
    U32 i = n;
    do
    {
        S32 c;
        U16 p;
        x++;
        YUVCOLOR();
        c = yuv_yprime[*yuvs.y0++];
        p = RGB16(c);
        ((U16*)yuvs.d0)[0] = p;
        *(U16*)(yuvs.d0 + yuvs.pitch) = p;
        c = yuv_yprime[*yuvs.y1++];
        p = RGB16(c);
        ((U16*)yuvs.d1)[0] = p;
        *(U16*)(yuvs.d1 + yuvs.pitch) = p;
        yuvs.d0 += 2;
        yuvs.d1 += 2;
        if (!(x & 1))
        {
            yuvs.u++;
            yuvs.v++;
        }
    } while (--i);
    return x;
}

void dounaligned16row2w(U32 x, U32 n)
{
    while (n--)
    {
        S32 c;
        U16 p;
        x++;
        YUVCOLOR();
        c = yuv_yprime[*yuvs.y0++];
        p = RGB16(c);
        ((U16*)yuvs.d0)[0] = p;
        ((U16*)yuvs.d0)[1] = p;
        yuvs.d0 += 4;
        if (!(x & 1))
        {
            yuvs.u++;
            yuvs.v++;
        }
    }
}

U32 dounaligned16col2w(U32 n, S32 x)
{
    U32 i = n;
    do
    {
        S32 c;
        U16 p;
        x++;
        YUVCOLOR();
        c = yuv_yprime[*yuvs.y0++];
        p = RGB16(c);
        ((U16*)yuvs.d0)[0] = p;
        ((U16*)yuvs.d0)[1] = p;
        c = yuv_yprime[*yuvs.y1++];
        p = RGB16(c);
        ((U16*)yuvs.d1)[0] = p;
        ((U16*)yuvs.d1)[1] = p;
        yuvs.d0 += 4;
        yuvs.d1 += 4;
        if (!(x & 1))
        {
            yuvs.u++;
            yuvs.v++;
        }
    } while (--i);
    return x;
}

void dounaligned16row2wh(U32 x, U32 n)
{
    while (n--)
    {
        S32 c;
        U16 p;
        x++;
        YUVCOLOR();
        c = yuv_yprime[*yuvs.y0++];
        p = RGB16(c);
        ((U16*)yuvs.d0)[0] = p;
        ((U16*)yuvs.d0)[1] = p;
        ((U16*)(yuvs.d0 + yuvs.pitch))[0] = p;
        ((U16*)(yuvs.d0 + yuvs.pitch))[1] = p;
        yuvs.d0 += 4;
        if (!(x & 1))
        {
            yuvs.u++;
            yuvs.v++;
        }
    }
}

U32 dounaligned16col2wh(U32 n, S32 x)
{
    U32 i = n;
    do
    {
        S32 c;
        U16 p;
        x++;
        YUVCOLOR();
        c = yuv_yprime[*yuvs.y0++];
        p = RGB16(c);
        ((U16*)yuvs.d0)[0] = p;
        ((U16*)yuvs.d0)[1] = p;
        ((U16*)(yuvs.d0 + yuvs.pitch))[0] = p;
        ((U16*)(yuvs.d0 + yuvs.pitch))[1] = p;
        c = yuv_yprime[*yuvs.y1++];
        p = RGB16(c);
        ((U16*)yuvs.d1)[0] = p;
        ((U16*)yuvs.d1)[1] = p;
        ((U16*)(yuvs.d1 + yuvs.pitch))[0] = p;
        ((U16*)(yuvs.d1 + yuvs.pitch))[1] = p;
        yuvs.d0 += 4;
        yuvs.d1 += 4;
        if (!(x & 1))
        {
            yuvs.u++;
            yuvs.v++;
        }
    } while (--i);
    return x;
}

void dounaligned16rowm(U32 x, U32 n)
{
    while (n--)
    {
        U16 c = (U16)yuv_y16[*yuvs.y0++];
        *(U16*)yuvs.d0 = c;
        yuvs.d0 += 2;
    }
}

U32 dounaligned16colm(U32 n, S32 x)
{
    U32 i = n;
    do
    {
        U16 c = (U16)yuv_y16[*yuvs.y0++];
        *(U16*)yuvs.d0 = c;
        c = (U16)yuv_y16[*yuvs.y1++];
        *(U16*)yuvs.d1 = c;
        yuvs.d0 += 2;
        yuvs.d1 += 2;
    } while (--i);
    return x + n;
}

void dounaligned16row(U32 x, U32 n)
{
    while (n--)
    {
        S32 c;
        x++;
        YUVCOLOR();
        c = yuv_yprime[*yuvs.y0++];
        *(U16*)yuvs.d0 = RGB16(c);
        yuvs.d0 += 2;
        if (!(x & 1))
        {
            yuvs.u++;
            yuvs.v++;
        }
    }
}

U32 dounaligned16col(U32 n, S32 x)
{
    U32 i = n;
    do
    {
        S32 c;
        x++;
        YUVCOLOR();
        c = yuv_yprime[*yuvs.y0++];
        *(U16*)yuvs.d0 = RGB16(c);
        c = yuv_yprime[*yuvs.y1++];
        *(U16*)yuvs.d1 = RGB16(c);
        yuvs.d0 += 2;
        yuvs.d1 += 2;
        if (!(x & 1))
        {
            yuvs.u++;
            yuvs.v++;
        }
    } while (--i);
    return x;
}


extern "C" void YUV_blit_16bpp(void* dst, U32 dx, U32 dy, U32 pitch, void* src, U32 sx, U32 sy, U32 w, U32 h, U32 stride, U32 uvy, U32 flags)
{
    YUV_blit(dst, dx, dy, pitch, src, sx, sy, w, h, stride, uvy, flags, 0, &blits16);
}

void dounaligned16a4rowm2h(U32 x, U32 n)
{
    while (n--)
    {
        U16 c = (U16)yuv_y16[*yuvs.y0++] | (U16)yuv_a16[*yuvs.a0++];
        ((U16*)yuvs.d0)[0] = c;
        *(U16*)(yuvs.d0 + yuvs.pitch) = c;
        yuvs.d0 += 2;
    }
}

U32 dounaligned16a4colm2h(U32 n, S32 x)
{
    U32 i = n;
    do
    {
        U16 c = (U16)yuv_y16[*yuvs.y0++] | (U16)yuv_a16[*yuvs.a0++];
        ((U16*)yuvs.d0)[0] = c;
        *(U16*)(yuvs.d0 + yuvs.pitch) = c;
        c = (U16)yuv_y16[*yuvs.y1++] | (U16)yuv_a16[*yuvs.a1++];
        ((U16*)yuvs.d1)[0] = c;
        *(U16*)(yuvs.d1 + yuvs.pitch) = c;
        yuvs.d0 += 2;
        yuvs.d1 += 2;
    } while (--i);
    return x + n;
}

void dounaligned16a4rowm2w(U32 x, U32 n)
{
    while (n--)
    {
        U16 c = (U16)yuv_y16[*yuvs.y0++] | (U16)yuv_a16[*yuvs.a0++];
        ((U16*)yuvs.d0)[0] = c;
        ((U16*)yuvs.d0)[1] = c;
        yuvs.d0 += 4;
    }
}

U32 dounaligned16a4colm2w(U32 n, S32 x)
{
    U32 i = n;
    do
    {
        U16 c = (U16)yuv_y16[*yuvs.y0++] | (U16)yuv_a16[*yuvs.a0++];
        ((U16*)yuvs.d0)[0] = c;
        ((U16*)yuvs.d0)[1] = c;
        c = (U16)yuv_y16[*yuvs.y1++] | (U16)yuv_a16[*yuvs.a1++];
        ((U16*)yuvs.d1)[0] = c;
        ((U16*)yuvs.d1)[1] = c;
        yuvs.d0 += 4;
        yuvs.d1 += 4;
    } while (--i);
    return x + n;
}

void dounaligned16a4rowm2wh(U32 x, U32 n)
{
    while (n--)
    {
        U16 c = (U16)yuv_y16[*yuvs.y0++] | (U16)yuv_a16[*yuvs.a0++];
        ((U16*)yuvs.d0)[0] = c;
        ((U16*)yuvs.d0)[1] = c;
        ((U16*)(yuvs.d0 + yuvs.pitch))[0] = c;
        ((U16*)(yuvs.d0 + yuvs.pitch))[1] = c;
        yuvs.d0 += 4;
    }
}

U32 dounaligned16a4colm2wh(U32 n, S32 x)
{
    U32 i = n;
    do
    {
        U16 c = (U16)yuv_y16[*yuvs.y0++] | (U16)yuv_a16[*yuvs.a0++];
        ((U16*)yuvs.d0)[0] = c;
        ((U16*)yuvs.d0)[1] = c;
        ((U16*)(yuvs.d0 + yuvs.pitch))[0] = c;
        ((U16*)(yuvs.d0 + yuvs.pitch))[1] = c;
        c = (U16)yuv_y16[*yuvs.y1++] | (U16)yuv_a16[*yuvs.a1++];
        ((U16*)yuvs.d1)[0] = c;
        ((U16*)yuvs.d1)[1] = c;
        ((U16*)(yuvs.d1 + yuvs.pitch))[0] = c;
        ((U16*)(yuvs.d1 + yuvs.pitch))[1] = c;
        yuvs.d0 += 4;
        yuvs.d1 += 4;
    } while (--i);
    return x + n;
}

void dounaligned16a4row2h(U32 x, U32 n)
{
    while (n--)
    {
        S32 c;
        U16 p;
        x++;
        YUVCOLOR();
        c = yuv_yprime[*yuvs.y0++];
        p = RGB16(c) | (U16)yuv_a16[*yuvs.a0++];
        ((U16*)yuvs.d0)[0] = p;
        *(U16*)(yuvs.d0 + yuvs.pitch) = p;
        yuvs.d0 += 2;
        if (!(x & 1))
        {
            yuvs.u++;
            yuvs.v++;
        }
    }
}

U32 dounaligned16a4col2h(U32 n, S32 x)
{
    U32 i = n;
    do
    {
        S32 c;
        U16 p;
        x++;
        YUVCOLOR();
        c = yuv_yprime[*yuvs.y0++];
        p = RGB16(c) | (U16)yuv_a16[*yuvs.a0++];
        ((U16*)yuvs.d0)[0] = p;
        *(U16*)(yuvs.d0 + yuvs.pitch) = p;
        c = yuv_yprime[*yuvs.y1++];
        p = RGB16(c) | (U16)yuv_a16[*yuvs.a1++];
        ((U16*)yuvs.d1)[0] = p;
        *(U16*)(yuvs.d1 + yuvs.pitch) = p;
        yuvs.d0 += 2;
        yuvs.d1 += 2;
        if (!(x & 1))
        {
            yuvs.u++;
            yuvs.v++;
        }
    } while (--i);
    return x;
}

void dounaligned16a4row2w(U32 x, U32 n)
{
    while (n--)
    {
        S32 c;
        U16 p;
        x++;
        YUVCOLOR();
        c = yuv_yprime[*yuvs.y0++];
        p = RGB16(c) | (U16)yuv_a16[*yuvs.a0++];
        ((U16*)yuvs.d0)[0] = p;
        ((U16*)yuvs.d0)[1] = p;
        yuvs.d0 += 4;
        if (!(x & 1))
        {
            yuvs.u++;
            yuvs.v++;
        }
    }
}

U32 dounaligned16a4col2w(U32 n, S32 x)
{
    U32 i = n;
    do
    {
        S32 c;
        U16 p;
        x++;
        YUVCOLOR();
        c = yuv_yprime[*yuvs.y0++];
        p = RGB16(c) | (U16)yuv_a16[*yuvs.a0++];
        ((U16*)yuvs.d0)[0] = p;
        ((U16*)yuvs.d0)[1] = p;
        c = yuv_yprime[*yuvs.y1++];
        p = RGB16(c) | (U16)yuv_a16[*yuvs.a1++];
        ((U16*)yuvs.d1)[0] = p;
        ((U16*)yuvs.d1)[1] = p;
        yuvs.d0 += 4;
        yuvs.d1 += 4;
        if (!(x & 1))
        {
            yuvs.u++;
            yuvs.v++;
        }
    } while (--i);
    return x;
}

void dounaligned16a4row2wh(U32 x, U32 n)
{
    while (n--)
    {
        S32 c;
        U16 p;
        x++;
        YUVCOLOR();
        c = yuv_yprime[*yuvs.y0++];
        p = RGB16(c) | (U16)yuv_a16[*yuvs.a0++];
        ((U16*)yuvs.d0)[0] = p;
        ((U16*)yuvs.d0)[1] = p;
        ((U16*)(yuvs.d0 + yuvs.pitch))[0] = p;
        ((U16*)(yuvs.d0 + yuvs.pitch))[1] = p;
        yuvs.d0 += 4;
        if (!(x & 1))
        {
            yuvs.u++;
            yuvs.v++;
        }
    }
}

U32 dounaligned16a4col2wh(U32 n, S32 x)
{
    U32 i = n;
    do
    {
        S32 c;
        U16 p;
        x++;
        YUVCOLOR();
        c = yuv_yprime[*yuvs.y0++];
        p = RGB16(c) | (U16)yuv_a16[*yuvs.a0++];
        ((U16*)yuvs.d0)[0] = p;
        ((U16*)yuvs.d0)[1] = p;
        ((U16*)(yuvs.d0 + yuvs.pitch))[0] = p;
        ((U16*)(yuvs.d0 + yuvs.pitch))[1] = p;
        c = yuv_yprime[*yuvs.y1++];
        p = RGB16(c) | (U16)yuv_a16[*yuvs.a1++];
        ((U16*)yuvs.d1)[0] = p;
        ((U16*)yuvs.d1)[1] = p;
        ((U16*)(yuvs.d1 + yuvs.pitch))[0] = p;
        ((U16*)(yuvs.d1 + yuvs.pitch))[1] = p;
        yuvs.d0 += 4;
        yuvs.d1 += 4;
        if (!(x & 1))
        {
            yuvs.u++;
            yuvs.v++;
        }
    } while (--i);
    return x;
}

void dounaligned16a4rowm(U32 x, U32 n)
{
    while (n--)
    {
        U16 c = (U16)yuv_y16[*yuvs.y0++] | (U16)yuv_a16[*yuvs.a0++];
        *(U16*)yuvs.d0 = c;
        yuvs.d0 += 2;
    }
}

U32 dounaligned16a4colm(U32 n, S32 x)
{
    U32 i = n;
    do
    {
        U16 c = (U16)yuv_y16[*yuvs.y0++] | (U16)yuv_a16[*yuvs.a0++];
        *(U16*)yuvs.d0 = c;
        c = (U16)yuv_y16[*yuvs.y1++] | (U16)yuv_a16[*yuvs.a1++];
        *(U16*)yuvs.d1 = c;
        yuvs.d0 += 2;
        yuvs.d1 += 2;
    } while (--i);
    return x + n;
}

void dounaligned16a4row(U32 x, U32 n)
{
    while (n--)
    {
        S32 c;
        x++;
        YUVCOLOR();
        c = yuv_yprime[*yuvs.y0++];
        *(U16*)yuvs.d0 = RGB16(c) | (U16)yuv_a16[*yuvs.a0++];
        yuvs.d0 += 2;
        if (!(x & 1))
        {
            yuvs.u++;
            yuvs.v++;
        }
    }
}

U32 dounaligned16a4col(U32 n, S32 x)
{
    U32 i = n;
    do
    {
        S32 c;
        x++;
        YUVCOLOR();
        c = yuv_yprime[*yuvs.y0++];
        *(U16*)yuvs.d0 = RGB16(c) | (U16)yuv_a16[*yuvs.a0++];
        c = yuv_yprime[*yuvs.y1++];
        *(U16*)yuvs.d1 = RGB16(c) | (U16)yuv_a16[*yuvs.a1++];
        yuvs.d0 += 2;
        yuvs.d1 += 2;
        if (!(x & 1))
        {
            yuvs.u++;
            yuvs.v++;
        }
    } while (--i);
    return x;
}


extern "C" void YUV_blit_16a4bpp(void* dst, U32 dx, U32 dy, U32 pitch, void* src, U32 sx, U32 sy, U32 w, U32 h, U32 stride,
                                U32 uvy, void* alpha, U32 flags)
{
    YUV_blit(dst, dx, dy, pitch, src, sx, sy, w, h, stride, uvy, flags, alpha, &blits16a4);
}


void dounalignedYUY2rowm2wh(U32 x, U32 n)
{
    while ((S32)(n -= 2) >= 0)
    {
        U32 p0, p1;
        p0 = 0x80008000 | yuvs.y0[0] | (yuvs.y0[0] << 16);
        p1 = 0x80008000 | yuvs.y0[1] | (yuvs.y0[1] << 16);
        ((U32*)yuvs.d0)[0] = p0;
        ((U32*)yuvs.d0)[1] = p1;
        ((U32*)(yuvs.d0 + yuvs.pitch))[0] = p0;
        ((U32*)(yuvs.d0 + yuvs.pitch))[1] = p1;
        yuvs.y0 += 2;
        yuvs.d0 += 8;
    }
}

U32 dounalignedYUY2colm2wh(U32 n, S32 x)
{
    U32 i = n;
    do
    {
        U32 p0, p1;
        p0 = 0x80008000 | yuvs.y0[0] | (yuvs.y0[0] << 16);
        p1 = 0x80008000 | yuvs.y0[1] | (yuvs.y0[1] << 16);
        ((U32*)yuvs.d0)[0] = p0;
        ((U32*)yuvs.d0)[1] = p1;
        ((U32*)(yuvs.d0 + yuvs.pitch))[0] = p0;
        ((U32*)(yuvs.d0 + yuvs.pitch))[1] = p1;
        p0 = 0x80008000 | yuvs.y1[0] | (yuvs.y1[0] << 16);
        p1 = 0x80008000 | yuvs.y1[1] | (yuvs.y1[1] << 16);
        ((U32*)yuvs.d1)[0] = p0;
        ((U32*)yuvs.d1)[1] = p1;
        ((U32*)(yuvs.d1 + yuvs.pitch))[0] = p0;
        ((U32*)(yuvs.d1 + yuvs.pitch))[1] = p1;
        yuvs.y0 += 2;
        yuvs.y1 += 2;
        yuvs.d0 += 8;
        yuvs.d1 += 8;
    } while ((S32)(i -= 2) > 0);
    return x + n;
}

void dounalignedYUY2rowm2w(U32 x, U32 n)
{
    while ((S32)(n -= 2) >= 0)
    {
        U32 y;
        y = yuvs.y0[0];
        ((U32*)yuvs.d0)[0] = 0x80008000 | y | (y << 16);
        y = yuvs.y0[1];
        yuvs.y0 += 2;
        ((U32*)yuvs.d0)[1] = 0x80008000 | y | (y << 16);
        yuvs.d0 += 8;
    }
}

U32 dounalignedYUY2colm2w(U32 n, S32 x)
{
    U32 i = n;
    do
    {
        U32 y;
        y = yuvs.y0[0];
        ((U32*)yuvs.d0)[0] = 0x80008000 | y | (y << 16);
        y = yuvs.y0[1];
        yuvs.y0 += 2;
        ((U32*)yuvs.d0)[1] = 0x80008000 | y | (y << 16);
        y = yuvs.y1[0];
        ((U32*)yuvs.d1)[0] = 0x80008000 | y | (y << 16);
        y = yuvs.y1[1];
        yuvs.y1 += 2;
        ((U32*)yuvs.d1)[1] = 0x80008000 | y | (y << 16);
        yuvs.d0 += 8;
        yuvs.d1 += 8;
    } while ((S32)(i -= 2) > 0);
    return x + n;
}

void dounalignedYUY2rowm2h(U32 x, U32 n)
{
    while ((S32)(n -= 2) >= 0)
    {
        U32 p;
        p = 0x80008000 | yuvs.y0[0] | (yuvs.y0[1] << 16);
        yuvs.y0 += 2;
        *(U32*)yuvs.d0 = p;
        *(U32*)(yuvs.d0 + yuvs.pitch) = p;
        yuvs.d0 += 4;
    }
}

U32 dounalignedYUY2colm2h(U32 n, S32 x)
{
    U32 i = n;
    do
    {
        U32 p;
        p = 0x80008000 | yuvs.y0[0] | (yuvs.y0[1] << 16);
        yuvs.y0 += 2;
        *(U32*)yuvs.d0 = p;
        *(U32*)(yuvs.d0 + yuvs.pitch) = p;
        p = 0x80008000 | yuvs.y1[0] | (yuvs.y1[1] << 16);
        yuvs.y1 += 2;
        *(U32*)yuvs.d1 = p;
        *(U32*)(yuvs.d1 + yuvs.pitch) = p;
        yuvs.d0 += 4;
        yuvs.d1 += 4;
    } while ((S32)(i -= 2) > 0);
    return x + n;
}

void dounalignedYUY2row2wh(U32 x, U32 n)
{
    while ((S32)(n -= 2) >= 0)
    {
        U32 p0, p1;
        p0 = yuvs.y0[0] | (*yuvs.u++ << 8) | (yuvs.y0[0] << 16) | (*yuvs.v++ << 24);
        p1 = (p0 & 0xFF00FF00) | yuvs.y0[1] | (yuvs.y0[1] << 16);
        ((U32*)yuvs.d0)[0] = p0;
        ((U32*)yuvs.d0)[1] = p1;
        ((U32*)(yuvs.d0 + yuvs.pitch))[0] = p0;
        ((U32*)(yuvs.d0 + yuvs.pitch))[1] = p1;
        yuvs.y0 += 2;
        yuvs.d0 += 8;
    }
}

U32 dounalignedYUY2col2wh(U32 n, S32 x)
{
    U32 i = n;
    do
    {
        U32 p0, p1;
        p0 = yuvs.y0[0] | (*yuvs.u++ << 8) | (yuvs.y0[0] << 16) | (*yuvs.v++ << 24);
        p1 = (p0 & 0xFF00FF00) | yuvs.y0[1] | (yuvs.y0[1] << 16);
        ((U32*)yuvs.d0)[0] = p0;
        ((U32*)yuvs.d0)[1] = p1;
        ((U32*)(yuvs.d0 + yuvs.pitch))[0] = p0;
        ((U32*)(yuvs.d0 + yuvs.pitch))[1] = p1;
        p0 = (p0 & 0xFF00FF00) | yuvs.y1[0] | (yuvs.y1[0] << 16);
        p1 = (p1 & 0xFF00FF00) | yuvs.y1[1] | (yuvs.y1[1] << 16);
        ((U32*)yuvs.d1)[0] = p0;
        ((U32*)yuvs.d1)[1] = p1;
        ((U32*)(yuvs.d1 + yuvs.pitch))[0] = p0;
        ((U32*)(yuvs.d1 + yuvs.pitch))[1] = p1;
        yuvs.y0 += 2;
        yuvs.y1 += 2;
        yuvs.d0 += 8;
        yuvs.d1 += 8;
    } while ((S32)(i -= 2) > 0);
    return x + n;
}

void dounalignedYUY2row2w(U32 x, U32 n)
{
    while ((S32)(n -= 2) >= 0)
    {
        U32 p = yuvs.y0[0] | (*yuvs.u++ << 8) | (yuvs.y0[0] << 16) | (*yuvs.v++ << 24);
        U32 y;
        ((U32*)yuvs.d0)[0] = p;
        y = yuvs.y0[1];
        yuvs.y0 += 2;
        ((U32*)yuvs.d0)[1] = (p & 0xFF00FF00) | y | (y << 16);
        yuvs.d0 += 8;
    }
}

U32 dounalignedYUY2col2w(U32 n, S32 x)
{
    U32 i = n;
    do
    {
        U32 p = yuvs.y0[0] | (*yuvs.u++ << 8) | (yuvs.y0[0] << 16) | (*yuvs.v++ << 24);
        U32 y;
        ((U32*)yuvs.d0)[0] = p;
        y = yuvs.y0[1];
        yuvs.y0 += 2;
        ((U32*)yuvs.d0)[1] = (p & 0xFF00FF00) | y | (y << 16);
        p &= 0xFF00FF00;
        ((U32*)yuvs.d1)[0] = p | yuvs.y1[0] | (yuvs.y1[0] << 16);
        y = yuvs.y1[1];
        yuvs.y1 += 2;
        ((U32*)yuvs.d1)[1] = p | y | (y << 16);
        yuvs.d0 += 8;
        yuvs.d1 += 8;
    } while ((S32)(i -= 2) > 0);
    return x + n;
}

void dounalignedYUY2row2h(U32 x, U32 n)
{
    while ((S32)(n -= 2) >= 0)
    {
        U32 p = yuvs.y0[0] | (*yuvs.u++ << 8) | (yuvs.y0[1] << 16) | (*yuvs.v++ << 24);
        yuvs.y0 += 2;
        ((U32*)yuvs.d0)[0] = p;
        ((U32*)(yuvs.d0 + yuvs.pitch))[0] = p;
        yuvs.d0 += 4;
    }
}

U32 dounalignedYUY2col2h(U32 n, S32 x)
{
    U32 i = n;
    do
    {
        U32 p = yuvs.y0[0] | (*yuvs.u++ << 8) | (yuvs.y0[1] << 16) | (*yuvs.v++ << 24);
        yuvs.y0 += 2;
        ((U32*)yuvs.d0)[0] = p;
        ((U32*)(yuvs.d0 + yuvs.pitch))[0] = p;
        p = (p & 0xFF00FF00) | yuvs.y1[0] | (yuvs.y1[1] << 16);
        yuvs.y1 += 2;
        ((U32*)yuvs.d1)[0] = p;
        ((U32*)(yuvs.d1 + yuvs.pitch))[0] = p;
        yuvs.d0 += 4;
        yuvs.d1 += 4;
    } while ((S32)(i -= 2) > 0);
    return x + n;
}

void dounalignedYUY2rowm(U32 x, U32 n)
{
    while ((S32)(n -= 2) >= 0)
    {
        U32 p;
        p = 0x80008000 | yuvs.y0[0] | (yuvs.y0[1] << 16);
        yuvs.y0 += 2;
        *(U32*)yuvs.d0 = p;
        yuvs.d0 += 4;
    }
}

U32 dounalignedYUY2colm(U32 n, S32 x)
{
    U32 i = n;
    do
    {
        U32 p;
        p = 0x80008000 | yuvs.y0[0] | (yuvs.y0[1] << 16);
        yuvs.y0 += 2;
        *(U32*)yuvs.d0 = p;
        p = 0x80008000 | yuvs.y1[0] | (yuvs.y1[1] << 16);
        yuvs.y1 += 2;
        *(U32*)yuvs.d1 = p;
        yuvs.d0 += 4;
        yuvs.d1 += 4;
    } while ((S32)(i -= 2) > 0);
    return x + n;
}

void dounalignedYUY2row(U32 x, U32 n)
{
    while ((S32)(n -= 2) >= 0)
    {
        U32 p = yuvs.y0[0] | (*yuvs.u++ << 8) | (yuvs.y0[1] << 16) | (*yuvs.v++ << 24);
        yuvs.y0 += 2;
        ((U32*)yuvs.d0)[0] = p;
        yuvs.d0 += 4;
    }
}

U32 dounalignedYUY2col(U32 n, S32 x)
{
    U32 i = n;
    do
    {
        U32 p = yuvs.y0[0] | (*yuvs.u++ << 8) | (yuvs.y0[1] << 16) | (*yuvs.v++ << 24);
        yuvs.y0 += 2;
        ((U32*)yuvs.d0)[0] = p;
        p = (p & 0xFF00FF00) | yuvs.y1[0] | (yuvs.y1[1] << 16);
        yuvs.y1 += 2;
        ((U32*)yuvs.d1)[0] = p;
        yuvs.d0 += 4;
        yuvs.d1 += 4;
    } while ((S32)(i -= 2) > 0);
    return x + n;
}

extern "C" void YUV_blit_YUY2(void* dst, U32 dx, U32 dy, U32 pitch, void* src, U32 sx, U32 sy, U32 w, U32 h,
                              U32 stride, U32 uvy, U32 flags)
{
    if (((U32)dst & 3) == 2)
    {
        dx++;
        dst = (void*)((U32)dst & ~3);
    }
    if (dx & 1)
    {
        if (sx & 1)
            sx++;
        dx++;
        w--;
    }
    else if (sx & 1)
    {
        sx++;
        w--;
    }
    if (w & 1)
        w--;
    YUV_blit(dst, dx, dy, pitch, src, sx, sy, w, h, stride, uvy, flags, 0, &blitsYUY2);
}

extern "C" void YUV_blit_32bpp_mask(void* dst, U32 dx, U32 dy, U32 pitch, U8* mask, U32 maskpitch, void* src, U32 sx, U32 sy, U32 w, U32 h, U32 stride, U32 uvy, U32 flags)
{
    YUV_blit_mask(dst, dx, dy, pitch, mask, maskpitch, src, sx, sy, w, h, stride, uvy, flags, 0, &blits32);
}

extern "C" void YUV_blit_32abpp_mask(void* dst, U32 dx, U32 dy, U32 pitch, U8* mask, U32 maskpitch, void* src, U32 sx, U32 sy, U32 w, U32 h, U32 stride, U32 uvy, void* alpha, U32 flags)
{
    YUV_blit_mask(dst, dx, dy, pitch, mask, maskpitch, src, sx, sy, w, h, stride, uvy, flags, alpha, &blits32a);
}

extern "C" void YUV_blit_16bpp_mask(void* dst, U32 dx, U32 dy, U32 pitch, U8* mask, U32 maskpitch, void* src, U32 sx, U32 sy, U32 w, U32 h, U32 stride, U32 uvy, U32 flags)
{
    YUV_blit_mask(dst, dx, dy, pitch, mask, maskpitch, src, sx, sy, w, h, stride, uvy, flags, 0, &blits16);
}

extern "C" void YUV_blit_16a4bpp_mask(void* dst, U32 dx, U32 dy, U32 pitch, U8* mask, U32 maskpitch, void* src, U32 sx, U32 sy, U32 w, U32 h, U32 stride, U32 uvy, void* alpha, U32 flags)
{
    YUV_blit_mask(dst, dx, dy, pitch, mask, maskpitch, src, sx, sy, w, h, stride, uvy, flags, alpha, &blits16a4);
}

extern "C" void YUV_blit_YUY2_mask(void* dst, U32 dx, U32 dy, U32 pitch, U8* mask, U32 maskpitch, void* src, U32 sx, U32 sy, U32 w, U32 h, U32 stride, U32 uvy, U32 flags)
{
    YUV_blit_mask(dst, dx, dy, pitch, mask, maskpitch, src, sx, sy, w, h, stride, uvy, flags, 0, &blitsYUY2);
}
