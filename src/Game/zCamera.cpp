#include "xCamera.h"
#include "xserializer.h"
#include "zGlobals.h"

signed int zCameraEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                          xBase* toParamWidget);

// Functions are in reverse DOL order (-inline deferred emits last to first).

void zCameraInit(xBase* b, xCamAsset* asset)
{
    xBaseInit(b, asset);
    if (b->linkCount)
    {
        b->link = (p2LinkAsset*)(asset + 1);
    }
    b->eventFunc = zCameraEventCB;
}

void zCameraSave(_tagxCam* ent, xSerial* s)
{
    xBaseSave(ent, s);
}

void zCameraLoad(_tagxCam* ent, xSerial* s)
{
    xBaseLoad(ent, s);
}

signed int zCameraEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                          xBase* toParamWidget)
{
    _tagxCam* cam = (_tagxCam*)to;

    switch (toEvent)
    {
    case 0xA:
        xBaseReset(cam, cam->asset);
        break;
    case 0x26:
        xCameraNext(&globals.camera, cam);
        break;
    }
    return 1;
}
