/*
 * Bink YUV -> YUY2 converters for the 4x2 block path.
 * Compiled with SN ProDG (GCC 2.95).
 */

#include "bink/yuvstate.h"

void YUY2_4x2Helper(S32 n, U32* out, U32* y, U32* u, U32* v);
void YUY2_x2_4x2Helper(S32 n, U32* out, U32* y, U32* u, U32* v);
void YUY2_m_4x2Helper(S32 n, U32* out, U32* y);
void YUY2_mx2_4x2Helper(S32 n, U32* out, U32* y);

inline void YUY2_4x2Helper(S32 n, U32* out, U32* y, U32* u, U32* v)
{
    U32 yy, uu, vv;
    U32 i = n >> 1;

    while (i--)
    {
        yy = *y++;
        uu = *u++;
        vv = *v++;
        *out++ = (yy & 0xFF000000) + ((uu >> 8) & 0xFF0000) + ((yy >> 8) & 0xFF00) + (vv >> 24);
        *out++ = ((yy & 0xFF00) << 16) + (uu & 0xFF0000) + ((yy & 0xFF) << 8) + ((vv >> 16) & 0xFF);
        yy = *y++;
        *out++ = (yy & 0xFF000000) + ((uu & 0xFF00) << 8) + ((yy >> 8) & 0xFF00) + ((vv >> 8) & 0xFF);
        *out++ = ((yy & 0xFF00) << 16) + ((uu & 0xFF) << 16) + ((yy & 0xFF) << 8) + (vv & 0xFF);
    }
    if (n & 1)
    {
        U16 u16, v16;

        U32 t;
        t = *y;
        u16 = *(U16*)u;
        v16 = *(U16*)v;
        out[0] = (t & 0xFF000000) + ((u16 & 0xFF00) << 8) + ((t >> 8) & 0xFF00) + (v16 >> 8);
        out[1] = ((t & 0xFF00) << 16) + ((u16 & 0xFF) << 16) + ((t & 0xFF) << 8) + (v16 & 0xFF);
    }
}

inline void YUY2_x2_4x2Helper(S32 n, U32* out, U32* y, U32* u, U32* v)
{
    U32 yy, uu, vv, c;
    U32 i = n >> 1;

    while (i--)
    {
        uu = *u++;
        vv = *v++;
        yy = *y++;
        c = ((uu >> 8) & 0xFF0000) + (vv >> 24);
        *out++ = (yy & 0xFF000000) + ((yy & 0xFF000000) >> 16) + c;
        *out++ = ((yy & 0xFF0000) << 8) + ((yy & 0xFF0000) >> 8) + c;
        c = (uu & 0xFF0000) + ((vv >> 16) & 0xFF);
        *out++ = ((yy & 0xFF00) << 16) + (yy & 0xFF00) + c;
        *out++ = ((yy & 0xFF) << 24) + ((yy & 0xFF) << 8) + c;
        c = ((uu & 0xFF00) << 8) + ((vv >> 8) & 0xFF);
        yy = *y++;
        *out++ = (yy & 0xFF000000) + ((yy & 0xFF000000) >> 16) + c;
        *out++ = ((yy & 0xFF0000) << 8) + ((yy & 0xFF0000) >> 8) + c;
        c = ((uu & 0xFF) << 16) + (vv & 0xFF);
        *out++ = ((yy & 0xFF00) << 16) + (yy & 0xFF00) + c;
        *out++ = ((yy & 0xFF) << 24) + ((yy & 0xFF) << 8) + c;
    }
    if (n & 1)
    {
        U16 u16, v16;
        U32 t, d;

        u16 = *(U16*)u;
        v16 = *(U16*)v;
        t = *y;
        d = ((u16 & 0xFF00) << 8) + (v16 >> 8);
        *out++ = (t & 0xFF000000) + ((t & 0xFF000000) >> 16) + d;
        *out++ = ((t & 0xFF0000) << 8) + ((t & 0xFF0000) >> 8) + d;
        d = ((u16 & 0xFF) << 16) + (v16 & 0xFF);
        *out++ = ((t & 0xFF00) << 16) + (t & 0xFF00) + d;
        *out++ = ((t & 0xFF) << 24) + ((t & 0xFF) << 8) + d;
    }
}

inline void YUY2_m_4x2Helper(S32 n, U32* out, U32* y)
{
    U32 yy;
    U32 i = n;
    while (i--)
    {
        yy = *y++;
        *out++ = (yy & 0xFF000000) + ((yy >> 8) & 0xFF00) + 0x800080;
        *out++ = ((yy & 0xFF00) << 16) + ((yy & 0xFF) << 8) + 0x800080;
    }
}

inline void YUY2_mx2_4x2Helper(S32 n, U32* out, U32* y)
{
    U32 yy;
    U32 i = n;

    while (i--)
    {
        yy = *y++;
        *out++ = (yy & 0xFF000000) + ((yy & 0xFF000000) >> 16) + 0x800080;
        *out++ = ((yy & 0xFF0000) << 8) + ((yy & 0xFF0000) >> 8) + 0x800080;
        *out++ = ((yy & 0xFF00) << 16) + (yy & 0xFF00) + 0x800080;
        *out++ = ((yy & 0xFF) << 24) + ((yy & 0xFF) << 8) + 0x800080;
    }
}

void YUY2_4x2(S32 n)
{
    YUY2_4x2Helper(n, (U32*)yuvs.d0, (U32*)yuvs.y0, (U32*)yuvs.u, (U32*)yuvs.v);
    YUY2_4x2Helper(n, (U32*)yuvs.d1, (U32*)yuvs.y1, (U32*)yuvs.u, (U32*)yuvs.v);
    yuvs.d0 += n * 8;
    yuvs.d1 += n * 8;
    yuvs.y0 += n * 4;
    yuvs.y1 += n * 4;
    yuvs.u += n * 2;
    yuvs.v += n * 2;
}

void YUY2_x2_4x2(S32 n)
{
    YUY2_x2_4x2Helper(n, (U32*)yuvs.d0, (U32*)yuvs.y0, (U32*)yuvs.u, (U32*)yuvs.v);
    YUY2_x2_4x2Helper(n, (U32*)yuvs.d1, (U32*)yuvs.y1, (U32*)yuvs.u, (U32*)yuvs.v);
    yuvs.d0 += n * 16;
    yuvs.d1 += n * 16;
    yuvs.y0 += n * 4;
    yuvs.y1 += n * 4;
    yuvs.u += n * 2;
    yuvs.v += n * 2;
}

void YUY2_m_4x2(S32 n)
{
    YUY2_m_4x2Helper(n, (U32*)yuvs.d0, (U32*)yuvs.y0);
    YUY2_m_4x2Helper(n, (U32*)yuvs.d1, (U32*)yuvs.y1);
    yuvs.d0 += n * 8;
    yuvs.d1 += n * 8;
    yuvs.y0 += n * 4;
    yuvs.y1 += n * 4;
}

void YUY2_mx2_4x2(S32 n)
{
    YUY2_mx2_4x2Helper(n, (U32*)yuvs.d0, (U32*)yuvs.y0);
    YUY2_mx2_4x2Helper(n, (U32*)yuvs.d1, (U32*)yuvs.y1);
    yuvs.d0 += n * 16;
    yuvs.d1 += n * 16;
    yuvs.y0 += n * 4;
    yuvs.y1 += n * 4;
}
