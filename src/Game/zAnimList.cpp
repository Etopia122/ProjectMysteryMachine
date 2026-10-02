#include "xAnim.h"
#include "xMemMgr.h"
#include "xstransvc.h"

class zAnimListAsset
{
public:
    unsigned int ids[10];
};

static signed int nals;
static unsigned int* aids;
static xAnimTable** atbls;
static signed int* anused;
static char* astnames[20] = {
    "stop0", "stop1", "stop2", "stop3", "stop4", "stop5", "stop6", "stop7", "stop8", "stop9",
    "loop0", "loop1", "loop2", "loop3", "loop4", "loop5", "loop6", "loop7", "loop8", "loop9",
};

// Functions are in reverse DOL order (-inline deferred emits last to first).

static unsigned int AlwaysConditional(xAnimTransition*, xAnimSingle*, void*)
{
    return 1;
}

void zAnimListInit()
{
    signed int i;

    nals = xSTAssetCountByType('ALST');
    if (nals == 0)
    {
        return;
    }

    aids = (unsigned int*)xMemAlloc(gActiveHeap, nals << 2);
    atbls = (xAnimTable**)xMemAlloc(gActiveHeap, nals << 2);
    anused = (signed int*)xMemAlloc(gActiveHeap, nals << 2);

    for (i = 0; i < nals; i++)
    {
        unsigned int size;
        zAnimListAsset* zala;
        st_xST_XASSETINFO ainfo;
        xAnimTable* atbl;
        void* buf;
        xAnimFile* afile;
        signed int idle_exists;
        signed int j;

        zala = (zAnimListAsset*)xSTFindAssetByType('ALST', i, &size);
        xSTGetAssetInfoByType('ALST', i, &ainfo);
        atbl = xAnimTableNew("", 0);
        aids[i] = ainfo.aid;
        atbls[i] = atbl;
        anused[i] = 0;

        idle_exists = (signed int)xSTFindAsset(zala->ids[0], &size);
        if (idle_exists)
        {
            afile = xAnimFileNew((void*)idle_exists, "", 0, 0);
            xAnimTableNewState(atbl, "idle", 0x10, 0, 1.0f, 0, 0, 0.0f, 0);
            xAnimTableAddFile(atbl, afile, "idle");
            anused[i]++;
        }

        for (j = 1; j < 10; j++)
        {
            xAnimFile* afile;

            if (zala->ids[j] == 0)
            {
                continue;
            }
            buf = xSTFindAsset(zala->ids[j], &size);
            if (buf == 0)
            {
                continue;
            }
            afile = xAnimFileNew(buf, "", 0, 0);
            if (idle_exists)
            {
                xAnimTableNewState(atbl, astnames[j], 0x20, 0, 1.0f, 0, 0, 0.0f, 0);
                xAnimTableNewTransition(atbl, astnames[j], "idle", AlwaysConditional, 0, 0x10, 0,
                                        0.0f, 0.0f, 0, 0, 0.0f, 0);
            }
            else
            {
                xAnimTableNewState(atbl, astnames[j], 0, 0, 1.0f, 0, 0, 0.0f, 0);
            }
            xAnimTableAddFile(atbl, afile, astnames[j]);
            xAnimTableNewState(atbl, astnames[j + 10], 0x10, 0, 1.0f, 0, 0, 0.0f, 0);
            xAnimTableAddFile(atbl, afile, astnames[j + 10]);
            anused[i]++;
        }
    }
}

void zAnimListExit()
{
    nals = 0;
    aids = 0;
    atbls = 0;
    anused = 0;
}

xAnimTable* zAnimListGetTable(unsigned int id)
{
    signed int i;
    for (i = 0; i < nals; i++)
    {
        if (id == aids[i])
        {
            return atbls[i];
        }
    }
    return 0;
}

signed int zAnimListGetNumUsed(unsigned int id)
{
    signed int i;
    for (i = 0; i < nals; i++)
    {
        if (id == aids[i])
        {
            return anused[i];
        }
    }
    return 0;
}
