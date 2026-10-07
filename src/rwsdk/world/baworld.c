/*
 * Worlds: creation and destruction, the BSP sector tree walks, the
 * registered-world list used to map sectors back to their world, and
 * rendering of the sectors inside the camera frustum.
 */

#include "rwsdk/rwcore.h"

#define rwPLUGIN_ID 2 /* world */

#define rpWORLD 7
#define rpWORLDMAXBSPDEPTH 64
#define rpWORLDTRISTRIP 0x01
#define rpWORLDSINGLEMALLOC 0x01
#define rpWORLDRENDERFRONT2BACK 2
#define rpATOMICRENDER 0x04

#define rwSECTORATOMIC -1
#define rwSECTORBUILD -2

typedef struct RpMaterial RpMaterial;
typedef struct RpBuildMesh RpBuildMesh;
typedef struct RpMeshHeader RpMeshHeader;
typedef struct RwResEntry RwResEntry;
typedef struct RxPipeline RxPipeline;
typedef struct RwCamera RwCamera;
typedef struct RwSphere RwSphere;

typedef struct RwSurfaceProperties {
    RwReal ambient;
    RwReal specular;
    RwReal diffuse;
} RwSurfaceProperties;

typedef struct RwTexCoords {
    RwReal u;
    RwReal v;
} RwTexCoords;

typedef struct RpMaterialList {
    RpMaterial** materials;
    RwInt32 numMaterials;
    RwInt32 space;
} RpMaterialList;

typedef struct RpPolygon {
    RwUInt16 matIndex;
    RwUInt16 vertIndex[3];
} RpPolygon;

typedef struct RpSector {
    RwInt32 type;
} RpSector;

typedef struct RpPlaneSector {
    /* 0x00 */ RwInt32 type;
    /* 0x04 */ RwReal value;
    /* 0x08 */ RpSector* leftSubTree;
    /* 0x0C */ RpSector* rightSubTree;
    /* 0x10 */ RwReal leftValue;
    /* 0x14 */ RwReal rightValue;
} RpPlaneSector;

typedef struct RpWorldSector {
    /* 0x00 */ RwInt32 type;
    /* 0x04 */ RpPolygon* polygons;
    /* 0x08 */ RwV3d* vertices;
    /* 0x0C */ RwV3d* normals;
    /* 0x10 */ RwTexCoords* texCoords[8];
    /* 0x30 */ RwRGBA* preLitLum;
    /* 0x34 */ RwResEntry* repEntry;
    /* 0x38 */ RwLinkList collAtomicsInWorldSector;
    /* 0x40 */ RwLinkList noCollAtomicsInWorldSector;
    /* 0x48 */ RwLinkList lightsInWorldSector;
    /* 0x50 */ RwBBox boundingBox;
    /* 0x68 */ RwBBox tightBoundingBox;
    /* 0x80 */ void* unk80;
    /* 0x84 */ RpMeshHeader* mesh;
    /* 0x88 */ RxPipeline* pipeline;
    /* 0x8C */ RwUInt16 matListWindowBase;
    /* 0x8E */ RwUInt16 numVertices;
    /* 0x90 */ RwUInt16 numPolygons;
    /* 0x92 */ RwUInt16 pad;
} RpWorldSector;

typedef RpWorldSector* (*RpWorldSectorCallBackRender)(RpWorldSector* worldSector);
typedef RpWorldSector* (*RpWorldSectorCallBack)(RpWorldSector* worldSector, void* data);

typedef struct RpWorld {
    /* 0x00 */ RwObject object;
    /* 0x08 */ RwUInt32 flags;
    /* 0x0C */ RwInt32 renderOrder;
    /* 0x10 */ RpMaterialList matList;
    /* 0x1C */ RpSector* rootSector;
    /* 0x20 */ RwInt32 numTexCoordSets;
    /* 0x24 */ RwInt32 numClumpsInWorld;
    /* 0x28 */ RwLLLink* currentClumpLink;
    /* 0x2C */ RwLinkList clumpList;
    /* 0x34 */ RwLinkList lightList;
    /* 0x3C */ RwLinkList directionalLightList;
    /* 0x44 */ RwV3d worldOrigin;
    /* 0x50 */ RwBBox boundingBox;
    /* 0x68 */ RwSurfaceProperties surfaceProps;
    /* 0x74 */ RpWorldSectorCallBackRender renderCallBack;
    /* 0x78 */ RxPipeline* pipeline;
} RpWorld;

