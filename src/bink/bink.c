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
U32 mult64anddiv(U32 a, U32 b, U32 c);
S32 RADCB_unregister_2_callbacks(void* h1, void* cb1, void* h2, void* cb2, U32 flags);
U32 dosilence(BINKSND* snd);
S32 BinkSetSoundOnOff(BINK* bink, S32 onoff);
void timeframe(BINK* bink, U32 time);
U32 mult64andshift(U32 a, U32 b, U32 shift);
void RADCB_suspend_2_callbacks(void* h1, void* cb1, void* h2, void* cb2);
void GotoFrame(BINK* bink, U32 frame);
S32 BinkDoFrame(BINK* bink);
void* memmove(void* dest, const void* src, U32 n);
void ExpandBink(void* y, void* prevy, void* a, void* preva, U8* mask, U32 w, U32 h, U32 yw, U32 yh, void* data,
                U32 key, void* state, U32 flags, U32 type);
S32 BinkSetVideoOnOff(BINK* bink, S32 onoff);
void BinkNextFrame(BINK* bink);
void* BinkAudioDecompressOpen(U32 rate, U32 chans, U32 flags);
void BinkAudioDecompress(void* ac, S16** outp, U32* outlen, void* inp, void** inpend);
void* bpopmalloc(BINK* bink, U32 bytes);
void endframe(BINK* bink, U32 time);
void* popmalloc(U32 bytes);
void BinkAudioDecompressClose(void* ac);
void checksound(BINK* bink);
U8* conv16to8(U8* dest, S16* src, U32 bytes);
S32 BinkCopyToBufferRect(BINK* bink, void* dest, S32 destpitch, U32 destheight, U32 destx, U32 desty, U32 srcx,
                         U32 srcy, U32 srcw, U32 srch, U32 flags);
void inittimer(BINK* bink);
void YUV_init(U32 type);
U32 strlen(const char* s);
void YUV_blit_32bpp(void* dst, U32 dx, U32 dy, U32 pitch, void* src, U32 sx, U32 sy, U32 w, U32 h, U32 stride, U32 uvy,
                    U32 flags);
void YUV_blit_32abpp(void* dst, U32 dx, U32 dy, U32 pitch, void* src, U32 sx, U32 sy, U32 w, U32 h, U32 stride, U32 uvy,
                     void* alpha, U32 flags);
void YUV_blit_16bpp(void* dst, U32 dx, U32 dy, U32 pitch, void* src, U32 sx, U32 sy, U32 w, U32 h, U32 stride, U32 uvy,
                    U32 flags);
void YUV_blit_16a4bpp(void* dst, U32 dx, U32 dy, U32 pitch, void* src, U32 sx, U32 sy, U32 w, U32 h, U32 stride,
                      U32 uvy, void* alpha, U32 flags);
void YUV_blit_YUY2(void* dst, U32 dx, U32 dy, U32 pitch, void* src, U32 sx, U32 sy, U32 w, U32 h, U32 stride, U32 uvy,
                   U32 flags);
void YUV_blit_32bpp_mask(void* dst, U32 dx, U32 dy, U32 pitch, U8* mask, U32 maskpitch, void* src, U32 sx, U32 sy,
                         U32 w, U32 h, U32 stride, U32 uvy, U32 flags);
void YUV_blit_32abpp_mask(void* dst, U32 dx, U32 dy, U32 pitch, U8* mask, U32 maskpitch, void* src, U32 sx, U32 sy,
                          U32 w, U32 h, U32 stride, U32 uvy, void* alpha, U32 flags);
void YUV_blit_16bpp_mask(void* dst, U32 dx, U32 dy, U32 pitch, U8* mask, U32 maskpitch, void* src, U32 sx, U32 sy,
                         U32 w, U32 h, U32 stride, U32 uvy, U32 flags);
void YUV_blit_16a4bpp_mask(void* dst, U32 dx, U32 dy, U32 pitch, U8* mask, U32 maskpitch, void* src, U32 sx, U32 sy,
                           U32 w, U32 h, U32 stride, U32 uvy, void* alpha, U32 flags);
void YUV_blit_YUY2_mask(void* dst, U32 dx, U32 dy, U32 pitch, U8* mask, U32 maskpitch, void* src, U32 sx, U32 sy,
                        U32 w, U32 h, U32 stride, U32 uvy, U32 flags);

typedef struct BINKHDR
{
    U32 Marker;
    U32 Size;
    U32 Frames;
    U32 LargestFrameSize;
    U32 InternalFrames;
    U32 Width;
    U32 Height;
    U32 FrameRate;
    U32 FrameRateDiv;
    U32 Flags;
    U32 NumTracks;
} BINKHDR;

typedef S32 (*BINKIOOPEN)(BINKIO* bio, const char* name, U32 flags);
typedef S32 (*BINKSNDOPEN)(BINKSND* snd, U32 freq, S32 bits, S32 chans, U32 flags, BINK* bink);
S32 BinkFileOpen(BINKIO* bio, const char* name, U32 flags);
void pushmalloc(void* ptr, U32 bytes);
void ExpandBundleSizes(U32* sizes, U32 width);
U32 BinkOpenNGCSound(U32 param);
void* RADCB_allocate_handler(U32 hz);
void RADCB_resume_handler(void* h);
void RADCB_register_callback(void* h, void* cb, void* check, void* func);
U32 high1secrate(S32 frames, U32* offsets, S32 rate, U32* frame, U32* allkeys);
U32 bink_get_priority_sound(U8* cb, U32 ticks);
void bink_sound_callback(U8* cb, U32 ticks);
void bink_suspend_io(U8* bio);
void bink_try_suspend_io(U8* bio);
void bink_resume_io(U8* bio);
void bink_idle_on_io(void);

extern char bink_error[256];
extern BINKSNDSYSOPEN bink_sndsysopen;
extern U32 bink_sndsys;
extern U32 bink_sndopencount;
extern void* bink_cbhandler;
extern void* bink_sndcbhandler;
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

U8* conv16to8(U8* dest, S16* src, U32 bytes)
{
    for (bytes >>= 1; bytes; bytes--)
        *dest++ = (U8)(*src++ >> 8);
    return dest;
}

U32 dosilence(BINKSND* snd)
{
    U32 amt, ms;
    U8* buf;
    U8* pos;

    if (snd->sndamt < snd->sndprime)
    {
        amt = snd->sndprime - snd->sndamt;
        buf = snd->sndbuf;
        pos = snd->sndreadpos - amt;
        snd->sndreadpos = pos;
        ms = (amt * 1000) / ((snd->freq * snd->bits * snd->chans) >> 3);
        if (pos < buf)
        {
            snd->sndreadpos = pos + snd->sndbufsize;
            memset(buf, 0, amt - (buf - pos));
            memset(snd->sndreadpos, 0, buf - pos);
        }
        else
        {
            memset(pos, 0, amt);
        }
        snd->sndamt += amt;
        return ms;
    }
    return 0;
}

