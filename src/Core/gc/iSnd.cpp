#include "iSnd.h"
#include "xSnd.h"
#include "xEnt.h"
#include "xstransvc.h"
#include "xMath.h"

#ifndef NULL
#define NULL 0
#endif

// Older Dolphin SDK layouts (only the fields iSnd touches).

class AXPBADDR
{
public:
    unsigned short loopFlag;         // 0x0
    unsigned short format;           // 0x2
    unsigned short loopAddressHi;    // 0x4
    unsigned short loopAddressLo;    // 0x6
    unsigned short endAddressHi;     // 0x8
    unsigned short endAddressLo;     // 0xA
    unsigned short currentAddressHi; // 0xC
    unsigned short currentAddressLo; // 0xE
};

class AXPB
{
public:
    unsigned char pad0[0x6E];
    AXPBADDR addr;           // 0x6E
    unsigned short adpcm[0x14]; // 0x7E
    unsigned short ratioHi;  // 0xA6
    unsigned short ratioLo;  // 0xA8
};

class _AXVPB
{
public:
    unsigned char pad0[0x1C];
    unsigned int sync;        // 0x1C
    unsigned char pad20[0x118];
    AXPB pb;                  // 0x138
};

class ARQRequest
{
public:
    ARQRequest* next;        // 0x0
    unsigned long owner;     // 0x4
    unsigned long type;      // 0x8
    unsigned long priority;  // 0xC
    unsigned long source;    // 0x10
    unsigned long dest;      // 0x14
    unsigned long length;    // 0x18
    void (*callback)(unsigned long); // 0x1C
};

class DVDCommandBlock
{
public:
    unsigned char pad0[0x2C];
    void* userData; // 0x2C
};

class DVDFileInfo
{
public:
    DVDCommandBlock cb;     // 0x0
    unsigned int startAddr; // 0x30
    unsigned int length;    // 0x34
    void* callback;         // 0x38
};

class tag_xFile
{
public:
    char relname[16];      // 0x0
    unsigned int flags;    // 0x10
    char path[128];        // 0x14
    signed int entrynum;   // 0x94
    DVDFileInfo fileInfo;  // 0x98
    unsigned char padD4[0x2C];
};

class xTRCState
{
public:
    signed int state;  // 0x0
    unsigned int mode; // 0x4
};

class iTRCDisk
{
public:
    static signed int CheckDVDAndResetState();
};

extern "C" {
void* memcpy(void* dst, const void* src, unsigned long n);
void* malloc(unsigned long size);
void free(void* p);
void exit(signed int status);
signed int OSDisableInterrupts();
signed int OSRestoreInterrupts(signed int level);
void OSSetSoundMode(unsigned int mode);
unsigned int OSGetSoundMode();
void DCInvalidateRange(void* addr, unsigned long n);
void DCStoreRange(void* addr, unsigned long n);
void AIInit(unsigned char* stack);
void ARInit(unsigned long* stack, unsigned long count);
unsigned long ARAlloc(unsigned long length);
unsigned long ARFree(unsigned long* length);
void ARQInit();
void ARQPostRequest(ARQRequest* task, unsigned long owner, unsigned long type,
                    unsigned long priority, unsigned long source, unsigned long dest,
                    unsigned long length, void (*callback)(unsigned long));
void ARQRemoveRequest(ARQRequest* task);
void ARQFlushQueue();
void AXInit();
void AXQuit();
void AXRegisterCallback(void (*callback)());
_AXVPB* AXAcquireVoice(unsigned long priority, void (*callback)(void*), unsigned long userContext);
void AXFreeVoice(_AXVPB* p);
void AXSetVoiceState(_AXVPB* p, unsigned short state);
void AXSetVoiceType(_AXVPB* p, unsigned short type);
void AXSetVoiceAddr(_AXVPB* p, AXPBADDR* addr);
void AXSetVoiceAdpcm(_AXVPB* p, void* adpcm);
void AXSetVoiceSrcType(_AXVPB* p, unsigned long type);
void AXSetVoiceLoop(_AXVPB* p, unsigned short loop);
void AXSetVoiceLoopAddr(_AXVPB* p, unsigned long addr);
void AXSetVoiceEndAddr(_AXVPB* p, unsigned long addr);
void MIXInit();
void MIXInitChannel(_AXVPB* p, unsigned long mode, signed int input, signed int auxA,
                    signed int auxB, signed int pan, signed int span, signed int fader);
void MIXReleaseChannel(_AXVPB* p);
void MIXUnMute(_AXVPB* p);
void MIXUpdateSettings();
signed int MIXGetInput(_AXVPB* p);
void MIXAdjustInput(_AXVPB* p, signed int dB);
signed int MIXGetFader(_AXVPB* p);
void MIXAdjustFader(_AXVPB* p, signed int dB);
signed int MIXGetPan(_AXVPB* p);
void MIXAdjustPan(_AXVPB* p, signed int pan);
signed int DVDCancelAsync(DVDCommandBlock* block, void* callback);
void DVDCancelAll();
signed int DVDReadAsyncPrio(DVDFileInfo* fileInfo, void* addr, signed int length,
                            signed int offset, void (*callback)(long, DVDFileInfo*),
                            signed int prio);
// TODO: unidentified xTRC functions
void fn_800443AC();
unsigned int fn_800443F8();
extern xTRCState gTrcDisk;
}

unsigned int iFileOpen(char* name, signed int flags, tag_xFile* file);
signed int iFileSeek(tag_xFile* file, signed int offset, signed int whence);
signed int iFileReadAsync(tag_xFile* file, void* buf, unsigned int size, void (*callback)(signed int));
float xVec3Normalize(_xVec3* o, _xVec3* v);
void zGameScreenTransitionUpdate(float percentComplete, char* msg, unsigned char* buf);

#ifdef PC_PORT
// The sound info's 32-bit addresses are copied onto AXPBADDR's Hi/Lo halfword
// pairs, which only lines up on a big-endian CPU: swap the halves back.
static inline void PCFixAddrHalves(AXPBADDR* a)
{
    unsigned short t;
    t = a->loopAddressHi; a->loopAddressHi = a->loopAddressLo; a->loopAddressLo = t;
    t = a->endAddressHi; a->endAddressHi = a->endAddressLo; a->endAddressLo = t;
    t = a->currentAddressHi; a->currentAddressHi = a->currentAddressLo; a->currentAddressLo = t;
}
#define PC_FIX_ADDR_HALVES(a) PCFixAddrHalves(a)
#else
#define PC_FIX_ADDR_HALVES(a)
#endif

