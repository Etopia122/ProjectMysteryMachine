#ifndef IMODEL_H
#define IMODEL_H

#include "xMath3.h"

class RpAtomic;
class RwMatrixTag;

class iModelTag
{
public:
    _xVec3 v;            // 0x0
    unsigned int matidx; // 0xC
    float wt[4];         // 0x10
};

void iModelTagEval(RpAtomic* model, iModelTag* tag, RwMatrixTag* mat, _xVec3* dest);

#endif