typedef struct RpAtomic {
    /* 0x00 */ RwObject object;
    /* 0x08 */ RwUInt8 pad08[0x48 - 0x08];
    /* 0x48 */ struct RpAtomic* (*renderCallBack)(struct RpAtomic* atomic);
    /* 0x4C */ RwUInt8 pad4C[0x60 - 0x4C];
    /* 0x60 */ RwUInt16 renderFrame;
} RpAtomic;

typedef struct RpLight {
    RwUInt8 pad00[0x34];
    /* 0x34 */ RwLLLink inWorld;
} RpLight;

typedef RpLight* (*RpLightCallBack)(RpLight* light, void* data);

typedef struct RpTie {
    RwLLLink lAtomicInWorldSector;
    RpAtomic* apAtom;
    RwLLLink lWorldSectorInAtomic;
    RpWorldSector* worldSector;
} RpTie;

typedef struct RpLightTie {
    RwLLLink lightInWorldSector;
    RpLight* light;
    RwLLLink WorldSectorInLight;
    RpWorldSector* sect;
} RpLightTie;

typedef struct rpWorldRegEntry {
    RpWorld* world;
    RwUInt32 memorySize;
    RwLLLink link;
} rpWorldRegEntry;

typedef struct rpWorldGlobals {
    RwFreeList* regEntryFreeList;
    RwLinkList worldList;
} rpWorldGlobals;

typedef struct rpFindSectorData {
    const RpWorldSector* worldSector;
    RwBool found;
} rpFindSectorData;

#define RWWORLDGLOBAL(var) (RWPLUGINOFFSET(rpWorldGlobals, RwEngineInstance, worldModule.globalsOffset)->var)

#define RpWorldGetDefaultSectorPipeline() \
    (*(RxPipeline**)((RwUInt8*)RwEngineInstance + lbl_8025F8A8.globalsOffset + 0x44))

#define rpCurWorld ((RpWorld*)RWSRCGLOBAL(curWorld))

#define SETCOORD(v, offset, value) (*(RwReal*)((RwUInt8*)&(v) + (offset)) = (value))

extern RwPluginRegistry sectorTKList;
extern RwModuleInfo lbl_8025F8A8;

RwPluginRegistry worldTKList = { sizeof(RpWorld), sizeof(RpWorld), 0, 0, NULL, NULL };

static RwSurfaceProperties defaultWorldSurfaceProps;

RwModuleInfo worldModule;

RwInt32 _rwPluginRegistryAddPluginStream(RwPluginRegistry* reg, RwUInt32 pluginID,
                                         RwPluginDataChunkReadCallBack readCB,
                                         RwPluginDataChunkWriteCallBack writeCB,
                                         RwPluginDataChunkGetSizeCallBack getSizeCB);
RpWorldSector* RpWorldSectorRender(RpWorldSector* worldSector);
const RwSphere* RpAtomicGetWorldBoundingSphere(RpAtomic* atomic);
RwInt32 RwCameraFrustumTestSphere(const RwCamera* camera, const RwSphere* sphere);
RwCamera* RwCameraForAllSectorsInFrustum(RwCamera* camera, RpWorldSectorCallBack callBack, void* data);
RwBool RwResourcesFreeResEntry(RwResEntry* entry);
void _rpTieDestroy(RpTie* tie);
void _rpLightTieDestroy(RpLightTie* tie);
RwBool fn_801542DC(void);
RwBool fn_801543D0(void);
RxPipeline* RxPipelineExecute(RxPipeline* pipeline, void* data, RwBool heapReset);
RwBBox* RwBBoxInitialize(RwBBox* boundingBox, const RwV3d* vertex);
RwBBox* RwBBoxAddPoint(RwBBox* boundingBox, const RwV3d* vertex);
RpBuildMesh* _rpBuildMeshCreate(RwUInt32 bufferSize);
RwBool _rpBuildMeshDestroy(RpBuildMesh* mesh);
RpBuildMesh* _rpBuildMeshAddTriangle(RpBuildMesh* mesh, RpMaterial* material, RwInt32 vert1, RwInt32 vert2,
                                     RwInt32 vert3);
RpMeshHeader* _rpMeshOptimise(RpBuildMesh* buildMesh, RwUInt32 flags);
RwBool _rpMeshDestroy(RpMeshHeader* meshHeader);
RpMaterialList* _rpMaterialListInitialize(RpMaterialList* matList);
RpMaterialList* _rpMaterialListDeinitialize(RpMaterialList* matList);
void* memset(void* dst, int val, unsigned long n);
RpWorld* RpWorldForAllWorldSectors(RpWorld* world, RpWorldSectorCallBack fpCallBack, void* data);

