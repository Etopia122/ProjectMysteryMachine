#ifndef ZGLOBALS_H
#define ZGLOBALS_H

#include "xCamera.h"

// Placeholder: the GameCube layout differs from PS2; only members used so far
// are declared, at their GameCube offsets.
class zGlobalsUnk1F0
{
public:
    unsigned char pad[0x2C];
    unsigned int flags; // 0x2C; bit 0x40000 resumes waiting scripts
};

class zGlobals
{
public:
    union
    {
        _tagxCamFrame camera; // 0x0
        unsigned char pad[0x1F0];
    };
    zGlobalsUnk1F0* unk1F0; // 0x1F0
    unsigned char pad1F4[0x2E8 - 0x1F4];
    void* unk2E8; // 0x2E8; portals only switch scenes when set
};

extern zGlobals globals;

#endif
