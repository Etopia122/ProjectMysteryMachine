#ifndef IMATH3_H
#define IMATH3_H

#include "xMath3.h"

void iBoxIsectSphere(const _xBox* box, const _xSphere* p, _xIsect* isx);
void iBoxIsectRay(const _xBox* b, const _xRay3* r, _xIsect* isx);
void iBoxIsectVec(const _xBox* b, const _xVec3* v, _xIsect* isx);
void iCylinderIsectRay(const _xCylinder* c, const _xRay3* r, _xIsect* isx);
void iCylinderIsectVec(const _xCylinder* c, const _xVec3* v, _xIsect* isx);
void iSphereIsectSphere(const _xSphere* s, const _xSphere* p, _xIsect* isx);
void iSphereIsectRay(const _xSphere* s, const _xRay3* r, _xIsect* isx);
void iSphereIsectVec(const _xSphere* s, const _xVec3* v, _xIsect* isx);

#endif