RpWorldSector* WorldFindSector(RpWorldSector* worldSector, void* data) {
    rpFindSectorData* findData = (rpFindSectorData*)data;

    if (findData->worldSector == worldSector) {
        findData->found = TRUE;
        return NULL;
    }

    return worldSector;
}

static inline void AtomicRenderVisible(RpAtomic* atomic) {
    if (atomic->object.flags & rpATOMICRENDER) {
        if (atomic->renderFrame != RWSRCGLOBAL(renderFrame)) {
            if (RwCameraFrustumTestSphere(RWSRCGLOBAL(curCamera), RpAtomicGetWorldBoundingSphere(atomic))) {
                atomic->renderCallBack(atomic);
            }

            atomic->renderFrame = RWSRCGLOBAL(renderFrame);
        }
    }
}

RpWorldSector* WorldSectorRender(RpWorldSector* worldSector, void* data) {
    if (RpWorldSectorRender(worldSector)) {
        {
            RwLLLink* end;
            RwLLLink* cur;

            cur = rwLinkListGetFirstLLLink(&worldSector->collAtomicsInWorldSector);
            end = rwLinkListGetTerminator(&worldSector->collAtomicsInWorldSector);
            while (cur != end) {
                AtomicRenderVisible(rwLLLinkGetData(cur, RpTie, lAtomicInWorldSector)->apAtom);
                cur = rwLLLinkGetNext(cur);
            }
        }

        {
            RwLLLink* cur;
            RwLLLink* end;

            cur = rwLinkListGetFirstLLLink(&worldSector->noCollAtomicsInWorldSector);
            end = rwLinkListGetTerminator(&worldSector->noCollAtomicsInWorldSector);
            while (cur != end) {
                AtomicRenderVisible(rwLLLinkGetData(cur, RpTie, lAtomicInWorldSector)->apAtom);
                cur = rwLLLinkGetNext(cur);
            }
        }
    }

    return worldSector;
}

void WorldSectorDeinstanceAll(RpSector* sector) {
    switch (sector->type) {
    case rwSECTORATOMIC: {
        RpWorldSector* worldSector = (RpWorldSector*)sector;
        RwLLLink* cur;
        RwLLLink* end;

        if (worldSector->repEntry) {
            RwResourcesFreeResEntry(worldSector->repEntry);
        }

        cur = rwLinkListGetFirstLLLink(&worldSector->collAtomicsInWorldSector);
        end = rwLinkListGetTerminator(&worldSector->collAtomicsInWorldSector);
        while (cur != end) {
            RpTie* tie = rwLLLinkGetData(cur, RpTie, lAtomicInWorldSector);

            cur = rwLLLinkGetNext(cur);
            _rpTieDestroy(tie);
        }

        cur = rwLinkListGetFirstLLLink(&worldSector->noCollAtomicsInWorldSector);
        end = rwLinkListGetTerminator(&worldSector->noCollAtomicsInWorldSector);
        while (cur != end) {
            RpTie* tie = rwLLLinkGetData(cur, RpTie, lAtomicInWorldSector);

            cur = rwLLLinkGetNext(cur);
            _rpTieDestroy(tie);
        }

        cur = rwLinkListGetFirstLLLink(&worldSector->lightsInWorldSector);
        end = rwLinkListGetTerminator(&worldSector->lightsInWorldSector);
        while (cur != end) {
            RpLightTie* tie = rwLLLinkGetData(cur, RpLightTie, lightInWorldSector);

            cur = rwLLLinkGetNext(cur);
            _rpLightTieDestroy(tie);
        }

        _rwPluginRegistryDeInitObject(&sectorTKList, worldSector);
        break;
    }
    case rwSECTORBUILD:
        break;
    default: {
        RpPlaneSector* planeSector = (RpPlaneSector*)sector;

        WorldSectorDeinstanceAll(planeSector->leftSubTree);
        WorldSectorDeinstanceAll(planeSector->rightSubTree);
        break;
    }
    }
}

