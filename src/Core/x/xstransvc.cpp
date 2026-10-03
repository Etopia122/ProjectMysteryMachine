#include "xstransvc.h"

extern "C" {
void* memset(void* dst, int c, unsigned int n);
char* strcpy(char* dst, const char* src);
int sprintf(char* s, const char* format, ...);
}

class st_PACKER_READ_DATA;
class st_PACKER_ASSETTYPE;

enum en_LAYER_TYPE
{
    PKR_LTYPE_ALL = -1
};

class st_PKR_ASSET_TOCINFO
{
public:
    unsigned int aid;
    unsigned int sector;
    unsigned int plus_offset;
    unsigned int size;
    void* mempos;
};

class st_PACKER_READ_FUNCS
{
public:
    unsigned int api_ver;
    st_PACKER_READ_DATA* (*Init)(void*, char*, unsigned int, signed int*, st_PACKER_ASSETTYPE*);
    void (*Done)(st_PACKER_READ_DATA*);
    signed int (*LoadLayer)(st_PACKER_READ_DATA*, en_LAYER_TYPE);
    unsigned int (*GetAssetSize)(st_PACKER_READ_DATA*, unsigned int);
    void* (*LoadAsset)(st_PACKER_READ_DATA*, unsigned int, char*, void*);
    void* (*AssetByType)(st_PACKER_READ_DATA*, unsigned int, signed int, unsigned int*);
    signed int (*AssetCount)(st_PACKER_READ_DATA*, unsigned int);
    signed int (*IsAssetReady)(st_PACKER_READ_DATA*, unsigned int);
    signed int (*SetActive)(st_PACKER_READ_DATA*, en_LAYER_TYPE);
    char* (*AssetName)(st_PACKER_READ_DATA*, unsigned int);
    unsigned int (*GetBaseSector)(st_PACKER_READ_DATA*);
    signed int (*GetAssetInfo)(st_PACKER_READ_DATA*, unsigned int, st_PKR_ASSET_TOCINFO*);
    signed int (*GetAssetInfoByType)(st_PACKER_READ_DATA*, unsigned int, signed int,
                                     st_PKR_ASSET_TOCINFO*);
    signed int (*PkgHasAsset)(st_PACKER_READ_DATA*, unsigned int);
    unsigned int (*PkgTimeStamp)(st_PACKER_READ_DATA*);
};

class st_STRAN_SCENE
{
public:
    unsigned int scnid;
    signed int lockid;
    st_PACKER_READ_DATA* spkg;
    void* userdata;
    char fnam[256];
};

class st_STRAN_DATA
{
public:
    st_STRAN_SCENE hipscn[8];
    unsigned int loadlock;
};

signed int PKRStartup();
signed int PKRShutdown();
signed int PKRLoadStep(signed int);
st_PACKER_READ_FUNCS* PKRGetReadFuncs(signed int apiver);
char* xUtil_idtag2string(unsigned int srctag, signed int bufidx);

class iTRCDisk
{
public:
    static void CheckDVDAndResetState();
};

// Not yet named: a varargs debug print in iSystem.cpp (a no-op here).
extern "C" signed int fn_80015EAC(char* fmt, ...);

static st_STRAN_DATA g_xstdata = { 0 };
static signed int g_straninit;
static st_PACKER_READ_FUNCS* g_pkrf;
static st_PACKER_ASSETTYPE* g_typeHandlers;

static inline signed int XST_cnt_locked()
{
    signed int cnt = 0;
    for (signed int i = 0; i < 8; i++)
    {
        if (g_xstdata.loadlock & (1 << i))
        {
            cnt++;
        }
    }
    return cnt;
}

static inline st_STRAN_SCENE* XST_nth_locked(signed int index)
{
    st_STRAN_SCENE* sdata = 0;
    signed int cnt = 0;
    for (signed int i = 0; i < 8; i++)
    {
        if (g_xstdata.loadlock & (1 << i))
        {
            if (cnt == index)
            {
                sdata = &g_xstdata.hipscn[i];
                break;
            }
            cnt++;
        }
    }
    return sdata;
}