void checksound(BINK* bink)
{
    U32 i, left, len;
    U8* addr;
    U8* src;

    if (bink->bsnd[bink->playingtracks - 1].Ready == 0)
        return;
    if (!RADCB_try_to_suspend_callback(bink_sndcbhandler, bink->soundcb))
        return;
    for (i = 0; i < (U32)bink->playingtracks; i++)
    {
        while (bink->bsnd[i].Ready(&bink->bsnd[i]) &&
               (bink->bsnd[i].sndamt > bink->bsnd[i].sndlowwater ||
                ((S32)bink->FrameNum > bink->bsnd[i].sndendframe && bink->bsnd[i].sndamt)))
        {
            if (!bink->bsnd[i].Lock(&bink->bsnd[i], &addr, &len))
                break;
            if (bink->bsnd[i].sndconvert8)
                len *= 2;
            if (len > bink->bsnd[i].sndamt)
                len = bink->bsnd[i].sndamt;
            if ((S32)bink->FrameNum <= bink->bsnd[i].sndendframe)
                len &= bink->bsnd[i].sndalign << bink->bsnd[i].sndconvert8;
            bink->bsnd[i].sndamt -= len;
            src = bink->bsnd[i].sndreadpos;
            left = bink->bsnd[i].sndend - src;
            if (left < len)
            {
                if (left)
                {
                    if (bink->bsnd[i].sndconvert8)
                    {
                        addr = conv16to8(addr, (S16*)src, left);
                    }
                    else
                    {
                        memcpy(addr, src, left);
                        addr += left;
                    }
                }
                if (bink->bsnd[i].sndconvert8)
                    conv16to8(addr, (S16*)bink->bsnd[i].sndbuf, len - left);
                else
                    memcpy(addr, bink->bsnd[i].sndbuf, len - left);
                bink->bsnd[i].sndreadpos = bink->bsnd[i].sndbuf + (len - left);
            }
            else
            {
                if (bink->bsnd[i].sndconvert8)
                    conv16to8(addr, (S16*)src, len);
                else
                    memcpy(addr, src, len);
                bink->bsnd[i].sndreadpos += len;
            }
            if (bink->bsnd[i].sndconvert8)
                len >>= 1;
            bink->bsnd[i].Unlock(&bink->bsnd[i], len);
        }
    }
    RADCB_resume_callback(bink_sndcbhandler, bink->soundcb);
}

void inittimer(BINK* bink)
{
    if (bink->timerstart == 0)
    {
        bink->timerstart = RADTimerRead();
        bink->timerframe = bink->runtimeframes - 1;
    }
}

