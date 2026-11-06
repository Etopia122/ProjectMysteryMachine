/*
 * Bink video plane expansion: bundle readers and Huffman tables.
 * Compiled with SN ProDG (GCC 2.95).
 */

#include "bink/varbits.h"

typedef struct BUNDLE
{
    U8* cur;      /* 0x00 */
    U8* end;      /* 0x04 */
    U32 bits;     /* 0x08 */
    U32 sign;     /* 0x0C */
    U8 syms[16];  /* 0x10 */
    U32 hbits;    /* 0x20 */
    U8* htab;     /* 0x24 */
    U32 lenbits;  /* 0x28 */
    U8* data;     /* 0x2C */
} BUNDLE;

extern U8 bink_huffbits[16];
extern U8* bink_hufftabs[16];

U32 ExpandPlane(void* out, void* prev, U32 w, U32 h, U32 pitch, void* data, U32 key, U8* mask, U32 plane,
                void* state, U32 flags);

void OpenReadBundle(U8* data, BUNDLE* b, U32 w, U32 h, U32 bits, U32 per, U32 sign)
{
    b->bits = bits;
    b->cur = 0;
    b->end = 0;
    b->lenbits = 32 - radcntlzw((w - 1 + (h >> 3) * per) & 0xFFFF);
    if (sign)
        b->sign = 1 << (bits - 1);
    else
        b->sign = 0;
    b->data = data;
}

#define GETBIT(vb, i)                                                                              \
    if ((vb)->bitlen == 0)                                                                         \
    {                                                                                              \
        i = *(vb)->cur;                                                                            \
        (vb)->bitlen = 31;                                                                         \
        (vb)->cur++;                                                                               \
        (vb)->bits = i >> 1;                                                                       \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
        i = (vb)->bits;                                                                            \
        (vb)->bitlen--;                                                                            \
        (vb)->bits = i >> 1;                                                                       \
    }

void mergesort(VARBITS* vb, U8* out, U8* a, U8* b, S32 n)
{
    U32 bit;
    S32 na = n;

    do
    {
        if (VarBitsGet1(*vb, bit))
        {
            *out++ = *b++;
            n--;
        }
        else
        {
            *out++ = *a++;
            na--;
        }
    } while (na && n);
    if (na)
    {
        do
        {
            *out++ = *a++;
        } while (--na);
        return;
    }
    if (n)
    {
        do
        {
            *out++ = *b++;
        } while (--n);
    }
}

#define GETBITS(vb, v, len)                                                                        \
    {                                                                                              \
        U32 bl_ = (vb)->bitlen;                                                                    \
        if (bl_ < (len))                                                                           \
        {                                                                                          \
            U32 ob_ = (vb)->bits;                                                                  \
            U32 nb_ = *(vb)->cur;                                                                  \
            (vb)->bitlen = bl_ + 32 - (len);                                                       \
            (vb)->cur++;                                                                           \
            (vb)->bits = nb_ >> ((len) - bl_);                                                     \
            v = ob_ | (nb_ << bl_);                                                                \
        }                                                                                          \
        else                                                                                       \
        {                                                                                          \
            v = (vb)->bits;                                                                        \
            (vb)->bitlen = bl_ - (len);                                                            \
            (vb)->bits = v >> (len);                                                               \
        }                                                                                          \
    }

