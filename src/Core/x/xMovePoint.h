#ifndef XMOVEPOINT_H
#define XMOVEPOINT_H

#include "xBase.h"
#include "xMath3.h"

class xSpline3;
class _xScene;

class p2MovePointAsset : public p2BaseAsset
{
public:
    _xVec3 pos;              // 0x8
    unsigned short wt;       // 0x14
    unsigned char on;        // 0x16
    unsigned char bezIndex;  // 0x17
    unsigned char flg_props; // 0x18
    unsigned char pad;
    unsigned short numPoints; // 0x1A
    float delay;              // 0x1C
};

class _xMovePoint : public xBase
{
public:
    p2MovePointAsset* asset;   // 0x10
    _xVec3* pos;               // 0x14
    _xMovePoint** nodes;       // 0x18
    _xMovePoint* prev;         // 0x1C
    unsigned int node_wt_sum;  // 0x20
    unsigned char on;          // 0x24
    unsigned char flg_props;   // 0x25
    float delay;               // 0x28
    xSpline3* spl;             // 0x2C
};

void xMovePointInit(_xMovePoint* m, p2MovePointAsset* asset);
void xMovePointSave(_xMovePoint* m, xSerial* s);
void xMovePointLoad(_xMovePoint* m, xSerial* s);
void xMovePointReset(_xMovePoint* m);
void xMovePointSetup(_xMovePoint* m, _xScene* sc);
void xMovePointSplineSetup(_xMovePoint* m);
void xMovePointSplineDestroy(_xMovePoint* m);
float xMovePointGetNext(_xMovePoint* m, _xMovePoint* prev, _xMovePoint** next, _xVec3* hdng);
_xVec3* xMovePointGetPos(_xMovePoint* m);

#endif
