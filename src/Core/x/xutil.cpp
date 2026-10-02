#include "iMath.h"

extern unsigned char __ctype_map[256];

static inline signed int isprint(signed int c)
{
    return __ctype_map[(unsigned char)c] & 0xDC;
}

static signed int g_xutilinit;
static signed int g_crc_needinit = 1;
static unsigned int g_crc32_table[256] = { 0 };

unsigned int xUtil_crc_init();

// Functions are in reverse DOL order (-inline deferred emits last to first).

signed int xUtilStartup()
{
    if (!g_xutilinit++)
    {
        xUtil_crc_init();
    }
    return g_xutilinit;
}

signed int xUtilShutdown()
{
    g_xutilinit--;
    return g_xutilinit;
}

char* xUtil_idtag2string(unsigned int srctag, signed int bufidx)
{
    unsigned int tag = srctag;
    char* strptr;
    char* uc = (char*)&tag;
    signed int l;
    char t;
    static char buf[6][10] = { 0 };

    if (bufidx < 0 || bufidx >= 7)
    {
        strptr = buf[0];
    }
    else
    {
        strptr = buf[bufidx];
    }

    l = 1;
    if ((signed int)((char*)&l)[3] != 0)
    {
        t = uc[0];
        uc[0] = uc[3];
        uc[3] = t;

        t = uc[1];
        uc[1] = uc[2];
        uc[2] = t;
    }

    switch (bufidx)
    {
    case 4:
    case 5:
        strptr[0] = isprint(uc[0]) ? uc[0] : '?';
        strptr[1] = isprint(uc[1]) ? uc[1] : '?';
        strptr[2] = isprint(uc[2]) ? uc[2] : '?';
        strptr[3] = isprint(uc[3]) ? uc[3] : '?';
        break;
    case 6:
    default:
        strptr[0] = isprint(uc[3]) ? uc[3] : '?';
        strptr[1] = isprint(uc[2]) ? uc[2] : '?';
        strptr[2] = isprint(uc[1]) ? uc[1] : '?';
        strptr[3] = isprint(uc[0]) ? uc[0] : '?';
        break;
    }

    strptr[4] = '\0';

    if (bufidx == 6)
    {
        strptr[4] = '/';
        strptr[5] = isprint(uc[0]) ? uc[0] : '?';
        strptr[6] = isprint(uc[1]) ? uc[1] : '?';
        strptr[7] = isprint(uc[2]) ? uc[2] : '?';
        strptr[8] = isprint(uc[3]) ? uc[3] : '?';
        strptr[9] = '\0';
    }

    return strptr;
}

#pragma auto_inline on
unsigned int xUtil_crc_init()
{
    signed int i;
    signed int j;
    unsigned int crc_accum;

    if (g_crc_needinit)
    {
        for (i = 0; i < 256; i++)
        {
            crc_accum = (unsigned int)i << 24;
            for (j = 0; j < 8; j++)
            {
                if (crc_accum & (1 << 31))
                {
                    crc_accum = (crc_accum << 1) ^ 0x04C11DB7;
                }
                else
                {
                    crc_accum = (crc_accum << 1);
                }
            }
            g_crc32_table[i] = crc_accum;
        }
        g_crc_needinit = 0;
    }
    return 0xFFFFFFFF;
}
#pragma auto_inline off

unsigned int xUtil_crc_update(unsigned int crc_accum, char* data, signed int datasize)
{
    signed int i;
    signed int j;

    if (g_crc_needinit)
    {
        xUtil_crc_init();
    }
    for (i = 0; i < datasize; i++)
    {
        j = ((crc_accum >> 24) ^ *data++) & 0xff;
        crc_accum = (crc_accum << 8) ^ g_crc32_table[j];
    }
    return crc_accum;
}

signed int xUtil_yesno()
{
    return (iurand() <= 0.5f) ? 1 : 0;
}

signed int xUtil_yesno(float wt_yes)
{
    if (0.0f == wt_yes)
    {
        return 0;
    }
    if (1.0f == wt_yes)
    {
        return 1;
    }
    return (iurand() <= wt_yes);
}
