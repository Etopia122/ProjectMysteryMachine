/*
 * Binary worlds: stream reading of a world and its BSP tree into a single
 * allocation, plus the right-to-render chunks for worlds and sectors.
 */

#include "rwsdk/rpworld_int.h"

#define rwPLUGIN_ID 2 /* world */

#define rwID_MATLIST 0x08
#define rwID_ATOMICSECT 0x09
#define rwID_PLANESECT 0x0A

#define E_RW_BADVERSION 0x80000004

#define rpWORLDTEXTURED 0x00000004
#define rpWORLDPRELIT 0x00000008
#define rpWORLDNORMALS 0x00000010
#define rpWORLDTEXTURED2 0x00000080
#define rpWORLDNATIVE 0x01000000
#define rpWORLDSECTORSOVERLAP 0x40000000

typedef struct _rpWorldSector {
    /* 0x00 */ RwInt32 matListWindowBase;
    /* 0x04 */ RwInt32 numPolygons;
    /* 0x08 */ RwInt32 numVertices;
    /* 0x0C */ RwV3d inf;
    /* 0x18 */ RwV3d sup;
    /* 0x24 */ RwBool collSectorPresent;
    /* 0x28 */ RwBool unused;
} _rpWorldSector;

typedef struct _rpPlane {
    RwInt32 type;
    RwReal value;
    RwBool leftIsWorldSector;
    RwBool rightIsWorldSector;
    RwReal leftValue;
    RwReal rightValue;
} _rpPlane;

typedef struct _rpWorld {
    /* 0x00 */ RwBool rootIsWorldSector;
    /* 0x04 */ RwV3d invWorldOrigin;
    /* 0x10 */ RwSurfaceProperties surfaceProps;
    /* 0x1C */ RwInt32 numPolygons;
    /* 0x20 */ RwInt32 numVertices;
    /* 0x24 */ RwInt32 numPlaneSectors;
    /* 0x28 */ RwInt32 numWorldSectors;
    /* 0x2C */ RwInt32 colSectorSize;
    /* 0x30 */ RwUInt32 format;
} _rpWorld;

typedef struct RpMaterial {
    RwTexture* texture;
} RpMaterial;

extern RwPluginRegistry sectorTKList;
extern RwPluginRegistry worldTKList;

/* .sbss is laid out in reverse declaration order. */
static RwModuleInfo binWorldModule;
static RwUInt32 worldRightsPluginID;
static RwUInt32 worldRightsExtraData;
static RwUInt32 sectRightsPluginID;
static RwUInt32 sectRightsExtraData;

RwUInt32 RwStreamRead(RwStream* stream, void* buffer, RwUInt32 length);
void* RwMemNative32(void* mem, RwUInt32 size);
void* RwMemNative16(void* mem, RwUInt32 size);
void* RwMemFloat32ToReal(void* mem, RwUInt32 size);
void* memset(void* dst, int val, unsigned long n);
void* memcpy(void* dst, const void* src, unsigned long n);
const RwPluginRegistry* _rwPluginRegistryReadDataChunks(const RwPluginRegistry* reg, RwStream* stream,
                                                        void* object);
const RwPluginRegistry* _rwPluginRegistryInvokeRights(const RwPluginRegistry* reg, RwUInt32 id, void* obj,
                                                      RwUInt32 extraData);
RpMaterialList* _rpMaterialListStreamRead(RwStream* stream, RpMaterialList* matList);
RpMaterial* RpMaterialSetTexture(RpMaterial* material, RwTexture* texture);
RwInt32 fn_80151C24(const RwUInt8* tree);
RpWorld* fn_80151CA4(RpWorld* world, const RwSurfaceProperties* surfaceProps);
void _rpWorldRegisterWorld(RpWorld* world, RwUInt32 memorySize);
void _rpWorldUnregisterWorld(RpWorld* world);
RwBool _rpWorldFindBBox(RpWorld* world, RwBBox* boundingBox);
RpWorld* _rpWorldSetupSectorBoundingBoxes(RpWorld* world);
RpWorld* RpWorldSetSectorRenderCallBack(RpWorld* world, RpWorldSectorCallBackRender fpCallBack);
RpWorld* RpWorldUnlock(RpWorld* world);
RwBool RpWorldDestroy(RpWorld* world);

