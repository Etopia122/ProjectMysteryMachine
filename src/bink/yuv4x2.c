/*
 * Bink YUV -> GX tiled texture converters: each step converts a 4x2 block of
 * pixels straight into the 4x4 texel tiles (RGBA8, RGB5A3, ...).
 * Compiled with SN ProDG (GCC 2.95).
 */

#include "bink/yuvstate.h"

/* Odd-pixel steps: the 4x2 converters always consume whole blocks. */
inline void YUV_16_4x2_odd(U32 n)
{
}

inline void YUV_32_4x2_odd(U32 n)
{
}

inline void YUV_16x2_4x2_odd(U32 n)
{
}

inline void YUV_16a4x2_4x2_odd(U32 n)
{
}

inline void YUV_32x2_4x2_odd(U32 n)
{
}

inline void YUV_16a4_4x2_odd(U32 n)
{
}

inline void YUV_32a_4x2_odd(U32 n)
{
}

inline void YUV_32ax2_4x2_odd(U32 n)
{
}

/* Address of a linear framebuffer position in the tiled RGBA8 layout. */
inline U8* GetTiledRgbLoc(U8* p, U32 tilepitch)
{
    U32 row = (S32)(p - yuvs.base) / yuvs.pitch;
    return yuvs.base + tilepitch * (row >> 2) + (row & 3) * 8 + ((p - (yuvs.base + row * yuvs.pitch)) << 2);
}

void YUV_32_4x2_even(U32 n)
{
    U32 tp = (yuvs.pitch * 4 + 63) & ~63;
    U32* o0 = (U32*)GetTiledRgbLoc(yuvs.d0, tp);
    U32* o1 = (U32*)GetTiledRgbLoc(yuvs.d1, tp);
    U16* u;
    U16* v;
    U32* y0;
    U32* y1;

    yuvs.d0 += n * 16;
    yuvs.d1 += n * 16;
    y1 = (U32*)yuvs.y1;
    u = (U16*)yuvs.u;
    v = (U16*)yuvs.v;
    y0 = (U32*)yuvs.y0;
    do
    {
        U32 uu = *u++;
        U32 vv = *v++;
        U32 yb = *y1++;
        U32 ya = *y0++;
        S32 cr, cg, cb;
        U32* a;
        U32* b;

        cr = yuvt.vr.t[vv >> 8];
        cg = yuvt.ug.t[uu >> 8] + yuvt.vg.t[vv >> 8];
        cb = yuvt.ub.t[uu >> 8];
        a = yuv_yclip[ya >> 24];
        b = yuv_yclip[(ya >> 16) & 255];
        o0[0] = (a[cr] << 16) | b[cr];
        o0[8] = (a[cg] << 24) | (a[cb] << 16) | (b[cg] << 8) | b[cb];
        a = yuv_yclip[yb >> 24];
        b = yuv_yclip[(yb >> 16) & 255];
        o1[0] = (a[cr] << 16) | b[cr];
        o1[8] = (a[cg] << 24) | (a[cb] << 16) | (b[cg] << 8) | b[cb];

        cr = yuvt.vr.t[vv & 255];
        cg = yuvt.ug.t[uu & 255] + yuvt.vg.t[vv & 255];
        cb = yuvt.ub.t[uu & 255];
        a = yuv_yclip[(ya >> 8) & 255];
        b = yuv_yclip[ya & 255];
        o0[1] = (a[cr] << 16) | b[cr];
        o0[9] = (a[cg] << 24) | (a[cb] << 16) | (b[cg] << 8) | b[cb];
        o0 += 16;
        a = yuv_yclip[(yb >> 8) & 255];
        b = yuv_yclip[yb & 255];
        o1[1] = (a[cr] << 16) | b[cr];
        o1[9] = (a[cg] << 24) | (a[cb] << 16) | (b[cg] << 8) | b[cb];
        o1 += 16;
    } while (--n);
    yuvs.y1 = (U8*)y1;
    yuvs.u = (U8*)u;
    yuvs.v = (U8*)v;
    yuvs.y0 = (U8*)y0;
}

void YUV_32m_4x2(U32 n)
{
    U32 tp = (yuvs.pitch * 4 + 63) & ~63;
    U32* o0 = (U32*)GetTiledRgbLoc(yuvs.d0, tp);
    U32* o1 = (U32*)GetTiledRgbLoc(yuvs.d1, tp);
    U32* y0;
    U32* y1;

    yuvs.d0 += n * 16;
    yuvs.d1 += n * 16;
    y1 = (U32*)yuvs.y1;
    y0 = (U32*)yuvs.y0;
    do
    {
        U32 ya = *y0++;
        U32 yb = *y1++;
        U32 p0, p1, p2, p3;

        p0 = yuv_ytab[ya >> 24];
        p1 = yuv_ytab[(ya >> 16) & 255];
        p2 = yuv_ytab[(ya >> 8) & 255];
        p3 = yuv_ytab[ya & 255];
        o0[0] = (p0 & 0xFFFF0000) | (p1 >> 16);
        o0[8] = (p0 << 16) | (p1 & 0xFFFF);
        o0[9] = (p2 << 16) | (p3 & 0xFFFF);
        o0[1] = (p2 & 0xFFFF0000) | (p3 >> 16);
        o0 += 16;
        p0 = yuv_ytab[yb >> 24];
        p1 = yuv_ytab[(yb >> 16) & 255];
        p2 = yuv_ytab[(yb >> 8) & 255];
        p3 = yuv_ytab[yb & 255];
        o1[0] = (p0 & 0xFFFF0000) | (p1 >> 16);
        o1[1] = (p2 & 0xFFFF0000) | (p3 >> 16);
        o1[8] = (p0 << 16) | (p1 & 0xFFFF);
        o1[9] = (p2 << 16) | (p3 & 0xFFFF);
        o1 += 16;
    } while (--n);
    yuvs.y1 = (U8*)y1;
    yuvs.y0 = (U8*)y0;
}