void WorldSectorDestroyRecurse(RpSector* sector) {
    switch (sector->type) {
    case rwSECTORATOMIC: {
        RpWorldSector* worldSector = (RpWorldSector*)sector;
        RwLLLink* cur;
        RwLLLink* end;
        RwInt32 i;

        if (worldSector->repEntry) {
            RwResourcesFreeResEntry(worldSector->repEntry);
        }

        cur = rwLinkListGetFirstLLLink(&worldSector->collAtomicsInWorldSector);
        end = rwLinkListGetTerminator(&worldSector->collAtomicsInWorldSector);
        while (cur != end) {
            RpTie* tie = rwLLLinkGetData(cur, RpTie, lAtomicInWorldSector);

            cur = rwLLLinkGetNext(cur);
            _rpTieDestroy(tie);
        }

        cur = rwLinkListGetFirstLLLink(&worldSector->noCollAtomicsInWorldSector);
        end = rwLinkListGetTerminator(&worldSector->noCollAtomicsInWorldSector);
        while (cur != end) {
            RpTie* tie = rwLLLinkGetData(cur, RpTie, lAtomicInWorldSector);

            cur = rwLLLinkGetNext(cur);
            _rpTieDestroy(tie);
        }

        cur = rwLinkListGetFirstLLLink(&worldSector->lightsInWorldSector);
        end = rwLinkListGetTerminator(&worldSector->lightsInWorldSector);
        while (cur != end) {
            RpLightTie* tie = rwLLLinkGetData(cur, RpLightTie, lightInWorldSector);

            cur = rwLLLinkGetNext(cur);
            _rpLightTieDestroy(tie);
        }

        _rwPluginRegistryDeInitObject(&sectorTKList, worldSector);

        if (worldSector->vertices) {
            RwFree(worldSector->vertices);
            worldSector->vertices = NULL;
        }

        if (worldSector->normals) {
            RwFree(worldSector->normals);
            worldSector->normals = NULL;
        }

        if (worldSector->preLitLum) {
            RwFree(worldSector->preLitLum);
            worldSector->preLitLum = NULL;
        }

        if (worldSector->polygons) {
            RwFree(worldSector->polygons);
            worldSector->polygons = NULL;
        }

        if (worldSector->unk80) {
            RwFree(worldSector->unk80);
            worldSector->unk80 = NULL;
        }

        for (i = 0; i < 8; i++) {
            if (worldSector->texCoords[i]) {
                RwFree(worldSector->texCoords[i]);
                worldSector->texCoords[i] = NULL;
            }
        }

        RwFree(worldSector);
        break;
    }
    case rwSECTORBUILD:
        RwFree(sector);
        break;
    default: {
        RpPlaneSector* planeSector = (RpPlaneSector*)sector;

        WorldSectorDestroyRecurse(planeSector->leftSubTree);
        planeSector->leftSubTree = NULL;
        WorldSectorDestroyRecurse(planeSector->rightSubTree);
        planeSector->rightSubTree = NULL;
        RwFree(planeSector);
        break;
    }
    }
}

void* WorldClose(void* instance, RwInt32 offset, RwInt32 size) {
    if (RWWORLDGLOBAL(regEntryFreeList)) {
        RwFreeListDestroy(RWWORLDGLOBAL(regEntryFreeList));
        RWWORLDGLOBAL(regEntryFreeList) = NULL;
    }

    fn_801542DC();

    worldModule.numInstances--;

    return instance;
}

void* WorldOpen(void* instance, RwInt32 offset, RwInt32 size) {
    defaultWorldSurfaceProps.ambient = 0.0f;
    defaultWorldSurfaceProps.diffuse = 0.0f;
    defaultWorldSurfaceProps.specular = 0.0f;

    worldModule.globalsOffset = offset;

    if (!fn_801543D0()) {
        return NULL;
    }

    RWWORLDGLOBAL(regEntryFreeList) = RwFreeListCreate(sizeof(rpWorldRegEntry), 3, 0);
    if (!RWWORLDGLOBAL(regEntryFreeList)) {
        return NULL;
    }

    rwLinkListInitialize(&RWWORLDGLOBAL(worldList));

    worldModule.numInstances++;

    return instance;
}

RpWorldSector* _rpSectorDefaultRenderCallBack(RpWorldSector* worldSector) {
    RxPipeline* pipeline;

    if (worldSector->numPolygons == 0) {
        return worldSector;
    }

        if (worldSector->pipeline) {
            pipeline = worldSector->pipeline;
        } else if (rpCurWorld->pipeline) {
            pipeline = rpCurWorld->pipeline;
        } else {
            pipeline = RpWorldGetDefaultSectorPipeline();
        }

    if (!RxPipelineExecute(pipeline, worldSector, TRUE)) {
        return NULL;
    }

    return worldSector;
}

