#ifndef ZSCENE_H
#define ZSCENE_H

#include "xScene.h"

class _zPortal;
class _zEnt;

class zScene : public _xScene
{
public:
    unsigned int sceneID;     // 0x6C
    _zPortal* pendingPortal;  // 0x70
    union
    {
        unsigned int num_ents;
        unsigned int num_base;
    };                        // 0x74
    union
    {
        xBase** base;
        _zEnt** ents;
    };                        // 0x78
    unsigned int baseCount[43]; // 0x7C
    xBase* baseList[43];        // 0x128
    unsigned int num_bubbles;   // 0x1D4
    _zEnt** bubbles;            // 0x1D8
};

xBase* zSceneFindObject(unsigned int gameID);

#endif
