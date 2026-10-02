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
};

extern zGlobals globals;

#endif
