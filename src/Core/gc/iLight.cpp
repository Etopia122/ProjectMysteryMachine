#include "rwtypes.h"
#include "xMath3.h"


class _xFColor
{
public:
    float r;
    float g;
    float b;
    float a;
};

class iLight
{
public:
    unsigned int type;
    RpLight* hw;
    _xSphere sph;
    float radius_sq;
    _xFColor color;
    _xVec3 dir;
    float coneangle;
};

extern "C" {
signed int RwFrameDestroy(RwFrame* frame);
signed int RpLightDestroy(RpLight* light);
RwMatrixTag* RwMatrixUpdate(RwMatrixTag* matrix);
RwFrame* RwFrameUpdateObjects(RwFrame* frame);
RwFrame* RwFrameTransform(RwFrame* frame, RwMatrixTag* m, signed int combine);
RpLight* RpLightSetColor(RpLight* light, RwRGBAReal* color);
RpLight* RpLightSetRadius(RpLight* light, float radius);
RpLight* RpLightSetConeAngle(RpLight* light, float angle);
RpLight* RpLightCreate(signed int type);
RwFrame* RwFrameCreate();
RpWorld* RpWorldAddLight(RpWorld* world, RpLight* light);
RpWorld* RpWorldRemoveLight(RpWorld* world, RpLight* light);
void* _rwObjectHasFrameSetFrame(void* object, RwFrame* frame);
}

extern const _xVec3 g_O3;

RpWorld* gLightWorld;

// Functions are in reverse DOL order (-inline deferred emits last to first).

void iLightInit(RpWorld* world)
{
    gLightWorld = world;
}

iLight* iLightCreate(iLight* light, unsigned int type)
{
    RwFrame* frame;

    switch (type)
    {
    case 1:
        light->hw = RpLightCreate(0x80);
        break;
    case 2:
        light->hw = RpLightCreate(0x81);
        break;
    case 3:
        light->hw = RpLightCreate(0x82);
        break;
    }

    frame = RwFrameCreate();
    light->hw->object.flags = 1;
    _rwObjectHasFrameSetFrame(light->hw, frame);
    RwFrameUpdateObjects(frame);

    light->type = type;
    light->sph.center.x = 0.0f;
    light->sph.center.y = 0.0f;
    light->sph.center.z = 0.0f;
    light->sph.r = 0.0f;
    light->color.r = 1.0f;
    light->color.g = 1.0f;
    light->color.b = 1.0f;
    light->color.a = 1.0f;
    light->dir.x = 0.0f;
    light->dir.y = 0.0f;
    light->dir.z = 1.0f;
    light->coneangle = 0.0f;
    return light;
}

void iLightModify(iLight* light, unsigned int flags)
{
    if (flags & 0x5)
    {
        RwFrame* frame = (RwFrame*)light->hw->object.parent;
        RwMatrixTag temp;

        *(_xVec3*)&temp.right = g_O3;
        *(_xVec3*)&temp.up = g_O3;
        *(_xVec3*)&temp.at = light->dir;
        *(_xVec3*)&temp.pos = light->sph.center;
        RwFrameTransform(frame, &temp, 0);
    }
    if (flags & 0x2)
    {
        RpLightSetRadius(light->hw, light->sph.r);
    }
    if (flags & 0x8)
    {
        RpLightSetColor(light->hw, (RwRGBAReal*)&light->color);
    }
    if (flags & 0x10)
    {
        if (light->type == 2 || light->type == 3)
        {
            RpLightSetConeAngle(light->hw, light->coneangle);
        }
    }
}

void iLightSetColor(iLight* light, _xFColor* col)
{
    RpLightSetColor(light->hw, (RwRGBAReal*)col);
}

void iLightSetPos(iLight* light, _xVec3* pos)
{
    RwFrame* f = (RwFrame*)light->hw->object.parent;
    RwMatrixTag* m = &f->modelling;

    m->pos.x = pos->x;
    m->pos.y = pos->y;
    m->pos.z = pos->z;
    RwMatrixUpdate(m);
    RwFrameUpdateObjects(f);
}

void iLightDestroy(iLight* light)
{
    RwFrame* frame;

    light->type = 0;
    frame = (RwFrame*)light->hw->object.parent;
    if (frame)
    {
        RwFrameDestroy(frame);
    }
    RpLightDestroy(light->hw);
}

void iLightAdd(iLight* light)
{
    RpWorldAddLight(gLightWorld, light->hw);
}

void iLightRemove(iLight* light)
{
    RpWorldRemoveLight(gLightWorld, light->hw);
}

void iLightEnv(iLight* light, signed int env)
{
    unsigned char flags = 0;

    switch (env)
    {
    case 0:
        flags = 0;
        break;
    case 1:
        flags = 2;
        break;
    case 2:
        flags = 1;
        break;
    case 3:
        flags = 3;
        break;
    }
    light->hw->object.flags = flags;
}