RwStream* _rpReadWorldRights(RwStream* stream, RwInt32 binaryLength, void* object, RwInt32 offsetInObject,
                             RwInt32 sizeInObject) {
    if (!RwStreamReadInt32(stream, (RwInt32*)&worldRightsPluginID, sizeof(RwInt32))) {
        return NULL;
    }

    if (binaryLength == 8) {
        if (!RwStreamReadInt32(stream, (RwInt32*)&worldRightsExtraData, sizeof(RwInt32))) {
            return NULL;
        }
    }

    return stream;
}

RwStream* _rpWriteWorldRights(RwStream* stream, RwInt32 binaryLength, const void* object, RwInt32 offsetInObject,
                              RwInt32 sizeInObject) {
    const RpWorld* world = (const RpWorld*)object;

    if (!RwStreamWriteInt32(stream, (const RwInt32*)&world->pipeline->pluginId, sizeof(RwInt32))) {
        return NULL;
    }

    if (!RwStreamWriteInt32(stream, (const RwInt32*)&world->pipeline->pluginData, sizeof(RwInt32))) {
        return NULL;
    }

    return stream;
}

RwInt32 _rpSizeWorldRights(const void* object, RwInt32 offsetInObject, RwInt32 sizeInObject) {
    const RpWorld* world = (const RpWorld*)object;

    if (world->pipeline && world->pipeline->pluginId) {
        return sizeof(RwUInt32) * 2;
    }

    return 0;
}

RwStream* _rpReadSectRights(RwStream* stream, RwInt32 binaryLength, void* object, RwInt32 offsetInObject,
                            RwInt32 sizeInObject) {
    if (!RwStreamReadInt32(stream, (RwInt32*)&sectRightsPluginID, sizeof(RwInt32))) {
        return NULL;
    }

    if (binaryLength == 8) {
        if (!RwStreamReadInt32(stream, (RwInt32*)&sectRightsExtraData, sizeof(RwInt32))) {
            return NULL;
        }
    }

    return stream;
}

RwStream* _rpWriteSectRights(RwStream* stream, RwInt32 binaryLength, const void* object, RwInt32 offsetInObject,
                             RwInt32 sizeInObject) {
    const RpWorldSector* sector = (const RpWorldSector*)object;

    if (!RwStreamWriteInt32(stream, (const RwInt32*)&sector->pipeline->pluginId, sizeof(RwInt32))) {
        return NULL;
    }

    if (!RwStreamWriteInt32(stream, (const RwInt32*)&sector->pipeline->pluginData, sizeof(RwInt32))) {
        return NULL;
    }

    return stream;
}

RwInt32 _rpSizeSectRights(const void* object, RwInt32 offsetInObject, RwInt32 sizeInObject) {
    const RpWorldSector* sector = (const RpWorldSector*)object;

    if (sector->pipeline && sector->pipeline->pluginId) {
        return sizeof(RwUInt32) * 2;
    }

    return 0;
}

static inline void* MemBlockAlloc(RwUInt8** memBlock, RwUInt32 size) {
    RwUInt8* mem = *memBlock;

    *memBlock = mem + size;
    return mem;
}

static inline void WorldSectorExpandPolygons(RpWorldSector* worldSector) {
    RwUInt32 bytes;
    RwInt32 numPolygons;
    RpPolygon* polygons;
    RwUInt8* oldPolygons;
    RwUInt8* src;
    RwInt32 i;

    numPolygons = worldSector->numPolygons;
    bytes = numPolygons * 4;
    polygons = worldSector->polygons;
    oldPolygons = (RwUInt8*)RwMalloc(bytes);
    src = oldPolygons;

    memcpy(oldPolygons, polygons, bytes);

    for (i = 0; i < numPolygons; i++) {
        polygons[i].matIndex = src[0];
        polygons[i].vertIndex[0] = src[1];
        polygons[i].vertIndex[1] = src[2];
        polygons[i].vertIndex[2] = src[3];
        src += 4;
    }

    RwFree(oldPolygons);
}