RwBool _rpWorldForAllGlobalLights(RpLightCallBack callBack, void* data) {
    RwLLLink* end;
    RwLLLink* next;
    RwLLLink* cur;

    cur = rwLinkListGetFirstLLLink(&rpCurWorld->directionalLightList);
    end = rwLinkListGetTerminator(&rpCurWorld->directionalLightList);
    while (cur != end) {
        RpLight* light = rwLLLinkGetData(cur, RpLight, inWorld);

        next = rwLLLinkGetNext(cur);
        if (light) {
            if (!callBack(light, data)) {
                return TRUE;
            }
        }

        cur = next;
    }

    return TRUE;
}

RpWorldSector* _rpWorldSectorForAllLocalLights(RpWorldSector* worldSector, RpLightCallBack callBack, void* data) {
    RwLLLink* end;
    RwLLLink* next;
    RwLLLink* cur;

    cur = rwLinkListGetFirstLLLink(&worldSector->lightsInWorldSector);
    end = rwLinkListGetTerminator(&worldSector->lightsInWorldSector);
    while (cur != end) {
        RpLight* light = rwLLLinkGetData(cur, RpLightTie, lightInWorldSector)->light;

        next = rwLLLinkGetNext(cur);
        if (light) {
            if (!callBack(light, data)) {
                return worldSector;
            }
        }

        cur = next;
    }

    return worldSector;
}

RwBool _rpWorldFindBBox(RpWorld* world, RwBBox* boundingBox) {
    RpSector* sectorStack[rpWORLDMAXBSPDEPTH];
    RpSector* sector;
    RwInt32 sectorStackPos;
    RwBool found;

    sectorStackPos = 0;
    found = FALSE;
    sector = world->rootSector;

    do {
        if (sector->type < 0) {
            RpWorldSector* worldSector = (RpWorldSector*)sector;

            if (!found) {
                RwBBoxInitialize(boundingBox, &worldSector->tightBoundingBox.inf);
                found = TRUE;
            } else {
                RwBBoxAddPoint(boundingBox, &worldSector->tightBoundingBox.inf);
            }

            RwBBoxAddPoint(boundingBox, &worldSector->tightBoundingBox.sup);

            sector = sectorStack[sectorStackPos--];
        } else {
            RpPlaneSector* planeSector = (RpPlaneSector*)sector;

            sector = planeSector->leftSubTree;
            sectorStack[++sectorStackPos] = planeSector->rightSubTree;
        }
    } while (sectorStackPos >= 0);

    return TRUE;
}

RpWorld* _rpWorldSetupSectorBoundingBoxes(RpWorld* world) {
    RwBBox bbox;
    RpSector* sectorStack[rpWORLDMAXBSPDEPTH];
    RwBBox bboxStack[rpWORLDMAXBSPDEPTH];
    RwInt32 sectorStackPos;
    RpSector* sector;

    bbox = world->boundingBox;
    sector = world->rootSector;
    sectorStackPos = 0;

    while (sectorStackPos >= 0) {
        if (sector->type < 0) {
            ((RpWorldSector*)sector)->boundingBox = bbox;

            sector = sectorStack[sectorStackPos];
            bbox = bboxStack[sectorStackPos--];
        } else {
            RpPlaneSector* planeSector = (RpPlaneSector*)sector;

            sectorStackPos++;
            sectorStack[sectorStackPos] = planeSector->rightSubTree;
            bboxStack[sectorStackPos] = bbox;
            SETCOORD(bboxStack[sectorStackPos].inf, planeSector->type, planeSector->rightValue);
            SETCOORD(bbox.sup, planeSector->type, planeSector->leftValue);
            sector = planeSector->leftSubTree;
        }
    }

    return world;
}

void _rpWorldRegisterWorld(RpWorld* world, RwUInt32 memorySize) {
    rpWorldRegEntry* entry = (rpWorldRegEntry*)RwFreeListAlloc(RWWORLDGLOBAL(regEntryFreeList));

    if (entry) {
        entry->world = world;
        entry->memorySize = memorySize;
        rwLinkListAddLLLink(&RWWORLDGLOBAL(worldList), &entry->link);
    }
}

void _rpWorldUnregisterWorld(RpWorld* world) {
    RwLLLink* end = rwLinkListGetTerminator(&RWWORLDGLOBAL(worldList));
    RwLLLink* cur = rwLinkListGetFirstLLLink(&RWWORLDGLOBAL(worldList));

    while (cur != end) {
        rpWorldRegEntry* entry = rwLLLinkGetData(cur, rpWorldRegEntry, link);

        if (entry->world == world) {
            rwLLLinkRemove(&entry->link);
            RwFreeListFree(RWWORLDGLOBAL(regEntryFreeList), entry);
            return;
        }

        cur = rwLLLinkGetNext(cur);
    }
}

