#ifndef BINK_BINKINT_H
#define BINK_BINKINT_H

/* Internal Bink structures (layout recovered from the GameCube build). */

#include "bink/rad.h"

typedef struct BINKRECT
{
    S32 Left;
    S32 Top;
    S32 Width;
    S32 Height;
} BINKRECT;

typedef struct BINKSND BINKSND;
typedef void (*BINKSNDVOLUME)(BINKSND* snd, S32 volume);
typedef void (*BINKSNDPAN)(BINKSND* snd, S32 pan);
typedef void (*BINKSNDMIXBINS)(BINKSND* snd, U32* bins, U32 total);
typedef void (*BINKSNDMIXBINVOLS)(BINKSND* snd, U32* bins, S32* volumes, U32 total);
typedef U32 (*BINKSNDSYSOPEN)(U32 param);
typedef S32 (*BINKSNDREADY)(BINKSND* snd);
typedef S32 (*BINKSNDLOCK)(BINKSND* snd, U8** addr, U32* len);
typedef void (*BINKSNDUNLOCK)(BINKSND* snd, U32 filled);
typedef void (*BINKSNDPAUSE)(BINKSND* snd, S32 pause);
typedef S32 (*BINKSNDONOFF)(BINKSND* snd, S32 onoff);
typedef void (*BINKSNDCLOSE)(BINKSND* snd);
typedef struct BINKIO BINKIO;
typedef U32 (*BINKIOREADHEADER)(BINKIO* bio, S32 offset, void* dest, U32 size);
typedef U32 (*BINKIOREADFRAME)(BINKIO* bio, U32 frame, S32 origofs, void* dest, U32 size);
typedef U32 (*BINKIOGETBUFFERSIZE)(BINKIO* bio, U32 size);
typedef void (*BINKIOSETINFO)(BINKIO* bio, void* buf, U32 size, U32 filesize, U32 simulate);
typedef void (*BINKIOCLOSE)(BINKIO* bio);
typedef S32 (*BINKIOIDLE)(BINKIO* bio);

struct BINKIO
{
    BINKIOREADHEADER ReadHeader; /* 0x0 */
    BINKIOREADFRAME ReadFrame; /* 0x4 */
    BINKIOGETBUFFERSIZE GetBufferSize; /* 0x8 */
    BINKIOSETINFO SetInfo; /* 0xC */
    BINKIOIDLE Idle; /* 0x10 */
    BINKIOCLOSE Close; /* 0x14 */
    void* bink; /* 0x18 */
    volatile U32 ReadError; /* 0x1C */
    volatile U32 DoingARead; /* 0x20 */
    volatile U32 BytesRead; /* 0x24 */
    volatile U32 Working; /* 0x28 */
    volatile U32 TotalTime; /* 0x2C */
    volatile U32 ForegroundTime; /* 0x30 */
    volatile U32 IdleTime; /* 0x34 */
    volatile U32 ThreadTime; /* 0x38 */
    volatile U32 BufSize; /* 0x3C */
    volatile U32 BufHighUsed; /* 0x40 */
    volatile U32 CurBufSize; /* 0x44 */
    volatile U32 CurBufUsed; /* 0x48 */
    U8 pad4C[0xA0];
    void* suspend_callback; /* 0xEC */
    void* try_suspend_callback; /* 0xF0 */
    void* resume_callback; /* 0xF4 */
    void* idle_on_callback; /* 0xF8 */
    U8 cb[28]; /* 0xFC */
    U8 pad118[0x24];
};

