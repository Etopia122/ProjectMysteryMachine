// RenderWare engine: globals, plugin registry, memory, free lists, resources.
#include "rwcore.h"
#include "pc_platform.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <vector>

extern "C" {
RwUInt8* RwEngineInstance;

// Module info the game reads directly (decomp names: lbl_8025F798 = resources
// module, lbl_8025E98C = the game's Dolphin file system module, lbl_8025E3F8 = its plugin id).
RwModuleInfo lbl_8025F798;
RwModuleInfo lbl_8025E98C;
RwUInt32 lbl_8025E3F8[2] = { 0x0000FFFE, 0 };
}

void pc_rw_device_open(void* displayID);
void pc_rw_device_close();

void* rwMalloc(size_t size) { return malloc(size ? size : 1); }
void* rwCalloc(size_t n, size_t size) { return calloc(n ? n : 1, size ? size : 1); }
void rwFree(void* p) { free(p); }

// ---------------------------------------------------------------- engine plugins
struct EnginePlugin
{
    RwInt32 size;
    RwUInt32 id;
    void* (*ctor)(void*, int, int);
    void* (*dtor)(void*, int, int);
    RwInt32 offset;
};
static std::vector<EnginePlugin> s_plugins;

// Layout of the engine instance: RwGlobals, then our resources globals, then plugins.
struct rwResourcesGlobals
{
    RwUInt8 pad[0x24];
    RwLinkList* usedEntries; // read by the game (xpkrsvc, iFMV)
    RwLinkList entries;
    RwUInt32 arenaSize;
    RwUInt32 arenaUsed;
};
static const RwInt32 RESOURCES_OFFSET = (sizeof(RwGlobals) + 15) & ~15;
static RwInt32 s_nextOffset = RESOURCES_OFFSET + ((sizeof(rwResourcesGlobals) + 15) & ~15);
static const RwInt32 INSTANCE_SIZE = 0x4000;

static rwResourcesGlobals* resGlobals() { return (rwResourcesGlobals*)(RwEngineInstance + RESOURCES_OFFSET); }

// ---------------------------------------------------------------- free lists (RwFreeList)
struct PcFreeList { RwUInt32 entrySize; RwUInt32 alignment; };

static void* freelist_alloc(void* fl)
{
    PcFreeList* f = (PcFreeList*)fl;
    return _aligned_malloc(f->entrySize ? f->entrySize : 1, f->alignment < 8 ? 8 : f->alignment);
}

static void* freelist_free(void* fl, void* p)
{
    _aligned_free(p);
    return fl;
}

