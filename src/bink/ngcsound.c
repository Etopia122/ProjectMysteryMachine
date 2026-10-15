/*
 * Bink GameCube sound output: streams decoded audio through ARAM into one
 * AX voice per channel. Compiled with SN ProDG (GCC 2.95).
 */

#include "bink/binkint.h"

typedef struct ARQRequest
{
    struct ARQRequest* next;
    U32 owner;
    U32 type;
    U32 priority;
    U32 source;
    U32 dest;
    U32 length;
    void (*callback)(U32 request);
} ARQRequest;

typedef struct AXPBMIX
{
    U16 vL, vDeltaL, vR, vDeltaR;
    U16 vAuxAL, vDeltaAuxAL, vAuxAR, vDeltaAuxAR;
    U16 vAuxBL, vDeltaAuxBL, vAuxBR, vDeltaAuxBR;
    U16 vAuxBS, vDeltaAuxBS, vS, vDeltaS, vAuxAS, vDeltaAuxAS;
} AXPBMIX;

typedef struct AXPBADDR
{
    U16 loopFlag;
    U16 format;
    U16 loopAddressHi;
    U16 loopAddressLo;
    U16 endAddressHi;
    U16 endAddressLo;
    U16 currentAddressHi;
    U16 currentAddressLo;
} AXPBADDR;

typedef struct AXPBSRC
{
    U16 ratioHi;
    U16 ratioLo;
    U16 currentAddressFrac;
    U16 last_samples[4];
} AXPBSRC;

typedef struct AXVPB
{
    U8 pad0[0x1C];
    U32 sync;          /* 0x1C */
    U8 pad20[0x124];
    U16 mixerCtrl;     /* 0x144 */
    U8 pad146[4];
    AXPBMIX mix;       /* 0x14A */
    U8 pad16E[0x2E];
    U16 currentVolume; /* 0x19C */
    S16 currentDelta;  /* 0x19E */
    U8 pad1A0[6];
    AXPBADDR addr;     /* 0x1A6 */
} AXVPB;

/* Private state kept in BINKSND.sysdata. */
typedef struct NGCSND
{
    S32 volume;          /* 0x00 */
    F32 pan;             /* 0x04 */
    S32 state;           /* 0x08: 0 priming, 1 starved, 2 playing */
    S32 paused;          /* 0x0C */
    volatile S32 lockindex; /* 0x10 */
    U32 writepos;        /* 0x14: next ARAM address to fill */
    U32 wrapend;         /* 0x18 */
    U32 chunksize;       /* 0x1C */
    U32 aramsize;        /* 0x20: ARAM bytes per channel */
    U32 lowwater;        /* 0x24 */
    U8* buffer;          /* 0x28 */
    U8* stereobuf;       /* 0x2C */
    U32 lasttime;        /* 0x30 */
    U32 timeout;         /* 0x34 */
    ARQRequest reqs[4];  /* 0x38: [channel * 2 + index] */
    AXVPB* voices[2];    /* 0xB8 */
    U32 arambase;        /* 0xC0 */
    U32 shift;           /* 0xC4 */
} NGCSND;

#define NGC(snd) ((NGCSND*)(snd)->sysdata)

void DCFlushRange(void* addr, U32 len);
void ARQPostRequest(ARQRequest* task, U32 owner, U32 type, U32 priority, U32 source, U32 dest, U32 length,
                    void (*callback)(U32));
void ARQRemoveRequest(ARQRequest* task);
AXVPB* AXAcquireVoice(U32 priority, void (*callback)(void*), U32 userContext);
void AXFreeVoice(AXVPB* p);
void AXSetVoiceState(AXVPB* p, U16 state);
void AXSetVoiceAddr(AXVPB* p, AXPBADDR* addr);
void AXSetVoiceLoopAddr(AXVPB* p, U32 addr);
void AXSetVoiceEndAddr(AXVPB* p, U32 addr);
void AXSetVoiceCurrentAddr(AXVPB* p, U32 addr);
void AXSetVoiceSrcType(AXVPB* p, U32 type);
void AXSetVoiceSrc(AXVPB* p, AXPBSRC* src);
void AXSetVoiceSrcRatio(AXVPB* p, F32 ratio);
F32 powf(F32 x, F32 y);
U32 RADTimerRead(void);
void* radaudiomalloc(U32 bytes);
void radaudiofree(void* ptr);

