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
void BinkDoFrame(BINK* bink);
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
void BinkCopyToBufferRect(BINK* bink, void* dest, S32 destpitch, U32 destheight, U32 destx, U32 desty, U32 srcx,
                          U32 srcy, U32 srcw, U32 srch, U32 flags);

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
    if (bink->framemem)
        radfree(bink->framemem);
    memset(bink, 0, sizeof(BINK));
    radfree(bink);
}
