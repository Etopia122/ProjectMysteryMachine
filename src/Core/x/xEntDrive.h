#ifndef XENTDRIVE_H
#define XENTDRIVE_H

#include "xEnt.h"

class _xEntDrive
{
public:
    unsigned int flags; // 0x0
    float otm;          // 0x4
    float otmr;         // 0x8
    float os;           // 0xC
    float tm;           // 0x10
    float tmr;          // 0x14
    float s;            // 0x18
    _xEnt* odriver;     // 0x1C
    _xEnt* driver;      // 0x20
    _xEnt* driven;      // 0x24
    _xVec3 op;          // 0x28
    _xVec3 p;           // 0x34
    _xVec3 q;           // 0x40
    float yaw;          // 0x4C
};

void xEntDriveInit(_xEntDrive* drv, _xEnt* driven);
void xEntDriveMount(_xEntDrive* drv, _xEnt* driver, float mt);
void xEntDriveDismount(_xEntDrive* drv, float dmt);
void xEntDriveUpdate(_xEntDrive* drv, _xScene* s, float dt);

#endif
