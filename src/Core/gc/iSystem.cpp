// GameCube system layer: RenderWare file system and memory callbacks,
// iSystemInit/Exit, date helpers and the TRC reset/disc code at the end of
// the unit. All of the unit's data lives in shared data splits, so it is
// referenced here by label.

#include "iSnd.h"

class RwResEntry
{
public:
    unsigned char pad0[0x18];
};

class xTRCState
{
public:
    signed int state; // 0x0
    signed int mode;  // 0x4
};

extern "C" {
signed int DVDGetDriveStatus();
signed int DVDCheckDisk();
void PADRecalibrate(unsigned int mask);
void GXDrawDone();
void VISetBlack(signed int black);
void VIFlush();
void VIWaitForRetrace();
signed int OSGetResetButtonState();
void OSResetSystem(signed int reset, unsigned int resetCode, signed int forceMenu);
RwResEntry* RwResourcesAllocateResEntry(void* owner, RwResEntry** ownerRef, signed int size,
                                        void* destroyNotify);
void RwResourcesFreeResEntry(RwResEntry* entry);
void __start();

extern unsigned int _stack_addr[];
extern unsigned int _stack_end[];

// TODO: unidentified (stops the movie player before a reset)
void GXAbortFrame();
// TODO: unidentified (size of the RenderWare resource arena)
signed int RwResourcesGetArenaSize();

// TODO: unidentified iSystem/xTRC globals
extern signed int lbl_8025E96C;       // last reset-button state
extern signed int lbl_8025E970;       // reset requested
extern void (*lbl_8025E974)(signed int);
extern signed int lbl_8025E978;
extern unsigned char lbl_801FE3D4[0x14];
extern xTRCState gTrcDisk;
}

enum _tagTRCState
{
    TRC_Unknown
};

// xTRCDisk returns void; its last store leaves &gTrcDisk in r3, which this
// function passes on as its result.
signed int xTRCDisk(_tagTRCState state);

class iTRCDisk
{
public:
    static signed int CheckDVDAndResetState();
};

extern "C" {
void fn_80016698(signed int toMenu);
void fn_80016730();
void fn_80016768();
unsigned int fn_80016900();
signed int fn_80016918();
}

// Functions are in reverse DOL order (-inline deferred emits last to first).

extern "C" signed int fn_80016918()
{
    return gTrcDisk.state;
}

extern "C" unsigned int fn_80016900()
{
    return (unsigned char)((&gTrcDisk)->mode == 6);
}

extern "C" unsigned char* fn_800168F4()
{
    return lbl_801FE3D4;
}

// Entry point: fills the stack with a marker, then starts the runtime.
extern "C" void fn_80016898()
{
    for (unsigned int* p = _stack_end; p < _stack_addr; p++)
    {
        *p = 0xDEADBEEF;
    }
    __start();
}

// Fills the free RenderWare resource arena with a marker.
extern "C" void fn_800167F0()
{
    signed int size = RwResourcesGetArenaSize();
    RwResEntry* entry = RwResourcesAllocateResEntry(0, 0, size, 0);

    while (!entry && size > 0)
    {
        size -= 0x400;
        entry = RwResourcesAllocateResEntry(0, 0, size, 0);
    }

    if (entry)
    {
        unsigned int* p = (unsigned int*)(entry + 1);
        for (unsigned int i = 0; i < (unsigned int)size / 4; i++)
        {
            p[i] = 0xDEADBEEF;
        }
        RwResourcesFreeResEntry(entry);
    }
}

extern "C" void fn_800167A8(signed int arg)
{
    lbl_8025E978 = arg;
    fn_80016768();
    if (lbl_8025E974)
    {
        lbl_8025E974(arg);
    }
}

extern "C" void fn_80016768()
{
    signed int pressed = OSGetResetButtonState();
    if (lbl_8025E96C == 1 && pressed == 0)
    {
        lbl_8025E970 = 1;
    }
    lbl_8025E96C = pressed;
}

extern "C" void fn_80016730()
{
    if (lbl_8025E970)
    {
        lbl_8025E970 = 0;
        fn_80016698(0);
    }
}

