#include "xParMgr.h"
#include "xPar.h"
#include "iParMgr.h"

static unsigned int sFrameCount;
static signed int sParMgrInit;

// Functions are in reverse DOL order (-inline deferred emits last to first).

void xParMgrInit()
{
    if (!sParMgrInit)
    {
        xParMemInit();
        iParMgrInit();
        sParMgrInit = 1;
    }
    else
    {
        iParMgrInit();
    }
}

// TODO: block layout differs - the original enters the countdown loop through
// a load placed after the loop body (both paths of the if jump to it).
void xParMgrUpdate(float elapsedTime)
{
    iParMgrUpdate(elapsedTime);
    sFrameCount++;
    if (sFrameCount > 10)
    {
        sFrameCount = 1;
    }
    while (sFrameCount >= 1)
    {
        sFrameCount--;
    }
}

void iParMgrRender();

void xParMgrRender()
{
    iParMgrRender();
}
