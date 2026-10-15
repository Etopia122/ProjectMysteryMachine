/*
 * Bink video player core (open/close, frame timing, tracks).
 * Compiled with SN ProDG (GCC 2.95).
 */

#include "bink/binkint.h"

char* strcpy(char* dst, const char* src);
U32 RADTimerRead(void);
void RADCB_suspend_callback(void* h, void* cb);
S32 RADCB_try_to_suspend_callback(void* h, void* cb);
void RADCB_resume_callback(void* h, void* cb);
void RADCB_idle_on_callbacks(void);
U32 popmalloctotal(void);
void* popmalloc(U32 bytes);
void BinkAudioDecompressClose(void* ac);
void checksound(BINK* bink);
void BinkCopyToBufferRect(BINK* bink, void* dest, S32 destpitch, U32 destheight, U32 destx, U32 desty, U32 srcx,
                          U32 srcy, U32 srcw, U32 srch, U32 flags);

extern char bink_error[256];
extern BINKSNDSYSOPEN bink_sndsysopen;
extern U32 bink_sndsys;
extern U32 bink_sndopencount;
extern void* bink_cbhandler;
extern U32 bink_forceframerate;
extern U32 bink_forceframeratediv;
extern U32 bink_iosize;
extern U32 bink_simulate;
extern U32 bink_numtracks;
extern U32 bink_io;
extern U32 bink_tracks[8];
extern U8 bink_logo[];

void* bpopmalloc(BINK* bink, U32 bytes)
{
    bink->totalmem += popmalloctotal() + bytes;
    return popmalloc(bytes);
}

void BinkSetError(const char* err)
{
    strcpy(bink_error, err);
}

char* BinkGetError(void)
{
    return bink_error;
}

S32 BinkSetSoundSystem(BINKSNDSYSOPEN open, U32 param)
{
    U32 snd;

    if (open == 0)
        return 0;
    if (bink_sndsysopen == 0)
        bink_sndsysopen = open;
    if (bink_sndsysopen != open)
    {
        if (bink_sndopencount)
            return 0;
        bink_sndsysopen = open;
    }
    snd = bink_sndsysopen(param);
    if (snd)
        bink_sndsys = snd;
    return (bink_sndsys) ? 1 : 0;
}

void conv16to8(U8* dest, S16* src, U32 bytes)
{
    for (bytes >>= 1; bytes; bytes--)
        *dest++ = (U8)(*src++ >> 8);
}

void inittimer(BINK* bink)
{
    if (bink->timerstart == 0)
    {
        bink->timerstart = RADTimerRead();
        bink->timerframe = bink->runtimeframes - 1;
    }
}

void BinkSetFrameRate(U32 rate, U32 div)
{
    bink_forceframerate = rate;
    bink_forceframeratediv = div;
}

void BinkSetIOSize(U32 size)
{
    bink_iosize = size;
}

void BinkSetIO(U32 io)
{
    bink_io = io;
}

void BinkSetSimulate(U32 sim)
{
    bink_simulate = sim;
}

void BinkSetSoundTrack(U32 total, U32* tracks)
{
    U32 i;

    if (total > 8)
        total = 8;
    bink_numtracks = total;
    for (i = 0; i < total; i++)
        bink_tracks[i] = tracks[i];
}

void bink_suspend_io(U8* bio)
{
    RADCB_suspend_callback(bink_cbhandler, bio + 0xFC);
}

void bink_try_suspend_io(U8* bio)
{
    RADCB_try_to_suspend_callback(bink_cbhandler, bio + 0xFC);
}

void bink_resume_io(U8* bio)
{
    RADCB_resume_callback(bink_cbhandler, bio + 0xFC);
}

void bink_idle_on_io(void)
{
    RADCB_idle_on_callbacks();
}

U32 bink_get_priority_sound(U8* cb, U32 ticks)
{
    if (*(U32*)(cb - 4) != ticks)
        return ~*(U32*)(cb - 4);
    return 0;
}

