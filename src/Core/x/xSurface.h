#ifndef XSURFACE_H
#define XSURFACE_H

#include "xBase.h"

class _xEnt;

class xSurface : public xBase
{
public:
    unsigned int idx;    // 0x10
    unsigned int type;   // 0x14
    union
    {
        unsigned int mat_idx;
        _xEnt* ent;
        void* obj;
    };                   // 0x18
    float friction;      // 0x1C
    unsigned char state; // 0x20
    unsigned char pad[3];
    void* moprops;       // 0x24
};

void xSurfaceInit(unsigned short num_surfs);
void xSurfaceExit();
void xSurfaceSave(xSurface* ent, xSerial* s);
void xSurfaceLoad(xSurface* ent, xSerial* s);
void xSurfaceReset(xSurface* ent);
unsigned short xSurfaceGetNumSurfaces();
xSurface* xSurfaceGetByIdx(unsigned short n);

#endif
