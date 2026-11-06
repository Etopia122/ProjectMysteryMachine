/*
 * Bink audio decoder: band-scaled, run-length coded transform coefficients
 * run through an inverse RDFT or DCT (fft.c), with the overlap between
 * blocks cross-faded. Compiled with SN ProDG (GCC 2.95).
 */

#include "bink/rad.h"
#include "bink/varbits.h"

double pow(double x, double y);
void rdft(int n, int isgn, float* a, int* ip, float* w);
void ddct(int n, int isgn, float* a, int* ip, float* w);
void pushmalloc(void* ptr, U32 bytes);
void* popmalloc(U32 bytes);

#define BINKACDCT 1

typedef struct BINKAC
{
    U32 size;     /* 0x00: transform size */
    F32 root;     /* 0x04: 2/sqrt(size) */
    U32 outsize;  /* 0x08: output bytes per block */
    U32 overlap;  /* 0x0C: bytes cross-faded with the previous block */
    U32 chans;    /* 0x10 */
    S16* samples; /* 0x14 */
    U32 start;    /* 0x18: first block (nothing to fade with) */
    U32 numbands; /* 0x1C */
    U32* bands;   /* 0x20 */
    int* ip;      /* 0x24: FFT work area */
    F32* w;       /* 0x28: FFT tables */
    S16* prev;    /* 0x2C: tail of the previous block */
    U32 flags;    /* 0x30 */
} BINKAC;

/* RLE run lengths, in units of 8 coefficients */
U8 bink_rlelens[16] = { 2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 13, 14, 15, 16, 32, 64 };

/* Band edges in Hz */
U32 bink_bandtopfreq[25] = { 0,    100,  200,  300,  400,  510,  630,  770,  920,  1080, 1270, 1480, 1720,
                             2000, 2320, 2700, 3150, 3700, 4400, 5300, 6400, 7700, 9500, 12000, 15500 };

/* Mantissa scale per exponent: 2^(e-23) */
double bink_fxptab[24] = {
    1.0 / 8388608.0, 1.0 / 4194304.0, 1.0 / 2097152.0, 1.0 / 1048576.0, 1.0 / 524288.0, 1.0 / 262144.0,
    1.0 / 131072.0,  1.0 / 65536.0,   1.0 / 32768.0,   1.0 / 16384.0,   1.0 / 8192.0,   1.0 / 4096.0,
    1.0 / 2048.0,    1.0 / 1024.0,    1.0 / 512.0,     1.0 / 256.0,     1.0 / 128.0,    1.0 / 64.0,
    1.0 / 32.0,      1.0 / 16.0,      1.0 / 8.0,       1.0 / 4.0,       1.0 / 2.0,      1.0,
};

static float fxptof(U32 v)
{
    float f = (float)((double)((v & ~0x10000000) >> 5) * bink_fxptab[v & 31]);
    if (v & 0x10000000)
        f = -f;
    return f;
}

static void quanttos16s(S16* out, F32* in, U32 n, F32 scale)
{
    for (; n; n--)
    {
        S32 v = (S32)(*in * scale);
        *out++ = (v >= 32767) ? 32767 : ((v <= -32768) ? -32768 : (S16)v);
        in++;
    }
}

static void quanttos16chans2(S16* out, F32* in, U32 n, F32 scale)
{
    U32 i = n;
    while (i--)
    {
        S32 v = (S32)(in[0] * scale);
        *out++ = (v >= 32767) ? 32767 : ((v <= -32768) ? -32768 : (S16)v);
        v = (S32)(in[n] * scale);
        *out++ = (v >= 32767) ? 32767 : ((v <= -32768) ? -32768 : (S16)v);
        in++;
    }
}

static void read_rle_samples(F32* out, U32 size, VARBITS* vb, F32* bandscales, U32* bands)
{
    U32 i, j, end, len, v, sgn, t;
    F32 scale = 0.0f;
    S32 band = 0;
    F32* o;

    while ((U32)(bands[band] * 2) < 2)
    {
        scale = bandscales[band];
        band++;
    }

    i = 2;
    o = out + 2;
    while (i < size)
    {
        if (VarBitsGet1(*vb, t))
        {
            VarBitsGet(end, U32, *vb, 4);
            end = i + bink_rlelens[end] * 8;
        }
        else
        {
            end = i + 8;
        }
        if (end > size)
            end = size;

        VarBitsGet(len, U32, *vb, 4);
        if (len == 0)
        {
            memset(o, 0, (end - i) * 4);
            o += end - i;
            i = end;
            while (i > (U32)(bands[band] * 2))
            {
                scale = bandscales[band];
                band++;
            }
        }
        else
        {
            for (; i < end; i++)
            {
                if (i == bands[band] * 2)
                {
                    scale = bandscales[band];
                    band++;
                }
                VarBitsGet(v, U32, *vb, len);
                if (v)
                {
                    sgn = -VarBitsGet1(*vb, j);
                    *o = (F32)(S32)((v ^ sgn) - sgn) * scale;
                }
                else
                {
                    *o = 0.0f;
                }
                o++;
            }
        }
    }
}