static inline RpWorld* WorldSectorFinishMesh(RpWorld* world, RpWorldSector* worldSector, RpBuildMesh* buildMesh) {
    RpMeshHeader* mesh;

    if (world->flags & rpWORLDTRISTRIP) {
        mesh = _rpMeshOptimise(buildMesh, 1);
    } else {
        mesh = _rpMeshOptimise(buildMesh, 0);
    }

    if (mesh) {
        worldSector->mesh = mesh;
    } else {
        _rpBuildMeshDestroy(buildMesh);
        return NULL;
    }

    return world;
}

static inline void WorldSectorAddTriangles(RpBuildMesh* buildMesh, RpWorldSector* worldSector, RpMaterial** materials) {
    RwInt32 i;

    for (i = 0; i < worldSector->numPolygons; i++) {
        RpPolygon* polygon = &worldSector->polygons[i];

        _rpBuildMeshAddTriangle(buildMesh, materials[polygon->matIndex], polygon->vertIndex[0],
                                polygon->vertIndex[1], polygon->vertIndex[2]);
    }
}

RpWorld* RpWorldUnlock(RpWorld* world) {
    RpSector* sectorStack[rpWORLDMAXBSPDEPTH];
    RpSector* sector;
    RwInt32 sectorStackPos;

    sectorStackPos = 0;
    sector = world->rootSector;

    do {
        if (sector->type < 0) {
            RpWorldSector* worldSector = (RpWorldSector*)sector;
            RpMaterial** materials = world->matList.materials + worldSector->matListWindowBase;

            if (!worldSector->mesh) {
                RpBuildMesh* buildMesh = _rpBuildMeshCreate(worldSector->numPolygons);

                if (buildMesh) {
                    WorldSectorAddTriangles(buildMesh, worldSector, materials);

                    world = WorldSectorFinishMesh(world, worldSector, buildMesh);
                    if (!world) {
                        return NULL;
                    }
                } else {
                    return NULL;
                }
            }

            sector = sectorStack[sectorStackPos--];
        } else {
            RpPlaneSector* planeSector = (RpPlaneSector*)sector;

            sector = planeSector->leftSubTree;
            sectorStack[++sectorStackPos] = planeSector->rightSubTree;
        }
    } while (sectorStackPos >= 0);

    return world;
}

RwInt32 fn_80151C24(const RwUInt8* tree) {
    if (tree) {
        RwInt32 stack[18];
        RwInt32 node = 1;
        RwInt32 maxNode = 1;
        RwInt32 stackPos = 0;

        do {
            if (node > maxNode) {
                maxNode = node;
            }

            if (tree[node * 3] & 0x80) {
                stack[++stackPos] = node + node + 1;
                node = node + node;
            } else {
                node = stack[stackPos--];
            }
        } while (stackPos >= 0);

        return (maxNode + 1) * 3;
    }

    return 0;
}

RpWorld* fn_80151CA4(RpWorld* world, const RwSurfaceProperties* surfaceProps) {
    world->surfaceProps = *surfaceProps;
    return world;
}

RpWorld* RpWorldSectorGetWorld(const RpWorldSector* worldSector) {
    RwLLLink* end = rwLinkListGetTerminator(&RWWORLDGLOBAL(worldList));
    RwLLLink* cur = rwLinkListGetFirstLLLink(&RWWORLDGLOBAL(worldList));

    while (cur != end) {
        rpWorldRegEntry* entry = rwLLLinkGetData(cur, rpWorldRegEntry, link);

        if (entry->world->object.privateFlags & rpWORLDSINGLEMALLOC) {
            if ((RwUInt8*)worldSector >= (RwUInt8*)entry->world &&
                (RwUInt8*)worldSector < (RwUInt8*)entry->world + entry->memorySize) {
                return entry->world;
            }
        } else {
            rpFindSectorData findData;

            findData.worldSector = worldSector;
            findData.found = FALSE;
            RpWorldForAllWorldSectors(entry->world, WorldFindSector, &findData);
            if (findData.found) {
                return entry->world;
            }
        }

        cur = rwLLLinkGetNext(cur);
    }

    return NULL;
}