extern "C" void fn_80016698(signed int toMenu)
{
    iSndDIEDIEDIE();
    PADRecalibrate(0xF0000000);
    GXAbortFrame();
    GXDrawDone();
    VISetBlack(1);
    VIFlush();
    VIWaitForRetrace();

    signed int state = fn_80016918();
    if (toMenu || state == 10 || state == 7 || !fn_80016900())
    {
        OSResetSystem(1, 0, 1);
    }
    else
    {
        OSResetSystem(0, 0, 0);
    }
}

signed int iTRCDisk::CheckDVDAndResetState()
{
    signed int state;

    fn_80016768();
    fn_80016730();

    switch (DVDGetDriveStatus())
    {
    case 4:
        state = 9;
        break;
    case 6:
        state = 10;
        break;
    case 5:
        state = 7;
        break;
    case 11:
        state = 11;
        break;
    case -1:
        state = 12;
        break;
    case 0:
        state = 8;
        break;
    default:
        if (DVDCheckDisk())
        {
            state = 8;
        }
        else
        {
            state = 5;
        }
        break;
    }

    return xTRCDisk((_tagTRCState)state);
}

// ---------------------------------------------------------------------------
// Everything below sits before the TRC code in the DOL.

class RwFileFunctions
{
public:
    signed int (*rwfexist)(const char* name);
    void* (*rwfopen)(const char* name, const char* access);
    signed int (*rwfclose)(void* fptr);
    unsigned int (*rwfread)(void* addr, unsigned int size, unsigned int count, void* fptr);
    unsigned int (*rwfwrite)(const void* addr, unsigned int size, unsigned int count, void* fptr);
    char* (*rwfgets)(char* buffer, signed int maxLen, void* fptr);
    signed int (*rwfputs)(const char* buffer, void* fptr);
    signed int (*rwfeof)(void* fptr);
    signed int (*rwfseek)(void* fptr, long offset, int origin);
    signed int (*rwfflush)(void* fptr);
    signed int (*rwftell)(void* fptr);
};

class RwMemoryFunctions
{
public:
    void* (*rwmalloc)(unsigned int size);
    void (*rwfree)(void* mem);
    void* (*rwrealloc)(void* mem, unsigned int newSize);
    void* (*rwcalloc)(unsigned int numObj, unsigned int sizeObj);
};

// Per-engine globals of the file system plugin.
class dlFSGlobals
{
public:
    RwFileFunctions oldFuncs; // 0x0
    void* fileFreeList;       // 0x2C
};

class RwModuleInfo
{
public:
    signed int globalsOffset;
    signed int numInstances;
};

class DVDCommandBlock
{
public:
    unsigned char pad[0x30];
};

class DVDFileInfo
{
public:
    DVDCommandBlock cb;     // 0x0
    unsigned int startAddr; // 0x30
    unsigned int length;    // 0x34
    void* callback;         // 0x38
};

#define DLFILE_BUF_SIZE 0x2800
#ifndef NULL
#define NULL 0
#endif

class dlFile
{
public:
    unsigned char readBuffer[DLFILE_BUF_SIZE]; // 0x0
    DVDFileInfo info;                          // 0x2800
    unsigned int pos;                          // 0x283C
    unsigned int length;                       // 0x2840
    unsigned int bufPos;                       // 0x2844
    signed int readable;                       // 0x2848
};

class RwRaster
{
public:
    unsigned char pad0[0x14];
    signed int depth; // 0x14
};

class RwTexture
{
public:
    RwRaster* raster; // 0x0
    unsigned char pad4[0xC];
    char name[0x20]; // 0x10
    char mask[0x20]; // 0x30
};

class RwGameCubeRasterExt
{
public:
    unsigned char pad[0xC];
    unsigned int format; // 0xC
};

typedef struct OSCalendarTime
{
    int sec;
    int min;
    int hour;
    int mday;
    int mon;
    int year;
    int wday;
    int yday;
    int msec;
    int usec;
} OSCalendarTime;

class DeviceConfig
{
public:
    void* renderMode;
    unsigned int unk4;
    unsigned int unk8;
};

class RwEngineOpenParams
{
public:
    void* displayID;
};

