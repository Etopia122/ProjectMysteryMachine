#ifndef XCOLLIDE_H
#define XCOLLIDE_H

#include "xMath3.h"

class _iCollis
{
public:
    unsigned int data;
};

class _xCollis
{
public:
    unsigned int flags; // 0x0
    unsigned int oid;   // 0x4
    void* optr;         // 0x8
    float dist;         // 0xC
    _xVec3 norm;        // 0x10
    _xVec3 tohit;       // 0x1C
    _xVec3 depen;       // 0x28
    _xVec3 hdng;        // 0x34
    _iCollis ixtra;     // 0x40
};

unsigned int xSphereHitsOBB(const _xSphere* a, const _xBox* b, const _xMat4x3* m, _xCollis* coll);
unsigned int xSphereHitsBox(const _xSphere* a, const _xBox* b, _xCollis* coll);
unsigned int xSphereHitsSphere(const _xSphere* a, const _xSphere* b, _xCollis* coll);

#endif