RpWorld* RpWorldRender(RpWorld* world) {
    RwCameraForAllSectorsInFrustum(RWSRCGLOBAL(curCamera), WorldSectorRender, world);
    return world;
}

static inline void WorldDestroyMeshes(RpWorld* world) {
    RpSector* sectorStack[rpWORLDMAXBSPDEPTH];
    RpSector* sector;
    RwInt32 sectorStackPos;

    sectorStackPos = 0;
    sector = world->rootSector;

    do {
        if (sector->type < 0) {
            RpWorldSector* worldSector = (RpWorldSector*)sector;

            if (worldSector->mesh) {
                _rpMeshDestroy(worldSector->mesh);
                worldSector->mesh = NULL;
            }

            sector = sectorStack[sectorStackPos--];
        } else {
            RpPlaneSector* planeSector = (RpPlaneSector*)sector;

            sectorStack[++sectorStackPos] = planeSector->rightSubTree;
            sector = planeSector->leftSubTree;
        }
    } while (sectorStackPos >= 0);
}

RwBool RpWorldDestroy(RpWorld* world) {
    _rpWorldUnregisterWorld(world);

    WorldDestroyMeshes(world);

    _rpMaterialListDeinitialize(&world->matList);

    if (world->object.privateFlags & rpWORLDSINGLEMALLOC) {
        WorldSectorDeinstanceAll(world->rootSector);
        _rwPluginRegistryDeInitObject(&worldTKList, world);
        RwFree(world);
    } else {
        WorldSectorDestroyRecurse(world->rootSector);
        _rwPluginRegistryDeInitObject(&worldTKList, world);
        RwFree(world);
    }

    return TRUE;
}

RpWorld* RpWorldSetSectorRenderCallBack(RpWorld* world, RpWorldSectorCallBackRender fpCallBack) {
    if (!fpCallBack) {
        fpCallBack = _rpSectorDefaultRenderCallBack;
    }

    world->renderCallBack = fpCallBack;
    return world;
}

RpWorld* RpWorldCreate(RwBBox* boundingBox) {
    RpWorld* world = (RpWorld*)RwMalloc(worldTKList.sizeOfStruct);
    RpWorldSector* worldSector;

    if (!world) {
        RWERROR((E_RW_NOMEM, worldTKList.sizeOfStruct));
        return NULL;
    }

    rwObjectInitialize(world, rpWORLD, 0);
    _rpMaterialListInitialize(&world->matList);
    world->renderOrder = rpWORLDRENDERFRONT2BACK;
    world->flags = 0;

    worldSector = (RpWorldSector*)RwMalloc(sectorTKList.sizeOfStruct);
    if (!worldSector) {
        RWERROR((E_RW_NOMEM, sizeof(RpWorldSector*)));
        RwFree(world);
        return NULL;
    }

    worldSector->type = rwSECTORATOMIC;
    worldSector->unk80 = NULL;
    worldSector->repEntry = NULL;
    worldSector->mesh = NULL;
    rwLinkListInitialize(&worldSector->collAtomicsInWorldSector);
    rwLinkListInitialize(&worldSector->noCollAtomicsInWorldSector);
    rwLinkListInitialize(&worldSector->lightsInWorldSector);
    worldSector->numVertices = 0;
    worldSector->numPolygons = 0;
    worldSector->vertices = NULL;
    worldSector->polygons = NULL;
    worldSector->normals = NULL;
    memset(worldSector->texCoords, 0, sizeof(worldSector->texCoords));
    worldSector->preLitLum = NULL;
    worldSector->boundingBox.inf = boundingBox->inf;
    worldSector->boundingBox.sup = boundingBox->sup;
    worldSector->tightBoundingBox.inf = boundingBox->inf;
    worldSector->tightBoundingBox.sup = boundingBox->sup;
    worldSector->pipeline = NULL;

    world->rootSector = (RpSector*)worldSector;
    world->numTexCoordSets = 0;
    world->worldOrigin.z = 0.0f;
    world->worldOrigin.y = 0.0f;
    world->worldOrigin.x = 0.0f;
    world->boundingBox.inf = boundingBox->inf;
    world->boundingBox.sup = boundingBox->sup;
    rwLinkListInitialize(&world->clumpList);
    world->numClumpsInWorld = 0;
    world->currentClumpLink = rwLinkListGetTerminator(&world->clumpList);
    world->surfaceProps = defaultWorldSurfaceProps;
    rwLinkListInitialize(&world->lightList);
    rwLinkListInitialize(&world->directionalLightList);
    world->renderCallBack = _rpSectorDefaultRenderCallBack;
    world->pipeline = NULL;

    _rpWorldRegisterWorld(world, worldTKList.sizeOfStruct);

    _rwPluginRegistryInitObject(&worldTKList, world);
    _rwPluginRegistryInitObject(&sectorTKList, worldSector);

    if (!RpWorldUnlock(world)) {
        RpWorldDestroy(world);
        return NULL;
    }

    return world;
}

