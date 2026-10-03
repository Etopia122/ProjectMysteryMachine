#include "xSnd.h"
#include "xEnt.h"
#include "zGlobals.h"
#include "zScene.h"

#include "iSnd.h"

extern "C" {
// TODO: unidentified iSnd function (1 when the console is in stereo mode)
unsigned int fn_800D8C14();
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
    iSndSuspendCD(1);
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

    voice = iSndFindFreeVoice(priority, vflags);
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
    if (!iSndPlay(vp))
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

    voice = iSndFindFreeVoice(priority, vflags);
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
    iSndPlay(vp);
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