extern "C" {
void* memcpy(void* dst, const void* src, unsigned long n);
void* memset(void* dst, int c, unsigned long n);
char* strcpy(char* dst, const char* src);
char* strcat(char* dst, const char* src);
unsigned long strlen(const char* s);
int sprintf(char* buf, const char* fmt, ...);

void DVDInit();
int DVDOpen(const char* fileName, DVDFileInfo* fileInfo);
int DVDClose(DVDFileInfo* fileInfo);
signed int DVDReadPrio(DVDFileInfo* fileInfo, void* addr, signed int length, signed int offset,
                       signed int prio);
void OSInit();
void VIInit();
void* VISetPreRetraceCallback(void (*cb)(signed int));
void OSPanic(const char* file, int line, const char* msg, ...);
long long OSGetTime();
void OSTicksToCalendarTime(long long ticks, OSCalendarTime* td);
void* OSAllocFromHeap(int heap, unsigned int size);
void OSFreeToHeap(int heap, void* ptr);
void GXSetMisc(signed int what, signed int value);

extern unsigned char* RwEngineInstance;
RwFileFunctions* RwOsGetFileInterface();
unsigned int RwEngineRegisterPlugin(signed int size, unsigned int pluginID, void* (*initCB)(void*, int, int),
                                    void* (*termCB)(void*, int, int));
void* RwFreeListCreate(unsigned int entrySize, unsigned int entriesPerBlock, unsigned int alignment); // RwFreeListCreate
void RwFreeListDestroy(void* freelist); // RwFreeListDestroy
signed int RwEngineInit(RwMemoryFunctions* memFuncs, unsigned int initFlags, unsigned int resArenaSize);
signed int RwResourcesSetArenaSize(unsigned int size);
signed int RwEngineOpen(RwEngineOpenParams* params);
signed int RwEngineStart();
signed int RwEngineStop();
signed int RwEngineClose();
signed int RwEngineTerm();
signed int RwEngineGetCurrentVideoMode();
void* RwEngineGetVideoModeInfo(void* modeinfo, signed int modeIndex);
signed int RwTextureSetReadCallBack(RwTexture* (*fpCallBack)(const char*, const char*));
signed int RwRenderStateSet(signed int state, void* value);
signed int RpWorldPluginAttach();
signed int RpCollisionPluginAttach();
signed int RpSkinPluginAttach();
signed int fn_8013D854(); // RpHAnimPluginAttach
signed int RpMatFXPluginAttach();
signed int RpUserDataPluginAttach();
extern signed int lbl_8025F870; // _RwGameCubeRasterExtOffset

void fn_80021B1C(); // xDebugExit
signed int fn_80015EAC(const char* fmt, ...);
void OSReport(const char* fmt, ...);
void* malloc(unsigned int size);
void free(void* ptr);
void fn_800162D0(char* elf, signed int profile);
void fn_800162D4();
void fn_800162D8();
void fn_80016218();
void fn_80015F50();
void fn_80015EA0(signed int addr, signed int size);
void fn_80015DE8(void* ptr);
signed int fn_80015A70(const char* mode);

extern "C" unsigned char GXNtsc480IntDf[];
extern DeviceConfig lbl_801FE3B8;    // deviceConfig
extern unsigned char lbl_801FE3C4[]; // sVideoMode
extern char* lbl_801AD940[12];       // month names
extern char* lbl_801AD970[7];        // weekday names
extern unsigned int lbl_8025E3F8;    // rwID_DOLPHINDEVICEMODULE
extern signed int lbl_8025E97C;      // last mem_null arguments
extern signed int lbl_8025E980;
extern signed int lbl_8025E984;      // allocate RenderWare memory from the xMem heap
extern signed int lbl_8025E988;      // open file count
extern RwModuleInfo lbl_8025E98C;    // file system module info
}

extern unsigned int mem_top_alloc;
extern unsigned int mem_base_alloc;
extern int the_heap;
extern unsigned int gActiveHeap;
void* xMemAlloc(unsigned int heapID, unsigned int size);
unsigned int xStrHash(char* str);
void* xSTFindAsset(unsigned int aid, unsigned int* size);
void xDebugInit();
void xMemInit();
void xMemExit();
void iFileInit();
void iFileExit();
void iTimeInit();
void iTimeExit();
void xPadInit();
void xPadKill();
void xSndInit();
void xSndExit();
void xMathInit();
void xMathExit();
void SetupShadow();
void xFXInit();
signed int DolphinInitMemorySystem(RwMemoryFunctions* memFuncs);

