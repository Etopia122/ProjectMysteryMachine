/*
 * Bink bit-plane coders (alpha / lossless planes).
 * Compiled with SN ProDG (GCC 2.95).
 */

#include "bink/varbits.h"

void readlossy(S8* out, VARBITS* vb, S32 q);

void readlossy(S8* out, VARBITS* src, S32 maxcount)
{
    U8 buf[136];
    U8 sig[64];
    VARBITS vb;
    S32 count, nsig, i, val;
    U32 bit, v;
    U8 b;
    U8 planes;
    U8* lp;
    U8* le;
    U8* nl;

    vb = *src;
    count = 0;
    memset(out, 0, 64);
    VarBitsGet(planes, U8, vb, 3);
    planes++;
    lp = buf + 68;
    lp[0] = 0x10;
    lp[1] = 0x60;
    lp[2] = 0xB0;
    val = (S8)(1 << (planes - 1));
    lp[3] = 2;
    le = lp + 4;
    nsig = 0;
    while (planes)
    {
        for (i = 0; i < nsig; i++)
        {
            if (VarBitsGet1(vb, bit))
            {
                S8 c = out[sig[i]];
                out[sig[i]] = c + ((c < 0) ? -val : val);
                if (count++ == maxcount)
                    goto done;
            }
        }
        nl = lp;
        if (lp < le)
        {
            do
            {
                b = *lp;
                if (b && VarBitsGet1(vb, bit))
                {
                    switch (b & 3)
                    {
                    case 0:
                        *lp = ((b >> 2) + 4) * 4 + 1;
                        goto kids;
                    case 1:
                        b >>= 2;
                        *lp = b * 4 + 2;
                        *le++ = (b + 4) * 4 + 2;
                        *le++ = (b + 8) * 4 + 2;
                        *le++ = (b + 12) * 4 + 2;
                        continue;
                    case 2:
                        *lp++ = 0;
                    kids:
                        b >>= 2;
                        if (VarBitsGet1(vb, bit))
                        {
                            *--nl = (b) * 4 + 3;
                        }
                        else
                        {
                            sig[nsig++] = b;
                            out[b] = VarBitsGet1(vb, bit) ? -val : val;
                            if (count++ == maxcount)
                                goto done;
                        }
                        b++;
                        if (VarBitsGet1(vb, bit))
                        {
                            *--nl = (b) * 4 + 3;
                        }
                        else
                        {
                            sig[nsig++] = b;
                            out[b] = VarBitsGet1(vb, bit) ? -val : val;
                            if (count++ == maxcount)
                                goto done;
                        }
                        b++;
                        if (VarBitsGet1(vb, bit))
                        {
                            *--nl = (b) * 4 + 3;
                        }
                        else
                        {
                            sig[nsig++] = b;
                            out[b] = VarBitsGet1(vb, bit) ? -val : val;
                            if (count++ == maxcount)
                                goto done;
                        }
                        b++;
                        if (VarBitsGet1(vb, bit))
                        {
                            *--nl = (b) * 4 + 3;
                        }
                        else
                        {
                            sig[nsig++] = b;
                            out[b] = VarBitsGet1(vb, bit) ? -val : val;
                            if (count++ == maxcount)
                                goto done;
                        }
                        continue;
                    case 3:
                        sig[nsig++] = b >> 2;
                        out[b >> 2] = VarBitsGet1(vb, bit) ? -val : val;
                        if (count++ == maxcount)
                            goto done;
                        *lp = 0;
                        break;
                    }
                }
                lp++;
            } while (lp < le);
        }
        val >>= 1;
        planes--;
        lp = nl;
    }
done:
    *src = vb;
}

