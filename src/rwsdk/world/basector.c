/*
 * World sectors: module bookkeeping, rendering through the world's callback
 * and plugin registration.
 */

#include "rwsdk/rwcore.h"

typedef struct RpWorldSector RpWorldSector;
typedef RpWorldSector* (*RpWorldSectorCallBackRender)(RpWorldSector* worldSector);

/* Only the vertex count and the world's render callback are used here. */
#define rpWorldSectorNumVertices(sector) (*(RwUInt16*)((RwUInt8*)(sector) + 0x8E))
#define rpWorldRenderCallBack(world) (*(RpWorldSectorCallBackRender*)((RwUInt8*)(world) + 0x74))

RwPluginRegistry sectorTKList = { 0x94, 0x94, 0, 0, NULL, NULL };

RwModuleInfo sectorModule;

RwInt32 _rwPluginRegistryAddPluginStream(RwPluginRegistry* reg, RwUInt32 pluginID,
                                         RwPluginDataChunkReadCallBack readCB,
                                         RwPluginDataChunkWriteCallBack writeCB,
                                         RwPluginDataChunkGetSizeCallBack getSizeCB);

void* _rpSectorOpen(void* instance, RwInt32 offset, RwInt32 size) {
    sectorModule.numInstances++;

    return instance;
}

void* _rpSectorClose(void* instance, RwInt32 offset, RwInt32 size) {
    sectorModule.numInstances--;

    return instance;
}

RwInt32 RpWorldSectorGetNumVertices(const RpWorldSector* worldSector) {
    return rpWorldSectorNumVertices(worldSector);
}

RpWorldSector* RpWorldSectorRender(RpWorldSector* worldSector) {
    return rpWorldRenderCallBack(RWSRCGLOBAL(curWorld))(worldSector);
}

RwInt32 RpWorldSectorRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB,
                                    RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB) {
    return _rwPluginRegistryAddPlugin(&sectorTKList, size, pluginID, constructCB, destructCB, copyCB);
}

RwInt32 RpWorldSectorRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB,
                                          RwPluginDataChunkWriteCallBack writeCB,
                                          RwPluginDataChunkGetSizeCallBack getSizeCB) {
    return _rwPluginRegistryAddPluginStream(&sectorTKList, pluginID, readCB, writeCB, getSizeCB);
}