#define FSGLOBALS() ((dlFSGlobals*)(RwEngineInstance + lbl_8025E98C.globalsOffset))

void iVSync()
{
    VIWaitForRetrace();
}

static inline void iCalendarNow(OSCalendarTime* td)
{
    OSTicksToCalendarTime(OSGetTime(), td);
}

signed int iGetDay()
{
    OSCalendarTime td;
    iCalendarNow(&td);
    return td.mday;
}

signed int iGetMonth()
{
    OSCalendarTime td;
    iCalendarNow(&td);
    return td.mon + 1;
}

unsigned int iGetCurrFormattedDate(char* str)
{
    char* start = str;
    OSCalendarTime td;

    iCalendarNow(&td);
    strcpy(str, lbl_801AD970[td.wday]);
    strcat(str, lbl_801AD940[td.mon]);
    str += strlen(str);
    if (td.mday >= 10)
    {
        *str++ = (td.mday / 10) + '0';
    }
    *str++ = (td.mday % 10) + '0';
    *str++ = ',';
    *str++ = ' ';
    *str++ = (td.year / 1000) + '0';
    *str++ = ((td.year / 100) % 10) + '0';
    *str++ = ((td.year / 10) % 100) + '0';
    *str++ = (td.year % 10) + '0';
    *str++ = '\0';
    return str - start;
}

unsigned int iGetCurrFormattedTime(char* str)
{
    char* start = str;
    signed int am = 0;
    OSCalendarTime td;

    iCalendarNow(&td);
    if (td.hour < 12)
    {
        am = 1;
    }
    else
    {
        td.hour -= 12;
    }
    if (td.hour == 0)
    {
        td.hour = 12;
    }
    if (td.hour >= 10)
    {
        *str++ = (td.hour / 10) + '0';
    }
    *str++ = (td.hour % 10) + '0';
    *str++ = ':';
    *str++ = (td.min / 10) + '0';
    *str++ = (td.min % 10) + '0';
    *str++ = ':';
    *str++ = (td.sec / 10) + '0';
    *str++ = (td.sec % 10) + '0';
    *str++ = ' ';
    *str++ = am ? 'A' : 'P';
    *str++ = '.';
    *str++ = 'M';
    *str++ = '.';
    *str++ = '\0';
    return str - start;
}

// Debug hooks, empty in retail.
extern "C" void fn_800162D8()
{
}

extern "C" void fn_800162D4()
{
}

extern "C" void fn_800162D0(char* elf, signed int profile)
{
}

static signed int RenderWareInit();

// iSystemInit
extern "C" void fn_80016218()
{
    lbl_801FE3B8.renderMode = GXNtsc480IntDf;
    lbl_801FE3B8.unk4 = 0;
    lbl_801FE3B8.unk8 = 0x40000;
    GXSetMisc(1, 8);
    OSInit();
    DVDInit();
    VIInit();
    if (!DolphinInitMemorySystem((RwMemoryFunctions*)lbl_801FE3D4))
    {
        OSPanic("iSystem.cpp", 0x216, "Unable to initialize memory system.\n");
    }
    xDebugInit();
    xMemInit();
    iFileInit();
    iTimeInit();
    xPadInit();
    xSndInit();
    RenderWareInit();
    xMathInit();
    lbl_8025E974 = (void (*)(signed int))VISetPreRetraceCallback(fn_800167A8);
}

static signed int RenderWareExit();

void iSystemExit()
{
    fn_80021B1C();
    xMathExit();
    RenderWareExit();
    xSndExit();
    xPadKill();
    iFileExit();
    iTimeExit();
    xMemExit();
    OSPanic("iSystem.cpp", 0x25d, "(With apologies to Jim Morrison) This the end, my only friend, The End.");
}

