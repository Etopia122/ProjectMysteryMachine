#ifndef XSPLINE_H
#define XSPLINE_H

#include "xMath3.h"

class xCoef
{
public:
    float a[4]; // 0x0
};

class xCoef3
{
public:
    xCoef x; // 0x0
    xCoef y; // 0x10
    xCoef z; // 0x20
};

class xSpline3
{
public:
    unsigned short type;    // 0x0
    unsigned short flags;   // 0x2
    unsigned int N;         // 0x4
    unsigned int allocN;    // 0x8
    _xVec3* points;         // 0xC
    float* time;            // 0x10
    _xVec3* p12;            // 0x14
    _xVec3* bctrl;          // 0x18
    float* knot;            // 0x1C
    xCoef3* coef;           // 0x20
    unsigned int arcSample; // 0x24
    float* arcLength;       // 0x28
};

void Tridiag_Solve(float* a, float* b, float* c, _xVec3* d, _xVec3* x, signed int n);
float ArcLength3(xCoef3* coef, double ustart, double uend);
void EvalBspline3(xSpline3* spl, float u, unsigned int deriv, _xVec3* o);
xCoef3* CoefSeg3(xSpline3* spl, unsigned int seg, xCoef3* tempCoef);
void xSpline3_EvalSeg(xSpline3* spl, float u, unsigned int deriv, _xVec3* o);
float ArcEvalIterate(xSpline3* spl, float s, unsigned int deriv, _xVec3* o, unsigned int iterations);
float xSpline3_EvalArcApprox(xSpline3* spl, float s, unsigned int deriv, _xVec3* o);
void xSpline3_ArcInit(xSpline3* spl, unsigned int sample);
xSpline3* xSpline3_Bezier(_xVec3* points, float* time, unsigned int numpoints,
                          unsigned int numalloc, _xVec3* p1, _xVec3* p2);
void xSpline3_Update(xSpline3* spl);
void xSpline3_Catmullize(xSpline3* spl);

#endif