// TODO: xSTQueueSceneAssets/xSTSwitchScene/xSTUnLoadScene (97-99%) compare
// scnid with sid in the other operand order and number the registers one
// lower than this inline does.
static inline st_STRAN_SCENE* XST_find_bySID(unsigned int sid)
{
    st_STRAN_SCENE* da_sdata = 0;
    for (signed int i = 0; i < 8; i++)
    {
        if (g_xstdata.loadlock & (1 << i))
        {
            if (g_xstdata.hipscn[i].scnid == sid)
            {
                da_sdata = &g_xstdata.hipscn[i];
                break;
            }
        }
    }
    return da_sdata;
}

static inline st_STRAN_SCENE* XST_lock_next()
{
    st_STRAN_SCENE* sdata = 0;
    signed int uselock = -1;
    for (signed int i = 0; i < 8; i++)
    {
        if (!(g_xstdata.loadlock & (1 << i)))
        {
            g_xstdata.loadlock |= 1 << i;
            sdata = &g_xstdata.hipscn[i];
            memset(sdata, 0, sizeof(st_STRAN_SCENE));
            uselock = i;
            break;
        }
    }
    if (sdata)
    {
        sdata->lockid = uselock;
    }
    return sdata;
}

static inline void XST_unlock(st_STRAN_SCENE* sdata)
{
    if (sdata)
    {
        if (g_xstdata.loadlock & (1 << sdata->lockid))
        {
            g_xstdata.loadlock &= ~(1 << sdata->lockid);
            memset(sdata, 0, sizeof(st_STRAN_SCENE));
        }
    }
}

static inline void XST_unlock_all()
{
    if (g_xstdata.loadlock)
    {
        for (signed int i = 0; i < 8; i++)
        {
            if (g_xstdata.loadlock & (1 << i))
            {
                XST_unlock(&g_xstdata.hipscn[i]);
            }
        }
    }
}

static inline char* XST_translate_sid(unsigned int sid, char* extension)
{
    static char fname[64] = { 0 };
    sprintf(fname, "%s%s", xUtil_idtag2string(sid, 0), extension);
    return fname;
}

static inline char* XST_translate_sid_path(unsigned int sid, char* extension)
{
    static char fname[64] = { 0 };
    char sep[2] = "/";
    sprintf(fname, "%c%c%s%s%s", xUtil_idtag2string(sid, 0)[0], xUtil_idtag2string(sid, 0)[1], sep,
            xUtil_idtag2string(sid, 0), extension);
    return fname;
}

static inline signed int XST_PreLoadScene(st_STRAN_SCENE* sdata, char* path)
{
    signed int cltver = 0;
    sdata->spkg = g_pkrf->Init(sdata->userdata, path, 0x2E, &cltver, g_typeHandlers);
    if (sdata->spkg)
    {
        return cltver;
    }
    return 0;
}

// Functions are in reverse DOL order (-inline deferred emits last to first).

signed int xSTStartup(st_PACKER_ASSETTYPE* handlers)
{
    if (g_straninit++ == 0)
    {
        g_typeHandlers = handlers;
        memset(&g_xstdata, 0, sizeof(st_STRAN_DATA));
        PKRStartup();
        g_pkrf = PKRGetReadFuncs(1);
    }
    return g_straninit;
}

signed int xSTShutdown()
{
    if (--g_straninit == 0)
    {
        g_typeHandlers = 0;
        XST_unlock_all();
        PKRShutdown();
    }
    return g_straninit;
}

