#ifndef BINK_BINKINT_H
#define BINK_BINKINT_H

/* Internal Bink structures (layout recovered from the GameCube build). */

#include "bink/rad.h"

typedef struct BINKSND BINKSND;
typedef void (*BINKSNDVOLUME)(BINKSND* snd, S32 volume);
typedef void (*BINKSNDPAN)(BINKSND* snd, S32 pan);
typedef void (*BINKSNDMIXBINS)(BINKSND* snd, U32* bins, U32 total);
typedef void (*BINKSNDMIXBINVOLS)(BINKSND* snd, U32* bins, S32* volumes, U32 total);
typedef U32 (*BINKSNDSYSOPEN)(U32 param);

struct BINKSND
{
    U8 pad0[0xC];
    BINKSNDVOLUME Volume; /* 0xC */
    BINKSNDPAN Pan; /* 0x10 */
    U8 pad14[0xC];
    BINKSNDMIXBINS MixBins; /* 0x20 */
    BINKSNDMIXBINVOLS MixBinVols; /* 0x24 */
    U8 pad28[0x150];
};

typedef struct BINK BINK;
struct BINK
{
    U32 Width; /* 0x0 */
    U32 Height; /* 0x4 */
    U32 Frames; /* 0x8 */
    U32 FrameNum; /* 0xC */
    U8 pad10[0xEC];
    U8 bio[4]; /* 0xFC */
    U8 pad100[0x15C];
    S32* trackindexes; /* 0x25C */
    U32* tracksizes; /* 0x260 */
    U32* tracktypes; /* 0x264 */
    S32* trackIDs; /* 0x268 */
    U8 pad26C[0x4];
    U32 runtimeframes; /* 0x270 */
    U8 pad274[0x8];
    U32 startblittime; /* 0x27C */
    U32 timerstart; /* 0x280 */
    U32 timerframe; /* 0x284 */
    U8 pad288[0x4];
    U32 startframetime; /* 0x28C */
    U32 slowestframetime; /* 0x290 */
    U32 slowestframe; /* 0x294 */
    U32 slowest2frametime; /* 0x298 */
    U32 slowest2frame; /* 0x29C */
    U8 pad2A0[0x4];
    S32 videoon; /* 0x2A4 */
    U32 totalmem; /* 0x2A8 */
    U8 pad2AC[0x8];
    U32 totalblittime; /* 0x2B4 */
    U8 pad2B8[0x38];
    S32 playingtracks; /* 0x2F0 */
    U8 pad2F4[0x4];
    BINKSND* bsnd; /* 0x2F8 */
    U8 pad2FC[0x44];
    U32 soundticks; /* 0x340 */
    U8 soundcb[0x1C]; /* 0x344 */
    U8 pad345[0x1B];
};


#endif
