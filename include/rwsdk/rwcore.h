#ifndef RWSDK_RWCORE_H
#define RWSDK_RWCORE_H

// Minimal RenderWare 3.x core declarations for decompiling the GameCube RW
// libraries, written from the PS2 DWARF layouts and the GameCube code (not
// from RenderWare's own headers). Extend as more of the library is decompiled.

#ifdef __cplusplus
extern "C" {
#endif

typedef int RwInt32;
typedef unsigned int RwUInt32;
typedef short RwInt16;
typedef unsigned short RwUInt16;
typedef signed char RwInt8;
typedef unsigned char RwUInt8;
typedef float RwReal;
typedef RwInt32 RwBool;

#ifndef NULL
#define NULL 0
#endif
#define TRUE 1
#define FALSE 0

typedef struct RwV3d {
    RwReal x;
    RwReal y;
    RwReal z;
} RwV3d;

typedef struct RwBBox {
    RwV3d sup;
    RwV3d inf;
} RwBBox;

typedef struct RwLine {
    RwV3d start;
    RwV3d end;
} RwLine;

typedef struct RwStream RwStream;
typedef struct RwFreeList RwFreeList;

// Doubly linked lists
typedef struct RwLLLink {
    struct RwLLLink* next;
    struct RwLLLink* prev;
} RwLLLink;

typedef struct RwLinkList {
    RwLLLink link;
} RwLinkList;

#define rwLinkListInitialize(list) ((list)->link.next = (RwLLLink*)(list), (list)->link.prev = (RwLLLink*)(list))
#define rwLinkListGetFirstLLLink(list) ((list)->link.next)
#define rwLinkListGetTerminator(list) (&((list)->link))
#define rwLinkListAddLLLink(list, linkvar)          \
    do {                                            \
        (linkvar)->next = (list)->link.next;        \
        (linkvar)->prev = &(list)->link;            \
        ((list)->link.next)->prev = (linkvar);      \
        (list)->link.next = (linkvar);              \
    } while (0)
#define rwLLLinkRemove(linkvar)                     \
    do {                                            \
        ((linkvar)->prev)->next = (linkvar)->next;  \
        ((linkvar)->next)->prev = (linkvar)->prev;  \
    } while (0)
#define rwLLLinkGetNext(linkvar) ((linkvar)->next)
#define rwLLLinkGetData(linkvar, type, entry) ((type*)(((RwUInt8*)(linkvar)) - offsetof(type, entry)))

#ifndef offsetof
#define offsetof(type, member) ((RwUInt32) & (((type*)0)->member))
#endif

// Objects
typedef struct RwObject {
    RwUInt8 type;
    RwUInt8 subType;
    RwUInt8 flags;
    RwUInt8 privateFlags;
    void* parent;
} RwObject;

#define rwObjectInitialize(o, t, s)                 \
    do {                                            \
        ((RwObject*)(o))->type = (RwUInt8)(t);      \
        ((RwObject*)(o))->subType = (RwUInt8)(s);   \
        ((RwObject*)(o))->flags = 0;                \
        ((RwObject*)(o))->privateFlags = 0;         \
        ((RwObject*)(o))->parent = NULL;            \
    } while (0)
#define rwObjectGetParent(o) (((const RwObject*)(o))->parent)
#define rwObjectSetParent(o, p) (((RwObject*)(o))->parent) = (void*)(p)
#define rwObjectGetPrivateFlags(o) (((const RwObject*)(o))->privateFlags)
#define rwObjectSetPrivateFlags(o, f) (((RwObject*)(o))->privateFlags = (RwUInt8)(f))

// Matrices
typedef struct RwMatrix {
    RwV3d right;
    RwUInt32 flags;
    RwV3d up;
    RwUInt32 pad1;
    RwV3d at;
    RwUInt32 pad2;
    RwV3d pos;
    RwUInt32 pad3;
} RwMatrix;

typedef enum RwOpCombineType {
    rwCOMBINEREPLACE,
    rwCOMBINEPRECONCAT,
    rwCOMBINEPOSTCONCAT,
} RwOpCombineType;

#define rwMATRIXTYPEORTHONORMAL 0x00000003
#define rwMATRIXINTERNALIDENTITY 0x00020000

RwMatrix* RwMatrixTransform(RwMatrix* matrix, const RwMatrix* transform, RwOpCombineType combineOp);
RwMatrix* RwMatrixRotate(RwMatrix* matrix, const RwV3d* axis, RwReal angle, RwOpCombineType combineOp);
RwMatrix* RwMatrixOrthoNormalize(RwMatrix* dst, const RwMatrix* src);

// Frames
typedef struct RwFrame {
    RwObject object;
    RwLLLink inDirtyListLink;
    RwMatrix modelling;
    RwMatrix ltm;
    RwLinkList objectList;
    struct RwFrame* child;
    struct RwFrame* next;
    struct RwFrame* root;
} RwFrame;

typedef struct RwObjectHasFrame {
    RwObject object;
    RwLLLink lFrame;
    struct RwObjectHasFrame* (*sync)(struct RwObjectHasFrame* object);
} RwObjectHasFrame;

typedef RwFrame* (*RwFrameCallBack)(RwFrame* frame, void* data);
typedef RwObject* (*RwObjectCallBack)(RwObject* object, void* data);

// Per-module bookkeeping
typedef struct RwModuleInfo {
    RwInt32 globalsOffset;
    RwInt32 numInstances;
} RwModuleInfo;

#define RWPLUGINOFFSET(_type, _base, _offset) ((_type*)((RwUInt8*)(_base) + (_offset)))

typedef struct RwMemoryFunctions {
    void* (*rwmalloc)(RwUInt32 size);
    void (*rwfree)(void* mem);
    void* (*rwrealloc)(void* mem, RwUInt32 newSize);
    void* (*rwcalloc)(RwUInt32 numObj, RwUInt32 sizeObj);
} RwMemoryFunctions;

typedef char RwChar;

typedef struct RwStringFunctions {
    int (*vecSprintf)(RwChar* buffer, const RwChar* format, ...);
    int (*vecVsprintf)(RwChar* buffer, const RwChar* format, void* argptr);
    RwChar* (*rwstrcpy)(RwChar* dst, const RwChar* src);
    RwChar* (*rwstrncpy)(RwChar* dst, const RwChar* src, RwUInt32 n);
    RwChar* (*rwstrcat)(RwChar* dst, const RwChar* src);
    RwChar* (*rwstrncat)(RwChar* dst, const RwChar* src, RwUInt32 n);
    RwChar* (*rwstrrchr)(const RwChar* s, RwInt32 c);
    RwChar* (*rwstrchr)(const RwChar* s, RwInt32 c);
    RwChar* (*rwstrstr)(const RwChar* s, const RwChar* ss);
    int (*rwstrcmp)(const RwChar* s1, const RwChar* s2);
    int (*rwstricmp)(const RwChar* s1, const RwChar* s2);
    RwUInt32 (*rwstrlen)(const RwChar* s);
    RwChar* (*rwstrupr)(RwChar* s);
    RwChar* (*rwstrlwr)(RwChar* s);
    RwChar* (*rwstrtok)(RwChar* s, const RwChar* delim);
    int (*rwsscanf)(const RwChar* buffer, const RwChar* format, ...);
} RwStringFunctions;

typedef RwInt32 RwRenderState;
typedef RwBool (*RwRenderStateSetFunction)(RwRenderState state, void* value);
typedef RwBool (*RwRenderStateGetFunction)(RwRenderState state, void* value);
typedef RwBool (*RwIm2DRenderLineFunction)(void* vertices, RwInt32 numVertices, RwInt32 vert1, RwInt32 vert2);
typedef RwBool (*RwIm2DRenderTriangleFunction)(void* vertices, RwInt32 numVertices, RwInt32 vert1, RwInt32 vert2,
                                               RwInt32 vert3);
typedef RwBool (*RwIm2DRenderPrimitiveFunction)(RwInt32 primType, void* vertices, RwInt32 numVertices);
typedef RwBool (*RwIm2DRenderIndexedPrimitiveFunction)(RwInt32 primType, void* vertices, RwInt32 numVertices,
                                                       RwUInt16* indices, RwInt32 numIndices);

// Device interface (only the start is named).
typedef struct RwDevice {
    /* 0x00 */ RwReal gammaCorrection;
    /* 0x04 */ void* fpSystem;
    /* 0x08 */ RwReal zBufferNear;
    /* 0x0C */ RwReal zBufferFar;
    /* 0x10 */ RwRenderStateSetFunction fpRenderStateSet;
    /* 0x14 */ RwRenderStateGetFunction fpRenderStateGet;
    /* 0x18 */ RwIm2DRenderLineFunction fpIm2DRenderLine;
    /* 0x1C */ RwIm2DRenderTriangleFunction fpIm2DRenderTriangle;
    /* 0x20 */ RwIm2DRenderPrimitiveFunction fpIm2DRenderPrimitive;
    /* 0x24 */ RwIm2DRenderIndexedPrimitiveFunction fpIm2DRenderIndexedPrimitive;
    /* 0x28 */ RwUInt8 _28[0xAC - 0x28];
} RwDevice;

typedef struct RwFileFunctions {
    RwBool (*rwfexist)(const RwChar* name);
    void* (*rwfopen)(const RwChar* name, const RwChar* mode);
    int (*rwfclose)(void* fptr);
    RwUInt32 (*rwfread)(void* addr, RwUInt32 size, RwUInt32 count, void* fptr);
    RwUInt32 (*rwfwrite)(const void* addr, RwUInt32 size, RwUInt32 count, void* fptr);
    RwChar* (*rwfgets)(RwChar* buffer, int maxLen, void* fptr);
    int (*rwfputs)(const RwChar* buffer, void* fptr);
    int (*rwfeof)(void* fptr);
    int (*rwfseek)(void* fptr, long offset, int origin);
    int (*rwfflush)(void* fptr);
    int (*rwftell)(void* fptr);
} RwFileFunctions;

// Engine globals: only the fields the decompiled code uses are named.
typedef struct RwGlobals {
    /* 0x000 */ void* curCamera;
    /* 0x004 */ void* curWorld;
    /* 0x008 */ RwUInt32 renderFrame;
    /* 0x00C */ RwUInt32 lightFrame;
    /* 0x010 */ RwDevice dOpenDevice;
    /* 0x0BC */ RwLinkList dirtyFrameList;
    /* 0x0C4 */ RwFileFunctions fileFuncs;
    /* 0x0F0 */ RwStringFunctions stringFuncs;
    /* 0x130 */ RwMemoryFunctions memoryFuncs;
    /* 0x140 */ void* (*memoryAlloc)(RwFreeList* fl);
    /* 0x144 */ RwFreeList* (*memoryFree)(RwFreeList* fl, void* pData);
} RwGlobals;

extern void* RwEngineInstance;

#define RWSRCGLOBAL(var) (((RwGlobals*)RwEngineInstance)->var)
#define RwMalloc(_s) ((RWSRCGLOBAL(memoryFuncs).rwmalloc)(_s))
#define RwFree(_p) ((RWSRCGLOBAL(memoryFuncs).rwfree)(_p))
#define RwFreeListAlloc(_f) ((RWSRCGLOBAL(memoryAlloc))(_f))
#define RwFreeListFree(_f, _p) ((RWSRCGLOBAL(memoryFree))((_f), (_p)))

typedef void (*RwFreeListCallBack)(void* pMem, void* pData);

RwFreeList* RwFreeListCreate(RwInt32 entrySize, RwInt32 entriesPerBlock, RwInt32 alignment);
RwBool RwFreeListDestroy(RwFreeList* freelist);
void RwFreeListForAllUsed(RwFreeList* freelist, RwFreeListCallBack fpCallBack, void* pData);
void* _rwFreeListAllocReal(RwFreeList* freelist);

// Plugin registries (one per extensible object type)
typedef struct RwPluginRegEntry RwPluginRegEntry;
typedef struct RwPluginRegistry {
    RwInt32 sizeOfStruct;
    RwInt32 origSizeOfStruct;
    RwInt32 maxSizeOfStruct;
    RwInt32 staticAlloc;
    RwPluginRegEntry* firstRegEntry;
    RwPluginRegEntry* lastRegEntry;
} RwPluginRegistry;

// Errors
typedef struct RwError {
    RwInt32 pluginID;
    RwInt32 errorCode;
} RwError;

#define E_RW_NOMEM 0x80000013

RwInt32 _rwerror(RwInt32 code, ...);
RwError* RwErrorSet(RwError* code);

// RWERROR((code, args...)) records an error for the current plugin.
#define RWERROR(ecode)                              \
    do {                                            \
        RwError _rwErrorCode;                       \
        _rwErrorCode.pluginID = rwPLUGIN_ID;        \
        _rwErrorCode.errorCode = _rwerror ecode;    \
        RwErrorSet(&_rwErrorCode);                  \
    } while (0)

// Streams
RwStream* RwStreamWriteInt32(RwStream* stream, const RwInt32* ints, RwUInt32 numBytes);
RwStream* RwStreamReadInt32(RwStream* stream, RwInt32* ints, RwUInt32 numBytes);
RwStream* RwStreamWriteReal(RwStream* stream, const RwReal* reals, RwUInt32 numBytes);
RwStream* RwStreamReadReal(RwStream* stream, RwReal* reals, RwUInt32 numBytes);
RwStream* RwStreamSkip(RwStream* stream, RwUInt32 offset);
RwBool RwStreamFindChunk(RwStream* stream, RwUInt32 type, RwUInt32* lengthOut, RwUInt32* versionOut);
RwBool _rwStreamReadChunkHeader(RwStream* stream, RwUInt32* type, RwUInt32* length, RwUInt32* version,
                                RwUInt32* buildNum);
RwStream* _rwStreamWriteVersionedChunkHeader(RwStream* stream, RwInt32 type, RwInt32 size, RwUInt32 version,
                                             RwUInt32 buildNum);

#define rwID_EXTENSION 0x03
#define rwCHUNKHEADERSIZE 12
#define rwLIBRARYBASEVERSION 0x31000
#define rwLIBRARYCURRENTVERSION 0x33002
#define rwLIBRARYBUILD 0xFFFF
#define RwStreamWriteChunkHeader(stream, type, size) \
    _rwStreamWriteVersionedChunkHeader(stream, type, size, rwLIBRARYCURRENTVERSION, rwLIBRARYBUILD)

#define E_RW_BADVERSION 0x80000004
#define E_RW_PLUGININIT 0x80000017

RwUInt32 _rwGetNumEngineInstances(void);

// Plugins
typedef void* (*RwPluginObjectConstructor)(void* object, RwInt32 offsetInObject, RwInt32 sizeInObject);
typedef void* (*RwPluginObjectDestructor)(void* object, RwInt32 offsetInObject, RwInt32 sizeInObject);
typedef void* (*RwPluginObjectCopy)(void* dstObject, const void* srcObject, RwInt32 offsetInObject,
                                    RwInt32 sizeInObject);
typedef RwStream* (*RwPluginDataChunkReadCallBack)(RwStream* stream, RwInt32 binaryLength, void* object,
                                                   RwInt32 offsetInObject, RwInt32 sizeInObject);
typedef RwStream* (*RwPluginDataChunkWriteCallBack)(RwStream* stream, RwInt32 binaryLength, const void* object,
                                                    RwInt32 offsetInObject, RwInt32 sizeInObject);
typedef RwInt32 (*RwPluginDataChunkGetSizeCallBack)(const void* object, RwInt32 offsetInObject,
                                                    RwInt32 sizeInObject);

RwInt32 RwEngineRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor initCB,
                               RwPluginObjectDestructor termCB);

RwInt32 _rwPluginRegistryAddPlugin(RwPluginRegistry* reg, RwInt32 size, RwUInt32 pluginID,
                                   RwPluginObjectConstructor constructCB, RwPluginObjectDestructor destructCB,
                                   RwPluginObjectCopy copyCB);
const RwPluginRegistry* _rwPluginRegistryInitObject(const RwPluginRegistry* reg, void* object);
const RwPluginRegistry* _rwPluginRegistryDeInitObject(const RwPluginRegistry* reg, void* object);

// Frames (functions)
RwFrame* _rwFrameSyncHierarchyLTM(RwFrame* frame);
RwFrame* RwFrameRemoveChild(RwFrame* child);

#ifdef __cplusplus
}
#endif

#endif
