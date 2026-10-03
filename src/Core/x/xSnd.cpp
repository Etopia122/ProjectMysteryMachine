#include "xSnd.h"
#include "xEnt.h"
#include "zGlobals.h"
#include "zScene.h"

class xSndVol
{
public:
    float left;  // 0x0
    float right; // 0x4
};

class iSndFileInfo
{
public:
    unsigned char pad0[0x8];
    unsigned int sample_rate; // 0x8
    unsigned char padC[0x64 - 0xC];
    signed int length; // 0x64
};

class xSndVoiceInfo
{
public:
    unsigned int assetID;     // 0x0
    unsigned int sndID;       // 0x4
    unsigned int parentID;    // 0x8
    _xVec3* parentPos;        // 0xC
    signed int internalID;    // 0x10
    unsigned short flags;     // 0x14
    unsigned short priority;  // 0x16
    float vol;                // 0x18
    float pitch;              // 0x1C
    unsigned int sample_rate; // 0x20
    unsigned int deadct;      // 0x24
    _xVec3 pos;               // 0x28
    float radius2;            // 0x34
    unsigned char ps[0x10];   // 0x38
};

class xSndGlobals
{
public:
    unsigned int stereo;       // 0x0
    unsigned int SndCount;     // 0x4
    xSndVol mastervol;         // 0x8
    xSndVol streamvol;         // 0x10
    xSndVol fxvol;             // 0x18
    xSndVoiceInfo voice[64];   // 0x20
    _xVec3 right;              // 0x1220
    _xVec3 up;                 // 0x122C
    _xVec3 at;                 // 0x1238
    _xVec3 pos;                // 0x1244
    float radius2;             // 0x1250
    _xVec3 nearpos;            // 0x1254
    _xVec3 farpos;             // 0x1260
    unsigned int suspendCD;    // 0x126C
};

void iSndInit();
void iSndExit();
void iSndUpdate(float dt);
void iSndStop(unsigned int snd);
void iSndPause(unsigned int snd, unsigned int pause);

extern "C" {
// TODO: unidentified iSnd functions
unsigned int fn_800D8C14();
void fn_800D8B64(unsigned int suspend);
unsigned int fn_800D955C(unsigned int priority, unsigned int flags);
unsigned int fn_800D8C40(xSndVoiceInfo* vp);
}

#define SCENE_ID(s) ((((s)[0] & ~0x20) << 24) | ((s)[1] << 16) | ((s)[2] << 8) | (s)[3])

xSndGlobals gSnd;

// Functions are in reverse DOL order (-inline deferred emits last to first).

void xSndInit()
{
    iSndInit();

    xSndVoiceInfo* voice = gSnd.voice;
    for (signed int i = 0; i < 64; i++, voice++)
    {
        voice->flags = 0;
    }

    gSnd.mastervol.left = 1.0f;
    gSnd.mastervol.right = 1.0f;
    gSnd.streamvol.left = 1.0f;
    gSnd.streamvol.right = 1.0f;
    gSnd.fxvol.left = 1.0f;
    gSnd.fxvol.right = 1.0f;
    gSnd.stereo = fn_800D8C14();

    _xVec3 zero = { 0.0f, 0.0f, 0.0f };
    gSnd.nearpos = zero;
    gSnd.farpos = zero;
    fn_800D8B64(1);
}

void xSndUpdate(float time_elapsed);
void xSndPauseAll(unsigned int pause_effects, unsigned int pause_streams);

void xSndSuspend()
{
    xSndPauseAll(1, 1);
    xSndUpdate(0.0f);
}

void xSndResume()
{
    xSndPauseAll(0, 0);
}

void xSndSetMasterVol(float left, float right)
{
    gSnd.mastervol.left = left;
    gSnd.mastervol.right = right;
}

void xSndSetStreamVol(float left, float right)
{
    gSnd.streamvol.left = left;
    gSnd.streamvol.right = right;
}

void xSndSetFXVol(float left, float right)
{
    gSnd.fxvol.left = left;
    gSnd.fxvol.right = right;
}

#pragma auto_inline on
void xSndPauseAll(unsigned int pause_effects, unsigned int pause_streams)
{
    for (unsigned int i = 0; i < 64; i++)
    {
        if (gSnd.voice[i].flags & 1)
        {
            if (gSnd.voice[i].flags & 2)
            {
                iSndPause(gSnd.voice[i].sndID, pause_effects);
            }
            else if (gSnd.voice[i].flags & 4)
            {
                iSndPause(gSnd.voice[i].sndID, pause_streams);
            }
        }
    }
}
#pragma auto_inline off

void xSndStopAll()
{
    for (unsigned int i = 0; i < 64; i++)
    {
        if (gSnd.voice[i].flags & 1)
        {
            iSndStop(gSnd.voice[i].sndID);
        }
    }
}

#pragma auto_inline on
void xSndUpdate(float time_elapsed)
{
    _xVec3 d;
    float len2;

    xVec3Sub(&d, &gSnd.farpos, &gSnd.nearpos);
    len2 = xVec3Dot(&d, &d);
    if (len2 < 20.0f)
    {
        xVec3Copy(&gSnd.pos, &gSnd.nearpos);
    }
    else if (len2 > 100.0f)
    {
        xVec3Copy(&gSnd.pos, &gSnd.farpos);
    }
    else
    {
        float t = (len2 - 20.0f) / 80.0f;
        gSnd.pos.x = d.x * t + gSnd.nearpos.x;
        gSnd.pos.y = d.y * t + gSnd.nearpos.y;
        gSnd.pos.z = d.z * t + gSnd.nearpos.z;
    }
    iSndUpdate(time_elapsed);
}
#pragma auto_inline off