// Returns nonzero if a plugin failed to attach.
static unsigned int RWAttachPlugins()
{
    if (!RpWorldPluginAttach())
    {
        return 1;
    }
    if (!RpCollisionPluginAttach())
    {
        return 1;
    }
    if (!RpSkinPluginAttach())
    {
        return 1;
    }
    if (!fn_8013D854())
    {
        return 1;
    }
    if (!RpMatFXPluginAttach())
    {
        return 1;
    }
    return !RpUserDataPluginAttach();
}

static RwTexture* TextureRead(const char* name, const char* maskName);
static signed int DolphinInstallFileSystem();

static signed int RenderWareInit()
{
    RwEngineOpenParams params;

    if (!RwEngineInit((RwMemoryFunctions*)fn_800168F4(), 1, 0x428000))
    {
        return 1;
    }
    RwResourcesSetArenaSize(0x428000);
    if (!DolphinInstallFileSystem())
    {
        return 1;
    }
    if (RWAttachPlugins())
    {
        return 1;
    }
    params.displayID = &lbl_801FE3B8;
    if (!RwEngineOpen(&params))
    {
        RwEngineTerm();
        return 1;
    }
    RwEngineGetVideoModeInfo(lbl_801FE3C4, RwEngineGetCurrentVideoMode());
    if (!RwEngineStart())
    {
        RwEngineClose();
        RwEngineTerm();
        return 1;
    }
    RwTextureSetReadCallBack(TextureRead);
    RwRenderStateSet(0x14, (void*)2);
    SetupShadow();
    xFXInit();
    return 0;
}

static signed int RenderWareExit()
{
    RwEngineStop();
    RwEngineClose();
    return RwEngineTerm();
}

static RwTexture* TextureRead(const char* name, const char* maskName)
{
    char buf[0x100];
    unsigned int size;
    RwTexture* tex;

    sprintf(buf, "%s.rw3", name);
    tex = (RwTexture*)xSTFindAsset(xStrHash(buf), &size);
    if (tex && tex->raster && tex->raster->depth < 8)
    {
        RwGameCubeRasterExt* ext = (RwGameCubeRasterExt*)((unsigned char*)tex->raster + lbl_8025F870);
        if (!ext || ext->format != 0xE)
        {
            return NULL;
        }
    }
    if (tex)
    {
        strcpy(tex->name, name);
        strcpy(tex->mask, maskName);
        return tex;
    }
    return NULL;
}

// null_func: a place to break on bad frees.
extern "C" void fn_80015F50()
{
    mem_base_alloc += 4;
}

// The retail OSReport prints nothing.
extern "C" void OSReport(const char* fmt, ...)
{
}

// Debug printf, compiled out.
extern "C" signed int fn_80015EAC(const char* fmt, ...)
{
    return 0;
}

// mem_null
extern "C" void fn_80015EA0(signed int addr, signed int size)
{
    lbl_8025E97C = addr;
    lbl_8025E980 = size;
}

extern "C" void* malloc(unsigned int size)
{
    return OSAllocFromHeap(the_heap, size);
}

extern "C" void free(void* ptr)
{
    if (ptr)
    {
        OSFreeToHeap(the_heap, ptr);
    }
}

// Blocks carry a 0x20-byte header: the size, then seven marker words.
// 0x1EE7D00D marks blocks taken from the xMem heap; those are released
// with the heap, not freed one by one.
#define RWMEM_XMEM 0x1EE7D00D
#define RWMEM_OS 0xDEADBEEF

// _rwDolphinHeapFree
extern "C" void fn_80015DE8(void* ptr)
{
    unsigned int mark;

    if (ptr)
    {
        if ((unsigned int)ptr < mem_top_alloc)
        {
            fn_80015F50();
        }
        mark = ((unsigned int*)ptr)[-7];
        if (mark != RWMEM_XMEM)
        {
            OSFreeToHeap(the_heap, (unsigned char*)ptr - 0x20);
        }
        else
        {
            fn_80015EA0((signed int)ptr, mark);
        }
    }
}