void ReadHuffTable(VARBITS* vb, U8** tab, U32* bits, U8* syms)
{
    U32 t, v, i, n, mask, bit;
    U8* p;
    U8 tmp2[16];
    U8 tmp[16];

    VarBitsGet(t, U32, *vb, 4);
    {
        U8* h = bink_hufftabs[t];
        U32 hb = bink_huffbits[t];
        *tab = h;
        *bits = hb;
    }
    if (t == 0)
    {
        for (i = 0; i < 16; i++)
            syms[i] = i;
    }
    else if (!VarBitsGet1(*vb, bit))
    {
        VarBitsGet(v, U32, *vb, 2);
        if (v == 0)
        {
            p = syms;
            for (i = 0; i < 16; i += 2)
            {
                if (VarBitsGet1(*vb, bit))
                {
                    p[1] = i;
                    p[0] = i + 1;
                }
                else
                {
                    p[0] = i;
                    p[1] = i + 1;
                }
                p += 2;
            }
        }
        else
        {
            for (i = 0; i < 16; i += 2)
            {
                if (VarBitsGet1(*vb, bit))
                {
                    tmp[i] = i + 1;
                    tmp[i + 1] = i;
                }
                else
                {
                    tmp[i] = i;
                    tmp[i + 1] = i + 1;
                }
            }
            if (v == 1)
            {
                mergesort(vb, syms, tmp, tmp + 2, 2);
                mergesort(vb, syms + 4, tmp + 4, tmp + 6, 2);
                mergesort(vb, syms + 8, tmp + 8, tmp + 10, 2);
                mergesort(vb, syms + 12, tmp + 12, tmp + 14, 2);
            }
            else
            {
                mergesort(vb, tmp2, tmp, tmp + 2, 2);
                mergesort(vb, tmp2 + 4, tmp + 4, tmp + 6, 2);
                mergesort(vb, tmp2 + 8, tmp + 8, tmp + 10, 2);
                mergesort(vb, tmp2 + 12, tmp + 12, tmp + 14, 2);
                if (v == 2)
                {
                    mergesort(vb, syms, tmp2, tmp2 + 4, 4);
                    mergesort(vb, syms + 8, tmp2 + 8, tmp2 + 12, 4);
                }
                else
                {
                    mergesort(vb, tmp, tmp2, tmp2 + 4, 4);
                    mergesort(vb, tmp + 8, tmp2 + 8, tmp2 + 12, 4);
                    mergesort(vb, syms, tmp, tmp + 8, 8);
                }
            }
        }
    }
    else
    {
        VarBitsGet(n, U32, *vb, 3);
        mask = 0xFFFF;
        for (i = 0; i <= n; i++)
        {
            U32 s;
            VarBitsGet(s, U32, *vb, 4);
            syms[i] = s;
            mask &= ~(1 << s);
        }
        i = 0;
        do
        {
            if (mask & 1)
                syms[++n] = i;
            i++;
            mask >>= 1;
        } while (mask);
    }
}

void StartReadHuff4Bundle(BUNDLE* b, VARBITS* vb)
{
    ReadHuffTable(vb, &b->htab, &b->hbits, b->syms);
}

typedef struct HUFF8
{
    U8 syms[16][16]; /* 0x000 */
    U32 bits[16];    /* 0x100 */
    U8* tabs[16];    /* 0x140 */
    U32 last;        /* 0x180 */
} HUFF8;

void StartReadHuff8Bundle(BUNDLE* b, VARBITS* vb, HUFF8* h)
{
    U32 i;

    for (i = 0; i < 16; i++)
        ReadHuffTable(vb, &h->tabs[i], &h->bits[i], h->syms[i]);
    ReadHuffTable(vb, &b->htab, &b->hbits, b->syms);
    h->last = 0;
}

#define HUFFDECODE(out, vb, tab, hb, syms)                                                         \
    if ((vb)->bitlen >= (hb))                                                                      \
    {                                                                                              \
        U32 e_ = (tab)[(vb)->bits & GetBitsLen(hb)];                                               \
        out = (syms)[e_ & 15];                                                                     \
        e_ >>= 4;                                                                                  \
        (vb)->bits >>= e_;                                                                         \
        (vb)->bitlen -= e_;                                                                        \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
        U32 nb_ = *(vb)->cur;                                                                      \
        U32 e_ = (tab)[((vb)->bits | (nb_ << (vb)->bitlen)) & GetBitsLen(hb)];                     \
        out = (syms)[e_ & 15];                                                                     \
        e_ >>= 4;                                                                                  \
        if ((vb)->bitlen >= e_)                                                                    \
        {                                                                                          \
            (vb)->bits >>= e_;                                                                     \
            (vb)->bitlen -= e_;                                                                    \
        }                                                                                          \
        else                                                                                       \
        {                                                                                          \
            (vb)->bits = nb_ >> (e_ - (vb)->bitlen);                                               \
            (vb)->bitlen = (vb)->bitlen + 32 - e_;                                                 \
            (vb)->cur++;                                                                           \
        }                                                                                          \
    }

#define VARGET(v, vb, len)                                                                         \
    if ((vb)->bitlen >= (len))                                                                     \
    {                                                                                              \
        v = (vb)->bits & GetBitsLen(len);                                                          \
        (vb)->bits >>= (len);                                                                      \
        (vb)->bitlen -= (len);                                                                     \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
        U32 nb_ = *(vb)->cur;                                                                      \
        v = ((vb)->bits | (nb_ << (vb)->bitlen)) & GetBitsLen(len);                                \
        (vb)->bits = nb_ >> ((len) - (vb)->bitlen);                                                \
        (vb)->bitlen = (vb)->bitlen + 32 - (len);                                                  \
        (vb)->cur++;                                                                               \
    }

static const U8 rle4lens[4] = {4, 8, 12, 32};