void bink_sound_callback(U8* cb, U32 ticks)
{
    BINK* bink = (BINK*)(cb - 0x344);
    if (bink->playingtracks)
        checksound(bink);
    bink->soundticks = ticks;
}

void BinkCopyToBuffer(BINK* bink, void* dest, S32 destpitch, U32 destheight, U32 destx, U32 desty, U32 flags)
{
    BinkCopyToBufferRect(bink, dest, destpitch, destheight, destx, desty, 0, 0, bink->Width, bink->Height, flags);
}

void timeframe(BINK* bink, U32 time)
{
    U32 start = bink->startblittime;
    if (start)
    {
        bink->startblittime = 0;
        bink->totalblittime += time - start;
    }
}

void endframe(BINK* bink, U32 time)
{
    if (bink->startframetime == 0)
        return;
    time -= bink->startframetime;
    bink->startframetime = 0;
    if (time > bink->slowestframetime)
    {
        bink->slowest2frametime = bink->slowestframetime;
        bink->slowest2frame = bink->slowestframe;
        bink->slowestframetime = time;
        bink->slowestframe = bink->FrameNum;
    }
    else if (time > bink->slowest2frametime)
    {
        bink->slowest2frametime = time;
        bink->slowest2frame = bink->FrameNum;
    }
}

void BinkService(BINK* bink)
{
    if (bink->playingtracks)
        checksound(bink);
}

S32 idtoindex(BINK* bink, S32 id)
{
    S32 i;
    for (i = 0; i < bink->playingtracks; i++)
    {
        if (bink->trackIDs[bink->trackindexes[i]] == id)
            return i;
    }
    return -1;
}

void BinkSetVolume(BINK* bink, U32 trackid, S32 volume)
{
    if (bink && bink->playingtracks)
    {
        S32 i = idtoindex(bink, trackid);
        if (i != -1)
        {
            if (bink->bsnd[i].Volume)
                bink->bsnd[i].Volume(&bink->bsnd[i], volume);
        }
    }
}

void BinkSetMixBins(BINK* bink, U32 trackid, U32* bins, U32 total)
{
    if (bink && bink->playingtracks)
    {
        S32 i = idtoindex(bink, trackid);
        if (i != -1)
        {
            if (bink->bsnd[i].MixBins)
                bink->bsnd[i].MixBins(&bink->bsnd[i], bins, total);
        }
    }
}

void BinkSetMixBinVolumes(BINK* bink, U32 trackid, U32* bins, S32* volumes, U32 total)
{
    if (bink && bink->playingtracks)
    {
        S32 i = idtoindex(bink, trackid);
        if (i != -1)
        {
            if (bink->bsnd[i].MixBinVols)
                bink->bsnd[i].MixBinVols(&bink->bsnd[i], bins, volumes, total);
        }
    }
}

void BinkSetPan(BINK* bink, U32 trackid, S32 pan)
{
    if (bink && bink->playingtracks)
    {
        S32 i = idtoindex(bink, trackid);
        if (i != -1)
        {
            if (bink->bsnd[i].Pan)
                bink->bsnd[i].Pan(&bink->bsnd[i], pan);
        }
    }
}

void* BinkLogoAddress(void)
{
    return bink_logo;
}

U32 BinkGetTrackType(BINK* bink, U32 track)
{
    if (bink)
        return bink->tracktypes[track];
    return 0;
}

U32 BinkGetTrackMaxSize(BINK* bink, U32 track)
{
    if (bink)
        return bink->tracksizes[track];
    return 0;
}

U32 BinkGetTrackID(BINK* bink, U32 track)
{
    if (bink)
        return bink->trackIDs[track];
    return 0;
}

typedef struct BINKTRACK
{
    U8 pad[0x14];
    void* ac; /* 0x14 */
} BINKTRACK;

void BinkCloseTrack(BINKTRACK* t)
{
    if (t)
    {
        if (t->ac)
        {
            BinkAudioDecompressClose(t->ac);
            t->ac = 0;
        }
        radfree(t);
    }
}

S32 BinkSetVideoOnOff(BINK* bink, S32 onoff)
{
    if (bink)
        bink->videoon = onoff;
    return onoff;
}
