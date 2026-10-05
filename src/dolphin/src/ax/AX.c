#include <dolphin.h>
#include <dolphin/ax.h>

#include "ax/__ax.h"

// Scooby's AX library is older than BFBB's: no version string and no
// AXInitEx; AXInit sets up the modules directly.

void AXInit(void)
{
    __AXAllocInit();
    __AXVPBInit();
    __AXSPBInit();
    __AXAuxInit();
    __AXClInit();
    __AXOutInit();
}

void AXQuit(void)
{
    __AXAllocQuit();
    __AXVPBQuit();
    __AXSPBQuit();
    __AXAuxQuit();
    __AXClQuit();
    __AXOutQuit();
}