void xSndExit()
{
    iSndExit();
}

// TODO (98.5%, and 99.4% for xSndPlay3D): the original keeps the masked flags in
// the parameter's saved register; here MWCC splits it into a volatile one.
unsigned int xSndPlay(unsigned int id, float volL, float volR, float pitch, unsigned int priority,
                      unsigned int flags, unsigned int parentID)
{
    unsigned int sceneID = globals.sceneCur->sceneID;
    iSndFileInfo* ip;
    xSndVoiceInfo* vp;
    unsigned int voice;
    unsigned int vflags;

    if (id == 0x91D67287 && sceneID == SCENE_ID("s005") && iSndLookup(0xFE619D7B))
    {
        id = 0xFE619D7B;
    }

    ip = (iSndFileInfo*)iSndLookup(id);
    if (!ip)
    {
        return -1;
    }

    flags &= 0x700;
    vflags = flags | 4;
    if (ip->length >= 0x1000)
    {
        vflags = flags | 2;
    }

    voice = fn_800D955C(priority, vflags);
    if (voice == 0xFFFFFFFE)
    {
        return 0xFFFFFFFE;
    }

    gSnd.voice[voice].priority = priority;
    vp = &gSnd.voice[voice];
    gSnd.voice[voice].pitch = pitch;
    vp->vol = volL > volR ? volL : volR;
    vp->sample_rate = ip->sample_rate;
    vp->assetID = id;
    vp->internalID = ip->length;
    vp->parentID = parentID;
    vp->parentPos = 0;
    vp->sndID = ++gSnd.SndCount;
    vp->flags = vflags;
    vp->flags |= 1;
    vp->deadct = 0;
    if (!fn_800D8C40(vp))
    {
        vp->flags &= ~1;
        return 0;
    }
    return vp->sndID;
}

unsigned int xSndPlay3D(unsigned int id, float vol, float pitch, unsigned int priority,
                        unsigned int flags, _xEnt* parent, float radius)
{
    iSndFileInfo* ip;
    xSndVoiceInfo* vp;
    unsigned int voice;
    unsigned int vflags;

    if (!parent)
    {
        return 0xFFFFFFFE;
    }

    ip = (iSndFileInfo*)iSndLookup(id);
    if (!ip)
    {
        return -1;
    }

    flags &= 0x700;
    vflags = flags | 4;
    if (ip->length >= 0x1000)
    {
        vflags = flags | 2;
    }

    voice = fn_800D955C(priority, vflags);
    if (voice == 0xFFFFFFFE)
    {
        return 0xFFFFFFFE;
    }

    vp = &gSnd.voice[voice];
    vp->priority = priority;
    vp->pitch = pitch;
    vp->vol = vol;
    vp->sample_rate = ip->sample_rate;
    vp->assetID = id;
    vp->internalID = ip->length;
    vp->sndID = ++gSnd.SndCount;
    vp->flags = vflags;
    vp->flags |= 1;
    vp->deadct = 0;
    vp->parentID = (unsigned int)parent;
    vp->parentPos = 0;
    xVec3Copy(&vp->pos,
              &((_xMat4x3*)((_xEnt*)((unsigned int)parent & ~3))->model->Mat)->pos);
    vp->radius2 = radius <= 0.0f ? 1000000.0f : radius * radius;
    vp->flags |= 0x18;
    fn_800D8C40(vp);
    return vp->sndID;
}

unsigned int xSndPlay3D(unsigned int id, float vol, float pitch, unsigned int priority,
                        unsigned int flags, _xVec3* pos, float radius)
{
    return -1;
}

void xSndSetFrame(_xVec3* right, _xVec3* up, _xVec3* at, _xVec3* pos, float radius)
{
    xVec3Copy(&gSnd.right, right);
    xVec3Copy(&gSnd.up, up);
    xVec3Copy(&gSnd.at, at);
    xVec3Copy(&gSnd.nearpos, pos);
    gSnd.radius2 = radius <= 0.0f ? 1000000.0f : radius * radius;
}

void xSndSetFrameFar(_xVec3* pos)
{
    xVec3Copy(&gSnd.farpos, pos);
}

void xSndPause(unsigned int snd, unsigned int pause)
{
    iSndPause(snd, pause);
}

void xSndStop(unsigned int snd)
{
    iSndStop(snd);
}

void xSndParentDied(unsigned int pid)
{
    xSndVoiceInfo* vp = gSnd.voice;
    for (signed int i = 0; i < 64; i++, vp++)
    {
        if (vp->parentID == pid)
        {
            vp->flags &= 0xFFEF;
        }
    }
}

void xSndStopChildren(unsigned int pid)
{
    unsigned int i;
    xSndVoiceInfo* vp;
    for (i = 0, vp = gSnd.voice; i < 64; i++, vp++)
    {
        if ((vp->flags & 1) && vp->parentID == pid)
        {
            iSndStop(vp->sndID);
            vp->flags &= 0xFFEF;
        }
    }
}