void NGC_SoundVolume(BINKSND* snd);

void MyAXSetVoiceCurrentAddr(NGCSND* n, U32 addr);
void MyAXSetVoiceLoopAddr(NGCSND* n, U32 addr);
void MyAXSetVoiceEndAddr(NGCSND* n, U32 addr);
void MyAXSetVoiceState(NGCSND* n, U16 state);
U32 get_play_pos(NGCSND* n);
U32 get_end_pos(NGCSND* n);
ARQRequest* get_task(NGCSND* n, U32 channel, U32 index);
void ConvDataToStereo8(U32* src, U16* left, U16* right, U32 bytes);
void ConvDataToStereo16(U32* src, U32* left, U32* right, U32 bytes);

inline void MyAXSetVoiceCurrentAddr(NGCSND* n, U32 addr)
{
    AXSetVoiceCurrentAddr(n->voices[0], addr >> n->shift);
    if (n->voices[1])
        AXSetVoiceCurrentAddr(n->voices[1], (addr + n->aramsize) >> n->shift);
}

inline void MyAXSetVoiceLoopAddr(NGCSND* n, U32 addr)
{
    AXSetVoiceLoopAddr(n->voices[0], addr >> n->shift);
    if (n->voices[1])
        AXSetVoiceLoopAddr(n->voices[1], (addr + n->aramsize) >> n->shift);
}

inline void MyAXSetVoiceEndAddr(NGCSND* n, U32 addr)
{
    AXSetVoiceEndAddr(n->voices[0], (addr >> n->shift) - 1);
    if (n->voices[1])
        AXSetVoiceEndAddr(n->voices[1], ((addr + n->aramsize) >> n->shift) - 1);
}

inline void MyAXSetVoiceState(NGCSND* n, U16 state)
{
    if (n->voices[0])
        AXSetVoiceState(n->voices[0], state);
    if (n->voices[1])
        AXSetVoiceState(n->voices[1], state);
}

inline U32 get_play_pos(NGCSND* n)
{
    return *(U32*)&n->voices[0]->addr.currentAddressHi << n->shift;
}

inline U32 get_end_pos(NGCSND* n)
{
    return *(U32*)&n->voices[0]->addr.endAddressHi << n->shift;
}

inline ARQRequest* get_task(NGCSND* n, U32 channel, U32 index)
{
    return &n->reqs[index + channel * 2];
}

inline void ConvDataToStereo8(U32* src, U16* left, U16* right, U32 bytes)
{
    U32 i, n, v;
    U16 s;

    n = bytes >> 2;
    for (i = 0; i < n; i++)
    {
        v = *src++;
        *left++ = ((v >> 16) & 0xFF00) | ((v >> 8) & 0xFF);
        *right++ = ((v >> 8) & 0xFF00) | (v & 0xFF);
    }
    n = (bytes - n * 4) >> 1;
    for (i = 0; i < n; i++)
    {
        s = *(U16*)src;
        src = (U32*)((U16*)src + 1);
        *(U8*)left = s >> 8;
        *(U8*)right = s;
        left = (U16*)((U8*)left + 1);
        right = (U16*)((U8*)right + 1);
    }
}

inline void ConvDataToStereo16(U32* src, U32* left, U32* right, U32 bytes)
{
    U32 i, n, a, b, c;

    n = bytes >> 3;
    for (i = 0; i < n; i++)
    {
        a = src[0];
        b = src[1];
        src += 2;
        *left++ = (a >> 16) | (b & 0xFFFF0000);
        *right++ = (a & 0xFFFF) | (b << 16);
    }
    n = (bytes - n * 8) >> 2;
    for (i = 0; i < n; i++)
    {
        c = *src++;
        *(U16*)left = c >> 16;
        *(U16*)right = c;
        left = (U32*)((U16*)left + 1);
        right = (U32*)((U16*)right + 1);
    }
}

