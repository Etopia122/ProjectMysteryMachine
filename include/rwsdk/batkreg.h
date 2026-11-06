#ifndef RWSDK_BATKREG_H
#define RWSDK_BATKREG_H

// Plugin registry entries (layout from the GameCube code).

#include "rwsdk/rwcore.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef RwBool (*RwPluginDataChunkAlwaysCallBack)(void* object, RwInt32 offsetInObject, RwInt32 sizeInObject);
typedef RwBool (*RwPluginDataChunkRightsCallBack)(void* object, RwInt32 offsetInObject, RwInt32 sizeInObject,
                                                  RwUInt32 extraData);
typedef RwInt32 (*RwPluginErrorStrCallBack)(void* code);

struct RwPluginRegEntry {
    RwInt32 offset;
    RwInt32 size;
    RwUInt32 pluginID;
    RwPluginDataChunkReadCallBack readCB;
    RwPluginDataChunkWriteCallBack writeCB;
    RwPluginDataChunkGetSizeCallBack getSizeCB;
    RwPluginDataChunkAlwaysCallBack alwaysCB;
    RwPluginDataChunkRightsCallBack rightsCB;
    RwPluginObjectConstructor constructCB;
    RwPluginObjectDestructor destructCB;
    RwPluginObjectCopy copyCB;
    RwPluginErrorStrCallBack errStrCB;
    RwPluginRegEntry* nextRegEntry;
    RwPluginRegEntry* prevRegEntry;
    RwPluginRegistry* parentRegistry;
};

#ifdef __cplusplus
}
#endif

#endif
