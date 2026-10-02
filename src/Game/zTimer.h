#ifndef ZTIMER_H
#define ZTIMER_H

#include "xBase.h"

class xSerial;

class zTimerAsset : public p2BaseAsset
{
public:
    float seconds;
};

class _zTimer : public xBase
{
public:
    zTimerAsset* tasset;
    unsigned char state;
    unsigned char pad[3];
    float secondsLeft;
};

void zTimerInit(void* b, void* tasset);
void zTimerSave(_zTimer* ent, xSerial* s);
void zTimerLoad(_zTimer* ent, xSerial* s);
signed int zTimerEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                         xBase* toParamWidget);
void zTimerUpdate(xBase* to, float dt);

#endif