void startVoices(U32 request)
{
    ARQRequest* req = (ARQRequest*)request;
    NGCSND* n = (NGCSND*)(req->owner & ~1);

    if (n && n->state == 0)
    {
        MyAXSetVoiceState(n, 1);
        n->state = 2;
    }
    req->owner &= ~1;
}

void NGC_SoundPlay(BINKSND* snd, U32 index, U32 size)
{
    ARQRequest* t;
    ARQRequest* t2;
    U32 end;

    t = get_task(NGC(snd), 0, index);
    end = NGC(snd)->writepos + size;
    if (NGC(snd)->voices[1])
    {
        t2 = get_task(NGC(snd), 1, index);
        DCFlushRange((void*)t2->source, size);
        ARQPostRequest(t2, 0, 0, 1, t2->source, NGC(snd)->writepos + NGC(snd)->aramsize, size, 0);
    }
    DCFlushRange((void*)t->source, size);
    ARQPostRequest(t, t->owner, 0, 1, t->source, NGC(snd)->writepos, size, startVoices);
    if (end > NGC(snd)->arambase + NGC(snd)->aramsize - NGC(snd)->chunksize)
    {
        if (get_play_pos(NGC(snd)) > end)
        {
            NGC(snd)->wrapend = end;
        }
        else
        {
            MyAXSetVoiceEndAddr(NGC(snd), end);
            end = NGC(snd)->arambase;
        }
    }
    NGC(snd)->writepos = end;
    NGC(snd)->lasttime = RADTimerRead();
}

S32 NGC_SoundReinit(BINKSND* snd)
{
    U32 i;

    MyAXSetVoiceState(NGC(snd), 0);
    NGC(snd)->lockindex = -1;
    NGC(snd)->state = 0;
    NGC(snd)->writepos = NGC(snd)->arambase;
    MyAXSetVoiceCurrentAddr(NGC(snd), NGC(snd)->writepos);
    MyAXSetVoiceLoopAddr(NGC(snd), NGC(snd)->writepos);
    MyAXSetVoiceEndAddr(NGC(snd), NGC(snd)->writepos + NGC(snd)->aramsize);
    NGC(snd)->wrapend = 0;
    for (i = 0; i < 4; i++)
    {
        if (NGC(snd)->reqs[i].owner & 1)
            ARQRemoveRequest(&NGC(snd)->reqs[i]);
        NGC(snd)->reqs[i].owner = (U32)NGC(snd);
    }
    return 1;
}

void SetStreamVolumePan(AXVPB* v, S32 volume, S32 left, S32 right)
{
    AXPBMIX* mix = &v->mix;

    memset(mix, 0, sizeof(AXPBMIX));
    mix->vL = left;
    mix->vR = right;
    v->currentVolume = volume;
    v->currentDelta = 0;
    v->mixerCtrl = 3;
    v->sync |= 0x212;
}

void NGC_SoundVolume(BINKSND* snd)
{
    S32 l, r;

    l = (S32)(powf(1.0f - NGC(snd)->pan, 0.3f) * 65535.0f);
    r = (S32)(powf(NGC(snd)->pan, 0.3f) * 65535.0f);
    if (NGC(snd)->voices[0])
        SetStreamVolumePan(NGC(snd)->voices[0], NGC(snd)->volume, l, (snd->chans == 1) ? r : 0);
    if (NGC(snd)->voices[1])
        SetStreamVolumePan(NGC(snd)->voices[1], NGC(snd)->volume, 0, r);
}