// DSPADPCM header of one sound, followed by its asset ID.
class sDSPADPCM
{
public:
    unsigned int num_samples;       // 0x0
    unsigned int num_adpcm_nibbles; // 0x4
    unsigned int sample_rate;       // 0x8
    unsigned short loop_flag;       // 0xC
    unsigned short format;          // 0xE
    unsigned int sa;                // 0x10
    unsigned int ea;                // 0x14
    unsigned int ca;                // 0x18
    unsigned short coef[16];        // 0x1C
    unsigned short gain;            // 0x3C
    unsigned short ps;              // 0x3E
    unsigned short yn1;             // 0x40
    unsigned short yn2;             // 0x42
    unsigned short lps;             // 0x44
    unsigned short lyn1;            // 0x46
    unsigned short lyn2;            // 0x48
    unsigned short pad[11];         // 0x4A
    unsigned int aid;               // 0x60
};

// The sound info asset: in-memory sounds, then streamed ones.
class iSndInfoAsset
{
public:
    unsigned int nsounds;     // 0x0
    unsigned int aramsize;    // 0x4
    unsigned int nstreams;    // 0x8
    sDSPADPCM entries[1];  // 0xC
};

class iSndBank
{
public:
    tag_xFile file;        // 0x0
    unsigned int aramBase; // 0x100
    iSndInfoAsset* info;   // 0x104
};

class iSndLookupInfo
{
public:
    sDSPADPCM info; // 0x0
    signed int handle; // 0x64
    tag_xFile file;    // 0x68
    unsigned char pad168[8];
};

class vinfo
{
public:
    _AXVPB* voice;      // 0x0
    unsigned int flags; // 0x4
    unsigned int aid;   // 0x8
    float time;         // 0xC
    signed int x10;     // 0x10
    signed int x14;     // 0x14
    signed int x18;     // 0x18
    signed int x1c;     // 0x1C
};

class iSndStream
{
public:
    vinfo vinf;             // 0x0
    sDSPADPCM info;      // 0x20
    unsigned int pad84;     // 0x84
    unsigned int offset;    // 0x88
    unsigned int size;      // 0x8C
    unsigned int read;      // 0x90
    unsigned int x94;       // 0x94
    DVDFileInfo fileInfo;   // 0x98
    unsigned int source_a;  // 0xD4
    unsigned int dest_a;    // 0xD8
    unsigned int dest_b;    // 0xDC
    ARQRequest request;     // 0xE0
    unsigned int pad100;    // 0x100
};

static unsigned long aram_array[40];
static vinfo voices[58];
static iSndStream streams[6];
static iSndBank banks[12];
static iSndLookupInfo lookup;

static void* ua_stream_buffer;
static void* stream_buffer;
static unsigned int silence_buffer;
static unsigned int zero_point;
static unsigned int zero_end;
static signed int nbanks;
static volatile unsigned int SoundFlags;
static volatile signed int fc;
static unsigned int houston_we_have_a_problem;
static signed int lookupFromScene;

static void dv_callback(void* userdata);
static void arq_callback(unsigned long);
static void dvdcb(long result, DVDFileInfo* info);
static void arqcb(unsigned long pointerToARQRequest);
static void fcb();
static void sndloadcb(signed int);
void iSndUpdateStreams(float dt);
void iSndCalcVol3d(xSndVoiceInfo* vp, vinfo* info);
unsigned int iSndPlaySound(xSndVoiceInfo* vp);
unsigned int iSndPlayStream(xSndVoiceInfo* vp);

static inline void iSndUpdateVoicePos(xSndVoiceInfo* vp)
{
    if ((vp->flags & 0x10) && (vp->flags & 0x8))
    {
        if (vp->parentPos != NULL)
        {
            vp->pos = *vp->parentPos;
        }
        else
        {
            _xEnt* ent = (_xEnt*)(vp->parentID & ~3);
            if (ent->baseFlags & 1)
            {
                vp->pos = ((_xMat4x3*)ent->model->Mat)->pos;
            }
            else
            {
                vp->flags &= (unsigned short)~0x10;
            }
        }
    }
}

static inline void iSndCalcVol(xSndVoiceInfo* vp, vinfo* info)
{
    if (vp->flags & 0x8)
    {
        iSndCalcVol3d(vp, info);
    }
    else
    {
        float vol = 1.0f;
        if (vp->flags & 0x400)
        {
            vol *= gSnd.streamvol.left > gSnd.streamvol.left ? gSnd.streamvol.left
                                                              : gSnd.streamvol.left;
        }
        else
        {
            vol *= gSnd.fxvol.left > gSnd.fxvol.right ? gSnd.fxvol.left : gSnd.fxvol.right;
        }
        vol *= gSnd.mastervol.left > gSnd.mastervol.right ? gSnd.mastervol.left
                                                          : gSnd.mastervol.right;
        signed int v = 200.0f * vol - 200.0f;
        if (v <= -190)
        {
            v = -900;
        }
        info->x10 = v;
        info->x14 = v;
        info->x18 = 0x40;
        info->x1c = 0x40;
        MIXAdjustFader(info->voice, v - MIXGetFader(info->voice));
        MIXAdjustPan(info->voice, 0x40 - MIXGetPan(info->voice));
    }
}

static inline void iSndStopStream(iSndStream* s)
{
    DVDCancelAsync(&s->fileInfo.cb, NULL);
    ARQRemoveRequest(&s->request);
    AXSetVoiceState(s->vinf.voice, 0);
    MIXReleaseChannel(s->vinf.voice);
}

static inline unsigned char iSndDiskReady()
{
    return gTrcDisk.mode == 6;
}

// TODO: NonMatching. iSndWaitForDeadSounds keeps its 140-frame limit in a saved
// register (BFBB's version has the same problem), arqcb lacks a redundant
// bne/blr pair, and the rest are register-allocation differences.

// Functions are in reverse DOL order (-inline deferred emits last to first).