RpWorldSector* WorldSectorStreamRead(RwStream* stream, RwUInt8** memBlock, RpWorld* world, RwUInt32 format) {
    RwUInt8 collData[0x180];
    _rpWorldSector ws;
    RpWorldSector* worldSector;
    RwUInt32 size;
    RwInt32 i;
    RwUInt32 version;

    if (!RwStreamFindChunk(stream, rwID_STRUCT, NULL, &version)) {
        return NULL;
    }

    if (!(version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION)) {
        RWERROR((E_RW_BADVERSION));
        return NULL;
    }

    if (RwStreamRead(stream, &ws, sizeof(ws)) != sizeof(ws)) {
        return NULL;
    }

    RwMemNative32(&ws, sizeof(ws));
    RwMemFloat32ToReal(&ws.inf, sizeof(ws.inf));
    RwMemFloat32ToReal(&ws.sup, sizeof(ws.sup));

    worldSector = (RpWorldSector*)MemBlockAlloc(memBlock, sectorTKList.sizeOfStruct);
    if (!worldSector) {
        RWERROR((E_RW_NOMEM, sectorTKList.sizeOfStruct));
        return NULL;
    }

    worldSector->type = rwSECTORATOMIC;
    worldSector->tightBoundingBox.inf = ws.inf;
    worldSector->tightBoundingBox.sup = ws.sup;
    worldSector->matListWindowBase = (RwUInt16)ws.matListWindowBase;
    worldSector->numPolygons = (RwUInt16)ws.numPolygons;
    worldSector->numVertices = (RwUInt16)ws.numVertices;
    worldSector->polygons = NULL;
    worldSector->vertices = NULL;
    worldSector->preLitLum = NULL;
    worldSector->normals = NULL;
    memset(worldSector->texCoords, 0, sizeof(worldSector->texCoords));
    worldSector->unk80 = NULL;
    worldSector->pipeline = NULL;
    worldSector->repEntry = NULL;
    worldSector->mesh = NULL;
    rwLinkListInitialize(&worldSector->collAtomicsInWorldSector);
    rwLinkListInitialize(&worldSector->noCollAtomicsInWorldSector);
    rwLinkListInitialize(&worldSector->lightsInWorldSector);

    if (!(world->flags & rpWORLDNATIVE)) {
        if (ws.numVertices) {
            size = ws.numVertices * sizeof(RwV3d);
            worldSector->vertices = (RwV3d*)MemBlockAlloc(memBlock, size);
            if (!worldSector->vertices) {
                RWERROR((E_RW_NOMEM, size));
                return NULL;
            }

            if (!RwStreamReadReal(stream, (RwReal*)worldSector->vertices, size)) {
                return NULL;
            }

            if (format & rpWORLDNORMALS) {
                size = ws.numVertices * sizeof(RwUInt32);
                worldSector->normals = (RwV3d*)MemBlockAlloc(memBlock, size);
                if (!worldSector->normals) {
                    RWERROR((E_RW_NOMEM, size));
                    return NULL;
                }

                if (RwStreamRead(stream, worldSector->normals, size) != size) {
                    return NULL;
                }
            }

            if (format & rpWORLDPRELIT) {
                size = ws.numVertices * sizeof(RwRGBA);
                worldSector->preLitLum = (RwRGBA*)MemBlockAlloc(memBlock, size);
                if (!worldSector->preLitLum) {
                    RWERROR((E_RW_NOMEM, size));
                    return NULL;
                }

                if (!RwStreamRead(stream, worldSector->preLitLum, size)) {
                    return NULL;
                }
            }

            if (world->numTexCoordSets > 0) {
                size = worldSector->numVertices * sizeof(RwTexCoords);
                for (i = 0; i < world->numTexCoordSets; i++) {
                    worldSector->texCoords[i] = (RwTexCoords*)MemBlockAlloc(memBlock, size);
                    if (!worldSector->texCoords[i]) {
                        RWERROR((E_RW_NOMEM, size));
                        return NULL;
                    }

                    if (!RwStreamReadReal(stream, (RwReal*)worldSector->texCoords[i], size)) {
                        return NULL;
                    }
                }
            }
        }

        if (ws.numPolygons) {
            RwUInt32 readSize;

            size = ws.numPolygons * sizeof(RpPolygon);
            worldSector->polygons = (RpPolygon*)MemBlockAlloc(memBlock, size);
            if (!worldSector->polygons) {
                RWERROR((E_RW_NOMEM, size));
                return NULL;
            }

            readSize = size;
            if (version < 0x30400) {
                readSize = (RwInt32)size / 2;
            }

            if (RwStreamRead(stream, worldSector->polygons, readSize) != readSize) {
                return NULL;
            }

            if (version < 0x30400) {
                WorldSectorExpandPolygons(worldSector);
            } else {
                RwMemNative16(worldSector->polygons, size);
            }
        }
    }

    if (ws.collSectorPresent) {
        if (RwStreamRead(stream, collData, sizeof(collData)) != sizeof(collData)) {
            return NULL;
        }

        size = (fn_80151C24(collData) + 3) & ~3;
        worldSector->unk80 = MemBlockAlloc(memBlock, size);
        if (!worldSector->unk80) {
            RWERROR((E_RW_NOMEM, size));
            return NULL;
        }

        memcpy(worldSector->unk80, collData, size);
    } else {
        worldSector->unk80 = NULL;
    }

    _rwPluginRegistryInitObject(&sectorTKList, worldSector);

    sectRightsPluginID = 0;
    sectRightsExtraData = 0;

    if (!_rwPluginRegistryReadDataChunks(&sectorTKList, stream, worldSector)) {
        return NULL;
    }

    if (sectRightsPluginID) {
        _rwPluginRegistryInvokeRights(&sectorTKList, sectRightsPluginID, worldSector, sectRightsExtraData);
    }

    return worldSector;
}