RpWorld* RpWorldForAllWorldSectors(RpWorld* world, RpWorldSectorCallBack fpCallBack, void* data) {
    RpSector* sectorStack[rpWORLDMAXBSPDEPTH];
    RpSector* sector;
    RwInt32 sectorStackPos;

    sector = world->rootSector;
    sectorStackPos = 0;

    do {
        if (sector->type < 0) {
            if (!fpCallBack((RpWorldSector*)sector, data)) {
                return world;
            }

            sector = sectorStack[sectorStackPos--];
        } else {
            RpPlaneSector* planeSector = (RpPlaneSector*)sector;

            sectorStack[++sectorStackPos] = planeSector->rightSubTree;
            sector = planeSector->leftSubTree;
        }
    } while (sectorStackPos >= 0);

    return world;
}

RwInt32 RpWorldRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB,
                              RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB) {
    return _rwPluginRegistryAddPlugin(&worldTKList, size, pluginID, constructCB, destructCB, copyCB);
}

RwInt32 RpWorldRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB,
                                    RwPluginDataChunkWriteCallBack writeCB,
                                    RwPluginDataChunkGetSizeCallBack getSizeCB) {
    return _rwPluginRegistryAddPluginStream(&worldTKList, pluginID, readCB, writeCB, getSizeCB);
}

void* _rpMaterialOpen(void* instance, RwInt32 offset, RwInt32 size);
void* _rpMaterialClose(void* instance, RwInt32 offset, RwInt32 size);
void* _rpMeshOpen(void* instance, RwInt32 offset, RwInt32 size);
void* _rpMeshClose(void* instance, RwInt32 offset, RwInt32 size);
void* _rpGeometryOpen(void* instance, RwInt32 offset, RwInt32 size);
void* _rpGeometryClose(void* instance, RwInt32 offset, RwInt32 size);
void* _rpClumpOpen(void* instance, RwInt32 offset, RwInt32 size);
void* _rpClumpClose(void* instance, RwInt32 offset, RwInt32 size);
void* _rpLightOpen(void* instance, RwInt32 offset, RwInt32 size);
void* _rpLightClose(void* instance, RwInt32 offset, RwInt32 size);
void* _rpSectorOpen(void* instance, RwInt32 offset, RwInt32 size);
void* _rpSectorClose(void* instance, RwInt32 offset, RwInt32 size);
void* _rpBinaryWorldOpen(void* instance, RwInt32 offset, RwInt32 size);
void* _rpBinaryWorldClose(void* instance, RwInt32 offset, RwInt32 size);
RwBool _rpWorldObjRegisterExtensions(void);
RwBool _rpClumpRegisterExtensions(void);
RwBool _rpWorldPipeAttach(void);

RwBool RpWorldPluginAttach(void) {
    RwInt32 success;

    success = RwEngineRegisterPlugin(4, 0x501, _rpMaterialOpen, _rpMaterialClose);
    success |= RwEngineRegisterPlugin(0x30, 0x502, _rpMeshOpen, _rpMeshClose);
    success |= RwEngineRegisterPlugin(4, 0x503, _rpGeometryOpen, _rpGeometryClose);
    success |= RwEngineRegisterPlugin(8, 0x504, _rpClumpOpen, _rpClumpClose);
    success |= RwEngineRegisterPlugin(4, 0x505, _rpLightOpen, _rpLightClose);
    success |= RwEngineRegisterPlugin(0, 0x50A, _rpSectorOpen, _rpSectorClose);
    success |= RwEngineRegisterPlugin(sizeof(rpWorldGlobals), 0x507, WorldOpen, WorldClose);
    success |= RwEngineRegisterPlugin(0, 0x50B, _rpBinaryWorldOpen, _rpBinaryWorldClose);

    if (success < 0) {
        return FALSE;
    }

    if (!_rpWorldObjRegisterExtensions()) {
        return FALSE;
    }

    if (!_rpClumpRegisterExtensions()) {
        return FALSE;
    }

    return _rpWorldPipeAttach() ? TRUE : FALSE;
}
