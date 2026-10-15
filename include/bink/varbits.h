#ifndef BINK_VARBITS_H
#define BINK_VARBITS_H

/* Little-endian bit reader over 32-bit words (low bits first). */

#include "bink/rad.h"

typedef struct VARBITS
{
    U32* cur;   /* next word */
    U32* init;  /* start of the stream */
    U32 bits;   /* buffered bits */
    U32 bitlen; /* number of buffered bits */
} VARBITS;

#define GetBitsLen(len) (0xFFFFFFFF >> (32 - (len)))

#define VarBitsGet(v, typ, vb, len)                                                                \
    {                                                                                              \
        if ((vb).bitlen >= (U32)(len))                                                             \
        {                                                                                          \
            v = (typ)((vb).bits & GetBitsLen(len));                                                \
            (vb).bits >>= (len);                                                                   \
            (vb).bitlen -= (len);                                                                  \
        }                                                                                          \
        else                                                                                       \
        {                                                                                          \
            U32 nb_ = *(vb).cur++;                                                                 \
            v = (typ)(((vb).bits | (nb_ << (vb).bitlen)) & GetBitsLen(len));                       \
            (vb).bits = nb_ >> ((len) - (vb).bitlen);                                              \
            (vb).bitlen = (vb).bitlen + 32 - (len);                                                \
        }                                                                                          \
    }

#define VarBitsGet1(vb, i)                                                                         \
    (((vb).bitlen) ? (i = (vb).bits & 1, (vb).bits >>= 1, (vb).bitlen--, i)                         \
                   : (i = *(vb).cur++, (vb).bits = i >> 1, (vb).bitlen = 31, i & 1))

#endif
