/*
 * Bink bit stream helpers: copy bits from a reader into a writer.
 * Compiled with SN ProDG (GCC 2.95).
 */

#include "bink/varbits.h"

extern U32 VarBitsLens[];

#define VarBitsPut(vb, val, size)                                                                  \
    {                                                                                              \
        U32 m_ = (val) & VarBitsLens[size];                                                        \
        (vb).bits |= m_ << (vb).bitlen;                                                            \
        (vb).bitlen += (size);                                                                     \
        if ((vb).bitlen >= 32)                                                                     \
        {                                                                                          \
            *(vb).cur = (vb).bits;                                                                 \
            (vb).cur++;                                                                            \
            (vb).bitlen -= 32;                                                                     \
            if ((vb).bitlen)                                                                       \
                (vb).bits = m_ >> ((size) - (vb).bitlen);                                          \
            else                                                                                   \
                (vb).bits = 0;                                                                     \
        }                                                                                          \
    }

void fn_80127FC8(VARBITS* out, VARBITS* in, U32 bits)
{
    U32 v;
    U32 n = 8;

    while (bits >= 8)
    {
        VarBitsGet(v, U32, *in, 8);
        VarBitsPut(*out, v, n);
        bits -= 8;
    }
    if (bits)
    {
        VarBitsGet(v, U32, *in, bits);
        VarBitsPut(*out, v, bits);
    }
}