// TODO (97%): the original folds the path null test into `addic. r21`.
signed int xSTPreLoadScene(unsigned int sid, void* userdata)
{
    signed int result;
    signed int i = 0;
    signed int attempt = 0;
    st_STRAN_SCENE* sdata;

    do
    {
        attempt++;
        fn_80015EAC("PreLoading 0x%08X (attempt #%d)\n", sid, attempt);
        sdata = XST_lock_next();
        sdata->scnid = sid;
        sdata->userdata = userdata;

        if (sid != 'BOOT' && sid != 'FONT')
        {
            char* path = XST_translate_sid_path(sid, ".HIP");
            if (path != 0)
            {
                strcpy(sdata->fnam, path);
                i = XST_PreLoadScene(sdata, path);
            }
        }
        if (i == 0)
        {
            char* path = XST_translate_sid(sid, ".HIP");
            if (path != 0)
            {
                strcpy(sdata->fnam, path);
                i = XST_PreLoadScene(sdata, path);
            }
        }
        if (i == 0)
        {
            XST_unlock(sdata);
            result = 0;
        }
        else
        {
            result = i;
        }
    } while (i == 0);

    return result;
}

signed int xSTQueueSceneAssets(unsigned int sid)
{
    signed int result = 1;
    st_STRAN_SCENE* sdata = XST_find_bySID(sid);
    if (!sdata)
    {
        result = 0;
    }
    else if (sdata->spkg)
    {
        g_pkrf->LoadLayer(sdata->spkg, PKR_LTYPE_ALL);
    }
    return result;
}

void xSTUnLoadScene(unsigned int sid)
{
    st_STRAN_SCENE* sdata;

    if (sid == 0)
    {
        signed int cnt = XST_cnt_locked();
        for (signed int i = 0; i < cnt; i++)
        {
            sdata = XST_nth_locked(i);
            if (sdata->spkg)
            {
                g_pkrf->Done(sdata->spkg);
            }
            sdata->spkg = 0;
        }
        XST_unlock_all();
    }
    else
    {
        sdata = XST_find_bySID(sid);
        if (sdata)
        {
            if (sdata->spkg)
            {
                g_pkrf->Done(sdata->spkg);
            }
            sdata->spkg = 0;
        }
        XST_unlock(sdata);
    }
}

float xSTLoadStep(unsigned int)
{
    float pct = PKRLoadStep(0) ? 0.0f : 1.00001f;
    iTRCDisk::CheckDVDAndResetState();
    return pct;
}

signed int xSTSwitchScene(unsigned int sid, void* userdata, signed int (*progmon)(void*, float))
{
    signed int rc = 1;
    st_STRAN_SCENE* sdata = 0;

    if (sid)
    {
        sdata = XST_find_bySID(sid);
    }
    if (!sdata)
    {
        rc = 0;
    }
    if (rc)
    {
        if (progmon)
        {
            progmon(userdata, 0.0f);
        }
        if (!g_pkrf->SetActive(sdata->spkg, PKR_LTYPE_ALL))
        {
            rc = 0;
        }
        if (progmon)
        {
            progmon(userdata, 1.0f);
        }
    }
    return rc;
}

char* xSTAssetName(unsigned int aid)
{
    char* aname = 0;
    signed int scncnt = XST_cnt_locked();
    for (signed int i = 0; i < scncnt; i++)
    {
        st_STRAN_SCENE* sdata = XST_nth_locked(i);
        if (sdata->spkg)
        {
            aname = g_pkrf->AssetName(sdata->spkg, aid);
            if (aname)
            {
                break;
            }
        }
    }
    return aname;
}

void* xSTFindAsset(unsigned int aid, unsigned int* size)
{
    void* memloc = 0;
    signed int scncnt = XST_cnt_locked();
    for (signed int i = 0; i < scncnt; i++)
    {
        st_STRAN_SCENE* sdata = XST_nth_locked(i);
        if (g_pkrf->PkgHasAsset(sdata->spkg, aid))
        {
            memloc = g_pkrf->LoadAsset(sdata->spkg, aid, 0, 0);
            if (!g_pkrf->IsAssetReady(sdata->spkg, aid))
            {
                memloc = 0;
                if (size)
                {
                    *size = 0;
                }
            }
            else if (size)
            {
                *size = g_pkrf->GetAssetSize(sdata->spkg, aid);
            }
            break;
        }
    }
    return memloc;
}