static void* _rwDolphinHeapAlloc(unsigned int size)
{
    unsigned int* block;
    unsigned int mark;
    signed int i;

    if (lbl_8025E984)
    {
        block = (unsigned int*)xMemAlloc(gActiveHeap, size + 0x20);
    }
    else
    {
        block = (unsigned int*)OSAllocFromHeap(the_heap, size + 0x20);
    }
    if (!block)
    {
        return NULL;
    }
    block[0] = size;
    mark = lbl_8025E984 ? RWMEM_XMEM : RWMEM_OS;
    for (i = 1; i < 8; i++)
    {
        block[i] = mark;
    }
    return block + 8;
}

static void* _rwDolphinHeapCalloc(unsigned int num, unsigned int size)
{
    unsigned int total = num * size;
    void* mem = _rwDolphinHeapAlloc(total);
    if (mem)
    {
        memset(mem, 0, total);
    }
    return mem;
}

static void* _rwDolphinHeapRealloc(void* mem, unsigned int newSize)
{
    unsigned int oldSize = mem ? ((unsigned int*)mem)[-8] : 0;
    void* dst;

    // Blocks from the xMem heap can't be resized.
    if (oldSize && ((unsigned int*)mem)[-7] == RWMEM_XMEM)
    {
        fn_80015EA0((signed int)mem, oldSize);
        return NULL;
    }
    dst = _rwDolphinHeapAlloc(newSize);
    if (dst)
    {
        if (oldSize < newSize)
        {
            memcpy(dst, mem, oldSize);
        }
        else
        {
            memcpy(dst, mem, newSize);
        }
        fn_80015DE8(mem);
    }
    return dst;
}

signed int DolphinInitMemorySystem(RwMemoryFunctions* memFuncs)
{
    memFuncs->rwmalloc = _rwDolphinHeapAlloc;
    memFuncs->rwfree = fn_80015DE8;
    memFuncs->rwrealloc = _rwDolphinHeapRealloc;
    memFuncs->rwcalloc = _rwDolphinHeapCalloc;
    return 1;
}

static inline signed int dlModeIs(const char* mode, char c)
{
    return mode[0] == c && (mode[1] == '\0' || (mode[1] == 'b' && mode[2] == '\0'));
}

static inline signed int dlModeIsPlus(const char* mode, char c)
{
    return mode[0] == c && mode[1] == '+' && (mode[2] == '\0' || (mode[2] == 'b' && mode[3] == '\0'));
}

// dlAccessToMode
extern "C" signed int fn_80015A70(const char* mode)
{
    if (!mode)
    {
        return 0;
    }
    if (dlModeIs(mode, 'r'))
    {
        return 1;
    }
    if (dlModeIs(mode, 'w'))
    {
        return 0xE;
    }
    if (dlModeIs(mode, 'a'))
    {
        return 6;
    }
    if (dlModeIsPlus(mode, 'r'))
    {
        return 3;
    }
    if (dlModeIsPlus(mode, 'w'))
    {
        return 0xF;
    }
    if (dlModeIsPlus(mode, 'a'))
    {
        return 7;
    }
    return 0;
}

typedef void* (*RwFreeListAllocFn)(void* freelist);
typedef void (*RwFreeListFreeFn)(void* freelist, void* mem);
#define RWENGINE_FREELISTALLOC(fl) ((*(RwFreeListAllocFn*)(RwEngineInstance + 0x140))(fl))
#define RWENGINE_FREELISTFREE(fl, p) ((*(RwFreeListFreeFn*)(RwEngineInstance + 0x144))(fl, p))

static void* dlFopen(const char* name, const char* access)
{
    signed int mode = fn_80015A70(access);
    dlFile* fp;

    if (!mode)
    {
        return NULL;
    }
    fp = (dlFile*)RWENGINE_FREELISTALLOC(FSGLOBALS()->fileFreeList);
    if (!fp)
    {
        return NULL;
    }
    fp->readable = 0;
    if (mode == 1)
    {
        fp->readable = 1;
    }
    if (!fp->readable)
    {
        RWENGINE_FREELISTFREE(FSGLOBALS()->fileFreeList, fp);
        return NULL;
    }
    if (!DVDOpen(name, &fp->info))
    {
        RWENGINE_FREELISTFREE(FSGLOBALS()->fileFreeList, fp);
        return NULL;
    }
    fp->length = fp->info.length;
    fp->pos = 0;
    fp->bufPos = DLFILE_BUF_SIZE;
    lbl_8025E988++;
    return fp;
}