static void dv_callback(void* userdata)
{
    unsigned int stream = ((unsigned int*)userdata)[5];
    xSndVoiceInfo* info = &gSnd.voice[stream];
    info->sndID = 0;
    info->flags = 0;
    if (stream < 6)
    {
        unsigned int idx = stream;
        if (streams[idx].vinf.voice == NULL)
        {
            return;
        }
        DVDCancelAsync(&streams[idx].fileInfo.cb, NULL);
        ARQRemoveRequest(&streams[idx].request);
        AXSetVoiceState(streams[idx].vinf.voice, 0);
        MIXReleaseChannel(streams[idx].vinf.voice);
        streams[idx].vinf.voice = NULL;
        streams[idx].vinf.flags = 0;
        streams[idx].vinf.aid = 0;
    }
    else
    {
        unsigned int idx = stream - 6;
        if (voices[idx].voice == NULL)
        {
            return;
        }
        MIXReleaseChannel(voices[idx].voice);
        AXSetVoiceState(voices[idx].voice, 0);
        voices[idx].voice = NULL;
        voices[idx].flags = 0;
        voices[idx].aid = 0;
    }
}

static void arq_callback(unsigned long)
{
    SoundFlags = 0;
}

static void dvdcb(long result, DVDFileInfo* info)
{
    iSndStream* s = (iSndStream*)info->cb.userData;
    if (s->vinf.voice == NULL)
    {
        return;
    }
    s->offset += result;
    s->read += result;
    s->vinf.flags |= 0x800;
    s->vinf.flags &= ~0x100;
    if (s->vinf.flags & 0x600)
    {
        houston_we_have_a_problem = 1;
    }
    if (s->vinf.flags & 0x4)
    {
        unsigned int dest = 0;
        if (!(s->vinf.flags & 0x1000))
        {
            s->vinf.flags |= 0x200;
            dest = s->dest_a;
        }
        else if (!(s->vinf.flags & 0x2000))
        {
            s->vinf.flags |= 0x400;
            dest = s->dest_b;
        }
        if (dest != 0)
        {
            ARQPostRequest(&s->request, (unsigned long)s, 0, 1, s->source_a, dest, 12000, arqcb);
        }
    }
}

static void arqcb(unsigned long pointerToARQRequest)
{
    iSndStream* s = (iSndStream*)((ARQRequest*)pointerToARQRequest)->owner;
    unsigned int left = s->size - s->read;
    if (((ARQRequest*)pointerToARQRequest)->length == 0)
    {
        return;
    }
    if (s->vinf.voice == NULL)
    {
        return;
    }
    if (s->vinf.flags & 0x100)
    {
        houston_we_have_a_problem = 1;
    }
    if (s->vinf.flags & 0x200)
    {
        s->vinf.flags |= 0x1000;
        s->vinf.flags &= ~0x200;
    }
    else if (s->vinf.flags & 0x400)
    {
        s->vinf.flags |= 0x2000;
        s->vinf.flags &= ~0x400;
    }
    if (left == 0)
    {
        s->vinf.flags |= 0x20;
    }
    else
    {
        s->vinf.flags |= 0x200000;
    }
    s->vinf.flags &= ~0x800;
    s->vinf.flags &= ~0x400000;
    if (s->vinf.flags & 0x4)
    {
        s->vinf.flags &= ~0x4;
        s->vinf.flags |= 0x8;
    }
}