static U32 Unquant(U32 size, U32 chans, U32 flags, int* ip, F32* w, S16* outp, void* inp, U32 numbands,
                   U32* bands, F32 scale)
{
    F32 bandscales[26];
    VARBITS vb;
    F32 samples[4096];
    F32* s;
    U32 c, i, v;

    vb.cur = vb.init = (U32*)inp;
    vb.bits = vb.bitlen = 0;
    if (flags & BINKACDCT)
    {
        /* the DCT stream starts with two unused bits */
        vb.cur = (U32*)inp + 1;
        vb.bitlen = 30;
        vb.bits = *(U32*)inp >> 2;
    }

    s = samples;
    for (c = 0; c < chans; c++)
    {
        VarBitsGet(v, U32, vb, 29);
        s[0] = fxptof(v);
        VarBitsGet(v, U32, vb, 29);
        s[1] = fxptof(v);

        for (i = 0; i < numbands; i++)
        {
            VarBitsGet(v, U32, vb, 8);
            bandscales[i] = (F32)pow(10.0, (F32)(S32)v * 0.664f * 0.1f);
        }

        read_rle_samples(s, size, &vb, bandscales, bands);

        if (flags & BINKACDCT)
            ddct(size, 1, s, ip, w);
        else
            rdft(size, -1, s, ip, w);
        s += size;
    }

    if (chans == 1)
        quanttos16s(outp, samples, size, scale);
    else
        quanttos16chans2(outp, samples, size, scale);

    return ((U8*)vb.cur - (U8*)vb.init) & 0x1FFFFFFF;
}

BINKAC* BinkAudioDecompressOpen(U32 rate, U32 chans, U32 flags)
{
    BINKAC* ac;
    U32 size, total, outsize, halfrate, half, i, numbands;
    U32* bands;
    int* ip;
    F32* w;
    S16* prev;
    S16* samples;

    if (rate >= 44100)
        size = 2048;
    else if (rate >= 22050)
        size = 1024;
    else
        size = 512;

    total = size * chans;
    outsize = total * 2;
    if (!(flags & BINKACDCT))
    {
        rate *= chans;
        size = total;
        chans = 1;
    }

    halfrate = (rate + 1) >> 1;
    half = size >> 1;
    for (i = 0; i < 25; i++)
    {
        if (bink_bandtopfreq[i] >= halfrate)
            break;
    }
    numbands = i;

    pushmalloc(&bands, (i + 1) * 4);
    pushmalloc(&ip, ((U32)radsqrtf((F32)half) + 2) * 4);
    if (flags & BINKACDCT)
        pushmalloc(&w, size * 5);
    else
        pushmalloc(&w, half * 4 - 1);
    pushmalloc(&prev, outsize >> 1);
    pushmalloc(&samples, outsize);

    ac = (BINKAC*)popmalloc(sizeof(BINKAC));
    if (ac == 0)
        return 0;

    memset(ac, 0, sizeof(BINKAC));
    ac->bands = bands;
    ac->ip = ip;
    ac->w = w;
    ac->prev = prev;
    ac->flags = flags;
    ac->samples = samples;
    ac->chans = chans;
    ac->numbands = numbands;
    ac->overlap = outsize >> 4;
    ac->size = size;
    ac->outsize = outsize;
    ac->root = 2.0f / radsqrtf((F32)size);

    for (i = 0; i < numbands; i++)
    {
        ac->bands[i] = (bink_bandtopfreq[i] * half) / halfrate;
        if (ac->bands[i] == 0)
            ac->bands[i] = 1;
    }
    ac->bands[i] = half;
    ac->ip[0] = 0;
    ac->start = 1;
    return ac;
}

void BinkAudioDecompress(BINKAC* ac, S16** outp, U32* outlen, void* inp, void** inpend)
{
    U32 used = Unquant(ac->size, ac->chans, ac->flags, ac->ip, ac->w, ac->samples, inp, ac->numbands,
                       ac->bands, ac->root);

    if (ac->start == 0)
    {
        U32 i, n = ac->overlap >> 1, j = n;
        for (i = 0; i < n; i++)
        {
            ac->samples[i] = (S16)((ac->samples[i] * i + ac->prev[i] * j) / n);
            j--;
        }
    }
    else
    {
        ac->start = 0;
    }

    memcpy(ac->prev, (U8*)ac->samples + (ac->outsize - ac->overlap), ac->overlap);
    if (outlen)
        *outlen = ac->outsize - ac->overlap;
    if (outp)
        *outp = ac->samples;
    if (inpend)
        *inpend = (U8*)inp + used;
}

void BinkAudioDecompressClose(BINKAC* ac)
{
    radfree(ac);
}