extern "C" {

void* RwFreeListCreate(unsigned int entrySize, unsigned int entriesPerBlock, unsigned int alignment)
{
    PcFreeList* f = (PcFreeList*)rwCalloc(1, sizeof(PcFreeList));
    f->entrySize = entrySize;
    f->alignment = alignment ? alignment : 4;
    return f;
}

void RwFreeListDestroy(void* freelist) { rwFree(freelist); }

// ---------------------------------------------------------------- string functions
static int rw_stricmp(const char* a, const char* b) { return strcasecmp(a, b); }
static char* rw_strupr(char* s) { for (char* p = s; *p; p++) *p = (char)toupper((unsigned char)*p); return s; }
static char* rw_strlwr(char* s) { for (char* p = s; *p; p++) *p = (char)tolower((unsigned char)*p); return s; }

// ---------------------------------------------------------------- default (host) file functions
static RwInt32 host_fexist(const char* name) { FILE* f = fopen(name, "rb"); if (f) fclose(f); return f != NULL; }
static void* host_fopen(const char* name, const char* mode) { return fopen(name, mode); }
static RwInt32 host_fclose(void* f) { return fclose((FILE*)f); }
static RwUInt32 host_fread(void* a, RwUInt32 s, RwUInt32 c, void* f) { return fread(a, s, c, (FILE*)f); }
static RwUInt32 host_fwrite(const void* a, RwUInt32 s, RwUInt32 c, void* f) { return fwrite(a, s, c, (FILE*)f); }
static char* host_fgets(char* b, RwInt32 n, void* f) { return fgets(b, n, (FILE*)f); }
static RwInt32 host_fputs(const char* b, void* f) { return fputs(b, (FILE*)f); }
static RwInt32 host_feof(void* f) { return feof((FILE*)f); }
static RwInt32 host_fseek(void* f, long o, int w) { return fseek((FILE*)f, o, w); }
static RwInt32 host_fflush(void* f) { return fflush((FILE*)f); }
static RwInt32 host_ftell(void* f) { return ftell((FILE*)f); }

RwInt32 RwEngineInit(RwMemoryFunctions* memFuncs, RwUInt32 initFlags, RwUInt32 resArenaSize)
{
    if (!RwEngineInstance)
    {
        RwEngineInstance = (RwUInt8*)rwCalloc(1, INSTANCE_SIZE);
    }
    RwGlobals* g = rwGlobals();
    rwLinkListInitialize(&g->dirtyFrameList);
    g->memoryAlloc = freelist_alloc;
    g->memoryFree = freelist_free;
    g->resArenaInitSize = resArenaSize;
    if (memFuncs)
    {
        g->memoryFuncs = *memFuncs;
    }

    RwFileFunctions& f = g->fileFuncs;
    f.rwfexist = host_fexist; f.rwfopen = host_fopen; f.rwfclose = host_fclose; f.rwfread = host_fread;
    f.rwfwrite = host_fwrite; f.rwfgets = host_fgets; f.rwfputs = host_fputs; f.rwfeof = host_feof;
    f.rwfseek = host_fseek; f.rwfflush = host_fflush; f.rwftell = host_ftell;

    RwStringFunctions& s = g->stringFuncs;
    s.vecSprintf = (void*)sprintf; s.vecVsprintf = (void*)vsprintf;
    s.vecStrcpy = strcpy; s.vecStrncpy = strncpy; s.vecStrcat = strcat; s.vecStrncat = strncat;
    s.vecStrrchr = (char* (*)(const char*, int))strrchr; s.vecStrchr = (char* (*)(const char*, int))strchr;
    s.vecStrstr = (char* (*)(const char*, const char*))strstr; s.vecStrcmp = strcmp; s.vecStricmp = rw_stricmp;
    s.vecStrlen = strlen; s.vecStrUpr = rw_strupr; s.vecStrLwr = rw_strlwr; s.vecStrtok = strtok;
    s.vecSscanf = (void*)sscanf;

    rwResourcesGlobals* r = resGlobals();
    rwLinkListInitialize(&r->entries);
    r->usedEntries = &r->entries;
    r->arenaSize = resArenaSize;
    lbl_8025F798.globalsOffset = RESOURCES_OFFSET;
    lbl_8025F798.numInstances = 1;
    return 1;
}

RwUInt32 RwEngineRegisterPlugin(RwInt32 size, RwUInt32 pluginID, void* (*initCB)(void*, int, int),
                                void* (*termCB)(void*, int, int))
{
    EnginePlugin p;
    p.size = size;
    p.id = pluginID;
    p.ctor = initCB;
    p.dtor = termCB;
    p.offset = s_nextOffset;
    s_nextOffset += (size + 15) & ~15;
    if (s_nextOffset > INSTANCE_SIZE)
    {
        pc_log("[rw] engine plugin space exhausted");
        return (RwUInt32)-1;
    }
    s_plugins.push_back(p);
    return (RwUInt32)p.offset;
}

RwInt32 RwEngineOpen(void* params)
{
    for (size_t i = 0; i < s_plugins.size(); i++)
    {
        if (s_plugins[i].ctor &&
            !s_plugins[i].ctor(RwEngineInstance, s_plugins[i].offset, s_plugins[i].size))
        {
            pc_log("[rw] engine plugin %08X failed to open", s_plugins[i].id);
            return 0;
        }
    }
    pc_rw_device_open(params ? *(void**)params : NULL);
    return 1;
}

RwInt32 RwEngineStart() { rwGlobals()->engineStatus = 3; return 1; }
RwInt32 RwEngineStop() { rwGlobals()->engineStatus = 2; return 1; }

RwInt32 RwEngineClose()
{
    for (size_t i = s_plugins.size(); i-- > 0;)
    {
        if (s_plugins[i].dtor)
        {
            s_plugins[i].dtor(RwEngineInstance, s_plugins[i].offset, s_plugins[i].size);
        }
    }
    pc_rw_device_close();
    return 1;
}

RwInt32 RwEngineTerm() { return 1; }

RwInt32 RwEngineGetCurrentVideoMode() { return 0; }

struct RwVideoMode { RwInt32 width, height, depth, flags; };
void* RwEngineGetVideoModeInfo(void* modeinfo, RwInt32 modeIndex)
{
    RwVideoMode* m = (RwVideoMode*)modeinfo;
    m->width = 640;
    m->height = 480;
    m->depth = 32;
    m->flags = 0;
    return m;
}

RwFileFunctions* RwOsGetFileInterface() { return &rwGlobals()->fileFuncs; }

// ---------------------------------------------------------------- resources arena
RwInt32 RwResourcesSetArenaSize(RwUInt32 size)
{
    resGlobals()->arenaSize = size;
    return 1;
}

RwInt32 RwResourcesGetArenaSize() { return (RwInt32)resGlobals()->arenaSize; }

RwResEntry* RwResourcesAllocateResEntry(void* owner, RwResEntry** ownerRef, RwInt32 size,
                                        void (*destroyNotify)(RwResEntry*))
{
    RwResEntry* e = (RwResEntry*)rwCalloc(1, sizeof(RwResEntry) + size);
    e->size = size;
    e->owner = owner;
    e->ownerRef = ownerRef;
    e->destroyNotify = destroyNotify;
    rwLinkListAddLLLink(resGlobals()->usedEntries, &e->link);
    if (ownerRef)
    {
        *ownerRef = e;
    }
    return e;
}

void RwResourcesFreeResEntry(RwResEntry* entry)
{
    if (!entry)
    {
        return;
    }
    if (entry->destroyNotify)
    {
        entry->destroyNotify(entry);
    }
    if (entry->link.next)
    {
        rwLinkListRemoveLLLink(&entry->link);
    }
    if (entry->ownerRef)
    {
        *entry->ownerRef = NULL;
    }
    rwFree(entry);
}

// Plugin attach calls: our implementations of these plugins are built in.
RwInt32 RpWorldPluginAttach() { return 1; }
RwInt32 RpCollisionPluginAttach() { return 1; }
RwInt32 RpSkinPluginAttach() { return 1; }
RwInt32 RpHAnimPluginAttach() { return 1; }
RwInt32 RpMatFXPluginAttach() { return 1; }
RwInt32 RpUserDataPluginAttach() { return 1; }

}