static void fcb()
{
    signed int need_update = 0;
    signed int i;

    fc++;
    for (i = 0; i < 6; i++)
    {
        unsigned int flags = streams[i].vinf.flags;
        xSndVoiceInfo* vp = &gSnd.voice[i];
        if (streams[i].vinf.voice == NULL)
        {
            continue;
        }

        unsigned short addrHi = streams[i].vinf.voice->pb.addr.currentAddressHi;
        unsigned short addrLo = streams[i].vinf.voice->pb.addr.currentAddressLo;
        unsigned int cur = ((unsigned int)addrHi << 16) + addrLo;
        if (cur >= zero_end - 0x5DC0 && cur < zero_end)
        {
            iSndStopStream(&streams[i]);
            AXFreeVoice(streams[i].vinf.voice);
            voices[i].voice = NULL;
            voices[i].flags = 0;
            voices[i].aid = 0;
            continue;
        }

        if (flags & 0x4 || flags & 0x40000 || flags & 0x80000 || flags & 0x20000 || flags & 0x2)
        {
            continue;
        }

        unsigned int addr = (addrHi << 16) + addrLo;
        if (addr < streams[i].dest_b * 2 && !(flags & 0x4000) && !(flags & 0x8))
        {
            streams[i].vinf.flags |= 0x4000;
            streams[i].vinf.flags &= ~0x8000;
            streams[i].vinf.flags &= ~0x2000;
            if (streams[i].read == streams[i].x94)
            {
                streams[i].vinf.flags |= 0x20;
            }
            else
            {
                streams[i].x94 = streams[i].read;
            }
            flags = streams[i].vinf.flags;
        }
        else if (addr >= streams[i].dest_b * 2 && !(flags & 0x8000))
        {
            streams[i].vinf.flags |= 0x8000;
            streams[i].vinf.flags &= ~0x4000;
            streams[i].vinf.flags &= ~0x1000;
            flags = streams[i].vinf.flags;
        }

        if ((flags & 0x20) && (flags & 0x4000))
        {
            streams[i].vinf.flags &= ~0x20;
            unsigned int end = streams[i].dest_a * 2 + streams[i].info.num_adpcm_nibbles % 48000;
            AXSetVoiceLoopAddr(streams[i].vinf.voice, zero_point);
            AXSetVoiceEndAddr(streams[i].vinf.voice, end - 1);
            AXSetVoiceLoop(streams[i].vinf.voice, 0);
            AXSetVoiceType(streams[i].vinf.voice, 0);
            streams[i].vinf.flags |= 0x40;
            flags = streams[i].vinf.flags;
        }

        if (flags & 0x100000)
        {
            AXFreeVoice(streams[i].vinf.voice);
            streams[i].vinf.voice = NULL;
            streams[i].vinf.flags = 0;
            streams[i].vinf.aid = 0;
            continue;
        }

        if (addr >= zero_point && addr < zero_end && (flags & 0x40))
        {
            streams[i].vinf.flags &= ~0x40;
            iSndStopStream(&streams[i]);
            if (streams[i].vinf.flags & 0x10000)
            {
                unsigned int priority = vp->priority;
                if (priority > 0xFF)
                {
                    priority = 0xFF;
                }
                AXFreeVoice(streams[i].vinf.voice);
                streams[i].vinf.voice = AXAcquireVoice(priority >> 3, dv_callback, i);
                if (streams[i].vinf.voice == NULL)
                {
                    streams[i].vinf.voice = NULL;
                    streams[i].vinf.flags = 0;
                    streams[i].vinf.aid = 0;
                    continue;
                }
                MIXInitChannel(streams[i].vinf.voice, 0xC, 0, -900, -900, 0x40, 0x7F, -900);
                streams[i].vinf.flags = 0x4;
                iSndPlay(vp);
                continue;
            }
            streams[i].vinf.flags = 0x40000;
            continue;
        }

        if (flags & 0x8)
        {
            if (!(streams[i].vinf.flags & 0x2))
            {
                AXSetVoiceState(streams[i].vinf.voice, 1);
            }
            MIXUnMute(streams[i].vinf.voice);
            need_update = 1;
            streams[i].vinf.flags |= 0x10;
            streams[i].vinf.flags |= 0x4000;
            streams[i].vinf.flags &= ~0x8;
            flags = streams[i].vinf.flags;
        }

        if ((flags & 0x200000) && !(flags & 0x200) && !(flags & 0x400) && !(flags & 0x100) &&
            !(flags & 0x400000))
        {
            streams[i].vinf.flags &= ~0x200000;
            streams[i].vinf.flags |= 0x100;
            streams[i].vinf.flags |= 0x400000;
            unsigned int size = streams[i].size - streams[i].read;
            if (size > 12000)
            {
                DVDReadAsyncPrio(&streams[i].fileInfo, (void*)streams[i].source_a, 12000, streams[i].offset, dvdcb, 2);
            }
            else
            {
                DVDReadAsyncPrio(&streams[i].fileInfo, (void*)streams[i].source_a, size, streams[i].offset, dvdcb, 2);
            }
        }

        if (flags & 0x800)
        {
            unsigned int length = 12000;
            if (streams[i].read % 12000)
            {
                length = streams[i].read % 12000;
            }
            unsigned int bit12 = flags & 0x1000;
            if (bit12 && (flags & 0x4000) && !(flags & 0x400) && !(flags & 0x2000))
            {
                streams[i].vinf.flags |= 0x400;
                ARQPostRequest(&streams[i].request, (unsigned long)&streams[i], 0, 1, streams[i].source_a, streams[i].dest_b, length,
                               arqcb);
            }
            else if ((flags & 0x2000) && (flags & 0x8000) && !(flags & 0x200) && !bit12)
            {
                streams[i].vinf.flags |= 0x200;
                ARQPostRequest(&streams[i].request, (unsigned long)&streams[i], 0, 1, streams[i].source_a, streams[i].dest_a, length,
                               arqcb);
            }
        }

        if (streams[i].vinf.x10 != streams[i].vinf.x14 || streams[i].vinf.x18 != streams[i].vinf.x1c)
        {
            streams[i].vinf.x14 = streams[i].vinf.x10;
            streams[i].vinf.x1c = streams[i].vinf.x18;
            MIXAdjustInput(streams[i].vinf.voice, streams[i].vinf.x1c - MIXGetInput(streams[i].vinf.voice));
            MIXAdjustPan(streams[i].vinf.voice, streams[i].vinf.x1c - MIXGetPan(streams[i].vinf.voice));
            need_update = 1;
        }
    }

    for (i = 0; i < 58; i++)
    {
        if (voices[i].voice == NULL || (voices[i].flags & 0x1))
        {
            continue;
        }
        unsigned int addr = (voices[i].voice->pb.addr.currentAddressHi << 16) +
                            voices[i].voice->pb.addr.currentAddressLo;
        if ((voices[i].flags & 0x4) && voices[i].voice->pb.addr.loopFlag == 0 && addr >= zero_point &&
            addr < zero_end)
        {
            voices[i].flags &= ~0x4;
            AXSetVoiceState(voices[i].voice, 0);
            MIXReleaseChannel(voices[i].voice);
            AXFreeVoice(voices[i].voice);
            voices[i].voice = NULL;
            voices[i].flags = 0;
            voices[i].aid = 0;
        }
        else if (voices[i].x10 != voices[i].x14 || voices[i].x18 != voices[i].x1c)
        {
            voices[i].x14 = voices[i].x10;
            voices[i].x1c = voices[i].x18;
            MIXAdjustInput(voices[i].voice, voices[i].x1c - MIXGetInput(voices[i].voice));
            MIXAdjustPan(voices[i].voice, voices[i].x1c - MIXGetPan(voices[i].voice));
            need_update = 1;
        }
    }

    if (need_update)
    {
        MIXUpdateSettings();
    }
}

void iSndInit()
{
    unsigned int offset;
    signed int i;
    unsigned int* buf;
    signed int enabled;
    unsigned int aram;
    ARQRequest request;

    AIInit(NULL);
    ARInit(aram_array, 40);
    ARQInit();
    AXInit();
    MIXInit();

    for (i = 0; i < 58; i++)
    {
        voices[i].voice = NULL;
        voices[i].flags = 0;
    }

    for (i = 0; i < 6; i++)
    {
        streams[i].vinf.voice = NULL;
        streams[i].vinf.flags = 0;
        streams[i].source_a = 0;
        streams[i].dest_a = 0;
        streams[i].dest_b = 0;
    }

    ua_stream_buffer = malloc(0x11A40);
    stream_buffer = (void*)(((unsigned int)ua_stream_buffer + 0x1F) & ~0x1F);
    buf = (unsigned int*)stream_buffer;
    for (i = 0; i < 15000; i++)
    {
        buf[i] = 0;
    }

    silence_buffer = ARAlloc(60000);
    zero_point = silence_buffer;
    zero_point *= 2;
    zero_point += 2;
    zero_end = zero_point + 120000;

    SoundFlags |= 1;
    enabled = OSDisableInterrupts();
    DCInvalidateRange((void*)silence_buffer, 60000);
    ARQPostRequest(&request, 0, 0, 1, (unsigned long)buf, silence_buffer, 60000, arq_callback);
    OSRestoreInterrupts(enabled);
    while (SoundFlags != 0)
    {
    }

    offset = 0;
    for (i = 0; i < 6; i++)
    {
        aram = ARAlloc(24000);
        streams[i].dest_a = aram;
        streams[i].dest_b = aram + 12000;
        streams[i].source_a = (unsigned int)stream_buffer + offset;
        offset += 12000;
    }
    AXRegisterCallback(fcb);
}