static signed int dlFclose(void* fptr)
{
    dlFile* fp = (dlFile*)fptr;

    if (fp && lbl_8025E988 && DVDClose(&fp->info))
    {
        RWENGINE_FREELISTFREE(FSGLOBALS()->fileFreeList, fp);
        lbl_8025E988--;
        return 0;
    }
    return -1;
}

static signed int dlFexist(const char* name)
{
    void* fp = RwOsGetFileInterface()->rwfopen(name, "r");
    if (fp)
    {
        RwOsGetFileInterface()->rwfclose(fp);
        return 1;
    }
    return 0;
}

static unsigned int dlFread(void* addr, unsigned int size, unsigned int count, void* fptr)
{
    dlFile* fp = (dlFile*)fptr;
    unsigned int want = size * count;
    unsigned int bytesRead = 0;
    unsigned int rest;

    if (fp->pos + want > fp->length)
    {
        want = fp->length - fp->pos;
    }

    // Whatever is left in the buffer first.
    if (fp->bufPos < DLFILE_BUF_SIZE && want > 0)
    {
        unsigned int buffered = DLFILE_BUF_SIZE - fp->bufPos;
        if (want < buffered)
        {
            buffered = want;
        }
        bytesRead = buffered;
        memcpy(addr, fp->readBuffer + fp->bufPos, buffered);
        addr = (char*)addr + buffered;
        fp->bufPos += buffered;
        fp->pos += buffered;
    }

    rest = want - bytesRead;
    if (rest == 0)
    {
        return bytesRead / size;
    }

    if (rest >= DLFILE_BUF_SIZE)
    {
        if (!((unsigned int)addr & 0x1F) && !(rest & 0x1F))
        {
            // Aligned: read straight into the destination.
            signed int got = DVDReadPrio(&fp->info, addr, rest, fp->pos, 2);
            rest = got < 0 ? 0 : got;
        }
        else
        {
            signed int left = want - bytesRead;
            signed int dvdpos = fp->pos;
            signed int loops = left / DLFILE_BUF_SIZE + 1;

            rest = 0;
            while (loops-- != 0)
            {
                signed int chunk = DLFILE_BUF_SIZE;
                signed int avail = fp->length - (fp->pos + rest);
                if (avail <= DLFILE_BUF_SIZE)
                {
                    chunk = avail;
                }
                if (DVDReadPrio(&fp->info, fp->readBuffer, (chunk + 0x1F) & ~0x1F, dvdpos, 2) == -1)
                {
                    return bytesRead + rest;
                }
                if (left >= chunk)
                {
                    memcpy(addr, fp->readBuffer, chunk);
                    left -= chunk;
                    rest += chunk;
                    dvdpos += chunk;
                    addr = (char*)addr + chunk;
                }
                else
                {
                    memcpy(addr, fp->readBuffer, left);
                    fp->bufPos = left;
                    rest += left;
                    dvdpos += left;
                }
            }
        }
    }
    else
    {
        signed int fill = (fp->length - fp->pos + 0x1F) & ~0x1F;
        if (fill > DLFILE_BUF_SIZE)
        {
            fill = DLFILE_BUF_SIZE;
        }
        if (DVDReadPrio(&fp->info, fp->readBuffer, fill, fp->pos, 2) == -1)
        {
            return bytesRead;
        }
        memcpy(addr, fp->readBuffer, rest);
        fp->bufPos = rest;
    }
    bytesRead += rest;
    fp->pos += rest;
    return bytesRead / size;
}

// dlFwrite: read-only file system.
extern "C" unsigned int fn_80015650(const void* addr, unsigned int size, unsigned int count, void* fptr)
{
    return 0;
}

