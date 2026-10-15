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
typedef void (*BINKSNDPAUSE)(BINKSND* snd, S32 pause);
typedef S32 (*BINKSNDONOFF)(BINKSND* snd, S32 onoff);
typedef void (*BINKSNDCLOSE)(BINKSND* snd);
typedef struct BINKIO BINKIO;
typedef U32 (*BINKIOREADFRAME)(BINKIO* bio, U32 frame, S32 origofs, void* dest, U32 size);
typedef void (*BINKIOCLOSE)(BINKIO* bio);
typedef S32 (*BINKIOIDLE)(BINKIO* bio);

struct BINKIO
{
    void* ReadHeader; /* 0x0 */
    BINKIOREADFRAME ReadFrame; /* 0x4 */
    U8 pad8[0x8];
    BINKIOIDLE Idle; /* 0x10 */
    BINKIOCLOSE Close; /* 0x14 */
    U8 pad18[0x10];
    volatile U32 Working; /* 0x28 */
    U8 pad2C[0x18];
    volatile U32 BufSize; /* 0x44 */
    volatile U32 CurBufUsed; /* 0x48 */
    U8 pad4C[0xB0];
    U8 cb[28]; /* 0xFC */
    U8 pad118[0x24];
};

struct BINKSND
{
    U8 pad0[0xC];
    BINKSNDVOLUME Volume; /* 0xC */
    BINKSNDPAN Pan; /* 0x10 */
    BINKSNDPAUSE Pause; /* 0x14 */
    BINKSNDONOFF SetOnOff; /* 0x18 */
    BINKSNDCLOSE Close; /* 0x1C */
    BINKSNDMIXBINS MixBins; /* 0x20 */
    BINKSNDMIXBINVOLS MixBinVols; /* 0x24 */
    U32 sndbufsize; /* 0x28 */
    U8* sndbuf; /* 0x2C */
    U8 pad30[0x4];
    U8* sndreadpos; /* 0x34 */
    U8* sndwritepos; /* 0x38 */
    void* ac; /* 0x3C */
    U32 sndamt; /* 0x40 */
    U8 pad44[0x4];
    S32 sndendframe; /* 0x48 */
    U32 sndprime; /* 0x4C */
    U8 pad50[0xC];
    U32 sndstarved; /* 0x5C */
    U8 pad60[0x8];
    U32 sndrateadj; /* 0x68 */
    U32 freq; /* 0x6C */
    U32 bits; /* 0x70 */
    U32 chans; /* 0x74 */
    U8 pad78[0x100];
};

typedef struct BINK BINK;
struct BINK
{
    U32 Width; /* 0x0 */
    U32 Height; /* 0x4 */
    U32 Frames; /* 0x8 */
    U32 FrameNum; /* 0xC */
    U32 LastFrameNum; /* 0x10 */
    U32 FrameRate; /* 0x14 */
    U32 FrameRateDiv; /* 0x18 */
    U32 ReadError; /* 0x1C */
    U32 OpenFlags; /* 0x20 */
    U8 pad24[0x98];
    void* framemem; /* 0xBC */
    U8 padC0[0x3C];
    S32 Paused; /* 0xFC */
    U8 pad100[0x4];
    U8* compframe; /* 0x104 */
    U8* preloadptr; /* 0x108 */
    U32* frameoffsets; /* 0x10C */
    BINKIO bio; /* 0x110 */
    void* iobuf; /* 0x24C */
    U8 pad250[0xC];
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
    S32 timerslop; /* 0x288 */
    U32 startframetime; /* 0x28C */
    U32 slowestframetime; /* 0x290 */
    U32 slowestframe; /* 0x294 */
    U32 slowest2frametime; /* 0x298 */
    U32 slowest2frame; /* 0x29C */
    S32 soundon; /* 0x2A0 */
    S32 videoon; /* 0x2A4 */
    U32 totalmem; /* 0x2A8 */
    U8 pad2AC[0x8];
    U32 totalblittime; /* 0x2B4 */
    U8 pad2B8[0x34];
    U32 lastdecodedframe; /* 0x2EC */
    S32 playingtracks; /* 0x2F0 */
    U32 soundstarves; /* 0x2F4 */
    BINKSND* bsnd; /* 0x2F8 */
    U32 lastblitdone; /* 0x2FC */
    U32 skippedblits; /* 0x300 */
    U8 pad304[0x2C];
    U32 soundskips; /* 0x330 */
    U32 soundskipframes; /* 0x334 */
    U8 pad338[0x8];
    U32 soundticks; /* 0x340 */
    U8 soundcb[28]; /* 0x344 */
    U8 pad360[0x24];
};


#endif