void iSndExit()
{
    AXQuit();
}

void iSndInitSceneLoaded()
{
}

#pragma auto_inline on
unsigned int iSndIsPlayingByID(unsigned int assetID)
{
    if (assetID == 0)
    {
        return 0;
    }

    for (signed int i = 0; i < 6; i++)
    {
        if (gSnd.voice[i].assetID == assetID)
        {
            if ((streams[i].vinf.flags & 0xC000) && !(streams[i].vinf.flags & 0x82))
            {
                return 1;
            }
            return 0;
        }
    }

    for (signed int i = 0; i < 58; i++)
    {
        if (gSnd.voice[i + 6].assetID == assetID)
        {
            if ((voices[i].flags & 0x4) && !(voices[i].flags & 0x8))
            {
                return 1;
            }
            return 0;
        }
    }
    return 0;
}

#pragma auto_inline reset

unsigned int iSndIsPlaying(unsigned int assetID, unsigned int parid)
{
    for (unsigned int i = 0; i < 64; i++)
    {
        if ((assetID == 0 || gSnd.voice[i].assetID == assetID) && gSnd.voice[i].parentID == parid)
        {
            if (iSndIsPlayingByID(gSnd.voice[i].assetID))
            {
                return 1;
            }
        }
    }
    return 0;
}

void* iSndLookup(unsigned int id)
{
    static signed int sHandle = 1;
    static signed int sSceneHandle = 0x1000;

    lookupFromScene = 0;
    if (id == 0)
    {
        return NULL;
    }

    for (signed int b = nbanks - 1; b >= 0; b--)
    {
        iSndBank* bank = &banks[b];
        iSndInfoAsset* info = bank->info;
        sDSPADPCM* entries = info->entries;
        unsigned int i;
        for (i = 0; i < info->nsounds; i++)
        {
            if (id == entries[i].aid)
            {
                memcpy(&lookup.info, &entries[i], sizeof(sDSPADPCM));
                memcpy(&lookup.file, &bank->file, sizeof(tag_xFile));
                lookup.handle = sSceneHandle++;
                if (sSceneHandle >= 0x7FFA)
                {
                    sSceneHandle = 0x1000;
                }
                return &lookup;
            }
        }
        for (; i < info->nstreams + info->nsounds; i++)
        {
            if (id == entries[i].aid)
            {
                memcpy(&lookup.info, &entries[i], sizeof(sDSPADPCM));
                memcpy(&lookup.file, &bank->file, sizeof(tag_xFile));
                if (b > 1)
                {
                    lookup.handle = sSceneHandle++;
                    if (sSceneHandle >= 0x7FFA)
                    {
                        sSceneHandle = 0x1000;
                    }
                    lookupFromScene = 1;
                }
                else
                {
                    lookup.handle = sHandle++;
                    if (sHandle >= 0xFFE)
                    {
                        sHandle = 1;
                    }
                }
                return &lookup;
            }
        }
    }
    return NULL;
}

void iSndPause(unsigned int snd, unsigned int pause)
{
    signed int i;
    for (i = 0; i < 64; i++)
    {
        if (gSnd.voice[i].sndID == snd)
        {
            break;
        }
    }

    if (i < 6)
    {
        signed int idx = i;
        if (streams[idx].vinf.voice != NULL)
        {
            if (pause)
            {
                AXSetVoiceState(streams[idx].vinf.voice, 0);
                streams[idx].vinf.flags |= 0x2;
            }
            else if (!fn_800443F8() && iSndDiskReady())
            {
                AXSetVoiceState(streams[idx].vinf.voice, 1);
                streams[idx].vinf.flags &= ~0x2;
            }
        }
    }
    else if (i < 64)
    {
        signed int idx = i - 6;
        if (voices[idx].voice != NULL)
        {
            if (pause)
            {
                AXSetVoiceState(voices[idx].voice, 0);
                voices[idx].flags |= 0x8;
            }
            else
            {
                AXSetVoiceState(voices[idx].voice, 1);
                voices[idx].flags &= ~0x8;
            }
        }
    }
}

void iSndStop(unsigned int snd)
{
    signed int enabled = OSDisableInterrupts();
    signed int i;
    for (i = 0; i < 64; i++)
    {
        if (gSnd.voice[i].sndID == snd)
        {
            gSnd.voice[i].sndID = 0;
            gSnd.voice[i].flags = 0;
            break;
        }
    }

    if (i < 6)
    {
        signed int idx = i;
        if (streams[idx].vinf.voice != NULL)
        {
            DVDCancelAsync(&streams[idx].fileInfo.cb, NULL);
            ARQRemoveRequest(&streams[idx].request);
            AXSetVoiceState(streams[idx].vinf.voice, 0);
            MIXReleaseChannel(streams[idx].vinf.voice);
            streams[idx].vinf.flags = 0x40000;
        }
    }
    else if (i < 64)
    {
        signed int idx = i - 6;
        if (voices[idx].voice != NULL)
        {
            MIXReleaseChannel(voices[idx].voice);
            AXSetVoiceState(voices[idx].voice, 0);
            AXFreeVoice(voices[idx].voice);
            voices[idx].voice = NULL;
            voices[idx].flags = 0;
            voices[idx].aid = 0;
        }
    }
    OSRestoreInterrupts(enabled);
}