void CheckReadRLEHuff4Bundle(BUNDLE* b, VARBITS* vb)
{
    U32 n, bit, hb, last, run, fill;
    U8 v;
    U8* p;
    U8* tab;
    U8* syms;

    if (b->cur == b->end)
    {
        VARGET(n, vb, b->lenbits);
        if (n)
        {
            b->cur = b->data;
            b->end = b->data + n;
            if (!VarBitsGet1(*vb, bit))
            {
                syms = b->syms;
                hb = (U8)b->hbits;
                p = b->data;
                last = 0;
                tab = b->htab;
                while (n)
                {
                    HUFFDECODE(v, vb, tab, hb, syms);
                    if (v >= 12)
                    {
                        fill = last | (last << 8);
                        run = rle4lens[v - 12];
                        fill |= fill << 16;
                        n -= run;
                        do
                        {
                            *(U32*)p = fill;
                            run = (U8)(run - 4);
                            p += 4;
                        } while (run);
                    }
                    else
                    {
                        *p++ = v;
                        last = v;
                        n--;
                    }
                }
            }
            else
            {
                VarBitsGet(v, U8, *vb, 4);
                memset(b->data, v, n);
            }
        }
        else
        {
            b->end = b->data;
            b->cur = b->data + 4;
        }
    }
}

void CheckReadHuff8Bundle(BUNDLE* b, VARBITS* vb, HUFF8* h)
{
    S32 n;
    U32 bit, v, lo, hb, last;
    U8* p;
    U8* tab;
    U8* syms;

    if (b->cur == b->end)
    {
        VARGET(n, vb, b->lenbits);
        if (n)
        {
            p = b->data;
            hb = b->hbits;
            b->cur = p;
            b->end = p + n;
            syms = b->syms;
            tab = b->htab;
            last = h->last;
            if (VarBitsGet1(*vb, bit))
                n = -n - 20;
            do
            {
                HUFFDECODE(last, vb, h->tabs[last], h->bits[last], h->syms[last]);
                HUFFDECODE(lo, vb, tab, hb, syms);
                v = ((last & 15) << 4) | lo;
                if (!(v & 0x80))
                    v = (U8)v | 0x80;
                else
                    v = 0x80 - (v & 0x7F);
                *p++ = v;
            } while (--n > 0);
            if (n < -22)
                memset(b->data, b->data[0], -(n + 21));
            h->last = last;
        }
        else
        {
            b->end = b->data;
            b->cur = b->data + 4;
        }
    }
}

void NewCheckReadHuff8Bundle(BUNDLE* b, VARBITS* vb, HUFF8* h)
{
    S32 n;
    U32 bit, lo, hb, last;
    U8* p;
    U8* tab;
    U8* syms;

    if (b->cur == b->end)
    {
        VARGET(n, vb, b->lenbits);
        if (n)
        {
            p = b->data;
            hb = b->hbits;
            b->cur = p;
            b->end = p + n;
            syms = b->syms;
            tab = b->htab;
            last = h->last;
            if (VarBitsGet1(*vb, bit))
                n = -n - 20;
            do
            {
                HUFFDECODE(last, vb, h->tabs[last], h->bits[last], h->syms[last]);
                HUFFDECODE(lo, vb, tab, hb, syms);
                *p++ = lo | (last << 4);
            } while (--n > 0);
            if (n < -22)
                memset(b->data, b->data[0], -(n + 21));
            h->last = last;
        }
        else
        {
            b->end = b->data;
            b->cur = b->data + 4;
        }
    }
}

void CheckReadHuff4Bundle(BUNDLE* b, VARBITS* vb)
{
    U32 n, bit, v, hb;
    U8* p;
    U8* tab;

    if (b->cur == b->end)
    {
        VARGET(n, vb, b->lenbits);
        if (n)
        {
            b->cur = b->data;
            b->end = b->data + n;
            if (!VarBitsGet1(*vb, bit))
            {
                U8* syms = b->syms;
                hb = b->hbits;
                p = b->data;
                tab = b->htab;
                while (n--)
                {
                    HUFFDECODE(*p++, vb, tab, hb, syms);
                }
            }
            else
            {
                VarBitsGet(v, U32, *vb, 4);
                memset(b->data, v, n);
            }
        }
        else
        {
            b->end = b->data;
            b->cur = b->data + 4;
        }
    }
}

void CheckReadHuff4PairBundle(BUNDLE* b, VARBITS* vb)
{
    U32 n, hb, lo, hi;
    U8* p;
    U8* tab;
    U8* syms;

    if (b->cur == b->end)
    {
        VARGET(n, vb, b->lenbits);
        if (n)
        {
            b->cur = b->data;
            b->end = b->data + n;
            syms = b->syms;
            hb = b->hbits;
            p = b->data;
            tab = b->htab;
            while (n--)
            {
                HUFFDECODE(lo, vb, tab, hb, syms);
                HUFFDECODE(hi, vb, tab, hb, syms);
                *p++ = lo | (hi << 4);
            }
        }
        else
        {
            b->end = b->data;
            b->cur = b->data + 4;
        }
    }
}

