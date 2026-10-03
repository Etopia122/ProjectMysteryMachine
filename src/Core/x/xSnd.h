#ifndef XSND_H
#define XSND_H

#include "xMath3.h"

class _xEnt;

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

extern xSndGlobals gSnd;

unsigned int xSndPlay(unsigned int id, float volL, float volR, float pitch, unsigned int priority,
                      unsigned int flags, unsigned int parentID);
unsigned int xSndPlay3D(unsigned int id, float vol, float pitch, unsigned int priority,
                        unsigned int flags, _xEnt* parent, float radius);
unsigned int xSndPlay3D(unsigned int id, float vol, float pitch, unsigned int priority,
                        unsigned int flags, _xVec3* pos, float radius);
void xSndPause(unsigned int snd, unsigned int pause);
void xSndStop(unsigned int snd);
void xSndParentDied(unsigned int pid);
void xSndStopChildren(unsigned int pid);

// GameCube: looks a sound up by asset ID (named after BFBB's iSndLookup)
void* iSndLookup(unsigned int id);

#endif
