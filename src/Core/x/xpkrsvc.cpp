#include "xordarray.h"
#include "xMemMgr.h"

extern "C" {
void* memset(void* dst, int c, unsigned int n);
char* strncpy(char* dst, const char* src, unsigned int n);
void exit(int status);

void fn_800167F0();
void fn_80164200();
}

enum en_READ_ASYNC_STATUS
{
    HIP_RDSTAT_NONE = -1,
    HIP_RDSTAT_INPROG = 0,
    HIP_RDSTAT_SUCCESS = 1,
    HIP_RDSTAT_FAILED = 2,
    HIP_RDSTAT_NOBYPASS = 3,
    HIP_RDSTAT_NOASYNC = 4
};

enum en_LAYER_TYPE
{
    PKR_LTYPE_ALL = -1,
    PKR_LTYPE_DEFAULT = 0,
    PKR_LTYPE_TEXTURE = 1,
    PKR_LTYPE_BSP = 2,
    PKR_LTYPE_MODEL = 3,
    PKR_LTYPE_ANIMATION = 4,
    PKR_LTYPE_VRAM = 5,
    PKR_LTYPE_SRAM = 6,
    PKR_LTYPE_SNDTOC = 7,
    PKR_LTYPE_CUTSCENE = 8,
    PKR_LTYPE_CUTSCENETOC = 9,
    PKR_LTYPE_NOMORE = 10
};

enum en_PKR_LAYER_LOAD_DEST
{
    PKR_LDDEST_SKIP,
    PKR_LDDEST_KEEPSTATIC,
    PKR_LDDEST_RWHANDOFF,
    PKR_LDDEST_NOMORE
};

class st_HIPLOADDATA;

class st_HIPLOADFUNCS
{
public:
    st_HIPLOADDATA* (*create)(char*, char*, signed int);
    void (*destroy)(st_HIPLOADDATA*);
    unsigned int (*basesector)(st_HIPLOADDATA*);
    unsigned int (*enter)(st_HIPLOADDATA*);
    void (*exit)(st_HIPLOADDATA*);
    signed int (*readBytes)(st_HIPLOADDATA*, char*, signed int);
    signed int (*readShorts)(st_HIPLOADDATA*, signed short*, signed int);
    signed int (*readLongs)(st_HIPLOADDATA*, signed int*, signed int);
    signed int (*readFloats)(st_HIPLOADDATA*, float*, signed int);
    signed int (*readString)(st_HIPLOADDATA*, char*);
    signed int (*setBypass)(st_HIPLOADDATA*, signed int, signed int);
    void (*setSpot)(st_HIPLOADDATA*, signed int);
    en_READ_ASYNC_STATUS (*pollRead)(st_HIPLOADDATA*);
};

class st_PACKER_ASSETTYPE
{
public:
    unsigned int typetag; // 0x0
    unsigned int tflags;  // 0x4
    signed int typalign;  // 0x8
    void* (*readXForm)(void*, unsigned int, void*, unsigned int, unsigned int*);        // 0xC
    void* (*writeXForm)(void*, unsigned int, void*, void*, unsigned int, unsigned int*); // 0x10
    signed int (*assetLoaded)(void*, unsigned int, void*, signed int);                   // 0x14
    void* (*makeData)(void*, unsigned int, void*, signed int*, signed int*);             // 0x18
    void (*cleanup)(void*, unsigned int, void*);                                         // 0x1C
    void (*assetUnloaded)(void*, unsigned int);                                          // 0x20
    void (*writePeek)(void*, unsigned int, void*, char*);                                // 0x24
};

class st_PACKER_READ_DATA;

class st_PACKER_ATOC_NODE
{
public:
    unsigned int aid;              // 0x0
    unsigned int asstype;          // 0x4
    signed int d_off;              // 0x8
    signed int d_size;             // 0xC
    signed int d_pad;              // 0x10
    unsigned int d_chksum;         // 0x14
    signed int assalign;           // 0x18
    signed int infoflag;           // 0x1C
    signed int loadflag;           // 0x20
    char* memloc;                  // 0x24
    signed int x_size;             // 0x28
    signed int readcnt;            // 0x2C
    signed int readrem;            // 0x30
    st_PACKER_ASSETTYPE* typeref;  // 0x34
    st_HIPLOADDATA* ownpkg;        // 0x38
    st_PACKER_READ_DATA* ownpr;    // 0x3C
    char name[32];                 // 0x40
};

class st_PACKER_LTOC_NODE
{
public:
    en_LAYER_TYPE laytyp;     // 0x0
    st_XORDEREDARRAY assref;  // 0x4
    signed int flg_ldstat;    // 0x14
    signed int danglecnt;     // 0x18
    unsigned int chksum;      // 0x1C
    signed int laysize;       // 0x20
    char* laymem;             // 0x24
};

class st_PACKER_READ_DATA
{
public:
    st_PACKER_ASSETTYPE* types;     // 0x0
    void* userdata;                 // 0x4
    unsigned int opts;              // 0x8
    unsigned int pkgver;            // 0xC
    signed int cltver;              // 0x10
    signed int subver;              // 0x14
    signed int compatver;           // 0x18
    st_HIPLOADDATA* pkg;            // 0x1C
    unsigned int base_sector;       // 0x20
    signed int lockid;              // 0x24
    char packfile[128];             // 0x28
    signed int asscnt;              // 0xA8
    signed int laycnt;              // 0xAC
    st_XORDEREDARRAY asstoc;        // 0xB0
    st_XORDEREDARRAY laytoc;        // 0xC0
    st_PACKER_ATOC_NODE* pool_anode; // 0xD0
    signed int pool_nextaidx;       // 0xD4
    st_XORDEREDARRAY typelist[129]; // 0xD8
    signed int time_made;           // 0x8E8
    signed int time_mod;            // 0x8EC
};

