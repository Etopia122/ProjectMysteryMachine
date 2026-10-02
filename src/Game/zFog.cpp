#include "xBase.h"
#include "iCamera.h"

class _xScene;

class xSerial;

class zFogAsset : public p2BaseAsset
{
public:
    unsigned char bkgndColor[4];
    unsigned char fogColor[4];
    float fogDensity;
    float fogStart;
    float fogStop;
    float transitionTime;
    unsigned char fogType;
    unsigned char padFog[3];
};

class _zFog : public xBase
{
public:
    zFogAsset* tasset;
};

signed int zFogEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                       xBase* toParamWidget);

// Functions are in reverse DOL order (-inline deferred emits last to first).

void zFogInit(void* b, void* tasset)
{
    _zFog* fog = (_zFog*)b;
    zFogAsset* asset = (zFogAsset*)tasset;

    xBaseInit(fog, asset);
    fog->eventFunc = zFogEventCB;
    fog->tasset = asset;
    if (fog->linkCount)
    {
        fog->link = (p2LinkAsset*)(fog->tasset + 1);
    }
    else
    {
        fog->link = 0;
    }
}

void zFogSave(_zFog* ent, xSerial* s)
{
    xBaseSave(ent, s);
}

void zFogLoad(_zFog* ent, xSerial* s)
{
    xBaseLoad(ent, s);
}

signed int zFogEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                       xBase* toParamWidget)
{
    _zFog* fog = (_zFog*)to;
    iFogParams fp;

    switch (toEvent)
    {
    case 0x26:
        fp.type = rwFOGTYPELINEAR;
        fp.start = fog->tasset->fogStart;
        fp.stop = fog->tasset->fogStop;
        fp.density = fog->tasset->fogDensity;
        fp.fogcolor.red = fog->tasset->fogColor[0];
        fp.fogcolor.green = fog->tasset->fogColor[1];
        fp.fogcolor.blue = fog->tasset->fogColor[2];
        fp.fogcolor.alpha = fog->tasset->fogColor[3];
        fp.bgcolor.red = fog->tasset->bkgndColor[0];
        fp.bgcolor.green = fog->tasset->bkgndColor[1];
        fp.bgcolor.blue = fog->tasset->bkgndColor[2];
        fp.bgcolor.alpha = fog->tasset->bkgndColor[3];
        fp.table = 0;
        iCameraSetFogParams(&fp, fog->tasset->transitionTime);
        break;
    case 0x27:
        iCameraSetFogParams(0, 0.0f);
        break;
    case 0xA:
        xBaseReset(fog, fog->tasset);
        break;
    }
    return 1;
}

void zFogUpdate(xBase* to, _xScene* sc, float dt)
{
}