S32 NGC_SoundInit(BINKSND* snd)
{
    AXPBADDR addr;
    AXPBSRC src;
    U32 bps, chunk, size, i, a, end;
    U16 fmt;
    AXVPB* v;

    NGC(snd)->timeout = 720;
    NGC(snd)->lockindex = -1;
    NGC(snd)->state = 0;
    bps = (snd->freq * snd->bits) >> 3;
    chunk = (bps / 25 + 31) & ~31;
    size = ((bps * 800) / 1000 + 63) & ~63;
    NGC(snd)->chunksize = chunk;
    NGC(snd)->aramsize = size;
    snd->sndlowwater = chunk * snd->chans;
    NGC(snd)->lowwater = size - (size * 90) / 100;
    if (snd->bits != 16)
        snd->sndlowwater = chunk * snd->chans * 2;
    snd->sndalign = -(snd->chans * 32);
    NGC(snd)->buffer = radmalloc(NGC(snd)->aramsize * snd->chans);
    if (NGC(snd)->buffer == 0)
        return 0;
    if (snd->chans == 2)
    {
        NGC(snd)->stereobuf = radmalloc(NGC(snd)->aramsize);
        if (NGC(snd)->stereobuf == 0)
            return 0;
    }
    NGC(snd)->arambase = (U32)radaudiomalloc(NGC(snd)->aramsize * snd->chans);
    if (NGC(snd)->arambase == 0)
        return 0;
    NGC(snd)->shift = (snd->bits == 16);
    for (i = 0; i < 4; i++)
        NGC(snd)->reqs[i].owner = (U32)NGC(snd);
    NGC(snd)->writepos = NGC(snd)->arambase;
    NGC(snd)->wrapend = 0;
    for (i = 0; i < snd->chans; i++)
    {
        v = NGC(snd)->voices[i] = AXAcquireVoice(31, 0, 0);
        if (v == 0)
            return 0;
        addr.loopFlag = 1;
        fmt = 0x19;
        end = ((NGC(snd)->arambase + NGC(snd)->aramsize * (i + 1)) >> NGC(snd)->shift) - 1;
        a = (NGC(snd)->arambase + i * NGC(snd)->aramsize) >> NGC(snd)->shift;
        if (snd->bits == 16)
            fmt = 10;
        addr.format = fmt;
        addr.endAddressHi = end >> 16;
        addr.endAddressLo = end;
        addr.loopAddressHi = a >> 16;
        addr.loopAddressLo = a;
        addr.currentAddressHi = a >> 16;
        addr.currentAddressLo = a;
        AXSetVoiceAddr(v, &addr);
        if (snd->freq == 32000)
        {
            AXSetVoiceSrcType(NGC(snd)->voices[i], 0);
        }
        else
        {
            AXSetVoiceSrcType(NGC(snd)->voices[i], 1);
            src.ratioHi = 1;
            src.ratioLo = 0;
            src.currentAddressFrac = 0;
            src.last_samples[0] = 0;
            src.last_samples[1] = 0;
            src.last_samples[2] = 0;
            src.last_samples[3] = 0;
            AXSetVoiceSrc(NGC(snd)->voices[i], &src);
            AXSetVoiceSrcRatio(NGC(snd)->voices[i], (F32)snd->freq / 32000.0f);
        }
    }
    NGC_SoundVolume(snd);
    return 1;
}

void NGC_SoundPause(BINKSND* snd)
{
    MyAXSetVoiceState(NGC(snd), 0);
}

void NGC_SoundResume(BINKSND* snd)
{
    MyAXSetVoiceState(NGC(snd), 1);
}

void NGC_SoundShutdown(BINKSND* snd)
{
    U32 i;

    NGC(snd)->paused = 1;
    MyAXSetVoiceState(NGC(snd), 0);
    for (i = 0; i < 2; i++)
    {
        if (NGC(snd)->voices[i])
        {
            AXFreeVoice(NGC(snd)->voices[i]);
            NGC(snd)->voices[i] = 0;
        }
    }
    if (NGC(snd)->buffer)
    {
        radfree(NGC(snd)->buffer);
        NGC(snd)->buffer = 0;
    }
    if (NGC(snd)->stereobuf)
    {
        radfree(NGC(snd)->stereobuf);
        NGC(snd)->stereobuf = 0;
    }
    if (NGC(snd)->arambase)
    {
        radaudiofree((void*)NGC(snd)->arambase);
        NGC(snd)->arambase = 0;
    }
}