static signed int dlFseek(void* fptr, long offset, int origin)
{
    dlFile* fp = (dlFile*)fptr;
    unsigned int oldPos = fp->pos;
    signed int delta;
    unsigned int bufStart;
    unsigned int fill;

    switch (origin)
    {
    case 1:
        fp->pos = oldPos + offset;
        delta = offset;
        break;
    case 2:
        fp->pos = fp->length + offset;
        delta = fp->pos - oldPos;
        break;
    case 0:
        fp->pos = offset;
        delta = offset - oldPos;
        break;
    default:
        return -1;
    }

    // Still inside the buffer?
    if ((signed int)(fp->bufPos + delta) >= 0 && fp->bufPos + delta <= DLFILE_BUF_SIZE && fp->pos <= fp->length)
    {
        fp->bufPos += delta;
        return 0;
    }

    if (fp->pos > fp->length)
    {
        fp->pos = oldPos;
        return -1;
    }

    bufStart = (fp->pos / DLFILE_BUF_SIZE) * DLFILE_BUF_SIZE;
    fill = DLFILE_BUF_SIZE;
    if (fp->length - bufStart <= DLFILE_BUF_SIZE)
    {
        fill = (fp->length - bufStart + 0x1F) & ~0x1F;
    }
    if (DVDReadPrio(&fp->info, fp->readBuffer, fill, bufStart, 2) == -1)
    {
        fp->pos = oldPos;
        return -1;
    }
    fp->bufPos = fp->pos - bufStart;
    return 0;
}

static char* dlFgets(char* buffer, signed int maxLen, void* fptr)
{
    dlFile* fp = (dlFile*)fptr;
    signed int i = 0;
    signed int numRead = dlFread(buffer, 1, maxLen - 1, fp);

    if (numRead == 0)
    {
        return NULL;
    }
    while (i < numRead)
    {
        if (buffer[i] == '\n')
        {
            i++;
            buffer[i] = '\0';
            dlFseek(fp, i - numRead, 1);
            return buffer;
        }
        if (buffer[i] == '\r')
        {
            if (i < numRead - 1 && buffer[i + 1] == '\n')
            {
                memcpy(&buffer[i], &buffer[i + 1], numRead - i - 1);
                numRead--;
            }
            else
            {
                i++;
            }
        }
        else
        {
            i++;
        }
    }
    if (numRead < maxLen && fp->pos == fp->length)
    {
        buffer[numRead] = '\0';
    }
    return buffer;
}

// dlFputs
extern "C" signed int fn_800153B8(const char* buffer, void* fptr)
{
    return -1;
}

// dlFeof
extern "C" signed int fn_8001539C(void* fptr)
{
    dlFile* fp = (dlFile*)fptr;
    return (unsigned char)(fp->pos >= fp->length);
}

// dlFflush
extern "C" signed int fn_80015394(void* fptr)
{
    return 0;
}

// dlFtell
extern "C" signed int fn_8001538C(void* fptr)
{
    return ((dlFile*)fptr)->pos;
}

static void* _rwDolphinFSOpen(void* instance, int offset, int size)
{
    RwFileFunctions* funcs;

    lbl_8025E98C.globalsOffset = offset;
    FSGLOBALS()->fileFreeList = RwFreeListCreate(sizeof(dlFile), 5, 0x20);
    if (!FSGLOBALS()->fileFreeList)
    {
        return NULL;
    }
    funcs = RwOsGetFileInterface();
    FSGLOBALS()->oldFuncs = *funcs;
    funcs->rwfexist = dlFexist;
    funcs->rwfopen = dlFopen;
    funcs->rwfclose = dlFclose;
    funcs->rwfread = dlFread;
    funcs->rwfwrite = fn_80015650;
    funcs->rwfgets = dlFgets;
    funcs->rwfputs = fn_800153B8;
    funcs->rwfeof = fn_8001539C;
    funcs->rwfseek = dlFseek;
    funcs->rwfflush = fn_80015394;
    funcs->rwftell = fn_8001538C;
    lbl_8025E98C.numInstances++;
    return instance;
}

static void* _rwDolphinFSClose(void* instance, int offset, int size)
{
    *RwOsGetFileInterface() = FSGLOBALS()->oldFuncs;
    RwFreeListDestroy(FSGLOBALS()->fileFreeList);
    lbl_8025E98C.numInstances--;
    return instance;
}

static signed int DolphinInstallFileSystem()
{
    DVDInit();
    return (RwEngineRegisterPlugin(0x30, lbl_8025E3F8, _rwDolphinFSOpen, _rwDolphinFSClose) >> 31) ^ 1;
}