void iSndCalcVol3d(xSndVoiceInfo* vp, vinfo* info)
{
    signed int pan;
    signed int v;
    _xVec3 dir;
    dir.x = vp->pos.x - gSnd.pos.x;
    dir.y = vp->pos.y - gSnd.pos.y;
    dir.z = vp->pos.z - gSnd.pos.z;
    float dist2 = dir.x * dir.x + dir.y * dir.y + dir.z * dir.z;
    xVec3Normalize(&dir, &dir);
    float dot = xVec3Dot(&dir, &gSnd.right);

    float radius2 = gSnd.radius2 < vp->radius2 ? gSnd.radius2 : vp->radius2;
    float att;
    if (dist2 <= radius2)
    {
        att = (radius2 - dist2) / radius2;
    }
    else
    {
        att = 0.0f;
    }
    att = xsqrt(att);

    float vol;
    if (vp->flags & 0x400)
    {
        vol = att * (gSnd.streamvol.left > gSnd.streamvol.right ? gSnd.streamvol.left
                                                                : gSnd.streamvol.right);
    }
    else
    {
        vol = att * (gSnd.fxvol.left > gSnd.fxvol.right ? gSnd.fxvol.left : gSnd.fxvol.right);
    }
    vol *= gSnd.mastervol.left > gSnd.mastervol.right ? gSnd.mastervol.left
                                                      : gSnd.mastervol.right;
    pan = (signed int)(64.0f * dot) + 0x40;
    v = 200.0f * vol - 200.0f;
    if (v <= -190)
    {
        v = -900;
    }
    if (pan < 0)
    {
        pan = 0;
    }
    else if (pan > 0x7F)
    {
        pan = 0x7F;
    }
    info->x10 = v;
    info->x14 = v;
    info->x18 = pan;
    info->x1c = pan;
    MIXAdjustFader(info->voice, v - MIXGetFader(info->voice));
    MIXAdjustPan(info->voice, pan - MIXGetPan(info->voice));
}

void iSndUpdateStreams(float dt)
{
    unsigned char ready = iSndDiskReady();
    unsigned int pause = fn_800443F8() | !ready;
    for (signed int i = 0; i < 6; i++)
    {
        iSndStream* s = &streams[i];
        if (s->vinf.voice == NULL)
        {
            continue;
        }
        unsigned int flags = s->vinf.flags;
        if (flags & 0x40000)
        {
            s->vinf.flags = 0x80000;
        }
        else if (flags & 0x80000)
        {
            s->vinf.flags = 0x100000;
        }
        else if (pause)
        {
            if (!(flags & 0x20000))
            {
                s->vinf.flags = flags | 0x20000;
                AXSetVoiceState(s->vinf.voice, 0);
            }
        }
        else if (flags & 0x20000)
        {
            s->vinf.flags = flags & ~0x20000;
            if (!(s->vinf.flags & 0x2))
            {
                AXSetVoiceState(s->vinf.voice, 1);
            }
        }
        else
        {
            xSndVoiceInfo* vp = &gSnd.voice[i];
            MIXUnMute(s->vinf.voice);
            iSndUpdateVoicePos(vp);
            iSndCalcVol(vp, &s->vinf);
        }
    }
}

void iSndUpdate(float dt)
{
    signed int i;
    vinfo* v;
    signed int enabled;
    xSndVoiceInfo* vp;
    enabled = OSDisableInterrupts();
    iSndUpdateStreams(dt);
    for (i = 0; i < 58; i++)
    {
        v = &voices[i];
        if (v->voice != NULL)
        {
            vp = &gSnd.voice[i + 6];
            MIXUnMute(v->voice);
            iSndUpdateVoicePos(vp);
            iSndCalcVol(vp, v);
        }
    }
    MIXUpdateSettings();
    OSRestoreInterrupts(enabled);
}

unsigned int iSndFindFreeVoice(unsigned int priority, unsigned int flags)
{
    if (priority > 0xFF)
    {
        priority = 0xFF;
    }

    if (flags & 0x4)
    {
        for (signed int i = 0; i < 6; i++)
        {
            if (streams[i].vinf.voice == NULL)
            {
                signed int enabled = OSDisableInterrupts();
                _AXVPB* voice = AXAcquireVoice(priority >> 3, dv_callback, i);
                if (voice != NULL)
                {
                    AXSetVoiceState(voice, 0);
                    MIXInitChannel(voice, 0xC, 0, -900, -900, 0x40, 0x7F, -900);
                    streams[i].vinf.voice = voice;
                    streams[i].vinf.flags = 0;
                    streams[i].vinf.flags |= 0x4;
                    OSRestoreInterrupts(enabled);
                    return i;
                }
                OSRestoreInterrupts(enabled);
                return -2;
            }
        }
    }
    else
    {
        for (signed int i = 0; i < 58; i++)
        {
            if (voices[i].voice == NULL)
            {
                voices[i].flags = 0;
                voices[i].flags |= 0x1;
                return i + 6;
            }
        }

        signed int i;
        unsigned int lowest = 0xFFFFFFFF;
        signed int found = 0;
        for (i = 6; i < 64; i++)
        {
            if (gSnd.voice[i].priority < lowest)
            {
                lowest = gSnd.voice[i].priority;
                found = i;
            }
        }

        if (lowest < (priority >> 3))
        {
            signed int idx = found - 6;
            signed int enabled = OSDisableInterrupts();
            MIXReleaseChannel(voices[idx].voice);
            AXSetVoiceState(voices[idx].voice, 0);
            AXFreeVoice(voices[idx].voice);
            voices[idx].voice = NULL;
            voices[idx].flags = 0;
            voices[idx].flags |= 0x1;
            voices[idx].aid = 0;
            OSRestoreInterrupts(enabled);
            return found;
        }
    }
    return -2;
}

