/*
 * Bink 8x8 inverse/forward DCTs: the AAN fast integer IDCT from the IJG JPEG
 * library (jidctfst.c), with 11-bit constants and the dequantisation folded
 * into the first pass. Compiled with SN ProDG (GCC 2.95).
 */

#include "bink/rad.h"

#define FIX_1_082392200 2217 /* FIX(1.082392200) */
#define FIX_1_414213562 2896 /* FIX(1.414213562) */
#define FIX_1_847759065 3784 /* FIX(1.847759065) */
#define FIX_2_613125930 5352 /* FIX(2.613125930) */

#define MULTIPLY(var, const) (((var) * (const)) >> 11)
#define DEQUANTIZE(coef, quantval) (((S32)(coef) * (quantval)) >> 11)
#define DESCALE(x) ((U8)(((x) + 127) >> 8))

extern const S32 bink_intra_quant[32][64];
extern const S32 bink_inter_quant[32][64];

void fastidct8x8(U8* out, S32 pitch, S16* coef, const S32* quant)
{
    S32 tmp[8];
    S32 tmp10, tmp11, tmp12, tmp13;
    S32 z5, z10, z11, z12, z13;
    S16* inptr;
    const S32* quantptr;
    S32* wsptr;
    S32 ctr;
    S32 workspace[64];

    /* Pass 1: process columns from input, store into work array. */
    quantptr = quant;
    inptr = coef;
    wsptr = workspace;
    for (ctr = 8; ctr > 0; ctr--)
    {
        if ((inptr[8 * 1] | inptr[8 * 2] | inptr[8 * 3] | inptr[8 * 4] | inptr[8 * 5] | inptr[8 * 6] | inptr[8 * 7]) == 0)
        {
            /* AC terms all zero */
            S32 dcval = DEQUANTIZE(inptr[8 * 0], quantptr[8 * 0]);

            wsptr[8 * 0] = dcval;
            wsptr[8 * 1] = dcval;
            wsptr[8 * 2] = dcval;
            wsptr[8 * 3] = dcval;
            wsptr[8 * 4] = dcval;
            wsptr[8 * 5] = dcval;
            wsptr[8 * 6] = dcval;
            wsptr[8 * 7] = dcval;

            inptr++;
            quantptr++;
            wsptr++;
            continue;
        }

        /* Even part */
        tmp[0] = DEQUANTIZE(inptr[8 * 0], quantptr[8 * 0]);
        tmp[1] = DEQUANTIZE(inptr[8 * 2], quantptr[8 * 2]);
        tmp[2] = DEQUANTIZE(inptr[8 * 4], quantptr[8 * 4]);
        tmp[3] = DEQUANTIZE(inptr[8 * 6], quantptr[8 * 6]);

        tmp10 = tmp[0] + tmp[2];
        tmp11 = tmp[0] - tmp[2];

        tmp13 = tmp[1] + tmp[3];
        tmp12 = MULTIPLY(tmp[1] - tmp[3], FIX_1_414213562) - tmp13;

        tmp[0] = tmp10 + tmp13;
        tmp[3] = tmp10 - tmp13;
        tmp[1] = tmp11 + tmp12;
        tmp[2] = tmp11 - tmp12;

        /* Odd part */
        tmp[4] = DEQUANTIZE(inptr[8 * 1], quantptr[8 * 1]);
        tmp[5] = DEQUANTIZE(inptr[8 * 3], quantptr[8 * 3]);
        tmp[6] = DEQUANTIZE(inptr[8 * 5], quantptr[8 * 5]);
        tmp[7] = DEQUANTIZE(inptr[8 * 7], quantptr[8 * 7]);

        z13 = tmp[6] + tmp[5];
        z10 = tmp[6] - tmp[5];
        z11 = tmp[4] + tmp[7];
        z12 = tmp[4] - tmp[7];

        tmp[7] = z11 + z13;
        tmp11 = MULTIPLY(z11 - z13, FIX_1_414213562);

        z5 = MULTIPLY(z10 + z12, FIX_1_847759065);
        tmp10 = MULTIPLY(z12, FIX_1_082392200) - z5;
        tmp12 = MULTIPLY(z10, -FIX_2_613125930) + z5;

        tmp[6] = tmp12 - tmp[7];
        tmp[5] = tmp11 - tmp[6];
        tmp[4] = tmp10 + tmp[5];

        wsptr[8 * 0] = tmp[0] + tmp[7];
        wsptr[8 * 7] = tmp[0] - tmp[7];
        wsptr[8 * 1] = tmp[1] + tmp[6];
        wsptr[8 * 6] = tmp[1] - tmp[6];
        wsptr[8 * 2] = tmp[2] + tmp[5];
        wsptr[8 * 5] = tmp[2] - tmp[5];
        wsptr[8 * 4] = tmp[3] + tmp[4];
        wsptr[8 * 3] = tmp[3] - tmp[4];

        inptr++;
        quantptr++;
        wsptr++;
    }

    /* Pass 2: process rows from work array, store into output array. */
    wsptr = workspace;
    for (ctr = 8; ctr > 0; ctr--)
    {
        /* Even part */
        tmp10 = wsptr[0] + wsptr[4];
        tmp11 = wsptr[0] - wsptr[4];

        tmp13 = wsptr[2] + wsptr[6];
        tmp12 = MULTIPLY(wsptr[2] - wsptr[6], FIX_1_414213562) - tmp13;

        tmp[0] = tmp10 + tmp13;
        tmp[3] = tmp10 - tmp13;
        tmp[1] = tmp11 + tmp12;
        tmp[2] = tmp11 - tmp12;

        /* Odd part */
        z13 = wsptr[5] + wsptr[3];
        z10 = wsptr[5] - wsptr[3];
        z11 = wsptr[1] + wsptr[7];
        z12 = wsptr[1] - wsptr[7];

        tmp[7] = z11 + z13;
        tmp11 = MULTIPLY(z11 - z13, FIX_1_414213562);

        z5 = MULTIPLY(z10 + z12, FIX_1_847759065);
        tmp10 = MULTIPLY(z12, FIX_1_082392200) - z5;
        tmp12 = MULTIPLY(z10, -FIX_2_613125930) + z5;

        tmp[6] = tmp12 - tmp[7];
        tmp[5] = tmp11 - tmp[6];
        tmp[4] = tmp10 + tmp[5];

        out[0] = DESCALE(tmp[0] + tmp[7]);
        out[7] = DESCALE(tmp[0] - tmp[7]);
        out[1] = DESCALE(tmp[1] + tmp[6]);
        out[6] = DESCALE(tmp[1] - tmp[6]);
        out[2] = DESCALE(tmp[2] + tmp[5]);
        out[5] = DESCALE(tmp[2] - tmp[5]);
        out[4] = DESCALE(tmp[3] + tmp[4]);
        out[3] = DESCALE(tmp[3] - tmp[4]);

        wsptr += 8;
        out += pitch;
    }
}

void FastmIDCT8x8(U8* out, S32 pitch, S16* coef, U32 q)
{
    fastidct8x8(out, pitch, coef, bink_inter_quant[q]);
}

void FastIDCT8x8(U8* out, S32 pitch, S16* coef, U32 q)
{
    fastidct8x8(out, pitch, coef, bink_intra_quant[q]);
}