void GotoFrame(BINK* bink, U32 frame)
{
    if (frame)
        frame--;
    bink->skippedblits = 0;
    if (bink->playingtracks)
        checksound(bink);
    bink->LastFrameNum = bink->FrameNum;
    if (frame == 0)
    {
        if (bink->soundon)
        {
            BinkSetSoundOnOff(bink, 0);
            BinkSetSoundOnOff(bink, 1);
        }
        bink->timerstart = 0;
        bink->soundskips = 0;
    }
    if (bink->preloadptr)
    {
        bink->compframe = bink->preloadptr + ((bink->frameoffsets[frame] & ~1) - (bink->frameoffsets[0] & ~1));
    }
    else
    {
        U32 ofs = bink->frameoffsets[frame] & ~1;
        bink->bio.ReadFrame(&bink->bio, frame, ofs, bink->compframe, (bink->frameoffsets[frame + 1] & ~1) - ofs);
    }
    if (bink->playingtracks)
        checksound(bink);
    bink->FrameNum = frame + 1;
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

U32 high1secrate(S32 frames, U32* offsets, S32 rate, U32* frame, U32* allkeys)
{
    U32 max = 0;
    S32 maxframe = 0;
    U32 keys = 1;
    S32 i;

    for (i = 0; i < frames - rate; i++)
    {
        U32 size = offsets[i + rate] - offsets[i];
        if (size > max)
        {
            max = size;
            maxframe = i;
        }
    }
    if (max == 0)
        max = mult64anddiv(offsets[frames] - offsets[0], rate, frames);
    for (i = 0; i < frames; i++)
    {
        if (!(offsets[i] & 1))
        {
            keys = 0;
            break;
        }
    }
    *allkeys = keys;
    *frame = maxframe;
    return max;
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

BINK* BinkOpen(const char* name, U32 flags)
{
    BINK b;
    BINKHDR h;
    BINK* bink;
    BINKIOOPEN open = BinkFileOpen;
    U32 scale, sim, size, i, freq, type;
    S32 j;
    U32 allkeys;

    if (!(flags & 0x4000))
    {
        bink_numtracks = 1;
        bink_tracks[0] = 0;
    }
    memset(&b, 0, sizeof(b));
    b.totalopentime = RADTimerRead();
    bink_error[0] = 0;
    if (flags & 0x4000000)
    {
        h = *(BINKHDR*)name;
    }
    else
    {
        if ((flags & 0x2000000) && bink_io)
            open = (BINKIOOPEN)bink_io;
        bink_io = 0;
        if (!open(&b.bio, name, flags))
        {
            if (bink_error[0] == 0)
                BinkSetError("Error opening file.");
            return 0;
        }
        b.bio.ReadHeader(&b.bio, 0, &h, sizeof(h));
    }

    if (h.Marker != 'fKIB' && h.Marker != 'gKIB' && h.Marker != 'hKIB' && h.Marker != 'iKIB')
    {
        BinkSetError("Not a Bink file.");
    fail:
        if (!(flags & 0x4000000))
            b.bio.Close(&b.bio);
        return 0;
    }
    if (h.Frames == 0)
    {
        BinkSetError("The file doesn't contain any compressed frames yet.");
        goto fail;
    }

    b.Width = h.Width;
    b.Height = h.Height;
    b.decompwidth = h.Width;
    b.decompheight = h.Height;
    b.BinkType = h.Flags;
    b.UVWidth = (((h.Width + 1) >> 1) + 7) & ~7;
    b.UVHeight = (((h.Height + 1) >> 1) + 7) & ~7;
    b.YWidth = b.UVWidth * 2;
    b.YHeight = b.UVHeight * 2;
    b.MaskPitch = b.YWidth >> 4;
    b.OpenFlags = (flags & 0x8FFFFFFF) | (h.Flags & 0x20000);
    if (!(h.Flags & 0x100000))
        b.OpenFlags &= ~0x100000;
    scale = flags & 0x70000000;
    if (scale != 0x70000000)
    {
        if (scale)
            b.OpenFlags |= scale;
        else
            b.OpenFlags |= h.Flags & 0x70000000;
        switch (b.OpenFlags & 0x70000000)
        {
        case 0x10000000:
        case 0x20000000:
            b.Height *= 2;
            break;
        case 0x30000000:
            b.Width *= 2;
            break;
        case 0x40000000:
        case 0x50000000:
            b.Width *= 2;
            b.Height *= 2;
            break;
        }
    }
    if (h.Marker == 'fKIB' || h.Marker == 'gKIB')
        b.OpenFlags |= 0x18000;
    else if (h.Marker == 'hKIB')
        b.OpenFlags |= 0x8000;
    b.Frames = h.Frames;
    b.InternalFrames = h.InternalFrames;
    if ((flags & 0x1000) && bink_forceframerate != (U32)-1)
    {
        b.FrameRate = bink_forceframerate;
        b.FrameRateDiv = bink_forceframeratediv;
        bink_forceframerate = -1;
    }
    else
    {
        b.FrameRate = h.FrameRate;
        b.FrameRateDiv = h.FrameRateDiv;
    }
    b.Size = h.Size;
    b.NumTracks = h.NumTracks;
    b.LargestFrameSize = h.LargestFrameSize;
    b.fileframerate = h.FrameRate;
    b.fileframeratediv = h.FrameRateDiv;
    b.rtframes = (h.FrameRate + (h.FrameRateDiv >> 1)) / h.FrameRateDiv;
    b.MaskLength = ((b.YWidth >> 4) * b.YHeight) >> 4;
    if (b.rtframes == 0)
        b.rtframes = 1;
    b.rtsize = (b.rtframes - 1) * 4;

    pushmalloc(&b.MaskPlane, b.MaskLength + 16);
    pushmalloc(&b.rtframetimes, b.rtframes * 4);
    pushmalloc(&b.rtadecomptimes, b.rtframes * 4);
    pushmalloc(&b.rtvdecomptimes, b.rtframes * 4);
    pushmalloc(&b.rtblittimes, b.rtframes * 4);
    pushmalloc(&b.rtreadtimes, b.rtframes * 4);
    pushmalloc(&b.rtidlereadtimes, b.rtframes * 4);
    pushmalloc(&b.rtthreadreadtimes, b.rtframes * 4);
    pushmalloc(&b.bsnd, bink_numtracks ? bink_numtracks * sizeof(BINKSND) : sizeof(BINKSND));
    pushmalloc(&b.trackindexes, bink_numtracks * 4);
    ExpandBundleSizes(b.expanddata, b.YWidth);
    pushmalloc(&b.expanddata[0], b.expanddata[0]);
    pushmalloc(&b.expanddata[1], b.expanddata[1]);
    pushmalloc(&b.expanddata[2], b.expanddata[2]);
    pushmalloc(&b.expanddata[3], b.expanddata[3]);
    pushmalloc(&b.expanddata[4], b.expanddata[4]);
    pushmalloc(&b.expanddata[5], b.expanddata[5]);
    pushmalloc(&b.expanddata[6], b.expanddata[6]);
    pushmalloc(&b.expanddata[7], b.expanddata[7]);
    pushmalloc(&b.expanddata[8], b.expanddata[8]);
    if (!(flags & 0x4000000))
    {
        pushmalloc(&b.frameoffsets, (b.InternalFrames + 1) * 4);
        pushmalloc(&b.tracksizes, b.NumTracks * 4);
        pushmalloc(&b.tracktypes, b.NumTracks * 4);
        pushmalloc(&b.trackIDs, b.NumTracks * 4);
    }

    bink = (BINK*)bpopmalloc(&b, sizeof(BINK));
    if (bink == 0)
    {
    oom:
        BinkSetError("Out of memory.");
        goto fail;
    }
    *bink = b;
    bink->bio.bink = bink;
    bink->rtadecomptimes[0] = 0;
    bink->rtvdecomptimes[0] = 0;
    bink->rtblittimes[0] = 0;
    bink->rtreadtimes[0] = 0;
    bink->rtidlereadtimes[0] = 0;
    bink->rtthreadreadtimes[0] = 0;
    if (flags & 0x4000000)
    {
        bink->tracksizes = (U32*)name + 11;
        bink->tracktypes = (U32*)name + 11 + bink->NumTracks;
        bink->trackIDs = (S32*)name + 11 + bink->NumTracks * 2;
        bink->frameoffsets = (U32*)name + 11 + bink->NumTracks * 3;
    }
    else
    {
        bink->bio.ReadHeader(&bink->bio, -1, bink->tracksizes, bink->NumTracks * 4);
        bink->bio.ReadHeader(&bink->bio, -1, bink->tracktypes, bink->NumTracks * 4);
        bink->bio.ReadHeader(&bink->bio, -1, bink->trackIDs, bink->NumTracks * 4);
        bink->bio.ReadHeader(&bink->bio, -1, bink->frameoffsets, (bink->InternalFrames + 1) * 4);
    }
    bink->Highest1SecRate =
        high1secrate(bink->Frames, bink->frameoffsets, bink->rtframes, &bink->Highest1SecFrame, &allkeys);
    if (bink->OpenFlags & 0x100000)
    {
        pushmalloc(&bink->APlane[0], bink->YWidth * bink->YHeight);
        if (allkeys == 0)
            pushmalloc(&bink->APlane[1], bink->YWidth * bink->YHeight);
    }
    if (allkeys == 0)
        pushmalloc(&bink->YPlane[1], bink->YWidth * bink->YHeight + bink->UVWidth * bink->UVHeight * 2);
    bink->YPlane[0] = bpopmalloc(bink, bink->YWidth * bink->YHeight + bink->UVWidth * bink->UVHeight * 2);
    if (bink->YPlane[0] == 0)
    {
    freebink:
        radfree(bink);
        goto oom;
    }
    if (allkeys)
    {
        bink->YPlane[1] = bink->YPlane[0];
        bink->APlane[1] = bink->APlane[0];
    }
    if ((flags & 0x1000000) && bink_iosize != (U32)-1)
    {
        bink->iosize = bink_iosize;
        bink_iosize = -1;
    }
    else
    {
        bink->iosize = bink->Highest1SecRate;
    }
    if ((flags & 0x400000) && bink_simulate != (U32)-1)
    {
        sim = bink_simulate;
        bink_simulate = -1;
    }
    else
    {
        sim = 0;
    }

    if (flags & 0x4000000)
    {
        bink->preloadptr = (U8*)name + (bink->frameoffsets[0] & ~1);
    }
    else
    {
        bink->iosize = bink->bio.GetBufferSize(&bink->bio, bink->iosize);
        if (bink->iosize >= bink->Size * 9 / 10)
        {
            flags |= 0x2000;
            bink->OpenFlags |= 0x2000;
        }
        if (flags & 0x2000)
        {
            {
                U32 start = (bink->frameoffsets[0] & ~1) - 8;
                size = bink->Size - start;
            }
            bink->preloadptr = bpopmalloc(bink, size);
            if (bink->preloadptr == 0)
            {
                radfree(b.YPlane[0]);
                goto freebink;
            }
            bink->bio.SetInfo(&bink->bio, 0, 0, bink->Size + 8, sim);
            bink->bio.ReadFrame(&bink->bio, 0, bink->frameoffsets[0] & ~1, bink->preloadptr, size);
            bink->bio.Close(&bink->bio);
            bink->bio.ForegroundTime = 0;
        }
        else
        {
            pushmalloc(&bink->compframe, bink->LargestFrameSize);
            bink->iobuf = bpopmalloc(bink, bink->iosize);
            if (bink->iobuf == 0)
                bink->iosize = 0;
            bink->bio.SetInfo(&bink->bio, bink->iobuf, bink->iosize, bink->Size + 8, sim);
        }
    }

    bink->MaskPlane[bink->MaskLength] = 0;
    bink->FrameNum = -1;
    if (bink->FrameRate)
        bink->timerslop = mult64anddiv(2000, bink->FrameRateDiv, bink->FrameRate);
    else
        bink->timerslop = 2000;
    bink->videoon = 1;
    GotoFrame(bink, 1);
    bink->totalopentime = RADTimerRead() - bink->totalopentime;
    bink->bsnd[0].sndrateadj = 0x10000;
    bink->playingtracks = 0;
    if (bink->NumTracks && bink_numtracks)
    {
        for (i = 0; i < bink_numtracks; i++)
        {
            for (j = 0; j < bink->NumTracks; j++)
            {
                if (bink->trackIDs[j] == bink_tracks[i])
                {
                    bink->trackindexes[bink->playingtracks++] = j;
                    break;
                }
            }
        }
    }
    bink_tracks[0] = 0;
    bink_numtracks = 1;

    for (i = 0; i < bink->playingtracks;)
    {
        if ((S32)bink->tracktypes[bink->trackindexes[i]] < 0 || (bink->tracktypes[bink->trackindexes[i]] & 0x10000000))
        {
            if (bink_sndsys == 0)
                BinkSetSoundSystem(BinkOpenNGCSound, 0);
            bink->bsnd[i].sndbuf = 0;
            if (bink_sndsys)
            {
                freq = bink->tracktypes[bink->trackindexes[i]] & 0xFFFF;
                if (b.FrameRate && b.FrameRateDiv)
                    freq = (U32)(((double)freq * b.FrameRate * b.fileframeratediv) /
                                 ((double)b.FrameRateDiv * b.fileframerate));
                if (((BINKSNDOPEN)bink_sndsys)(&bink->bsnd[i], freq,
                                              ((bink->tracktypes[bink->trackindexes[i]] >> 27) & 8) + 8,
                                              ((bink->tracktypes[bink->trackindexes[i]] >> 29) & 1) + 1,
                                              bink->OpenFlags, bink))
                {
                    if (bink->bsnd[i].sndalign == 0)
                        bink->bsnd[i].sndalign = -1;
                    bink->bsnd[i].sndbufsize = (bink->tracksizes[bink->trackindexes[i]] + 255) & ~255;
                    bink->bsnd[i].sndbuf = bpopmalloc(bink, bink->bsnd[i].sndbufsize);
                    if (bink->bsnd[i].sndbuf == 0)
                    {
                        bink->bsnd[i].Close(&bink->bsnd[i]);
                    }
                    else
                    {
                        bink_sndopencount++;
                        bink->bsnd[i].sndconvert8 = (((bink->tracktypes[bink->trackindexes[i]] >> 27) & 8) + 8) == 8;
                        bink->bsnd[i].sndend = bink->bsnd[i].sndbuf + bink->bsnd[i].sndbufsize;
                        bink->bsnd[i].sndwritepos = bink->bsnd[i].sndbuf;
                        bink->bsnd[i].sndreadpos = bink->bsnd[i].sndbuf;
                        bink->bsnd[i].sndprime =
                            mult64anddiv((freq << ((bink->tracktypes[bink->trackindexes[i]] >> 29) & 1)) << 1,
                                         750 - bink->bsnd[i].latency, 1000) &
                            ~3;
                        if (bink->bsnd[i].sndprime > bink->bsnd[i].sndbufsize)
                            bink->bsnd[i].sndprime = bink->bsnd[i].sndbufsize;
                        type = bink->tracktypes[bink->trackindexes[i]];
                        bink->bsnd[i].ac = BinkAudioDecompressOpen(type & 0xFFFF, ((type >> 29) & 1) + 1,
                                                                   ((S32)type >= 0 && (type & 0x10000000)) ? 1 : 0);
                        bink->bsnd[i].sndendframe =
                            bink->Frames - (bink->fileframerate * 3) / (bink->fileframeratediv * 4);
                        bink->bsnd[i].sndamt = 0;
                    }
                }
            }
        }
        if (bink->bsnd[i].sndrateadj == 0)
            bink->bsnd[i].sndrateadj = 0x10000;
        if (bink->bsnd[i].sndbuf == 0)
        {
            bink->playingtracks--;
            memcpy(&bink->trackindexes[i], &bink->trackindexes[i + 1], (bink->playingtracks - i) * 4);
        }
        else
        {
            i++;
        }
    }
    if (bink->playingtracks)
    {
        bink->soundon = 1;
        if (bink_sndcbhandler == 0)
        {
            bink_sndcbhandler = RADCB_allocate_handler(20);
            RADCB_resume_handler(bink_sndcbhandler);
        }
        RADCB_register_callback(bink_sndcbhandler, bink->soundcb, bink_get_priority_sound, bink_sound_callback);
    }
    bink->bio.resume_callback = bink_resume_io;
    bink->bio.suspend_callback = bink_suspend_io;
    bink->bio.try_suspend_callback = bink_try_suspend_io;
    bink->bio.idle_on_callback = bink_idle_on_io;
    if (bink->preloadptr == 0 && !(flags & 0x200000))
    {
        while (bink->bio.Idle(&bink->bio))
            ;
    }
    return bink;
}


void BinkCopyToBuffer(BINK* bink, void* dest, S32 destpitch, U32 destheight, U32 destx, U32 desty, U32 flags)
{
    BinkCopyToBufferRect(bink, dest, destpitch, destheight, destx, desty, 0, 0, bink->Width, bink->Height, flags);
}

S32 BinkCopyToBufferRect(BINK* bink, void* dest, S32 destpitch, U32 destheight, U32 destx, U32 desty, U32 srcx,
                         U32 srcy, U32 srcw, U32 srch, U32 flags)
{
    S32 skipped = 0;
    U32 now, t;

    if (bink == 0 || dest == 0)
        return 0;
    srcx = mult64anddiv(srcx, bink->decompwidth, bink->Width);
    srcw = mult64anddiv(srcw, bink->decompwidth, bink->Width);
    srcy = mult64anddiv(srcy, bink->decompheight, bink->Height);
    srch = mult64anddiv(srch, bink->decompheight, bink->Height);
    if (srcx >= bink->decompwidth)
        srcx = bink->decompwidth;
    if (srcy >= bink->decompheight)
        srcy = bink->decompheight;
    if (srcx + srcw > bink->decompwidth)
        srcw = bink->decompwidth - srcx;
    if (srcy + srch > bink->decompheight)
        srch = bink->decompheight - srcy;
    if (srcw == 0 || srch == 0 || bink->skippedblits)
        return bink->skippedblits;

    bink->bio.Working = 1;
    flags |= bink->OpenFlags & 0x70030000;
    YUV_init(flags & 15);
    if (destpitch < 0)
    {
        dest = (U8*)dest + -destpitch * (destheight - desty - 1);
        desty = 0;
    }

    if (bink->playingtracks && bink->FrameRate && bink->Paused == 0)
    {
        inittimer(bink);
        now = RADTimerRead();
        t = mult64andshift(
            mult64anddiv((bink->runtimeframes - bink->timerframe) * 1000, bink->FrameRateDiv, bink->FrameRate),
            bink->bsnd[0].sndrateadj, 16);
        now -= bink->timerstart + bink->soundskips;
        if (now >= t)
        {
            now -= t;
            if (now > (U32)bink->timerslop)
            {
                if (++bink->skipcount >= 4)
                {
                    bink->skipcount = 0;
                }
                else if ((flags & 0x80000) || (bink->OpenFlags & 0x80000))
                {
                    skipped = 1;
                }
                else
                {
                    bink->bio.Working = 0;
                    bink->lastblitdone = 1;
                    bink->totalskippedblits++;
                    return 1;
                }
                goto blit;
            }
            bink->skipcount = 0;
        }
        bink->lastblitdone = 0;
    }

blit:
    if (bink->preloadptr == 0 && bink->bio.DoingARead == 0 &&
        (bink->bio.CurBufUsed + 1) * 100 / (bink->bio.CurBufSize + 1) < 75)
    {
        if (RADCB_try_to_suspend_callback(bink_cbhandler, bink->bio.cb))
        {
            bink->bio.Idle(&bink->bio);
            RADCB_resume_callback(bink_cbhandler, bink->bio.cb);
        }
        else if (!(bink->OpenFlags & 0x8000000))
        {
            RADCB_idle_on_callbacks();
        }
    }

    bink->copyflags = flags;
    if ((S32)flags < 0)
    {
    nomask:
        switch (flags & 15)
        {
        case 5:
            if (bink->APlane[0])
            {
                    YUV_blit_32abpp(dest, destx, desty, destpitch, bink->YPlane[bink->PlaneNum], srcx, srcy, srcw, srch,
                                    bink->YWidth, bink->YHeight, bink->APlane[bink->PlaneNum], flags);
                break;
            }
        case 3:
            YUV_blit_32bpp(dest, destx, desty, destpitch, bink->YPlane[bink->PlaneNum], srcx, srcy, srcw, srch,
                           bink->YWidth, bink->YHeight, flags);
            break;
        case 7:
            if (bink->APlane[0])
            {
                    YUV_blit_16a4bpp(dest, destx, desty, destpitch, bink->YPlane[bink->PlaneNum], srcx, srcy, srcw,
                                     srch, bink->YWidth, bink->YHeight, bink->APlane[bink->PlaneNum], flags);
                break;
            }
        case 9:
        case 10:
        case 11:
        case 12:
            YUV_blit_16bpp(dest, destx, desty, destpitch, bink->YPlane[bink->PlaneNum], srcx, srcy, srcw, srch,
                           bink->YWidth, bink->YHeight, flags);
            break;
        case 13:
            YUV_blit_YUY2(dest, destx, desty, destpitch, bink->YPlane[bink->PlaneNum], srcx, srcy, srcw, srch,
                          bink->YWidth, bink->YHeight, flags);
            break;
        }
    }
    else
    {
        if (strlen((char*)bink->MaskPlane) >= bink->MaskLength)
            goto nomask;
        switch (flags & 15)
        {
        case 5:
            if (bink->APlane[0])
            {
                    YUV_blit_32abpp_mask(dest, destx, desty, destpitch, bink->MaskPlane, bink->MaskPitch,
                                         bink->YPlane[bink->PlaneNum], srcx, srcy, srcw, srch, bink->YWidth,
                                         bink->YHeight, bink->APlane[bink->PlaneNum], flags);
                break;
            }
        case 3:
            YUV_blit_32bpp_mask(dest, destx, desty, destpitch, bink->MaskPlane, bink->MaskPitch,
                                bink->YPlane[bink->PlaneNum], srcx, srcy, srcw, srch, bink->YWidth, bink->YHeight,
                                flags);
            break;
        case 7:
            if (bink->APlane[0])
            {
                    YUV_blit_16a4bpp_mask(dest, destx, desty, destpitch, bink->MaskPlane, bink->MaskPitch,
                                          bink->YPlane[bink->PlaneNum], srcx, srcy, srcw, srch, bink->YWidth,
                                          bink->YHeight, bink->APlane[bink->PlaneNum], flags);
                break;
            }
        case 9:
        case 10:
        case 11:
        case 12:
            YUV_blit_16bpp_mask(dest, destx, desty, destpitch, bink->MaskPlane, bink->MaskPitch,
                                bink->YPlane[bink->PlaneNum], srcx, srcy, srcw, srch, bink->YWidth, bink->YHeight,
                                flags);
            break;
        case 13:
            YUV_blit_YUY2_mask(dest, destx, desty, destpitch, bink->MaskPlane, bink->MaskPitch,
                               bink->YPlane[bink->PlaneNum], srcx, srcy, srcw, srch, bink->YWidth, bink->YHeight,
                               flags);
            break;
        }
    }
    bink->bio.Working = 0;
    return skipped;
}


S32 BinkDoFrame(BINK* bink)
{
    S32 suspended = 0;
    S32 i, j;
    U32* p;
    U32* next;
    U32 size, amt, space, len;
    U8* out;
    U32* in;
    U32 t, now;

    if (bink == 0 || bink->lastdecodedframe == bink->FrameNum)
        return 0;
    bink->skippedblits = 0;
    bink->bio.Working = 1;
    bink->framestart = RADTimerRead();
    memmove(bink->rtframetimes + 1, bink->rtframetimes, bink->rtsize);
    memmove(bink->rtvdecomptimes + 1, bink->rtvdecomptimes, bink->rtsize);
    memmove(bink->rtadecomptimes + 1, bink->rtadecomptimes, bink->rtsize);
    memmove(bink->rtblittimes + 1, bink->rtblittimes, bink->rtsize);
    memmove(bink->rtreadtimes + 1, bink->rtreadtimes, bink->rtsize);
    memmove(bink->rtidlereadtimes + 1, bink->rtidlereadtimes, bink->rtsize);
    memmove(bink->rtthreadreadtimes + 1, bink->rtthreadreadtimes, bink->rtsize);
    bink->rtframetimes[0] = bink->framestart;
    bink->rtvdecomptimes[0] = bink->totalvdecomptime;
    bink->rtadecomptimes[0] = bink->totaladecomptime;
    bink->rtblittimes[0] = bink->totalblittime;
    bink->rtreadtimes[0] = bink->bio.ForegroundTime;
    bink->rtidlereadtimes[0] = bink->bio.IdleTime;
    bink->rtthreadreadtimes[0] = bink->bio.ThreadTime;
    if (bink->opentime == 0)
    {
        bink->rtframetimes[1] = bink->framestart - mult64anddiv(1000, bink->fileframeratediv, bink->fileframerate);
        bink->bio.ThreadTime = 0;
        bink->bio.IdleTime = 0;
        bink->opentime = bink->framestart;
    }
    else if (bink->playingtracks)
    {
        checksound(bink);
    }
    bink->startframetime = bink->framestart;

    p = (U32*)bink->compframe;
    if (bink->bio.ReadError)
        bink->ReadError = 1;
    if (bink->ReadError)
        return 1;

    for (i = 0; i < bink->NumTracks; i++)
    {
        size = *p++;
        for (j = 0; j < bink->playingtracks; j++)
        {
            if (bink->trackindexes[j] == i)
                goto found;
        }
        j = -1;
    found:
        next = (U32*)((U8*)p + size);
        if (j != -1 && size)
        {
            in = p;
            amt = *in++;
            if (!suspended)
            {
                suspended = 1;
                RADCB_suspend_callback(bink_sndcbhandler, bink->soundcb);
            }
            while (amt)
            {
                space = bink->bsnd[j].sndbufsize - bink->bsnd[j].sndamt;
                BinkAudioDecompress(bink->bsnd[j].ac, (S16**)&out, &len, in, (void**)&in);
                if (len > amt)
                    len = amt;
                amt -= len;
                if (len > space)
                {
                    space = len - space;
                    bink->bsnd[j].sndreadpos += space;
                    bink->bsnd[j].sndamt -= space;
                    if (bink->bsnd[j].sndreadpos > bink->bsnd[j].sndend)
                        bink->bsnd[j].sndreadpos -= bink->bsnd[j].sndbufsize;
                }
                bink->bsnd[j].sndamt += len;
                space = bink->bsnd[j].sndend - bink->bsnd[j].sndwritepos;
                if (space < len)
                {
                    if (space)
                    {
                        memcpy(bink->bsnd[j].sndwritepos, out, space);
                        len -= space;
                        out += space;
                    }
                    memcpy(bink->bsnd[j].sndbuf, out, len);
                    bink->bsnd[j].sndwritepos = bink->bsnd[j].sndbuf + len;
                }
                else
                {
                    memcpy(bink->bsnd[j].sndwritepos, out, len);
                    bink->bsnd[j].sndwritepos += len;
                }
            }
        }
        p = next;
    }
    if (suspended)
        RADCB_resume_callback(bink_sndcbhandler, bink->soundcb);

    t = RADTimerRead();
    if (bink->videoon)
    {
        if (bink->lastblitdone == 0)
            memset(bink->MaskPlane, 0, (bink->YWidth >> 4) * (bink->YHeight >> 4));
        if (bink->preloadptr == 0 && bink->bio.DoingARead == 0 &&
            (bink->bio.CurBufUsed + 1) * 100 / (bink->bio.CurBufSize + 1) < 75)
        {
            if (RADCB_try_to_suspend_callback(bink_cbhandler, bink->bio.cb))
            {
                bink->bio.Idle(&bink->bio);
                RADCB_resume_callback(bink_cbhandler, bink->bio.cb);
            }
            else if (!(bink->OpenFlags & 0x8000000))
            {
                RADCB_idle_on_callbacks();
            }
        }
        ExpandBink(bink->YPlane[bink->PlaneNum ^ 1], bink->YPlane[bink->PlaneNum], bink->APlane[bink->PlaneNum ^ 1],
                   bink->APlane[bink->PlaneNum], bink->MaskPlane, bink->decompwidth, bink->decompheight,
                   bink->YWidth, bink->YHeight, p, bink->frameoffsets[bink->FrameNum - 1] & 1, bink->expanddata,
                   bink->OpenFlags, bink->BinkType);
        bink->PlaneNum ^= 1;
    }
    if (bink->playingtracks)
        checksound(bink);
    bink->NumRects = -1;
    bink->runtimeframes++;
    inittimer(bink);
    now = RADTimerRead();
    bink->totaladecomptime += t - bink->framestart;
    bink->totalvdecomptime += now - t;
    bink->lastdecodedframe = bink->FrameNum;
    bink->LastFrameNum = bink->FrameNum;
    bink->startblittime = now;
    bink->bio.Working = 0;
    return 0;
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

void BinkNextFrame(BINK* bink)
{
    S32 starved = 0;
    U32 i;

    if (bink == 0)
        return;
    bink->bio.Working = 1;
    bink->bio.Working = 0;
    for (i = 0; i < (U32)bink->playingtracks; i++)
    {
        if (bink->bsnd[i].sndstarved)
        {
            bink->bsnd[i].sndstarved = 0;
            if (bink->FrameNum > 1 && (S32)bink->FrameNum <= bink->bsnd[i].sndendframe)
                starved = 1;
        }
    }
    if (starved)
    {
        bink->soundstarves++;
        RADCB_suspend_2_callbacks(bink_sndcbhandler, bink->soundcb, bink_cbhandler, bink->bio.cb);
        BinkSetSoundOnOff(bink, 0);
        while (((bink->bio.CurBufUsed + 1) * 100) / (bink->bio.CurBufSize + 1) < 90)
        {
            if (!bink->bio.Idle(&bink->bio))
                break;
        }
        bink->timerstart = 0;
        bink->soundskips = 0;
        BinkSetSoundOnOff(bink, 1);
        RADCB_resume_callback(bink_sndcbhandler, bink->soundcb);
        RADCB_resume_callback(bink_cbhandler, bink->bio.cb);
    }
    bink->bio.Working = 1;
    if (bink->playingtracks)
        checksound(bink);
    timeframe(bink, RADTimerRead());
    GotoFrame(bink, (bink->FrameNum < bink->Frames) ? bink->FrameNum + 1 : 1);
    bink->bio.Working = 0;
}

U32 BinkGetKeyFrame(BINK* bink, U32 frame, S32 flags)
{
    S32 back;
    U32 f;

    if (bink == 0)
        return 0;
    if (!(flags & 0x80))
    {
        if (bink->frameoffsets[frame - 1] & 1)
            return frame;
    }
    switch (flags & 0x7F)
    {
        case 0:
            back = frame - 2;
            if (back > 0 && !(bink->frameoffsets[back] & 1))
            {
                do
                {
                    if (--back <= 0)
                        break;
                } while (!(bink->frameoffsets[back] & 1));
            }
            return back + 1;
        case 1:
            f = frame;
            while (f < bink->Frames)
            {
                if (bink->frameoffsets[f++] & 1)
                    return f;
            }
            break;
        case 2:
            back = frame - 2;
            f = frame;
            for (;;)
            {
                if (back >= 0)
                {
                    if (bink->frameoffsets[back] & 1)
                        return back + 1;
                    if (f < bink->Frames && (bink->frameoffsets[f++] & 1))
                        return f;
                }
                else
                {
                    if (f >= bink->Frames)
                        return 0;
                    if (bink->frameoffsets[f++] & 1)
                        return f;
                }
                back--;
            }
    }
    return 0;
}

void BinkGoto(BINK* bink, U32 frame, S32 flags)
{
    U32 start, key;
    S32 videooff;
    S32 soundwason;

    if (bink == 0)
        return;
    if (frame == 0)
        frame = 1;
    bink->bio.Working = 1;
    if (frame > bink->Frames)
        frame = bink->Frames;
    if ((flags & 2) || bink->playingtracks == 0)
    {
        start = frame;
    }
    else
    {
        start = (bink->FrameRate + (bink->FrameRateDiv - 1)) / bink->FrameRateDiv;
        if (start >= frame)
            start = 1;
        else
            start = frame - start;
    }
    if (bink->FrameNum != frame)
    {
        if ((flags & 3) != 3)
        {
            if (!(flags & 1))
                key = BinkGetKeyFrame(bink, frame, 0);
            else
                key = frame;
            if (key > start)
            {
                videooff = bink->videoon;
                BinkSetVideoOnOff(bink, 0);
            }
            else
            {
                start = key;
                videooff = 0;
            }
            if ((frame >= bink->FrameNum && start <= bink->FrameNum) || (GotoFrame(bink, start), start != frame))
            {
                RADCB_suspend_callback(bink_sndcbhandler, bink->soundcb);
                soundwason = bink->soundon;
                if (soundwason)
                    BinkSetSoundOnOff(bink, 0);
                if (bink->FrameNum != bink->lastdecodedframe)
                {
                    BinkDoFrame(bink);
                    bink->lastblitdone = 1;
                }
                BinkNextFrame(bink);
                while (bink->FrameNum != frame)
                {
                    if (videooff && bink->FrameNum == key)
                    {
                        BinkSetVideoOnOff(bink, 1);
                        videooff = 0;
                    }
                    BinkDoFrame(bink);
                    bink->lastblitdone = 1;
                    BinkNextFrame(bink);
                }
                bink->timerstart = 0;
                if (videooff)
                    BinkSetVideoOnOff(bink, 1);
                RADCB_resume_callback(bink_sndcbhandler, bink->soundcb);
                if (soundwason)
                    BinkSetSoundOnOff(bink, 1);
            }
        }
        else
        {
            GotoFrame(bink, frame);
        }
    }
    bink->bio.Working = 0;
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

S32 BinkWait(BINK* bink)
{
    U32 start, now, adj;
    S32 diff;

    if (bink && (bink->runtimeframes || bink->Paused) && bink->ReadError == 0)
    {
        start = bink->timerstart;
        if (start == 0)
        {
            inittimer(bink);
            start = bink->timerstart;
        }
        if (bink->playingtracks)
            checksound(bink);
        now = RADTimerRead();
        timeframe(bink, now);
        endframe(bink, now);
        if (bink->Paused || (bink->playingtracks && bink->soundon == 0))
            goto idle;
        if (bink->FrameRate)
        {
            adj = mult64andshift(
                mult64anddiv((bink->runtimeframes - bink->timerframe) * 1000, bink->FrameRateDiv, bink->FrameRate),
                bink->bsnd[0].sndrateadj, 16);
            diff = now - (start + bink->soundskips);
            if (diff < (S32)adj)
                goto idle;
            diff -= adj;
            if (diff > bink->timerslop)
            {
                if (bink->playingtracks == 0)
                {
                    bink->timerstart = now;
                    bink->timerframe = bink->runtimeframes - 1;
                }
                else
                {
                    bink->soundskips = diff;
                    bink->soundskipframes = mult64anddiv(diff, bink->FrameRateDiv, bink->FrameRate);
                }
            }
            if (bink->soundskips < bink->soundskipframes)
                bink->soundskips = 0;
            else
                bink->soundskips = bink->soundskips - bink->soundskipframes;
        }
    }
    return 0;

idle:
    if (bink->preloadptr == 0)
    {
        if (RADCB_try_to_suspend_callback(bink_cbhandler, bink->bio.cb))
        {
            bink->bio.Idle(&bink->bio);
            RADCB_resume_callback(bink_cbhandler, bink->bio.cb);
        }
    }
    return 1;
}

void BinkGetSummary(BINK* bink, BINKSUMMARY* sum)
{
    U32 t, size;

    if (bink && sum)
    {
        t = RADTimerRead();
        timeframe(bink, t);
        endframe(bink, t);
        memset(sum, 0, sizeof(BINKSUMMARY));
        sum->FrameRate = bink->FrameRate;
        sum->FrameRateDiv = bink->FrameRateDiv;
        sum->SkippedBlits = bink->totalskippedblits;
        sum->SoundSkips = bink->soundstarves;
        sum->FileFrameRate = bink->fileframerate;
        sum->FileFrameRateDiv = bink->fileframeratediv;
        sum->TotalFrames = bink->Frames;
        sum->TotalPlayedFrames = bink->runtimeframes;
        sum->TotalTime = RADTimerRead() - bink->opentime;
        sum->TotalOpenTime = bink->totalopentime;
        sum->TotalAudioDecompTime = bink->totaladecomptime;
        sum->TotalVideoDecompTime = bink->totalvdecomptime;
        sum->TotalBlitTime = bink->totalblittime;
        sum->HighestMemAmount += bink->totalmem;
        sum->TotalIOMemory = bink->iosize;
        sum->TotalReadSpeed = mult64anddiv(bink->bio.BytesRead, 1000, bink->bio.TotalTime + 1);
        sum->TotalReadTime = bink->bio.ForegroundTime;
        sum->TotalIdleReadTime = bink->bio.IdleTime;
        sum->TotalBackReadTime = bink->bio.ThreadTime;
        size = bink->Size - (bink->frameoffsets[0] & ~1);
        sum->AverageDataRate = mult64anddiv(size, bink->fileframerate, bink->fileframeratediv * bink->Frames);
        sum->AverageFrameSize = size / bink->Frames;
        sum->Highest1SecRate = bink->Highest1SecRate;
        sum->Highest1SecFrame = bink->Highest1SecFrame + 1;
        sum->Width = bink->Width;
        sum->Height = bink->Height;
        sum->SlowestFrameTime = bink->slowestframetime;
        sum->Slowest2FrameTime = bink->slowest2frametime;
        sum->SlowestFrameNum = bink->slowestframe;
        sum->Slowest2FrameNum = bink->slowest2frame;
        sum->TotalIOMemory = bink->bio.BufSize;
        sum->HighestIOUsed = bink->bio.BufHighUsed;
    }
}

void BinkGetRealtime(BINK* bink, BINKREALTIME* rt, U32 frames)
{
    U32 t;

    t = RADTimerRead();
    timeframe(bink, t);
    endframe(bink, t);
    if (frames == 0 || frames >= bink->rtframes)
        frames = bink->rtframes - 1;
    if (frames > bink->FrameNum)
    {
        frames = bink->FrameNum - 1;
        if (frames == 0)
            frames = 1;
    }
    rt->FrameNum = bink->LastFrameNum;
    rt->FrameRate = bink->FrameRate;
    rt->FrameRateDiv = bink->FrameRate;
    rt->ReadBufferSize = bink->bio.CurBufSize;
    rt->ReadBufferUsed = bink->bio.CurBufUsed;
    rt->FramesDataRate = mult64anddiv(bink->frameoffsets[bink->FrameNum] - bink->frameoffsets[bink->FrameNum - frames],
                                      bink->fileframerate, frames * bink->fileframeratediv);
    rt->Frames = frames;
    rt->FramesTime = bink->rtframetimes[0] - bink->rtframetimes[frames];
    if (rt->FramesTime == 0)
        rt->FramesTime = 1;
    rt->FramesVideoDecompTime = bink->rtvdecomptimes[0] - bink->rtvdecomptimes[frames];
    rt->FramesAudioDecompTime = bink->rtadecomptimes[0] - bink->rtadecomptimes[frames];
    rt->FramesBlitTime = bink->rtblittimes[0] - bink->rtblittimes[frames];
    rt->FramesReadTime = bink->rtreadtimes[0] - bink->rtreadtimes[frames];
    rt->FramesIdleReadTime = bink->rtidlereadtimes[0] - bink->rtidlereadtimes[frames];
    rt->FramesThreadReadTime = bink->rtthreadreadtimes[0] - bink->rtthreadreadtimes[frames];
}

S32 smallestrect(BINKRECT* out, U8* mask, S32 pitch, BINKRECT* r)
{
    S32 x, y, w, h, x2, last;
    U8* p;

    mask += r->Left / 16;
    mask += (r->Top / 16) * pitch;
    h = r->Height / 16;
    w = r->Width / 16;

    p = mask;
    for (y = 0; y < h; y++)
    {
        for (x = 0; x < w; x++)
        {
            if (p[x])
                goto foundtop;
        }
        p += pitch;
    }
    return 0;

foundtop:
    h -= y;
    out->Top = y * 16 + r->Top;
    mask += pitch * y;
    out->Height = r->Height - y * 16;
    if (out->Height >= 2)
    {
        p = mask + (h - 1) * pitch;
        for (y = 0; y < h; y++)
        {
            for (x = 0; x < w; x++)
            {
                if (p[x])
                    goto foundbottom;
            }
            p -= pitch;
        }
    foundbottom:
        h -= y;
        out->Height -= y * 16;
    }

    for (x = 0; x < w; x++)
    {
        p = mask + x;
        for (y = 0; y < h; y++)
        {
            if (*p)
                goto foundleft;
            p += pitch;
        }
    }
foundleft:
    out->Left = x * 16 + r->Left;
    out->Width = r->Width - x * 16;
    if (out->Width >= 2)
    {
        last = w - x - 1;
        for (x2 = last; x2 >= 0; x2--)
        {
            p = mask + x + x2;
            for (y = 0; y < h; y++)
            {
                if (*p)
                    goto foundright;
                p += pitch;
            }
        }
    foundright:
        out->Width -= (last - x2) * 16;
    }
    return 1;
}

S32 trysplit(BINKRECT* a, BINKRECT* b, BINKRECT* r, U8* mask, S32 pitch)
{
    S32 best, s;
    BINKRECT a2, b2, t;

    if (r->Width >= 32)
    {
        t = *r;
        best = (t.Width / 2 + 15) & ~15;
        t.Width = best;
        smallestrect(a, mask, pitch, &t);
        t = *r;
        t.Width -= best;
        t.Left += best;
        smallestrect(b, mask, pitch, &t);
        best = r->Width * r->Height - a->Width * a->Height - b->Width * b->Height;
    }
    else
    {
        best = 0;
    }
    if (r->Height >= 32)
    {
        t = *r;
        s = (t.Height / 2 + 15) & ~15;
        t.Height = s;
        smallestrect(&a2, mask, pitch, &t);
        t = *r;
        t.Height -= s;
        t.Top += s;
        smallestrect(&b2, mask, pitch, &t);
        s = r->Width * r->Height - a2.Width * a2.Height - b2.Width * b2.Height;
        if (s > best)
        {
            *a = a2;
            *b = b2;
            return s;
        }
    }
    return best;
}

S32 BinkGetRects(BINK* bink, U32 flags)
{
    BINKRECT rects[8];
    BINKRECT a[8];
    BINKRECT b[8];
    S32 gains[8];
    S32 i, j, best, bi, m;
    U32 v, minv;

    if ((flags & 0x4000000) || (!(flags & 0x80000) && !(bink->OpenFlags & 0x80000) && bink->lastblitdone))
        return 0;
    if (bink->NumRects == -1)
    {
        rects[0].Top = 0;
        rects[0].Left = 0;
        rects[0].Width = bink->YWidth;
        rects[0].Height = bink->YHeight;
        bink->NumRects = smallestrect(&bink->FrameRects[0], bink->MaskPlane, bink->MaskPitch, &rects[0]);
        if (!(flags & 0x8000000) && bink->NumRects)
        {
            rects[0] = bink->FrameRects[0];
            gains[0] = trysplit(&a[0], &b[0], &rects[0], bink->MaskPlane, bink->MaskPitch);
            for (;;)
            {
                bi = -1;
                best = 0;
                for (i = 0; i < bink->NumRects; i++)
                {
                    if (gains[i] > best)
                    {
                        best = gains[i];
                        bi = i;
                    }
                }
                if (bi == -1)
                    break;
                rects[bi] = a[bi];
                rects[bink->NumRects] = b[bi];
                if (++bink->NumRects == 8)
                    break;
                gains[bi] = trysplit(&a[bi], &b[bi], &rects[bi], bink->MaskPlane, bink->MaskPitch);
                bi = bink->NumRects - 1;
                gains[bi] = trysplit(&a[bi], &b[bi], &rects[bi], bink->MaskPlane, bink->MaskPitch);
            }
        }
        for (i = 0; i < bink->NumRects; i++)
        {
            m = 0;
            minv = 0x7FFF0000;
            for (j = 0; j < bink->NumRects; j++)
            {
                v = rects[j].Top * 65536 + rects[j].Left;
                if (v < minv)
                {
                    minv = v;
                    m = j;
                }
            }
            switch (bink->copyflags & 0x70000000)
            {
            case 0x10000000:
            case 0x20000000:
                rects[m].Top *= 2;
                rects[m].Height *= 2;
                break;
            case 0x30000000:
                rects[m].Left *= 2;
                rects[m].Width *= 2;
                break;
            case 0x40000000:
            case 0x50000000:
                rects[m].Left *= 2;
                rects[m].Width *= 2;
                rects[m].Top *= 2;
                rects[m].Height *= 2;
                break;
            }
            if ((U32)(rects[m].Left + rects[m].Width) > bink->Width)
                rects[m].Width -= (rects[m].Left + rects[m].Width) - bink->Width;
            if ((U32)(rects[m].Top + rects[m].Height) > bink->Height)
                rects[m].Height -= (rects[m].Top + rects[m].Height) - bink->Height;
            bink->FrameRects[i] = rects[m];
            rects[m].Left = 0;
            rects[m].Top = 0x7FFFFFFF;
        }
    }
    return bink->NumRects;
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

BINKTRACK* BinkOpenTrack(BINK* bink, U32 track)
{
    U32 type;
    void* ac;
    BINKTRACK* t;

    if (bink && track < (U32)bink->NumTracks)
    {
        if ((S32)bink->tracktypes[track] < 0 || (bink->tracktypes[track] & 0x10000000))
        {
            type = bink->tracktypes[track];
            ac = BinkAudioDecompressOpen(type & 0xFFFF, ((type >> 29) & 1) + 1,
                                         ((S32)type >= 0 && (type & 0x10000000)) ? 1 : 0);
            if (ac == 0)
                return 0;
            t = (BINKTRACK*)bpopmalloc(bink, sizeof(BINKTRACK));
            if (t == 0)
            {
                BinkAudioDecompressClose(ac);
            }
            else
            {
                memset(t, 0, sizeof(BINKTRACK));
                t->sndcomp = ac;
                t->bink = bink;
                t->Frequency = bink->tracktypes[track] & 0xFFFF;
                t->Bits = ((bink->tracktypes[track] >> 27) & 8) + 8;
                t->Channels = ((bink->tracktypes[track] >> 29) & 1) + 1;
                t->MaxSize = (bink->tracksizes[track] + 3) & ~3;
                if (t->Bits == 8)
                    t->MaxSize >>= 1;
                t->trackindex = track;
                return t;
            }
        }
    }
    return 0;
}

void BinkCloseTrack(BINKTRACK* t)
{
    if (t)
    {
        if (t->sndcomp)
        {
            BinkAudioDecompressClose(t->sndcomp);
            t->sndcomp = 0;
        }
        radfree(t);
    }
}

U32 BinkGetTrackData(BINKTRACK* t, U8* dest)
{
    S32 i;
    U32* p;
    U32 size, left, amt, j, total;
    S16* samples;

    if (t)
    {
        p = (U32*)t->bink->compframe;
        for (i = 0; i < t->bink->NumTracks; i++)
        {
            size = *p++;
            if (i == t->trackindex && size)
            {
                left = *p++;
                total = 0;
                while (left)
                {
                    BinkAudioDecompress(t->sndcomp, &samples, &amt, p, (void**)&p);
                    if (amt > left)
                        amt = left;
                    left -= amt;
                    if (t->Bits == 16)
                    {
                        memcpy(dest, samples, amt);
                    }
                    else
                    {
                        U8* d;
                        S16* sp;
                        amt >>= 1;
                        d = dest;
                        sp = samples;
                        for (j = 0; j < amt; j++)
                            *d++ = (U8)(*sp++ >> 8) + 0x80;
                    }
                    dest += amt;
                    total += amt;
                }
                return total;
            }
            p = (U32*)((U8*)p + size);
        }
    }
    return 0;
}

S32 BinkSetVideoOnOff(BINK* bink, S32 onoff)
{
    if (bink)
        bink->videoon = onoff;
    return onoff;
}

S32 BinkPause(BINK* bink, S32 pause)
{
    U32 i;
    U32 t;

    if (bink == 0)
        return 0;
    t = RADTimerRead();
    timeframe(bink, t);
    endframe(bink, t);
    if (pause == 0 && bink->Paused)
    {
        bink->timerstart = 0;
        bink->soundskips = 0;
    }
    bink->Paused = pause;
    for (i = 0; i < (U32)bink->playingtracks; i++)
        bink->bsnd[i].Pause(&bink->bsnd[i], pause);
    if (bink->runtimeframes && bink->playingtracks)
        checksound(bink);
    return bink->Paused;
}

S32 BinkSetSoundOnOff(BINK* bink, S32 onoff)
{
    S32 ret = 0;
    U32 i;
    S32 suspended;

    if (bink)
    {
        suspended = 0;
        for (i = 0; i < (U32)bink->playingtracks; i++)
        {
            if (bink->bsnd[i].SetOnOff)
            {
                if (!suspended)
                {
                    suspended = 1;
                    RADCB_suspend_callback(bink_sndcbhandler, bink->soundcb);
                }
                if (!bink->bsnd[i].SetOnOff(&bink->bsnd[i], onoff != 0))
                {
                    if (bink->soundon)
                    {
                        bink->bsnd[i].sndwritepos = bink->bsnd[i].sndbuf;
                        bink->bsnd[i].sndreadpos = bink->bsnd[i].sndbuf;
                        bink->bsnd[i].sndamt = 0;
                        dosilence(bink->bsnd + i);
                        bink->soundon = 0;
                    }
                }
                else
                {
                    if (bink->soundon == 0)
                    {
                        bink->soundskips = 0;
                        bink->soundon = 1;
                        bink->timerstart = 0;
                    }
                    ret = 1;
                }
            }
        }
        if (suspended)
            RADCB_resume_callback(bink_sndcbhandler, bink->soundcb);
    }
    return ret;
}

void BinkClose(BINK* bink)
{
    void* ioh;
    void* sndh;
    U32 i;

    if (bink == 0)
        return;
    BinkPause(bink, 1);
    ioh = 0;
    if (!(bink->OpenFlags & 0x8000000) && bink->preloadptr == 0)
        ioh = bink_cbhandler;
    sndh = 0;
    if (bink->playingtracks)
        sndh = bink_sndcbhandler;
    switch (RADCB_unregister_2_callbacks(ioh, bink->bio.cb, sndh, bink->soundcb, 1))
    {
        case 1:
            bink_cbhandler = 0;
            break;
        case 2:
            bink_sndcbhandler = 0;
            break;
        case 3:
            bink_sndcbhandler = 0;
            bink_cbhandler = 0;
            break;
    }
    for (i = 0; i < (U32)bink->playingtracks; i++)
    {
        bink->bsnd[i].Close(&bink->bsnd[i]);
        BinkAudioDecompressClose(bink->bsnd[i].ac);
        if (bink->bsnd[i].sndbuf)
            radfree(bink->bsnd[i].sndbuf);
    }
    if (bink->preloadptr)
    {
        if (!(bink->OpenFlags & 0x4000000))
            radfree(bink->preloadptr);
    }
    else
    {
        bink->bio.Close(&bink->bio);
        radfree(bink->iobuf);
    }
    if (bink->YPlane[0])
        radfree(bink->YPlane[0]);
    memset(bink, 0, sizeof(BINK));
    radfree(bink);
}