struct BINKSND
{
    BINKSNDREADY Ready; /* 0x0 */
    BINKSNDLOCK Lock; /* 0x4 */
    BINKSNDUNLOCK Unlock; /* 0x8 */
    BINKSNDVOLUME Volume; /* 0xC */
    BINKSNDPAN Pan; /* 0x10 */
    BINKSNDPAUSE Pause; /* 0x14 */
    BINKSNDONOFF SetOnOff; /* 0x18 */
    BINKSNDCLOSE Close; /* 0x1C */
    BINKSNDMIXBINS MixBins; /* 0x20 */
    BINKSNDMIXBINVOLS MixBinVols; /* 0x24 */
    U32 sndbufsize; /* 0x28 */
    U8* sndbuf; /* 0x2C */
    U8* sndend; /* 0x30 */
    U8* sndwritepos; /* 0x34 */
    U8* sndreadpos; /* 0x38 */
    void* ac; /* 0x3C */
    U32 sndamt; /* 0x40 */
    U32 sndconvert8; /* 0x44 */
    S32 sndendframe; /* 0x48 */
    U32 sndprime; /* 0x4C */
    U8 pad50[0x4];
    U32 sndlowwater; /* 0x54 */
    U32 sndalign; /* 0x58 */
    U32 sndstarved; /* 0x5C */
    U8 pad60[0x4];
    S32 latency; /* 0x64 */
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
    U32 BinkType; /* 0x24 */
    U32 Size; /* 0x28 */
    U32 FrameSize; /* 0x2C */
    U32 SndSize; /* 0x30 */
    BINKRECT FrameRects[8]; /* 0x34 */
    S32 NumRects; /* 0xB4 */
    U32 PlaneNum; /* 0xB8 */
    void* YPlane[2]; /* 0xBC */
    void* APlane[2]; /* 0xC4 */
    U32 YWidth; /* 0xCC */
    U32 YHeight; /* 0xD0 */
    U32 UVWidth; /* 0xD4 */
    U32 UVHeight; /* 0xD8 */
    U8* MaskPlane; /* 0xDC */
    U32 MaskPitch; /* 0xE0 */
    U32 MaskLength; /* 0xE4 */
    U32 LargestFrameSize; /* 0xE8 */
    U32 InternalFrames; /* 0xEC */
    S32 NumTracks; /* 0xF0 */
    U32 Highest1SecRate; /* 0xF4 */
    U32 Highest1SecFrame; /* 0xF8 */
    S32 Paused; /* 0xFC */
    U8 pad100[0x4];
    U8* compframe; /* 0x104 */
    U8* preloadptr; /* 0x108 */
    U32* frameoffsets; /* 0x10C */
    BINKIO bio; /* 0x110 */
    void* iobuf; /* 0x24C */
    U32 iosize; /* 0x250 */
    U32 decompwidth; /* 0x254 */
    U32 decompheight; /* 0x258 */
    S32* trackindexes; /* 0x25C */
    U32* tracksizes; /* 0x260 */
    U32* tracktypes; /* 0x264 */
    S32* trackIDs; /* 0x268 */
    U8 pad26C[0x4];
    U32 runtimeframes; /* 0x270 */
    U32 opentime; /* 0x274 */
    U32 framestart; /* 0x278 */
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
    U32 totalvdecomptime; /* 0x2AC */
    U32 totaladecomptime; /* 0x2B0 */
    U32 totalblittime; /* 0x2B4 */
    U32 totalopentime; /* 0x2B8 */
    U32 fileframerate; /* 0x2BC */
    U32 fileframeratediv; /* 0x2C0 */
    U32 rtframes; /* 0x2C4 */
    U32 rtsize; /* 0x2C8 */
    U32* rtframetimes; /* 0x2CC */
    U32* rtadecomptimes; /* 0x2D0 */
    U32* rtvdecomptimes; /* 0x2D4 */
    U32* rtblittimes; /* 0x2D8 */
    U32* rtreadtimes; /* 0x2DC */
    U32* rtidlereadtimes; /* 0x2E0 */
    U32* rtthreadreadtimes; /* 0x2E4 */
    U32 copyflags; /* 0x2E8 */
    U32 lastdecodedframe; /* 0x2EC */
    S32 playingtracks; /* 0x2F0 */
    U32 soundstarves; /* 0x2F4 */
    BINKSND* bsnd; /* 0x2F8 */
    U32 lastblitdone; /* 0x2FC */
    U32 skippedblits; /* 0x300 */
    U32 totalskippedblits; /* 0x304 */
    U32 expanddata[9]; /* 0x308 */
    U32 skipcount; /* 0x32C */
    U32 soundskips; /* 0x330 */
    U32 soundskipframes; /* 0x334 */
    U8 pad338[0x8];
    U32 soundticks; /* 0x340 */
    U8 soundcb[28]; /* 0x344 */
    U8 pad360[0x24];
};


typedef struct BINKTRACK
{
    U32 Frequency;   /* 0x00 */
    U32 Bits;        /* 0x04 */
    U32 Channels;    /* 0x08 */
    U32 MaxSize;     /* 0x0C */
    BINK* bink;      /* 0x10 */
    void* sndcomp;   /* 0x14 */
    S32 trackindex;  /* 0x18 */
} BINKTRACK;

typedef struct BINKSUMMARY
{
    U32 Width;                /* 0x00 */
    U32 Height;               /* 0x04 */
    U32 TotalTime;            /* 0x08 */
    U32 FileFrameRate;        /* 0x0C */
    U32 FileFrameRateDiv;     /* 0x10 */
    U32 FrameRate;            /* 0x14 */
    U32 FrameRateDiv;         /* 0x18 */
    U32 TotalOpenTime;        /* 0x1C */
    U32 TotalFrames;          /* 0x20 */
    U32 TotalPlayedFrames;    /* 0x24 */
    U32 SkippedFrames;        /* 0x28 */
    U32 SkippedBlits;         /* 0x2C */
    U32 SoundSkips;           /* 0x30 */
    U32 TotalBlitTime;        /* 0x34 */
    U32 TotalReadTime;        /* 0x38 */
    U32 TotalVideoDecompTime; /* 0x3C */
    U32 TotalAudioDecompTime; /* 0x40 */
    U32 TotalIdleReadTime;    /* 0x44 */
    U32 TotalBackReadTime;    /* 0x48 */
    U32 TotalReadSpeed;       /* 0x4C */
    U32 SlowestFrameTime;     /* 0x50 */
    U32 Slowest2FrameTime;    /* 0x54 */
    U32 SlowestFrameNum;      /* 0x58 */
    U32 Slowest2FrameNum;     /* 0x5C */
    U32 AverageDataRate;      /* 0x60 */
    U32 AverageFrameSize;     /* 0x64 */
    U32 HighestMemAmount;     /* 0x68 */
    U32 TotalIOMemory;        /* 0x6C */
    U32 HighestIOUsed;        /* 0x70 */
    U32 Highest1SecRate;      /* 0x74 */
    U32 Highest1SecFrame;     /* 0x78 */
} BINKSUMMARY;

typedef struct BINKREALTIME
{
    U32 FrameNum;              /* 0x00 */
    U32 FrameRate;             /* 0x04 */
    U32 FrameRateDiv;          /* 0x08 */
    U32 Frames;                /* 0x0C */
    U32 FramesTime;            /* 0x10 */
    U32 FramesVideoDecompTime; /* 0x14 */
    U32 FramesAudioDecompTime; /* 0x18 */
    U32 FramesReadTime;        /* 0x1C */
    U32 FramesIdleReadTime;    /* 0x20 */
    U32 FramesThreadReadTime;  /* 0x24 */
    U32 FramesBlitTime;        /* 0x28 */
    U32 ReadBufferSize;        /* 0x2C */
    U32 ReadBufferUsed;        /* 0x30 */
    U32 FramesDataRate;        /* 0x34 */
} BINKREALTIME;

#endif
