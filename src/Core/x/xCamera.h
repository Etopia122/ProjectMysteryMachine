#ifndef XCAMERA_H
#define XCAMERA_H

#include "xBase.h"

// Placeholder layouts: only the members used so far are meaningful.
class _tagxCamFrame
{
public:
    unsigned char data[0x1F0]; // GameCube size unknown (0x220 on PS2)
};

class xCamAsset : public p2BaseAsset
{
public:
    unsigned char data[0x88 - 0x8];
};

class _tagxCam : public xBase
{
public:
    unsigned char data[0x9C - 0x10];
    xCamAsset* asset; // 0x9C on GameCube (0xA0 on PS2)
};

void xCameraNext(_tagxCamFrame* cam, _tagxCam* trans_cam);

#endif
