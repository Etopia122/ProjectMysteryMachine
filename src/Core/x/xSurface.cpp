#include "xBase.h"
#include "xMemMgr.h"

class _xEnt;

class xSurface : public xBase
{
public:
    unsigned int idx;
    unsigned int type;
    union
    {
        unsigned int mat_idx;
        _xEnt* ent;
        void* obj;
    };
    float friction;
    unsigned char state;
    unsigned char pad[3];
    void* moprops;
};

static xSurface* surfs;
static unsigned short nsurfs;

// Functions are in reverse DOL order (-inline deferred emits last to first).

void xSurfaceInit(unsigned short num_surfs)
{
    nsurfs = num_surfs;
    if (num_surfs)
    {
        surfs = (xSurface*)xMemAlloc(gActiveHeap, num_surfs * sizeof(xSurface));
        for (unsigned short i = 0; i < nsurfs; i++)
        {
            surfs[i].idx = i;
        }
    }
    else
    {
        surfs = 0;
    }
}

void xSurfaceExit()
{
}

void xSurfaceSave(xSurface* ent, xSerial* s)
{
    xBaseSave(ent, s);
}

void xSurfaceLoad(xSurface* ent, xSerial* s)
{
    xBaseLoad(ent, s);
}

void xSurfaceReset()
{
}

unsigned short xSurfaceGetNumSurfaces()
{
    return nsurfs;
}

xSurface* xSurfaceGetByIdx(unsigned short n)
{
    if (surfs)
    {
        return &surfs[n];
    }
    return 0;
}