S32 Lock(BINKSND* snd, U8** addr, U32* len)
{
    U32 amt, play, half;
    U8* p;

    if (NGC(snd)->lockindex >= 0)
    {
        play = get_play_pos(NGC(snd));
        if (NGC(snd)->writepos >= play)
            amt = NGC(snd)->arambase + NGC(snd)->aramsize - NGC(snd)->writepos;
        else
            amt = play - NGC(snd)->writepos - 1;
        half = NGC(snd)->aramsize >> 1;
        if (amt > half)
            amt = half;
        else if (amt < NGC(snd)->chunksize)
            amt = NGC(snd)->chunksize;
        amt *= snd->chans;
        p = NGC(snd)->buffer + ((NGC(snd)->lockindex * NGC(snd)->aramsize) >> 1);
        get_task(NGC(snd), 0, NGC(snd)->lockindex)->source = (U32)p;
        get_task(NGC(snd), 1, NGC(snd)->lockindex)->source = (U32)p + NGC(snd)->aramsize;
        if (snd->chans == 2)
            p = NGC(snd)->stereobuf;
        *addr = p;
        *len = amt;
        return 1;
    }
    return 0;
}

S32 Unlock(BINKSND* snd, U32 filled)
{
    NGCSND* n;
    U32 i, size;
    U32* src;
    U32* l;
    U32* r;

    if (NGC(snd)->lockindex == -1)
        return 0;
    n = NGC(snd);
    get_task(n, 0, NGC(snd)->lockindex)->owner |= 1;
    if (snd->chans == 2)
    {
        l = (U32*)get_task(n, 0, NGC(snd)->lockindex)->source;
        r = (U32*)get_task(n, 1, NGC(snd)->lockindex)->source;
        src = (U32*)NGC(snd)->stereobuf;
        if (snd->bits == 16)
            ConvDataToStereo16(src, l, r, filled);
        else
            ConvDataToStereo8(src, (U16*)l, (U16*)r, filled);
        filled >>= 1;
    }
    if (filled & 31)
    {
        size = (filled + 31) & ~31;
        for (i = 0; i < snd->chans; i++)
            memset((U8*)get_task(n, i, NGC(snd)->lockindex)->source + filled, 0, size - filled);
        filled = size;
    }
    if (NGC(snd)->state == 0)
    {
        get_task(n, 0, NGC(snd)->lockindex)->length = filled;
        if (NGC(snd)->lockindex == 1)
        {
            NGC_SoundPlay(snd, 0, get_task(NGC(snd), 0, 0)->length);
            NGC_SoundPlay(snd, 1, get_task(NGC(snd), 0, 1)->length);
        }
    }
    else
    {
        NGC_SoundPlay(snd, NGC(snd)->lockindex, filled);
    }
    return 1;
}

void NGC_StarvedClear(BINKSND* snd)
{
    NGCSND* n = NGC(snd);
    S32 i;
    U32 idx, start, end;
    U8* p;

    i = 0;
again:
    idx = i & 1;
    if (get_task(n, 0, idx)->owner & 1)
        goto busy;
    start = NGC(snd)->writepos;
    end = start + NGC(snd)->chunksize;
    if (snd->chans == 2)
        p = NGC(snd)->stereobuf;
    else
        p = NGC(snd)->buffer + ((idx * NGC(snd)->aramsize) >> 1);
    memset(p, 0, NGC(snd)->chunksize);
    get_task(n, 0, idx)->source = (U32)p;
    get_task(n, 1, idx)->source = (U32)p;
    NGC_SoundPlay(snd, idx, NGC(snd)->chunksize);
    if (get_play_pos(n) > end)
        MyAXSetVoiceCurrentAddr(n, start);
    MyAXSetVoiceLoopAddr(n, start);
    MyAXSetVoiceEndAddr(n, end);
    return;

busy:
    if (++i <= 99999)
        goto again;
}

