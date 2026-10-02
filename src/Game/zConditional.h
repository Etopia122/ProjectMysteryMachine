#ifndef ZCONDITIONAL_H
#define ZCONDITIONAL_H

#include "xBase.h"

class xSerial;

class zCondAsset : public p2BaseAsset
{
public:
    unsigned int constNum;
    unsigned int expr1;
    unsigned int op;
};

class _zConditional : public xBase
{
public:
    zCondAsset* asset;
};

void zConditionalInit(void* b, void* asset);
void zConditionalSave(_zConditional* ent, xSerial* s);
void zConditionalLoad(_zConditional* ent, xSerial* s);
unsigned int zConditional_Evaluate(_zConditional* c);
signed int zConditionalEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                               xBase* toParamWidget);

#endif