void CheckReadHuff4SBundle(BUNDLE* b, VARBITS* vb)
{
    U32 n, bit, v, hb;
    S8 s;
    S8* p;
    U8* tab;
    U8* syms;

    if (b->cur == b->end)
    {
        VARGET(n, vb, b->lenbits);
        if (n)
        {
            b->cur = b->data;
            b->end = b->data + n;
            if (!VarBitsGet1(*vb, bit))
            {
                hb = b->hbits;
                p = (S8*)b->data;
                syms = b->syms;
                tab = b->htab;
                while (n--)
                {
                    HUFFDECODE(s, vb, tab, hb, syms);
                    if (s)
                    {
                        if (VarBitsGet1(*vb, bit))
                            s = -s;
                    }
                    *p++ = s;
                }
            }
            else
            {
                VarBitsGet(v, U32, *vb, 4);
                if (v)
                {
                    if (VarBitsGet1(*vb, bit))
                        v = (S8)-v;
                }
                memset(b->data, v, n);
            }
        }
        else
        {
            b->end = b->data;
            b->cur = b->data + 4;
        }
    }
}

void radmemset16(void* dest, U32 value, U32 bytes);

void CheckReadDelta16Bundle(BUNDLE* b, VARBITS* vb)
{
    U32 n, run, nbits, bit, total;
    U16 v, d16;
    S32 d;
    U16* p;

    if (b->cur == b->end)
    {
        VARGET(n, vb, b->lenbits);
        if (n)
        {
            p = (U16*)b->data;
            if (b->sign == 0)
            {
                VARGET(v, vb, b->bits);
            }
            else
            {
                VARGET(v, vb, b->bits - 1);
                if (v)
                {
                    if (VarBitsGet1(*vb, bit))
                        v = -v;
                }
            }
            *p++ = v;
            total = n;
            n--;
            b->cur = b->data;
            b->end = b->data + total * 2;
            while (n)
            {
                run = n;
                if (run > 8)
                    run = 8;
                VarBitsGet(nbits, U32, *vb, 4);
                if (nbits == 0)
                {
                    radmemset16(p, v, run * 2);
                    p += run;
                    n -= run;
                }
                else
                {
                    n -= run;
                    while (run--)
                    {
                        VARGET(d16, vb, nbits);
                        d = (S16)d16;
                        if (d)
                        {
                            if (VarBitsGet1(*vb, bit))
                                d = (S16)-d16;
                        }
                        v += d;
                        *p++ = v;
                    }
                }
            }
        }
        else
        {
            b->end = b->data;
            b->cur = b->data + 4;
        }
    }
}

U32 getbunsize(U32 count, U32 width, U32 bits, U32 per)
{
    if (bits <= 8)
        bits = 8;
    else if (bits < 16)
        bits = 16;
    return (((count * bits) >> 3) + (((width >> 3) * per * bits) >> 3) + 3) & ~3;
}

void ExpandBundleSizes(U32* sizes, U32 width)
{
    sizes[0] = getbunsize(512, width, 4, 1);
    sizes[1] = getbunsize(512, width >> 1, 4, 1);
    sizes[2] = getbunsize(512, width, 8, 64);
    sizes[3] = getbunsize(512, width, 8, 8);
    sizes[4] = getbunsize(512, width, 5, 1);
    sizes[5] = getbunsize(512, width, 5, 1);
    sizes[6] = getbunsize(512, width, 11, 1);
    sizes[7] = getbunsize(512, width, 11, 1);
    sizes[8] = getbunsize(512, width, 4, 48);
}

void ExpandBink(U8* y, U8* prevy, U8* a, U8* preva, U8* mask, U32 w, U32 h, U32 yw, U32 yh, U32* data, U32 key,
                void* state, U32 flags, U32 type)
{
    U32 next;

    if (type & 0x100000)
    {
        if (flags & 0x100000)
            ExpandPlane(a, preva, (w + 7) & ~7, (h + 7) & ~7, yw, data + 1, key, mask, 1, state, flags);
        data = (U32*)((U8*)data + *data);
    }
    if (!(flags & 0x8000))
        data++;
    next = ExpandPlane(y, prevy, (w + 7) & ~7, (h + 7) & ~7, yw, data, key, mask, 1, state, flags);
    if (!(flags & 0x8000))
        next = (U32)((U8*)data + data[-1] - 4);
    if (!(flags & 0x20000))
    {
        y += yw * yh;
        prevy += yw * yh;
        w = (((w + 1) >> 1) + 7) & ~7;
        h = (((h + 1) >> 1) + 7) & ~7;
        yw >>= 1;
        yh >>= 1;
        next = ExpandPlane(y, prevy, w, h, yw, (void*)next, key, mask, 2, state, flags);
        ExpandPlane(y + yw * yh, prevy + yw * yh, w, h, yw, (void*)next, key, mask, 2, state, flags);
    }
}
