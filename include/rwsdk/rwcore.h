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

typedef struct RwMemoryFunctions {
    void* (*rwmalloc)(RwUInt32 size);
    void (*rwfree)(void* mem);
    void* (*rwrealloc)(void* mem, RwUInt32 newSize);
    void* (*rwcalloc)(RwUInt32 numObj, RwUInt32 sizeObj);
} RwMemoryFunctions;

// Engine globals: only the fields the decompiled code uses are named.
typedef struct RwGlobals {
    /* 0x000 */ RwUInt8 _000[0x130];
    /* 0x130 */ RwMemoryFunctions memoryFuncs;
} RwGlobals;

extern void* RwEngineInstance;

#define RWSRCGLOBAL(var) (((RwGlobals*)RwEngineInstance)->var)
#define RwMalloc(_s) ((RWSRCGLOBAL(memoryFuncs).rwmalloc)(_s))
#define RwFree(_p) ((RWSRCGLOBAL(memoryFuncs).rwfree)(_p))

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

#ifdef __cplusplus
}
#endif

#endif