class st_PKR_ASSET_TOCINFO
{
public:
    unsigned int aid;         // 0x0
    unsigned int sector;      // 0x4
    unsigned int plus_offset; // 0x8
    unsigned int size;        // 0xC
    void* mempos;             // 0x10
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

class RwLLLink
{
public:
    RwLLLink* next;
    RwLLLink* prev;
};

class RwLinkList
{
public:
    RwLLLink link;
};

class RwResEntry
{
public:
    RwLLLink link;                          // 0x0
    signed int size;                        // 0x8
    void* owner;                            // 0xC
    RwResEntry** ownerRef;                  // 0x10
    void (*destroyNotify)(RwResEntry*);     // 0x14
};

class RwModuleInfo
{
public:
    signed int globalsOffset;
    signed int numInstances;
};

extern "C" {
RwResEntry* RwResourcesAllocateResEntry(void* owner, RwResEntry** ownerRef, signed int size,
                                        void (*destroyNotify)(RwResEntry*));
signed int RwResourcesFreeResEntry(RwResEntry* entry);
}
extern void* RwEngineInstance;
extern RwModuleInfo lbl_8025F798; // resourcesModule

class rwResourcesGlobals
{
public:
    unsigned char pad[0x24];
    RwLinkList* usedEntries; // 0x24
};

#define RWRESOURCESUSEDENTRIES                                                                        (((rwResourcesGlobals*)((unsigned char*)RwEngineInstance + lbl_8025F798.globalsOffset))               ->usedEntries)
#define rwLinkListRemoveLLLink(_lnk)                                                                   ((_lnk)->prev->next = (_lnk)->next, (_lnk)->next->prev = (_lnk)->prev)
#define rwLinkListAddLLLink(_list, _lnk)                                                               ((_lnk)->next = (_list)->link.next, (_lnk)->prev = &(_list)->link,                                  (_list)->link.next->prev = (_lnk), (_list)->link.next = (_lnk))
#define RwResourcesUseResEntry(_ntry)                                                                  ((_ntry)->link.next ? (rwLinkListRemoveLLLink(&(_ntry)->link),                                                            rwLinkListAddLLLink(RWRESOURCESUSEDENTRIES, &(_ntry)->link))                                    : 0)

st_HIPLOADFUNCS* get_HIPLFuncs();

st_PACKER_READ_DATA* PKR_ReadInit(void* userdata, char* pkgfile, unsigned int opts,
                                  signed int* cltver, st_PACKER_ASSETTYPE* typelist);
void PKR_ReadDone(st_PACKER_READ_DATA* pr);
signed int PKR_LoadLayer(st_PACKER_READ_DATA* pr, en_LAYER_TYPE layer);
unsigned int PKR_GetAssetSize(st_PACKER_READ_DATA* pr, unsigned int aid);
void* PKR_LoadAsset(st_PACKER_READ_DATA* pr, unsigned int aid, char*, void*);
void* PKR_AssetByType(st_PACKER_READ_DATA* pr, unsigned int type, signed int idx,
                      unsigned int* size);
signed int PKR_AssetCount(st_PACKER_READ_DATA* pr, unsigned int type);
signed int PKR_IsAssetReady(st_PACKER_READ_DATA* pr, unsigned int aid);
signed int PKR_SetActive(st_PACKER_READ_DATA* pr, en_LAYER_TYPE layer);
char* PKR_AssetName(st_PACKER_READ_DATA* pr, unsigned int aid);
unsigned int PKR_GetBaseSector(st_PACKER_READ_DATA* pr);
signed int PKR_GetAssetInfo(st_PACKER_READ_DATA* pr, unsigned int aid,
                            st_PKR_ASSET_TOCINFO* tocinfo);
signed int PKR_GetAssetInfoByType(st_PACKER_READ_DATA* pr, unsigned int type, signed int idx,
                                  st_PKR_ASSET_TOCINFO* tocinfo);
signed int PKR_PkgHasAsset(st_PACKER_READ_DATA* pr, unsigned int aid);
unsigned int PKR_getPackTimestamp(st_PACKER_READ_DATA* pr);
signed int PKR_LoadStep_Async();
signed int PKR_parse_TOC(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr);
signed int PKR_findNextLayerToLoad(st_PACKER_READ_DATA** work_on_pkg,
                                   st_PACKER_LTOC_NODE** next_layer);
void PKR_updateLayerAssets(st_PACKER_LTOC_NODE* laynode);
void PKR_xform_asset(st_PACKER_ATOC_NODE* assnode);
signed int PKR_FRIEND_assetIsGameDup(unsigned int aid, const st_PACKER_READ_DATA* skippr,
                                      signed int oursize, unsigned int ourtype,
                                      unsigned int chksum, char*);
signed int OrdTest_R_AssetID(void* vkey, void* vitem);
signed int OrdTest_R_Asset(void* vkey, void* vitem);
signed int LOD_r_AHDR(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr);
signed int LOD_r_LHDR(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr);
void PKR_bld_typecnt(st_PACKER_READ_DATA* pr);
void PKR_special_loadbuf_killed(RwResEntry* resEntry);
static char* PKR_RWarenaAlloc(st_PACKER_READ_DATA* pr, signed int size);
static void PKR_RWarenaFree();
static void PKR_RWarenaUse();

static st_PACKER_READ_FUNCS g_pkr_read_funcmap = {
    1,
    PKR_ReadInit,
    PKR_ReadDone,
    PKR_LoadLayer,
    PKR_GetAssetSize,
    PKR_LoadAsset,
    PKR_AssetByType,
    PKR_AssetCount,
    PKR_IsAssetReady,
    PKR_SetActive,
    PKR_AssetName,
    PKR_GetBaseSector,
    PKR_GetAssetInfo,
    PKR_GetAssetInfoByType,
    PKR_PkgHasAsset,
    PKR_getPackTimestamp,
};
static st_PACKER_READ_DATA g_readdatainst[8] = {};
static st_HIPLOADFUNCS* g_hiprf;
static unsigned int g_loadlock;
static signed int pkr_sector_size;
static signed int g_packinit;

#pragma auto_inline on

static void* PKR_getmem(signed int amount, signed int isTemp)
{
    void* memptr = 0;

    if (amount)
    {
        if (isTemp)
        {
            memptr = xMemPushTemp(amount);
        }
        else
        {
            memptr =
                (void*)(((unsigned int)xMemAlloc(gActiveHeap, amount + 0x3F) + 0x3F) & ~0x3F);
        }
        if (memptr)
        {
            memset(memptr, 0, amount);
        }
    }
    return memptr;
}

static void PKR_relmem(void* memptr, signed int isTemp)
{
    if (memptr && isTemp)
    {
        xMemPopTemp(memptr);
    }
}

static st_PACKER_ASSETTYPE* PKR_type2typeref(unsigned int type, st_PACKER_ASSETTYPE* typelist)
{
    st_PACKER_ASSETTYPE* da_type = 0;

    if (typelist)
    {
        for (st_PACKER_ASSETTYPE* tmptype = typelist; tmptype->typetag != 0; tmptype++)
        {
            if (tmptype->typetag == type)
            {
                da_type = tmptype;
                break;
            }
        }
    }
    return da_type;
}

static signed int PKR_typeHdlr_idx(st_PACKER_READ_DATA* pr, unsigned int type)
{
    signed int idx = -1;
    signed int i = 0;
    st_PACKER_ASSETTYPE* tmptype = pr->types;

    while (tmptype->typetag != 0)
    {
        if (tmptype->typetag == type)
        {
            idx = i;
            break;
        }
        i++;
        tmptype++;
    }
    return idx;
}

static inline en_PKR_LAYER_LOAD_DEST PKR_layerLoadDest(en_LAYER_TYPE layer)
{
    en_PKR_LAYER_LOAD_DEST rc;

    switch (layer)
    {
    case PKR_LTYPE_SRAM:
    case PKR_LTYPE_CUTSCENE:
        rc = PKR_LDDEST_SKIP;
        break;
    case PKR_LTYPE_TEXTURE:
    case PKR_LTYPE_BSP:
    case PKR_LTYPE_MODEL:
        rc = PKR_LDDEST_RWHANDOFF;
        break;
    case PKR_LTYPE_DEFAULT:
    case PKR_LTYPE_ANIMATION:
    case PKR_LTYPE_VRAM:
    case PKR_LTYPE_SNDTOC:
    case PKR_LTYPE_CUTSCENETOC:
        rc = PKR_LDDEST_KEEPSTATIC;
        break;
    default:
        rc = PKR_LDDEST_KEEPSTATIC;
        break;
    }
    return rc;
}

static inline signed int PKR_layerTypeNeedsXForm(en_LAYER_TYPE layer)
{
    signed int rc;

    switch (layer)
    {
    case PKR_LTYPE_TEXTURE:
    case PKR_LTYPE_BSP:
    case PKR_LTYPE_MODEL:
        rc = 1;
        break;
    case PKR_LTYPE_DEFAULT:
    case PKR_LTYPE_ANIMATION:
    case PKR_LTYPE_VRAM:
    case PKR_LTYPE_SNDTOC:
    case PKR_LTYPE_CUTSCENETOC:
        rc = 1;
        break;
    case PKR_LTYPE_SRAM:
    case PKR_LTYPE_CUTSCENE:
        rc = 0;
        break;
    default:
        rc = 0;
        break;
    }
    return rc;
}

static inline void PKR_xformLayerAssets(st_PACKER_LTOC_NODE* laynode)
{
    signed int will_be_dumped = 0;

    if (PKR_layerLoadDest(laynode->laytyp) == PKR_LDDEST_RWHANDOFF)
    {
        will_be_dumped = 1;
    }

    for (signed int i = 0; i < laynode->assref.cnt; i++)
    {
        st_PACKER_ATOC_NODE* tmpass = (st_PACKER_ATOC_NODE*)laynode->assref.list[i];
        if (!(tmpass->loadflag & 0x100000))
        {
            PKR_xform_asset(tmpass);
            if (will_be_dumped && tmpass->x_size < 1)
            {
                tmpass->memloc = 0;
            }
        }
    }
}

static st_PACKER_ATOC_NODE* PKR_newassnode(st_PACKER_READ_DATA* pr, unsigned int aid)
{
    signed int idx = pr->pool_nextaidx;
    st_PACKER_ATOC_NODE* newnode = &pr->pool_anode[idx];
    pr->pool_nextaidx = idx + 1;

    memset(newnode, 0, sizeof(st_PACKER_ATOC_NODE));
    newnode->aid = aid;
    return newnode;
}

static st_PACKER_LTOC_NODE* PKR_newlaynode(en_LAYER_TYPE layer, signed int refcnt)
{
    st_PACKER_LTOC_NODE* newnode =
        (st_PACKER_LTOC_NODE*)PKR_getmem(sizeof(st_PACKER_LTOC_NODE), 0);
    memset(newnode, 0, sizeof(st_PACKER_LTOC_NODE));

    newnode->laytyp = layer;
    XOrdInit(&newnode->assref, refcnt > 1 ? refcnt : 2);
    return newnode;
}

static signed int LOD_r_HIPA(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    pr->pkgver = 'HIPA';
    return 1;
}

static signed int LOD_r_PVER(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    signed int ver = 0;
    g_hiprf->readLongs(pkg, &ver, 1);
    pr->subver = ver;
    ver = -1;
    g_hiprf->readLongs(pkg, &ver, 1);
    pr->cltver = ver;
    ver = -1;
    if (g_hiprf->readLongs(pkg, &ver, 1) != 1)
    {
        pr->compatver = 1;
    }
    else
    {
        pr->compatver = ver;
    }
    return 1;
}

static signed int LOD_r_PFLG(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    signed int flg = 0;
    g_hiprf->readLongs(pkg, &flg, 1);
    return 1;
}

static signed int LOD_r_PCNT(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    signed int cnt = 0;
    g_hiprf->readLongs(pkg, &cnt, 1);
    pr->asscnt = cnt;
    g_hiprf->readLongs(pkg, &cnt, 1);
    pr->laycnt = cnt;
    g_hiprf->readLongs(pkg, &cnt, 1);
    g_hiprf->readLongs(pkg, &cnt, 1);
    g_hiprf->readLongs(pkg, &cnt, 1);
    return 1;
}

static signed int LOD_r_PCRT(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    signed int time = 0;
    char arr[256] = {};
    g_hiprf->readLongs(pkg, &time, 1);
    pr->time_made = time;
    if (pr->subver > 1)
    {
        g_hiprf->readString(pkg, arr);
    }
    return 1;
}

static signed int LOD_r_PMOD(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    signed int time_mod = 0;
    g_hiprf->readLongs(pkg, &time_mod, 1);
    pr->time_mod = time_mod;
    return 1;
}

static signed int LOD_r_PACK(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    unsigned int cid = g_hiprf->enter(pkg);
    while (cid != 0)
    {
        switch (cid)
        {
        case 'PVER':
            LOD_r_PVER(pkg, pr);
            break;
        case 'PFLG':
            LOD_r_PFLG(pkg, pr);
            break;
        case 'PCNT':
            LOD_r_PCNT(pkg, pr);
            break;
        case 'PCRT':
            LOD_r_PCRT(pkg, pr);
            break;
        case 'PMOD':
            LOD_r_PMOD(pkg, pr);
            break;
        }
        g_hiprf->exit(pkg);
        cid = g_hiprf->enter(pkg);
    }
    return 1;
}

static signed int LOD_r_AINF(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    signed int ival = 0;
    g_hiprf->readLongs(pkg, &ival, 1);
    return 1;
}

static signed int LOD_r_ATOC(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    unsigned int cid = g_hiprf->enter(pkg);
    while (cid != 0)
    {
        switch (cid)
        {
        case 'AINF':
            LOD_r_AINF(pkg, pr);
            break;
        case 'AHDR':
            LOD_r_AHDR(pkg, pr);
            break;
        }
        g_hiprf->exit(pkg);
        cid = g_hiprf->enter(pkg);
    }
    return 1;
}

static signed int LOD_r_LINF(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    signed int ival = 0;
    g_hiprf->readLongs(pkg, &ival, 1);
    return 1;
}

static signed int LOD_r_LTOC(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    unsigned int cid = g_hiprf->enter(pkg);
    while (cid != 0)
    {
        switch (cid)
        {
        case 'LINF':
            LOD_r_LINF(pkg, pr);
            break;
        case 'LHDR':
            LOD_r_LHDR(pkg, pr);
            break;
        }
        g_hiprf->exit(pkg);
        cid = g_hiprf->enter(pkg);
    }
    return 1;
}

static signed int LOD_r_DICT(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    unsigned int cid = g_hiprf->enter(pkg);
    while (cid != 0)
    {
        switch (cid)
        {
        case 'ATOC':
            LOD_r_ATOC(pkg, pr);
            XOrdSort(&pr->asstoc, OrdTest_R_Asset);
            break;
        case 'LTOC':
            LOD_r_LTOC(pkg, pr);
            break;
        }
        g_hiprf->exit(pkg);
        cid = g_hiprf->enter(pkg);
    }
    return 1;
}

static signed int LOD_r_DHDR(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    signed int ivar = 0;
    g_hiprf->readLongs(pkg, &ivar, 1);
    return 1;
}

static signed int LOD_r_DPAK(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    return 1;
}

static signed int LOD_r_STRM(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    unsigned int cid = g_hiprf->enter(pkg);
    while (cid != 0)
    {
        switch (cid)
        {
        case 'DHDR':
            LOD_r_DHDR(pkg, pr);
            break;
        case 'DPAK':
            LOD_r_DPAK(pkg, pr);
            break;
        }
        g_hiprf->exit(pkg);
        cid = g_hiprf->enter(pkg);
    }
    return 1;
}

static signed int LOD_r_ADBG(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr,
                             st_PACKER_ATOC_NODE* assnode)
{
    signed int ival = 0;
    char tmpbuf[256] = {};

    g_hiprf->readLongs(pkg, &ival, 1);
    assnode->assalign = ival;
    g_hiprf->readString(pkg, tmpbuf);
    strncpy(assnode->name, tmpbuf, sizeof(assnode->name) - 1);
    assnode->name[sizeof(assnode->name) - 1] = 0;
    tmpbuf[0] = 0;
    g_hiprf->readString(pkg, tmpbuf);
    if (pr->subver > 1)
    {
        g_hiprf->readLongs(pkg, &ival, 1);
        assnode->d_chksum = ival;
    }
    return 1;
}

static signed int LOD_r_LDBG(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr,
                             st_PACKER_LTOC_NODE* laynode)
{
    signed int ivar = 0;
    if (pr->subver > 1)
    {
        g_hiprf->readLongs(pkg, &ivar, 1);
        laynode->chksum = ivar;
    }
    return 1;
}

#pragma auto_inline off

// Functions are in reverse DOL order (-inline deferred emits last to first).

st_PACKER_READ_FUNCS* PKRGetReadFuncs(signed int apiver)
{
    switch (apiver)
    {
    case 1:
        return &g_pkr_read_funcmap;
    default:
        return 0;
    }
}

signed int PKRStartup()
{
    if (g_packinit++ == 0)
    {
        g_hiprf = get_HIPLFuncs();
        pkr_sector_size = 32;
    }
    return g_packinit;
}

signed int PKRShutdown()
{
    g_packinit--;
    return g_packinit;
}

signed int PKRLoadStep(signed int)
{
    return PKR_LoadStep_Async();
}

st_PACKER_READ_DATA* PKR_ReadInit(void* userdata, char* pkgfile, unsigned int opts,
                                  signed int* cltver, st_PACKER_ASSETTYPE* typelist)
{
    st_PACKER_READ_DATA* pr = 0;
    signed int uselock = -1;
    char* tocbuf_RAW = (char*)PKR_getmem(0x8080, 1);
    char* tocbuf_aligned = (char*)(((unsigned int)tocbuf_RAW + 0x3F) & ~0x3F);

    for (signed int i = 0; i < 8; i++)
    {
        if (!(g_loadlock & (1 << i)))
        {
            uselock = i;
            g_loadlock |= 1 << i;
            pr = &g_readdatainst[i];
            break;
        }
    }

    if (pr)
    {
        memset(pr, 0, sizeof(st_PACKER_READ_DATA));
        pr->lockid = uselock;
        pr->userdata = userdata;
        pr->opts = opts;
        pr->types = typelist;
        pr->cltver = -1;
        strncpy(pr->packfile, pkgfile, sizeof(pr->packfile));

        if (tocbuf_aligned == 0)
        {
            pr->pkg = g_hiprf->create(pkgfile, 0, 0);
        }
        else
        {
            pr->pkg = g_hiprf->create(pkgfile, tocbuf_aligned, 0x8000);
        }

        if (pr->pkg)
        {
            pr->base_sector = g_hiprf->basesector(pr->pkg);
            PKR_parse_TOC(pr->pkg, pr);
            *cltver = pr->cltver;
            g_hiprf->setBypass(pr->pkg, 1, 1);
        }
        else
        {
            PKR_ReadDone(pr);
            pr = 0;
            *cltver = -1;
        }
    }
    else
    {
        pr = 0;
        *cltver = -1;
    }

    PKR_relmem(tocbuf_RAW, 1);
    return pr;
}

void PKR_ReadDone(st_PACKER_READ_DATA* pr)
{
    signed int j;
    signed int i;
    signed int lockid;
    st_PACKER_ATOC_NODE* assnode;
    st_PACKER_LTOC_NODE* laynode;
    st_XORDEREDARRAY* tmplist;

    if (pr == 0)
    {
        return;
    }

    for (i = pr->laytoc.cnt - 1; i >= 0; i--)
    {
        laynode = (st_PACKER_LTOC_NODE*)pr->laytoc.list[i];
        for (j = laynode->assref.cnt - 1; j >= 0; j--)
        {
            assnode = (st_PACKER_ATOC_NODE*)laynode->assref.list[j];
            if (assnode->typeref && assnode->typeref->assetUnloaded &&
                !(assnode->loadflag & 0x100000))
            {
                assnode->typeref->assetUnloaded(assnode->memloc, assnode->aid);
            }
        }
    }

    for (i = 0; i < pr->laytoc.cnt; i++)
    {
        laynode = (st_PACKER_LTOC_NODE*)pr->laytoc.list[i];
        if (laynode->laymem)
        {
            laynode->laymem = 0;
        }
    }

    if (pr->asscnt)
    {
        pr->pool_anode = 0;
        pr->pool_nextaidx = 0;
    }

    for (i = 0; i < pr->laytoc.cnt; i++)
    {
        laynode = (st_PACKER_LTOC_NODE*)pr->laytoc.list[i];
        XOrdDone(&laynode->assref);
    }

    XOrdDone(&pr->asstoc);
    XOrdDone(&pr->laytoc);

    for (i = 0; i < 129; i++)
    {
        tmplist = &pr->typelist[i];
        if (tmplist->max != 0)
        {
            XOrdDone(tmplist);
        }
    }

    g_hiprf->destroy(pr->pkg);
    lockid = pr->lockid;
    memset(pr, 0, sizeof(st_PACKER_READ_DATA));
    g_loadlock &= ~(1 << lockid);
}

signed int PKR_SetActive(st_PACKER_READ_DATA* pr, en_LAYER_TYPE layer)
{
    signed int rc = 1;
    signed int i;
    signed int j;
    st_PACKER_ATOC_NODE* assnode;
    st_PACKER_LTOC_NODE* laynode;

    if (pr == 0)
    {
        return 0;
    }

    for (i = 0; i < pr->laytoc.cnt; i++)
    {
        laynode = (st_PACKER_LTOC_NODE*)pr->laytoc.list[i];
        if (layer > PKR_LTYPE_DEFAULT && laynode->laytyp != layer)
        {
            continue;
        }

        for (j = 0; j < laynode->assref.cnt; j++)
        {
            assnode = (st_PACKER_ATOC_NODE*)laynode->assref.list[j];
            if (assnode->loadflag & 0x10000 || !(assnode->loadflag & 0x80000))
            {
                continue;
            }

            if (assnode->typeref && assnode->typeref->assetLoaded)
            {
                if (!assnode->typeref->assetLoaded(pr->userdata, assnode->aid, assnode->memloc,
                                                   assnode->d_size))
                {
                    rc = 0;
                }
                else
                {
                    assnode->loadflag |= 0x10000;
                }
            }
        }
    }

    return rc;
}

signed int PKR_parse_TOC(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    signed int done = 0;
    signed int is_ok = 0;
    unsigned int cid = g_hiprf->enter(pkg);

    while (cid != 0)
    {
        switch (cid)
        {
        case 'HIPA':
            done = LOD_r_HIPA(pkg, pr);
            break;
        case 'PACK':
            LOD_r_PACK(pkg, pr);
            if (done)
            {
                if (pr->asscnt > 0)
                {
                    XOrdInit(&pr->asstoc, pr->asscnt);
                    if (pr->asscnt)
                    {
                        st_PACKER_ATOC_NODE* assnode = (st_PACKER_ATOC_NODE*)PKR_getmem(
                            pr->asscnt * sizeof(st_PACKER_ATOC_NODE), 0);
                        if (assnode)
                        {
                            pr->pool_anode = assnode;
                            pr->pool_nextaidx = 0;
                        }
                    }
                }
                if (pr->laycnt > 0)
                {
                    XOrdInit(&pr->laytoc, pr->laycnt);
                }
            }
            break;
        case 'DICT':
            LOD_r_DICT(pkg, pr);
            PKR_bld_typecnt(pr);
            is_ok = 1;
            break;
        case 'STRM':
            LOD_r_STRM(pkg, pr);
            break;
        }

        if (!done)
        {
            break;
        }

        g_hiprf->exit(pkg);

        if (is_ok)
        {
            break;
        }

        cid = g_hiprf->enter(pkg);
    }

    return done;
}

signed int PKR_LoadStep_Async()
{
    signed int rc;
    signed int laysize;
    static st_PACKER_READ_DATA* curpr = 0;
    static st_PACKER_LTOC_NODE* asynlay = 0;

    if (asynlay == 0)
    {
        PKR_findNextLayerToLoad(&curpr, &asynlay);

        if (asynlay)
        {
            if (PKR_layerLoadDest(asynlay->laytyp) != PKR_LDDEST_SKIP &&
                (laysize = asynlay->laysize) > 1 && asynlay->assref.cnt > 0)
            {
                if (asynlay->laymem == 0)
                {
                    if (PKR_layerLoadDest(asynlay->laytyp) == PKR_LDDEST_RWHANDOFF)
                    {
                        asynlay->laymem = PKR_RWarenaAlloc(curpr, laysize);
                    }
                    else
                    {
                        asynlay->laymem = (char*)PKR_getmem(laysize, 0);
                    }
                }
                st_PACKER_ATOC_NODE* tmpass = (st_PACKER_ATOC_NODE*)asynlay->assref.list[0];
                g_hiprf->setSpot(tmpass->ownpkg, tmpass->d_off);

                if (g_hiprf->readBytes(tmpass->ownpkg, asynlay->laymem, asynlay->laysize))
                {
                    asynlay->flg_ldstat |= 0x1000000;
                }
                else
                {
                    if (PKR_layerLoadDest(asynlay->laytyp) == PKR_LDDEST_RWHANDOFF)
                    {
                        PKR_RWarenaFree();
                    }
                    asynlay->laymem = 0;
                    asynlay->flg_ldstat &= 0xFCFFFFFF;
                    curpr = 0;
                    asynlay = 0;
                }

                rc = 1;
            }
            else
            {
                rc = 1;
                asynlay->flg_ldstat |= 0x2000000;
                asynlay = 0;
            }
        }
        else
        {
            rc = 0;
            curpr = 0;
            asynlay = 0;
        }
    }
    else
    {
        signed int moretodo = g_hiprf->pollRead(curpr->pkg);

        if (!moretodo)
        {
            rc = 1;
            if (PKR_layerLoadDest(asynlay->laytyp) == PKR_LDDEST_RWHANDOFF)
            {
                PKR_RWarenaUse();
            }
        }
        else if (moretodo == 1)
        {
            PKR_updateLayerAssets(asynlay);

            if (PKR_layerTypeNeedsXForm(asynlay->laytyp))
            {
                PKR_xformLayerAssets(asynlay);
            }

            if (PKR_layerLoadDest(asynlay->laytyp) == PKR_LDDEST_RWHANDOFF)
            {
                PKR_RWarenaFree();
                asynlay->laymem = 0;
            }

            rc = 1;
            asynlay->flg_ldstat |= 0x2000000;
            asynlay = 0;
        }
        else
        {
            if (asynlay->laymem)
            {
                if (PKR_layerLoadDest(asynlay->laytyp) == PKR_LDDEST_RWHANDOFF)
                {
                    PKR_RWarenaFree();
                    asynlay->laymem = 0;
                }
            }

            rc = 1;
            asynlay->flg_ldstat &= 0xFCFFFFFF;
            asynlay = 0;
            curpr = 0;
        }
    }

    return rc;
}

static RwResEntry* g_RWarena_resEntry;
static RwResEntry* g_RWarena_resOwner;

#pragma auto_inline on

static char* PKR_RWarenaAlloc(st_PACKER_READ_DATA* pr, signed int size)
{
    char* mem;

    fn_800167F0();
    fn_80164200();
    g_RWarena_resEntry =
        RwResourcesAllocateResEntry(pr, &g_RWarena_resOwner, size + 0x80, PKR_special_loadbuf_killed);
    if (g_RWarena_resEntry == 0)
    {
        exit(-1);
        mem = 0;
    }
    else
    {
        mem = (char*)(((unsigned int)g_RWarena_resEntry + sizeof(RwResEntry) + 0x3F) & ~0x3F);
        memset(mem, 0, size);
    }
    return mem;
}

static void PKR_RWarenaUse()
{
    RwResEntry* resEntry = g_RWarena_resEntry;
    RwResourcesUseResEntry(resEntry);
}

static void PKR_RWarenaFree()
{
    RwResEntry* resEntry = g_RWarena_resEntry;
    g_RWarena_resOwner = 0;
    g_RWarena_resEntry = 0;
    RwResourcesFreeResEntry(resEntry);
}

#pragma auto_inline off

signed int PKR_findNextLayerToLoad(st_PACKER_READ_DATA** work_on_pkg,
                                   st_PACKER_LTOC_NODE** next_layer)
{
    st_PACKER_READ_DATA* tmppr;
    st_PACKER_LTOC_NODE* tmplay;

    *next_layer = 0;
    if (*work_on_pkg)
    {
        tmppr = *work_on_pkg;
        for (signed int i = 0; i < tmppr->laytoc.cnt; i++)
        {
            tmplay = (st_PACKER_LTOC_NODE*)tmppr->laytoc.list[i];
            if (!(tmplay->flg_ldstat & 0x2000000))
            {
                *next_layer = tmplay;
                *work_on_pkg = tmppr;
                break;
            }
        }
    }

    if (*next_layer == 0)
    {
        for (signed int i = 0; i < 8; i++)
        {
            if (!(g_loadlock & (1 << i)) || &g_readdatainst[i] == *work_on_pkg)
            {
                continue;
            }
            tmppr = &g_readdatainst[i];

            for (signed int j = 0; j < tmppr->laytoc.cnt; j++)
            {
                tmplay = (st_PACKER_LTOC_NODE*)tmppr->laytoc.list[j];
                if (!(tmplay->flg_ldstat & 0x2000000))
                {
                    *next_layer = tmplay;
                    *work_on_pkg = tmppr;
                    break;
                }
            }

            if (*next_layer)
            {
                break;
            }
        }
    }

    return *next_layer != 0;
}

void PKR_updateLayerAssets(st_PACKER_LTOC_NODE* laynode)
{
    signed int i;
    st_PACKER_ATOC_NODE* tmpass = 0;

    for (i = 0; i < laynode->assref.cnt; i++)
    {
        tmpass = (st_PACKER_ATOC_NODE*)laynode->assref.list[i];
        if (tmpass->d_off > 0 && tmpass->d_size > 0)
        {
            break;
        }
        tmpass = 0;
    }

    if (tmpass)
    {
        signed int lay_hip_pos = tmpass->d_off;
        for (i = 0; i < laynode->assref.cnt; i++)
        {
            st_PACKER_ATOC_NODE* tmpass = (st_PACKER_ATOC_NODE*)laynode->assref.list[i];
            if (!(tmpass->loadflag & 0x100000))
            {
                if (tmpass->loadflag & 0x200000)
                {
                    tmpass->memloc = 0;
                }
                else
                {
                    tmpass->memloc = laynode->laymem + (tmpass->d_off - lay_hip_pos);
                    tmpass->loadflag |= 0x80000;
                }
            }
        }
    }
}

// TODO: 98.1%. The readXForm test should branch over a "b end" (bne/b) instead of beq.
void PKR_xform_asset(st_PACKER_ATOC_NODE* assnode)
{
    if (!(assnode->infoflag & 4))
    {
        if (assnode->typeref == 0)
        {
            return;
        }
        if (assnode->typeref->readXForm != 0)
        {
        }
        else
        {
            return;
        }
    }

    st_PACKER_ASSETTYPE* atype = assnode->typeref;
    if (atype && atype->readXForm)
    {
        if (assnode->d_size == 0)
        {
            assnode->memloc = 0;
        }

        char* xformloc = (char*)atype->readXForm(assnode->ownpr->userdata, assnode->aid,
                                                 assnode->memloc, assnode->d_size,
                                                 (unsigned int*)&assnode->x_size);
        if (assnode->memloc == xformloc && assnode->x_size != 0)
        {
        }
        else if (assnode->d_size == 0 || assnode->x_size == 0)
        {
            assnode->memloc = 0;
            assnode->loadflag |= 0x200000;
        }
        else
        {
            assnode->memloc = xformloc;
        }
    }
}

signed int PKR_LoadLayer(st_PACKER_READ_DATA* pr, en_LAYER_TYPE layer)
{
    return 0;
}

void* PKR_LoadAsset(st_PACKER_READ_DATA* pr, unsigned int aid, char*, void*)
{
    st_PACKER_ATOC_NODE* assnode = 0;
    signed int idx = XOrdLookup(&pr->asstoc, (void*)aid, OrdTest_R_AssetID);

    if (!(idx < 0))
    {
        assnode = (st_PACKER_ATOC_NODE*)pr->asstoc.list[idx];
    }

    if (assnode)
    {
        return assnode->memloc;
    }
    return 0;
}

unsigned int PKR_GetAssetSize(st_PACKER_READ_DATA* pr, unsigned int aid)
{
    st_PACKER_ATOC_NODE* assnode = 0;
    signed int idx = XOrdLookup(&pr->asstoc, (void*)aid, OrdTest_R_AssetID);

    if (idx > -1)
    {
        assnode = (st_PACKER_ATOC_NODE*)pr->asstoc.list[idx];
    }

    if (assnode)
    {
        if (assnode->x_size > 0)
        {
            return assnode->x_size;
        }
        return assnode->d_size;
    }
    return 0;
}

signed int PKR_AssetCount(st_PACKER_READ_DATA* pr, unsigned int type)
{
    signed int count = 0;

    if (type == 0)
    {
        return pr->asstoc.cnt;
    }

    signed int idx = PKR_typeHdlr_idx(pr, type);
    if (idx >= 0)
    {
        st_XORDEREDARRAY* tmplist = &pr->typelist[idx];
        count = tmplist->cnt;
    }
    return count;
}

void* PKR_AssetByType(st_PACKER_READ_DATA* pr, unsigned int type, signed int idx,
                      unsigned int* size)
{
    if (size)
    {
        *size = 0;
    }

    if (idx < 0)
    {
        idx = 0;
    }

    signed int typeidx = PKR_typeHdlr_idx(pr, type);
    if (typeidx < 0)
    {
        return 0;
    }

    st_XORDEREDARRAY* tmplist = &pr->typelist[typeidx];
    if (idx >= tmplist->cnt)
    {
        return 0;
    }

    st_PACKER_ATOC_NODE* assnode = (st_PACKER_ATOC_NODE*)tmplist->list[idx];
    if (size)
    {
        *size = assnode->d_size;
    }
    return assnode->memloc;
}

signed int PKR_IsAssetReady(st_PACKER_READ_DATA* pr, unsigned int aid)
{
    signed int ready = 0;
    signed int idx = XOrdLookup(&pr->asstoc, (void*)aid, OrdTest_R_AssetID);

    if (idx >= 0)
    {
        st_PACKER_ATOC_NODE* assnode = (st_PACKER_ATOC_NODE*)pr->asstoc.list[idx];
        if (assnode->loadflag & 0x80000)
        {
            ready = 1;
        }
        else
        {
            ready = 0;
        }
    }
    return ready;
}

unsigned int PKR_getPackTimestamp(st_PACKER_READ_DATA* pr)
{
    return pr->time_made;
}

char* PKR_AssetName(st_PACKER_READ_DATA* pr, unsigned int aid)
{
    char* name = 0;
    signed int idx = XOrdLookup(&pr->asstoc, (void*)aid, OrdTest_R_AssetID);

    if (idx >= 0)
    {
        name = ((st_PACKER_ATOC_NODE*)pr->asstoc.list[idx])->name;
    }
    return name;
}

unsigned int PKR_GetBaseSector(st_PACKER_READ_DATA* pr)
{
    return pr->base_sector;
}

signed int PKR_GetAssetInfo(st_PACKER_READ_DATA* pr, unsigned int aid,
                            st_PKR_ASSET_TOCINFO* tocinfo)
{
    memset(tocinfo, 0, sizeof(st_PKR_ASSET_TOCINFO));
    signed int idx = XOrdLookup(&pr->asstoc, (void*)aid, OrdTest_R_AssetID);
    if (idx >= 0)
    {
        st_PACKER_ATOC_NODE* assnode = (st_PACKER_ATOC_NODE*)pr->asstoc.list[idx];
        tocinfo->aid = aid;
        tocinfo->sector = pr->base_sector + assnode->d_off / pkr_sector_size;
        tocinfo->plus_offset = assnode->d_off % pkr_sector_size;
        tocinfo->size = assnode->d_size;
        tocinfo->mempos = assnode->memloc;
    }
    return idx >= 0 ? 1 : 0;
}

signed int PKR_GetAssetInfoByType(st_PACKER_READ_DATA* pr, unsigned int type, signed int idx,
                                  st_PKR_ASSET_TOCINFO* tocinfo)
{
    memset(tocinfo, 0, sizeof(st_PKR_ASSET_TOCINFO));
    if (idx < 0)
    {
        idx = 0;
    }

    signed int typeidx = PKR_typeHdlr_idx(pr, type);
    if (typeidx < 0)
    {
        return 0;
    }

    st_XORDEREDARRAY* tmplist = &pr->typelist[typeidx];
    if (idx >= tmplist->cnt)
    {
        return 0;
    }

    st_PACKER_ATOC_NODE* assnode = (st_PACKER_ATOC_NODE*)tmplist->list[idx];
    tocinfo->aid = assnode->aid;
    tocinfo->sector = pr->base_sector + assnode->d_off / pkr_sector_size;
    tocinfo->plus_offset = assnode->d_off % pkr_sector_size;
    tocinfo->size = assnode->d_size;
    tocinfo->mempos = assnode->memloc;
    return 1;
}

signed int PKR_PkgHasAsset(st_PACKER_READ_DATA* pr, unsigned int aid)
{
    signed int idx = XOrdLookup(&pr->asstoc, (void*)aid, OrdTest_R_AssetID);
    if (idx < 0)
    {
        return 0;
    }

    signed int rc = 1;
    st_PACKER_ATOC_NODE* assnode = (st_PACKER_ATOC_NODE*)pr->asstoc.list[idx];
    if (assnode->loadflag & 0x100000)
    {
        rc = 0;
    }
    else if (assnode->loadflag & 0x200000)
    {
        rc = 0;
    }
    return rc;
}

signed int PKR_FRIEND_assetIsGameDup(unsigned int aid, const st_PACKER_READ_DATA* skippr,
                                      signed int oursize, unsigned int ourtype,
                                      unsigned int chksum, char*)
{
    signed int is_dup = 0;

    if (aid == 0x7AB6743A)
    {
        return 0;
    }

    for (signed int i = 0; i < 8; i++)
    {
        if (!(g_loadlock & (1 << i)) || &g_readdatainst[i] == skippr)
        {
            continue;
        }

        signed int idx = XOrdLookup(&g_readdatainst[i].asstoc, (void*)aid, OrdTest_R_AssetID);
        if (idx < 0)
        {
            continue;
        }

        st_PACKER_ATOC_NODE* tmp_ass = (st_PACKER_ATOC_NODE*)g_readdatainst[i].asstoc.list[idx];
        if (!(tmp_ass->loadflag & 0x80000) && tmp_ass->asstype != 'SND ' &&
            tmp_ass->asstype != 'SNDS')
        {
            continue;
        }

        is_dup = 1;
        break;
    }

    return is_dup;
}

signed int OrdTest_R_Asset(void* vkey, void* vitem)
{
    signed int rc;

    if (*(unsigned int*)vkey < *(unsigned int*)vitem)
    {
        rc = -1;
    }
    else if (*(unsigned int*)vkey > *(unsigned int*)vitem)
    {
        rc = 1;
    }
    else
    {
        rc = 0;
    }
    return rc;
}

signed int OrdTest_R_AssetID(void* vkey, void* vitem)
{
    signed int rc;

    if ((unsigned int)vkey < *(unsigned int*)vitem)
    {
        rc = -1;
    }
    else if ((unsigned int)vkey > *(unsigned int*)vitem)
    {
        rc = 1;
    }
    else
    {
        rc = 0;
    }
    return rc;
}

signed int LOD_r_AHDR(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    signed int ival = 0;

    g_hiprf->readLongs(pkg, &ival, 1);
    st_PACKER_ATOC_NODE* assnode = PKR_newassnode(pr, ival);
    assnode->ownpr = pr;
    assnode->ownpkg = pkg;
    XOrdAppend(&pr->asstoc, assnode);
    g_hiprf->readLongs(pkg, &ival, 1);
    assnode->asstype = ival;
    assnode->typeref = PKR_type2typeref(assnode->asstype, pr->types);
    g_hiprf->readLongs(pkg, &ival, 1);
    assnode->d_off = ival;
    g_hiprf->readLongs(pkg, &ival, 1);
    assnode->d_size = ival;
    assnode->readrem = ival;
    assnode->readcnt = 0;
    if (assnode->d_size < 1)
    {
        assnode->loadflag |= 0x200000;
    }
    g_hiprf->readLongs(pkg, &ival, 1);
    assnode->d_pad = ival;
    g_hiprf->readLongs(pkg, &ival, 1);
    assnode->infoflag = ival;

    unsigned int cid = g_hiprf->enter(pkg);
    while (cid != 0)
    {
        switch (cid)
        {
        case 'ADBG':
            LOD_r_ADBG(pkg, pr, assnode);
            break;
        }
        g_hiprf->exit(pkg);
        cid = g_hiprf->enter(pkg);
    }

    if (PKR_FRIEND_assetIsGameDup(assnode->aid, pr, assnode->d_size, assnode->asstype,
                                  assnode->d_chksum, 0))
    {
        assnode->loadflag |= 0x100000;
    }
    return 1;
}

signed int LOD_r_LHDR(st_HIPLOADDATA* pkg, st_PACKER_READ_DATA* pr)
{
    signed int i;
    signed int ival = 0;
    signed int refcnt = 0;

    g_hiprf->readLongs(pkg, &ival, 1);
    en_LAYER_TYPE laytyp = (en_LAYER_TYPE)ival;
    g_hiprf->readLongs(pkg, &refcnt, 1);
    st_PACKER_LTOC_NODE* laynode = PKR_newlaynode(laytyp, refcnt);
    XOrdAppend(&pr->laytoc, laynode);

    for (i = 0; i < refcnt; i++)
    {
        g_hiprf->readLongs(pkg, &ival, 1);
        signed int idx = XOrdLookup(&pr->asstoc, (void*)ival, OrdTest_R_AssetID);
        st_PACKER_ATOC_NODE* assnode = (st_PACKER_ATOC_NODE*)pr->asstoc.list[idx];
        XOrdAppend(&laynode->assref, assnode);
        if (i != refcnt - 1)
        {
            laynode->laysize += assnode->d_size + assnode->d_pad;
        }
        else
        {
            laynode->laysize += assnode->d_size;
        }
    }

    if (laynode->laysize > 0)
    {
        laynode->laysize = (laynode->laysize + pkr_sector_size - 1) & -pkr_sector_size;
    }

    unsigned int cid = g_hiprf->enter(pkg);
    while (cid != 0)
    {
        switch (cid)
        {
        case 'LDBG':
            LOD_r_LDBG(pkg, pr, laynode);
            break;
        }
        g_hiprf->exit(pkg);
        cid = g_hiprf->enter(pkg);
    }
    return 1;
}

void PKR_bld_typecnt(st_PACKER_READ_DATA* pr)
{
    st_PACKER_LTOC_NODE* laynode;
    st_PACKER_ATOC_NODE* assnode;
    signed int j;
    signed int i;
    signed int typcnt[129] = {};
    unsigned int lasttype = 0;
    signed int lastidx = 0;

    for (i = 0; i < pr->laytoc.cnt; i++)
    {
        laynode = (st_PACKER_LTOC_NODE*)pr->laytoc.list[i];
        for (j = 0; j < laynode->assref.cnt; j++)
        {
            assnode = (st_PACKER_ATOC_NODE*)laynode->assref.list[j];
            if (!(assnode->loadflag & 0x100000) && !(assnode->loadflag & 0x200000))
            {
                signed int idx;
                if (lasttype != 0 && assnode->asstype == lasttype)
                {
                    idx = lastidx;
                }
                else
                {
                    idx = PKR_typeHdlr_idx(pr, assnode->asstype);
                    lastidx = idx;
                    lasttype = assnode->asstype;
                }

                if (idx < 0)
                {
                    typcnt[128]++;
                }
                else
                {
                    typcnt[idx]++;
                }
            }
        }
    }

    for (i = 0; i < 129; i++)
    {
        if (typcnt[i] >= 1)
        {
            XOrdInit(&pr->typelist[i], typcnt[i] > 1 ? typcnt[i] : 2);
        }
    }

    for (i = 0; i < pr->laytoc.cnt; i++)
    {
        laynode = (st_PACKER_LTOC_NODE*)pr->laytoc.list[i];
        for (j = 0; j < laynode->assref.cnt; j++)
        {
            assnode = (st_PACKER_ATOC_NODE*)laynode->assref.list[j];
            if (!(assnode->loadflag & 0x100000) && !(assnode->loadflag & 0x200000))
            {
                signed int idx;
                if (lasttype != 0 && assnode->asstype == lasttype)
                {
                    idx = lastidx;
                }
                else
                {
                    idx = PKR_typeHdlr_idx(pr, assnode->asstype);
                    lastidx = idx;
                    lasttype = assnode->asstype;
                }

                st_XORDEREDARRAY* tmplist;
                if (idx < 0)
                {
                    tmplist = &pr->typelist[128];
                }
                else
                {
                    tmplist = &pr->typelist[idx];
                }
                XOrdAppend(tmplist, assnode);
            }
        }
    }
}

void PKR_special_loadbuf_killed(RwResEntry* resEntry)
{
}