signed int xSTAssetCountByType(unsigned int type)
{
    signed int sum = 0;
    signed int scncnt = XST_cnt_locked();
    for (signed int i = 0; i < scncnt; i++)
    {
        st_STRAN_SCENE* sdata = XST_nth_locked(i);
        sum += g_pkrf->AssetCount(sdata->spkg, type);
    }
    return sum;
}

void* xSTFindAssetByType(unsigned int type, signed int idx, unsigned int* size)
{
    void* memptr = 0;
    signed int i;
    signed int sum = 0;
    signed int scncnt = XST_cnt_locked();
    for (i = 0; i < scncnt; i++)
    {
        st_STRAN_SCENE* sdata = XST_nth_locked(i);
        signed int cnt = g_pkrf->AssetCount(sdata->spkg, type);
        if (idx >= sum && idx < sum + cnt)
        {
            memptr = g_pkrf->AssetByType(sdata->spkg, type, idx - sum, size);
            break;
        }
        sum += cnt;
    }
    return memptr;
}

signed int xSTGetAssetInfo(unsigned int aid, st_xST_XASSETINFO* ainfo)
{
    st_PKR_ASSET_TOCINFO tocinfo = { 0 };
    signed int rc = 0;

    memset(ainfo, 0, sizeof(st_xST_XASSETINFO));
    signed int scncnt = XST_cnt_locked();
    for (signed int i = 0; i < scncnt; i++)
    {
        st_STRAN_SCENE* sdata = XST_nth_locked(i);
        if (g_pkrf->PkgHasAsset(sdata->spkg, aid))
        {
            g_pkrf->GetBaseSector(sdata->spkg);
            if (g_pkrf->GetAssetInfo(sdata->spkg, aid, &tocinfo))
            {
                ainfo->aid = tocinfo.aid;
                ainfo->sector = tocinfo.sector;
                ainfo->plus_offset = tocinfo.plus_offset;
                ainfo->size = tocinfo.size;
                ainfo->mempos = tocinfo.mempos;
                rc = 1;
                break;
            }
        }
    }
    return rc;
}

signed int xSTGetAssetInfoByType(unsigned int type, signed int idx, st_xST_XASSETINFO* ainfo)
{
    st_PKR_ASSET_TOCINFO tocinfo = { 0 };
    signed int rc = 0;
    signed int sum = 0;

    memset(ainfo, 0, sizeof(st_xST_XASSETINFO));
    signed int scncnt = XST_cnt_locked();
    for (signed int i = 0; i < scncnt; i++)
    {
        st_STRAN_SCENE* sdata = XST_nth_locked(i);
        signed int cnt = g_pkrf->AssetCount(sdata->spkg, type);
        if (idx >= sum && idx < sum + cnt)
        {
            g_pkrf->GetBaseSector(sdata->spkg);
            if (g_pkrf->GetAssetInfoByType(sdata->spkg, type, idx - sum, &tocinfo))
            {
                ainfo->aid = tocinfo.aid;
                ainfo->sector = tocinfo.sector;
                ainfo->plus_offset = tocinfo.plus_offset;
                ainfo->size = tocinfo.size;
                ainfo->mempos = tocinfo.mempos;
                rc = 1;
                break;
            }
        }
        sum += cnt;
    }
    return rc;
}

inline char* xST_xAssetID_HIPFullPath(unsigned int aid, unsigned int* sceneID)
{
    char* id = 0;
    signed int cnt = XST_cnt_locked();
    for (signed int i = 0; i < cnt; i++)
    {
        st_STRAN_SCENE* sdata = XST_nth_locked(i);
        if (g_pkrf->PkgHasAsset(sdata->spkg, aid))
        {
            id = sdata->fnam;
            if (sceneID != 0)
            {
                *sceneID = sdata->scnid;
            }
            break;
        }
    }
    return id;
}

char* xST_xAssetID_HIPFullPath(unsigned int aid)
{
    return xST_xAssetID_HIPFullPath(aid, 0);
}