void ReadBPLossy(S16* out, VARBITS* vb, S32 q)
{
    S16 t[32];

    readlossy((S8*)t, vb, q);
    out[0] = t[0];
    out[1] = t[2];
    out[2] = t[4];
    out[3] = t[6];
    out[4] = t[1];
    out[5] = t[3];
    out[6] = t[5];
    out[7] = t[7];
    out[8] = t[12];
    out[9] = t[22];
    out[10] = t[8];
    out[11] = t[10];
    out[12] = t[13];
    out[13] = t[23];
    out[14] = t[9];
    out[15] = t[11];
    out[16] = t[14];
    out[17] = t[16];
    out[18] = t[24];
    out[19] = t[26];
    out[20] = t[15];
    out[21] = t[17];
    out[22] = t[25];
    out[23] = t[27];
    out[24] = t[18];
    out[25] = t[20];
    out[26] = t[28];
    out[27] = t[30];
    out[28] = t[19];
    out[29] = t[21];
    out[30] = t[29];
    out[31] = t[31];
}

void ReadBPLossyWithMotion(U8* out, S32 pitch, VARBITS* vb, S32 q, U8* prev)
{
    U8 t[64];
    U8* p;

    readlossy((S8*)t, vb, q);
    p = out + pitch;
    out[0] = t[0] + prev[0];
    out[1] = t[1] + prev[1];
    out[2] = t[4] + prev[2];
    out[3] = t[5] + prev[3];
    out[4] = t[8] + prev[4];
    out[5] = t[9] + prev[5];
    out[6] = t[12] + prev[6];
    out[7] = t[13] + prev[7];
    p[0] = t[2] + prev[8];
    p[1] = t[3] + prev[9];
    p[2] = t[6] + prev[10];
    p[3] = t[7] + prev[11];
    p[4] = t[10] + prev[12];
    p[5] = t[11] + prev[13];
    p[6] = t[14] + prev[14];
    p[7] = t[15] + prev[15];
    p += pitch;
    p[0] = t[24] + prev[16];
    p[1] = t[25] + prev[17];
    p[2] = t[44] + prev[18];
    p[3] = t[45] + prev[19];
    p[4] = t[16] + prev[20];
    p[5] = t[17] + prev[21];
    p[6] = t[20] + prev[22];
    p[7] = t[21] + prev[23];
    p += pitch;
    p[0] = t[26] + prev[24];
    p[1] = t[27] + prev[25];
    p[2] = t[46] + prev[26];
    p[3] = t[47] + prev[27];
    p[4] = t[18] + prev[28];
    p[5] = t[19] + prev[29];
    p[6] = t[22] + prev[30];
    p[7] = t[23] + prev[31];
    p += pitch;
    p[0] = t[28] + prev[32];
    p[1] = t[29] + prev[33];
    p[2] = t[32] + prev[34];
    p[3] = t[33] + prev[35];
    p[4] = t[48] + prev[36];
    p[5] = t[49] + prev[37];
    p[6] = t[52] + prev[38];
    p[7] = t[53] + prev[39];
    p += pitch;
    p[0] = t[30] + prev[40];
    p[1] = t[31] + prev[41];
    p[2] = t[34] + prev[42];
    p[3] = t[35] + prev[43];
    p[4] = t[50] + prev[44];
    p[5] = t[51] + prev[45];
    p[6] = t[54] + prev[46];
    p[7] = t[55] + prev[47];
    p += pitch;
    p[0] = t[36] + prev[48];
    p[1] = t[37] + prev[49];
    p[2] = t[40] + prev[50];
    p[3] = t[41] + prev[51];
    p[4] = t[56] + prev[52];
    p[5] = t[57] + prev[53];
    p[6] = t[60] + prev[54];
    p[7] = t[61] + prev[55];
    p += pitch;
    p[0] = t[38] + prev[56];
    p[1] = t[39] + prev[57];
    p[2] = t[42] + prev[58];
    p[3] = t[43] + prev[59];
    p[4] = t[58] + prev[60];
    p[5] = t[59] + prev[61];
    p[6] = t[62] + prev[62];
    p[7] = t[63] + prev[63];
}