RpPlaneSector* PlaneSectorStreamRead(RwStream* stream, RwUInt8** memBlock, RpWorld* world, RwUInt32 format) {
    _rpPlane ps;
    RpPlaneSector* planeSector;
    RwUInt32 size;
    RwUInt32 version;

    if (!RwStreamFindChunk(stream, rwID_STRUCT, &size, &version)) {
        return NULL;
    }

    if (!(version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION)) {
        RWERROR((E_RW_BADVERSION));
        return NULL;
    }

    memset(&ps, 0, sizeof(ps));
    if (size != RwStreamRead(stream, &ps, size)) {
        return NULL;
    }

    RwMemNative32(&ps, sizeof(ps));
    RwMemFloat32ToReal(&ps.value, sizeof(ps.value));
    RwMemFloat32ToReal(&ps.leftValue, sizeof(ps.leftValue));
    RwMemFloat32ToReal(&ps.rightValue, sizeof(ps.rightValue));

    planeSector = (RpPlaneSector*)MemBlockAlloc(memBlock, sizeof(RpPlaneSector));
    if (!planeSector) {
        RWERROR((E_RW_NOMEM, sizeof(RpPlaneSector)));
        return NULL;
    }

    planeSector->type = ps.type;
    planeSector->value = ps.value;

    if (format & rpWORLDSECTORSOVERLAP) {
        planeSector->leftValue = ps.leftValue;
        planeSector->rightValue = ps.rightValue;
    } else {
        planeSector->leftValue = ps.value;
        planeSector->rightValue = ps.value;
    }

    if (ps.leftIsWorldSector) {
        if (!RwStreamFindChunk(stream, rwID_ATOMICSECT, NULL, &version)) {
            return NULL;
        }

        if (!(version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION)) {
            RWERROR((E_RW_BADVERSION));
            return NULL;
        }

        if (!(planeSector->leftSubTree = (RpSector*)WorldSectorStreamRead(stream, memBlock, world, format))) {
            return NULL;
        }
    } else {
        if (!RwStreamFindChunk(stream, rwID_PLANESECT, NULL, &version)) {
            return NULL;
        }

        if (!(version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION)) {
            RWERROR((E_RW_BADVERSION));
            return NULL;
        }

        if (!(planeSector->leftSubTree = (RpSector*)PlaneSectorStreamRead(stream, memBlock, world, format))) {
            return NULL;
        }
    }

    if (ps.rightIsWorldSector) {
        if (!RwStreamFindChunk(stream, rwID_ATOMICSECT, NULL, &version)) {
            return NULL;
        }

        if (!(version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION)) {
            RWERROR((E_RW_BADVERSION));
            return NULL;
        }

        if (!(planeSector->rightSubTree = (RpSector*)WorldSectorStreamRead(stream, memBlock, world, format))) {
            return NULL;
        }
    } else {
        if (!RwStreamFindChunk(stream, rwID_PLANESECT, NULL, &version)) {
            return NULL;
        }

        if (!(version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION)) {
            RWERROR((E_RW_BADVERSION));
            return NULL;
        }

        if (!(planeSector->rightSubTree = (RpSector*)PlaneSectorStreamRead(stream, memBlock, world, format))) {
            return NULL;
        }
    }

    return planeSector;
}

