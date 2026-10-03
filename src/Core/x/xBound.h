#ifndef XBOUND_H
#define XBOUND_H

#include "xMath3.h"
#include "xCollide.h"
#include "xQuickCull.h"

class xModelInstance;

class _xBound
{
public:
    unsigned char type;    // 0x0
    unsigned char pad[3];
    union
    {
        _xSphere sph;      // 0x4
        _xBBox box;        // 0x4
        _xCylinder cyl;    // 0x4
    };
    _xMat4x3* mat;         // 0x28
    xQCData qcd;           // 0x2C
};

void xVecHitsBound(const _xVec3* v, const _xBound* b, _xCollis* c);
void xRayHitsBound(const _xRay3* r, const xQCData* q, const _xBound* b, _xCollis* c);
void xRayHitsBound(const _xRay3* r, const _xBound* b, _xCollis* c);
void xBoundHitsModel(_xBound* b, xModelInstance* m, _xCollis* colls, unsigned char ncolls,
                     float sth);
void xBoundHitsBound(_xBound* a, _xBound* b, _xCollis* c);
void xBoundUpdate(_xBound* b);
_xVec3* xBoundCenter(_xBound* b);

#endif