S32 Ready(BINKSND* snd)
{
    NGCSND* n;
    S32 idx = -1;
    U32 now, play, end, amt;

    if (NGC(snd)->paused || snd->onoff == 0 || NGC(snd)->voices[0] == 0)
        return 0;
    now = RADTimerRead();
    n = NGC(snd);
    play = get_play_pos(n);
    if (NGC(snd)->state == 2)
    {
        amt = 0;
        end = get_end_pos(n);
        if (play < NGC(snd)->wrapend)
        {
            end = NGC(snd)->wrapend;
            MyAXSetVoiceEndAddr(n, end);
            NGC(snd)->wrapend = 0;
            NGC(snd)->writepos = NGC(snd)->arambase;
        }
        if (now - NGC(snd)->lasttime < NGC(snd)->timeout)
        {
            if (play < NGC(snd)->writepos)
                amt = NGC(snd)->writepos - play;
            else
                amt = (end - play) + (NGC(snd)->writepos - NGC(snd)->arambase);
            if (amt >= NGC(snd)->lowwater)
                goto check;
        }
        NGC_StarvedClear(snd);
        NGC(snd)->state = 1;
        snd->sndstarved = 1;
    }
check:
    if (NGC(snd)->wrapend == 0)
    {
        if (NGC(snd)->writepos >= play || play - NGC(snd)->writepos > NGC(snd)->chunksize)
        {
            for (idx = 0; idx < 2; idx++)
            {
                if (!(n->reqs[idx].owner & 1))
                    goto found;
            }
        }
        idx = -1;
    }
found:
    NGC(snd)->lasttime = now;
    NGC(snd)->lockindex = idx;
    return idx != -1;
}

void Volume(BINKSND* snd, S32 volume)
{
    if (volume < 0)
        volume = 0;
    if (volume > 0x7FFF)
        volume = 0x7FFF;
    NGC(snd)->volume = volume;
    NGC_SoundVolume(snd);
}

void Pan(BINKSND* snd, S32 pan)
{
    if (pan < 0)
        pan = 0;
    if (pan > 0x10000)
        pan = 0x10000;
    NGC(snd)->pan = (F32)pan * (1.0f / 65536.0f);
    NGC_SoundVolume(snd);
}

S32 SetOnOff(BINKSND* snd, S32 onoff)
{
    if (onoff == 1 && snd->onoff == 0)
    {
        if (!NGC_SoundReinit(snd))
            goto done;
    }
    else if (onoff == 0 && snd->onoff == 1)
    {
        MyAXSetVoiceState(NGC(snd), 0);
    }
    else
    {
        goto done;
    }
    snd->onoff = onoff;
done:
    return snd->onoff;
}

S32 Pause(BINKSND* snd, S32 pause)
{
    if (pause)
    {
        NGC_SoundPause(snd);
        NGC(snd)->paused = 1;
    }
    else
    {
        NGC_SoundResume(snd);
        NGC(snd)->paused = 0;
    }
    return NGC(snd)->paused;
}

void Close(BINKSND* snd)
{
    NGC_SoundShutdown(snd);
}

S32 Open(BINKSND* snd, U32 freq, S32 bits, S32 chans, U32 flags, void* bink)
{
    memset(snd, 0, sizeof(BINKSND));
    snd->freq = freq;
    snd->bits = bits;
    snd->chans = chans;
    snd->sndstarved = 0;
    NGC(snd)->paused = 0;
    NGC(snd)->volume = 0x7FFF;
    NGC(snd)->pan = 0.5f;
    snd->onoff = 1;
    snd->Ready = Ready;
    snd->Lock = Lock;
    snd->Unlock = (BINKSNDUNLOCK)Unlock;
    snd->Volume = Volume;
    snd->Pan = Pan;
    snd->Pause = (BINKSNDPAUSE)Pause;
    snd->SetOnOff = SetOnOff;
    snd->Close = Close;
    if (!NGC_SoundInit(snd))
        return 0;
    snd->latency = 0;
    return 1;
}

U32 BinkOpenNGCSound(U32 param)
{
    return (U32)Open;
}