static inline void WorldStreamReadFail(RpWorld* world) {
    _rpWorldUnregisterWorld(world);
    RwFree(world);
}

RpWorld* RpWorldStreamRead(RwStream* stream) {
    _rpWorld w;
    RwUInt32 numTexCoordSets;
    RwSurfaceProperties* surfaceProps;
    RpWorld* world;
    RwUInt32 memSize;
    RwUInt32 format;
    RwInt32 i;
    RwUInt8* memBlock;
    RwUInt32 size;
    RwUInt32 version;

    if (!RwStreamFindChunk(stream, rwID_STRUCT, &size, &version)) {
        return NULL;
    }

    if (!(version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION)) {
        RWERROR((E_RW_BADVERSION));
        return NULL;
    }

    memset(&w, 0, sizeof(w));
    if (size != RwStreamRead(stream, &w, size)) {
        return NULL;
    }

    RwMemNative32(&w, sizeof(w));
    RwMemFloat32ToReal(&w.invWorldOrigin, sizeof(w.invWorldOrigin));
    surfaceProps = &w.surfaceProps;
    RwMemFloat32ToReal(surfaceProps, sizeof(RwSurfaceProperties));

    format = w.format;
    memSize = worldTKList.sizeOfStruct;
    memSize += w.numPlaneSectors * sizeof(RpPlaneSector);
    memSize += w.numWorldSectors * sectorTKList.sizeOfStruct;

    if (w.format & 0x00FF0000) {
        numTexCoordSets = (RwInt32)(w.format & 0x00FF0000) >> 16;
    } else if (w.format & rpWORLDTEXTURED2) {
        numTexCoordSets = 2;
    } else {
        numTexCoordSets = (w.format & rpWORLDTEXTURED) ? 1 : 0;
    }

    if (!(format & rpWORLDNATIVE)) {
        memSize += w.numVertices * sizeof(RwV3d);

        if (format & rpWORLDNORMALS) {
            memSize += w.numVertices * sizeof(RwUInt32);
        }

        if (format & rpWORLDPRELIT) {
            memSize += w.numVertices * sizeof(RwRGBA);
        }

        if (numTexCoordSets) {
            memSize += numTexCoordSets * w.numVertices * sizeof(RwTexCoords);
        }

        memSize += w.numPolygons * sizeof(RpPolygon);
    }

    memSize += w.colSectorSize;

    world = (RpWorld*)RwMalloc(memSize);
    if (!world) {
        RWERROR((E_RW_NOMEM, memSize));
        return NULL;
    }

    memset(world, 0, memSize);
    memBlock = (RwUInt8*)world + worldTKList.sizeOfStruct;

    rwObjectInitialize(world, rpWORLD, 0);
    world->object.privateFlags = rpWORLDSINGLEMALLOC;

    _rpWorldRegisterWorld(world, memSize);

    world->flags = format;
    world->renderOrder = rpWORLDRENDERBACK2FRONT;
    fn_80151CA4(world, surfaceProps);

    world->worldOrigin.x = -1.0f * w.invWorldOrigin.x;
    world->worldOrigin.y = -1.0f * w.invWorldOrigin.y;
    world->worldOrigin.z = -1.0f * w.invWorldOrigin.z;

    world->numTexCoordSets = numTexCoordSets;
    world->rootSector = NULL;
    world->pipeline = NULL;

    if (!RwStreamFindChunk(stream, rwID_MATLIST, NULL, &version)) {
        WorldStreamReadFail(world);
        return NULL;
    }

    if (!(version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION)) {
        RWERROR((E_RW_BADVERSION));
        return NULL;
    }

    if (!_rpMaterialListStreamRead(stream, &world->matList)) {
        WorldStreamReadFail(world);
        return NULL;
    }

    if (w.rootIsWorldSector) {
        if (!RwStreamFindChunk(stream, rwID_ATOMICSECT, NULL, &version)) {
            WorldStreamReadFail(world);
            return NULL;
        }

        if (!(version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION)) {
            RWERROR((E_RW_BADVERSION));
            WorldStreamReadFail(world);
            return NULL;
        }

        if (!(world->rootSector = (RpSector*)WorldSectorStreamRead(stream, &memBlock, world, format))) {
            WorldStreamReadFail(world);
            return NULL;
        }
    } else {
        if (!RwStreamFindChunk(stream, rwID_PLANESECT, NULL, &version)) {
            WorldStreamReadFail(world);
            return NULL;
        }

        if (!(version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION)) {
            RWERROR((E_RW_BADVERSION));
            WorldStreamReadFail(world);
            return NULL;
        }

        if (!(world->rootSector = (RpSector*)PlaneSectorStreamRead(stream, &memBlock, world, format))) {
            WorldStreamReadFail(world);
            return NULL;
        }
    }

    rwLinkListInitialize(&world->clumpList);
    world->numClumpsInWorld = 0;
    world->currentClumpLink = rwLinkListGetTerminator(&world->clumpList);
    rwLinkListInitialize(&world->lightList);
    rwLinkListInitialize(&world->directionalLightList);

    _rpWorldFindBBox(world, &world->boundingBox);
    _rpWorldSetupSectorBoundingBoxes(world);
    RpWorldSetSectorRenderCallBack(world, NULL);

    for (i = 0; i < world->matList.numMaterials; i++) {
        RpMaterial* material = world->matList.materials[i];

        if (material->texture) {
            RpMaterialSetTexture(material, material->texture);
        }
    }

    _rwPluginRegistryInitObject(&worldTKList, world);

    worldRightsPluginID = 0;
    worldRightsExtraData = 0;

    if (!_rwPluginRegistryReadDataChunks(&worldTKList, stream, world)) {
        WorldStreamReadFail(world);
        return NULL;
    }

    if (worldRightsPluginID) {
        _rwPluginRegistryInvokeRights(&worldTKList, worldRightsPluginID, world, worldRightsExtraData);
    }

    if (!RpWorldUnlock(world)) {
        RpWorldDestroy(world);
        return NULL;
    }

    return world;
}

void* _rpBinaryWorldClose(void* instance, RwInt32 offset, RwInt32 size) {
    binWorldModule.numInstances--;
    return instance;
}

void* _rpBinaryWorldOpen(void* instance, RwInt32 offset, RwInt32 size) {
    binWorldModule.numInstances++;
    return instance;
}