unsigned int iSndPlayStream(xSndVoiceInfo* vp)
{
    signed int idx = vp - gSnd.voice;
    iSndStream* s = &streams[idx];
    if (s->vinf.voice != NULL && (s->vinf.flags & 1))
    {
        s->offset = s->info.sa;
        s->read = 0;
        DVDFileInfo* fi = &s->fileInfo;
        signed int enabled = OSDisableInterrupts();
        s->vinf.flags |= 0x400000;
        unsigned int size = 12000;
        if (s->size <= 12000)
        {
            size = s->size;
        }
        DVDReadAsyncPrio(fi, (void*)s->source_a, size, s->offset, dvdcb, 2);

        AXPBADDR addr;
        memcpy(&addr, &s->info.loop_flag, sizeof(AXPBADDR));
        PC_FIX_ADDR_HALVES(&addr);
        unsigned int start = s->dest_a * 2;
        addr.loopFlag = 1;
        addr.format = 0;
        unsigned int end = start + 0xBB7F;
        start += 2;
        addr.loopAddressHi = start >> 16;
        addr.loopAddressLo = start;
        addr.currentAddressHi = start >> 16;
        addr.currentAddressLo = start;
        addr.endAddressHi = end >> 16;
        addr.endAddressLo = end;
        AXSetVoiceAddr(s->vinf.voice, &addr);
        AXSetVoiceAdpcm(s->vinf.voice, s->info.coef);
        AXSetVoiceSrcType(s->vinf.voice, 1);
        AXSetVoiceType(s->vinf.voice, 1);

        float ratio = (float)s->info.sample_rate / 32000.0f;
        s->vinf.voice->pb.ratioHi = (signed int)ratio;
        s->vinf.voice->pb.ratioLo = (signed int)(65536.0f * ratio);
        s->vinf.voice->sync |= 0x80000000;
        streams[idx].vinf.x14 = 0x7FFFFFFF;
        streams[idx].vinf.x1c = 0x7FFFFFFF;
        MIXUnMute(s->vinf.voice);
        iSndUpdateVoicePos(vp);
        iSndCalcVol(vp, &s->vinf);
        OSRestoreInterrupts(enabled);
        return vp->sndID;
    }
    return 0;
}

unsigned int iSndPlaySound(xSndVoiceInfo* vp)
{
    if (vp->flags & 0x200)
    {
        for (signed int i = 0; i < 64; i++)
        {
            if (&gSnd.voice[i] != vp && gSnd.voice[i].assetID == vp->assetID &&
                gSnd.voice[i].parentID == vp->parentID && (gSnd.voice[i].flags & 1))
            {
                return 0;
            }
        }
    }

    if (lookup.info.aid != vp->assetID)
    {
        iSndLookup(vp->assetID);
        if (lookup.info.aid != vp->assetID)
        {
            return 0;
        }
    }

    if (lookupFromScene)
    {
        vp->flags |= 0x400;
        lookupFromScene = 0;
    }

    unsigned int priority = vp->priority;
    signed int idx = vp - gSnd.voice;
    if (priority >= 0xFF)
    {
        priority = 0xFF;
    }

    signed int i = idx - 6;
    if (voices[i].voice == NULL)
    {
        voices[i].aid = vp->assetID;
        if (voices[i].aid == 0x84838AEC)
        {
            vp->flags |= 0x400;
        }
        voices[i].time = 1000.0f * lookup.info.num_samples;
        voices[i].time = voices[i].time / lookup.info.sample_rate;

        AXPBADDR addr;
        memcpy(&addr, &lookup.info.loop_flag, sizeof(AXPBADDR));
        PC_FIX_ADDR_HALVES(&addr);
        if (lookup.info.loop_flag == 0)
        {
            addr.loopAddressLo = zero_point;
            addr.loopAddressHi = zero_point >> 16;
        }

        voices[i].voice = AXAcquireVoice(priority >> 3, dv_callback, idx);
        if (voices[i].voice == NULL)
        {
            return 0;
        }

        signed int enabled = OSDisableInterrupts();
        AXSetVoiceAddr(voices[i].voice, &addr);
        AXSetVoiceType(voices[i].voice, 0);
        AXSetVoiceAdpcm(voices[i].voice, lookup.info.coef);
        AXSetVoiceSrcType(voices[i].voice, 1);
        MIXInitChannel(voices[i].voice, 4, 0, -904, -904, 0x40, 0x7F, -904);
        MIXUnMute(voices[i].voice);
        iSndUpdateVoicePos(vp);
        iSndCalcVol(vp, &voices[i]);

        float ratio = (float)lookup.info.sample_rate / 32000.0f;
        voices[i].voice->pb.ratioHi = (signed int)ratio;
        voices[i].voice->pb.ratioLo = (signed int)(65536.0 * ratio);
        voices[i].voice->sync |= 0x80000000;
        MIXUnMute(voices[i].voice);
        MIXUpdateSettings();
        AXSetVoiceState(voices[i].voice, 1);
        OSRestoreInterrupts(enabled);
        voices[i].flags &= ~1;
        voices[i].flags |= 4;
        return vp->sndID;
    }
    return 0;
}

#pragma auto_inline on
unsigned int iSndPlay(xSndVoiceInfo* vp)
{
    iSndStream* s;
    signed int idx = vp - gSnd.voice;
    if (idx < 0 || idx >= 64)
    {
        return 0;
    }
    if (idx < 6)
    {
        s = &streams[idx];
        if (lookup.info.aid != vp->assetID &&
            (iSndLookup(vp->assetID), lookup.info.aid != vp->assetID))
        {
            idx = 64;
        }
        else
        {
            memcpy(&s->info, &lookup.info, sizeof(sDSPADPCM));
            memcpy(&s->fileInfo, &lookup.file.fileInfo, sizeof(DVDFileInfo));
            s->fileInfo.cb.userData = s;
            s->size = (((s->info.num_adpcm_nibbles + 1) >> 1) + 0x1F) & ~0x1F;
            s->vinf.flags |= 1;
            s->vinf.aid = lookup.info.aid;
            if (s->info.loop_flag != 0 || (vp->flags & 0x100))
            {
                s->vinf.flags |= 0x10000;
            }
        }
        if ((unsigned int)idx < 58)
        {
            return iSndPlayStream(vp);
        }
        return idx;
    }
    return iSndPlaySound(vp);
}

#pragma auto_inline reset

extern "C" unsigned int fn_800D8C14()
{
    return (unsigned char)(OSGetSoundMode() == 1);
}

void iSndStereo(unsigned int stereo)
{
    if (stereo == 0)
    {
        OSSetSoundMode(0);
        gSnd.stereo = 0;
    }
    else
    {
        OSSetSoundMode(1);
        gSnd.stereo = 1;
    }
}

void iSndWaitForDeadSounds()
{
    fc = 0;
    while (fc < 140)
    {
        signed int t = fc + 14;
        while (fc < t)
        {
        }
        iSndUpdate(0.07f);
    }
}

