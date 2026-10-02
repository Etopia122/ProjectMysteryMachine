#ifndef ZCOUNTER_H
#define ZCOUNTER_H

#include "xBase.h"

class xSerial;

class zCounterAsset : public p2BaseAsset
{
public:
    signed short count;
};

class _zCounter : public xBase
{
public:
    zCounterAsset* asset;
    signed short count;
    unsigned char state;
    unsigned char pad;
};

void zCounterInit(void* b, void* asset);
void zCounterReset(xBase* b);
void zCounterSave(_zCounter* ent, xSerial* s);
void zCounterLoad(_zCounter* ent, xSerial* s);
signed int zCounterEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                           xBase* toParamWidget);

#endif
