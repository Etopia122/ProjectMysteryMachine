#include "xBase.h"
#include "iFX.h"
#include "xString.h"
#include "zScene.h"

class RpWorld;

void xFX_SceneEnter(RpWorld* world);
void xFX_SceneExit(RpWorld* world);

// Not yet named: an xFX.cpp setter.
extern "C" void fn_8002A5F4(signed int);

// Functions are in reverse DOL order (-inline deferred emits last to first).

void zFX_SceneEnter(RpWorld* world)
{
    fn_8002A5F4(0);
    xFX_SceneEnter(world);

    signed int mode = 1;
    xBase* obj = zSceneFindObject(xStrHash("tide"));
    if (obj)
    {
        mode = 1;
    }
    else
    {
        obj = zSceneFindObject(xStrHash("tree1"));
        if (obj)
        {
            mode = 5;
        }
    }

    if (obj)
    {
        iFXWarbInit(obj, mode);
    }
}

void zFX_SceneExit(RpWorld* world)
{
    xFX_SceneExit(world);
    iFXWarbSetParams(0, 0);
}