void iSndSuspendCD(unsigned int suspend)
{
}

void iSndSceneExit()
{
    unsigned long length;
    signed int done;
    signed int enabled;
    signed int i;
    done = 0;
    enabled = OSDisableInterrupts();
    ARQFlushQueue();
    DVDCancelAll();
    for (i = 0; i < 64; i++)
    {
        if (gSnd.voice[i].sndID != 0)
        {
            iSndStop(gSnd.voice[i].sndID);
        }
    }
    for (i = 0; i < 64; i++)
    {
        if (gSnd.voice[i].sndID != 0)
        {
            iSndStop(gSnd.voice[i].sndID);
        }
    }
    OSRestoreInterrupts(enabled);

    fc = 0;
    while (!done && fc < 400)
    {
        done = 1;
        signed int t = fc + 14;
        while (fc < t)
        {
        }
        iSndUpdate(0.07f);
        for (i = 0; i < 6; i++)
        {
            if (streams[i].vinf.voice != NULL)
            {
                done = 0;
            }
        }
        for (i = 0; i < 58; i++)
        {
            if (voices[i].voice != NULL)
            {
                done = 0;
            }
        }
    }

    ARFree(&length);
    nbanks--;
    free(banks[nbanks].info);
}

static void sndloadcb(signed int)
{
    SoundFlags = 0;
}

void iSndLoadSounds(void* data)
{
    iSndInfoAsset* info;
    sDSPADPCM* entries;
    unsigned int lo;
    char* name;
    unsigned int i;
    unsigned int hi;
    tag_xFile* file;
    unsigned int total;
    info = (iSndInfoAsset*)data;
    total = 0;
    lo = 0xFFFFFFFF;
    hi = 0;

    if (info->nsounds == 0 && info->nstreams == 0)
    {
        return;
    }

    if (nbanks >= 12)
    {
        exit(-1);
    }

    entries = info->entries;
    banks[nbanks].info = info;
    name = xST_xAssetID_HIPFullPath(info->entries[0].aid);
    file = &banks[nbanks].file;
    iFileOpen(name, 0, file);
    iFileSeek(file, 0, 0);

    for (i = 0; i < info->nsounds; i++)
    {
        st_xST_XASSETINFO ainfo;
        sDSPADPCM* e = &entries[i];
        xSTGetAssetInfo(e->aid, &ainfo);
        unsigned int start = ainfo.plus_offset + ((ainfo.sector - file->fileInfo.startAddr) << 5);
        unsigned int end = ainfo.size + start;
        if (lo > start)
        {
            lo = start;
        }
        if (hi < end)
        {
            hi = end;
        }
        total += ainfo.size;
        e->sa += start * 2;
        e->ea += start * 2;
        e->ca += start * 2;
    }

    for (i = 0; i < info->nstreams; i++)
    {
        st_xST_XASSETINFO ainfo;
        xSTGetAssetInfo(entries[i + info->nsounds].aid, &ainfo);
        if (nbanks < 2)
        {
            entries[i + info->nsounds].sa =
                ainfo.plus_offset + ((ainfo.sector - file->fileInfo.startAddr) << 5);
        }
        else
        {
            unsigned int start = ainfo.plus_offset + ((ainfo.sector - file->fileInfo.startAddr) << 5);
            unsigned int end = ainfo.size + start;
            if (lo > start)
            {
                lo = start;
            }
            if (hi < end)
            {
                hi = end;
            }
            total += ainfo.size;
            entries[i + info->nsounds].sa += start * 2;
            entries[i + info->nsounds].ea += start * 2;
            entries[i + info->nsounds].ca += start * 2;
        }
    }

    if (info->nsounds != 0 || (nbanks >= 2 && info->nstreams != 0))
    {
        info->aramsize = total;
        unsigned int aram = ARAlloc(total);
        unsigned int delta = (aram - lo) * 2;
        for (i = 0; i < info->nsounds; i++)
        {
            entries[i].sa += delta;
            entries[i].ea += delta;
            entries[i].ca += delta;
        }
        if (nbanks >= 2)
        {
            for (i = 0; i < info->nstreams; i++)
            {
                entries[i + info->nsounds].sa += delta;
                entries[i + info->nsounds].ea += delta;
                entries[i + info->nsounds].ca += delta;
            }
        }

        banks[nbanks].aramBase = lo;
        for (unsigned int done = 0; done < total;)
        {
            unsigned int size = 72000;
            if (done + 72000 >= total)
            {
                size = total - done;
            }
            SoundFlags |= 1;
            iFileSeek(file, lo + done, 0);
            iFileReadAsync(file, stream_buffer, size, sndloadcb);
            while (SoundFlags != 0)
            {
                iTRCDisk::CheckDVDAndResetState();
                fn_800443AC();
                if (!(fc & 0xF))
                {
                    zGameScreenTransitionUpdate(0.5f, NULL, NULL);
                }
            }
            signed int enabled = OSDisableInterrupts();
            DCStoreRange(stream_buffer, size);
            ARQRequest request;
            ARQPostRequest(&request, 0, 0, 1, (unsigned long)stream_buffer, aram + done, size,
                           arq_callback);
            SoundFlags |= 1;
            OSRestoreInterrupts(enabled);
            while (SoundFlags != 0)
            {
            }
            done += size;
        }
        while (SoundFlags != 0)
        {
        }
    }
    nbanks++;
}

void iSndDIEDIEDIE()
{
    signed int i;
    signed int enabled = OSDisableInterrupts();
    for (i = 0; i < 6; i++)
    {
        if (streams[i].vinf.voice != NULL)
        {
            MIXReleaseChannel(streams[i].vinf.voice);
            AXSetVoiceState(streams[i].vinf.voice, 0);
            AXFreeVoice(streams[i].vinf.voice);
            streams[i].vinf.voice = NULL;
        }
    }
    for (i = 0; i < 58; i++)
    {
        if (voices[i].voice != NULL)
        {
            MIXReleaseChannel(voices[i].voice);
            AXSetVoiceState(voices[i].voice, 0);
            AXFreeVoice(voices[i].voice);
            voices[i].voice = NULL;
        }
    }
    AXQuit();
    OSRestoreInterrupts(enabled);
}
