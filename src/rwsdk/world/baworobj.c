/*
 * World objects: the camera, atomic, clump and light extensions that tie
 * frame-carrying objects into the world's sector tree, plus the mesh and
 * native-data stream extensions for geometries and world sectors.
 */

#include "rwsdk/rpworld_int.h"

#define rwPLUGIN_ID 2 /* world */

#define rwID_WORLDOBJPLUGIN 0x509
#define rwID_BINMESHPLUGIN 0x50E
#define rwID_NATIVEDATAPLUGIN 0x510
#define rwID_RIGHTTORENDER 0x1F

#define rwSTANDARDHINTRENDERF2B 22

#define E_RW_FRAMESYNC 0x80000016
#define E_RP_WORLD_CLUMPINWORLD 3

#define rpWORLDSECTORGROWBY 50

typedef struct RpCameraExt {
    /* 0x00 */ RpWorldSector** frustumSectors;
    /* 0x04 */ RwInt32 space;
    /* 0x08 */ RwInt32 position;
    /* 0x0C */ RpWorld* world;
    /* 0x10 */ RwCamera* (*oldBeginUpdate)(RwCamera* camera);
    /* 0x14 */ RwCamera* (*oldEndUpdate)(RwCamera* camera);
    /* 0x18 */ RwObjectHasFrame* (*oldSync)(RwObjectHasFrame* object);
} RpCameraExt;

typedef struct RpAtomicExt {
    RpWorld* world;
    RwObjectHasFrame* (*oldSync)(RwObjectHasFrame* object);
} RpAtomicExt;

typedef struct RpClumpExt {
    RpWorld* world;
} RpClumpExt;

typedef struct RpLightExt {
    RpWorld* world;
    RwObjectHasFrame* (*oldSync)(RwObjectHasFrame* object);
} RpLightExt;

typedef struct rpWorldObjGlobals {
    RwFreeList* tieFreeList;
    RwFreeList* lightTieFreeList;
} rpWorldObjGlobals;

typedef RpAtomic* (*RpAtomicCallBack)(RpAtomic* atomic, void* data);
typedef RpLight* (*RpLightCallBack2)(RpLight* light, void* data);
typedef RwCamera* (*RwCameraCallBack)(RwCamera* camera, void* data);

#define RWWORLDOBJGLOBAL(var) (RWPLUGINOFFSET(rpWorldObjGlobals, RwEngineInstance, worldObjModule.globalsOffset)->var)

#define CAMERAEXT(camera) (RWPLUGINOFFSET(RpCameraExt, camera, cameraExtOffset))
#define ATOMICEXT(atomic) (RWPLUGINOFFSET(RpAtomicExt, atomic, atomicExtOffset))
#define CLUMPEXT(clump) (RWPLUGINOFFSET(RpClumpExt, clump, clumpExtOffset))
#define LIGHTEXT(light) (RWPLUGINOFFSET(RpLightExt, light, lightExtOffset))

#define RwRealloc(_p, _s) ((RWSRCGLOBAL(memoryFuncs).rwrealloc)((_p), (_s)))

/* The integer views are volatile so the float is always reread from memory. */
typedef union RwSplitBits {
    RwReal nReal;
    volatile RwInt32 nInt;
    volatile RwUInt32 nUInt;
} RwSplitBits;

RwModuleInfo worldObjModule;
static RwInt32 lightExtOffset;
static RwInt32 clumpExtOffset;
static RwInt32 atomicExtOffset;
static RwInt32 cameraExtOffset;

RwInt32 RwCameraRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB,
                               RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);
RwInt32 RpAtomicRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB,
                               RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);
RwInt32 RpAtomicRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB,
                                     RwPluginDataChunkWriteCallBack writeCB,
                                     RwPluginDataChunkGetSizeCallBack getSizeCB);
RwInt32 RpClumpRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB,
                              RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);
RwInt32 RpLightRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB,
                              RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);
RwInt32 RpGeometryRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB,
                                 RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);
RwInt32 RpGeometryRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB,
                                       RwPluginDataChunkWriteCallBack writeCB,
                                       RwPluginDataChunkGetSizeCallBack getSizeCB);
RwInt32 RpWorldSectorRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB,
                                    RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);
RwInt32 RpWorldSectorRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB,
                                          RwPluginDataChunkWriteCallBack writeCB,
                                          RwPluginDataChunkGetSizeCallBack getSizeCB);
RwInt32 RpWorldRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB,
                              RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);
RwInt32 RpWorldRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB,
                                    RwPluginDataChunkWriteCallBack writeCB,
                                    RwPluginDataChunkGetSizeCallBack getSizeCB);

RwStream* _rpReadAtomicRights(RwStream* stream, RwInt32 binaryLength, void* object, RwInt32 offsetInObject,
                              RwInt32 sizeInObject);
RwStream* _rpWriteAtomicRights(RwStream* stream, RwInt32 binaryLength, const void* object,
                               RwInt32 offsetInObject, RwInt32 sizeInObject);
RwInt32 _rpSizeAtomicRights(const void* object, RwInt32 offsetInObject, RwInt32 sizeInObject);
RwStream* _rpReadWorldRights(RwStream* stream, RwInt32 binaryLength, void* object, RwInt32 offsetInObject,
                             RwInt32 sizeInObject);
RwStream* _rpWriteWorldRights(RwStream* stream, RwInt32 binaryLength, const void* object,
                              RwInt32 offsetInObject, RwInt32 sizeInObject);
RwInt32 _rpSizeWorldRights(const void* object, RwInt32 offsetInObject, RwInt32 sizeInObject);
RwStream* _rpReadSectRights(RwStream* stream, RwInt32 binaryLength, void* object, RwInt32 offsetInObject,
                            RwInt32 sizeInObject);
RwStream* _rpWriteSectRights(RwStream* stream, RwInt32 binaryLength, const void* object,
                             RwInt32 offsetInObject, RwInt32 sizeInObject);
RwInt32 _rpSizeSectRights(const void* object, RwInt32 offsetInObject, RwInt32 sizeInObject);

RwStream* _rpMeshWrite(const RpMeshHeader* meshHeader, const void* object, RwStream* stream,
                       const RpMaterialList* matList);
RpMeshHeader* _rpMeshRead(RwStream* stream, const void* object, const RpMaterialList* matList);
RwInt32 _rpMeshSize(const RpMeshHeader* meshHeader, const void* object);
RwStream* _rpGeometryNativeWrite(RwStream* stream, const RpGeometry* geometry);
RwStream* _rpGeometryNativeRead(RwStream* stream, RpGeometry* geometry);
RwInt32 _rpGeometryNativeSize(const RpGeometry* geometry);
RwStream* _rpWorldSectorNativeWrite(RwStream* stream, const RpWorldSector* sector);
RwStream* _rpWorldSectorNativeRead(RwStream* stream, RpWorldSector* sector);
RwInt32 _rpWorldSectorNativeSize(const RpWorldSector* sector);
RwBool fn_8015470C(void);

const RwSphere* RpAtomicGetWorldBoundingSphere(RpAtomic* atomic);
RwFrame* RwFrameUpdateObjects(RwFrame* frame);
RwMatrix* RwFrameGetLTM(RwFrame* frame);
RwMatrix* RwMatrixOptimize(RwMatrix* matrix, const void* tolerance);
RpWorld* RpWorldSectorGetWorld(const RpWorldSector* worldSector);
RpClump* RpClumpForAllAtomics(RpClump* clump, RpAtomicCallBack callback, void* data);
RpClump* RpClumpForAllLights(RpClump* clump, RpLightCallBack2 callback, void* data);
RpClump* RpClumpForAllCameras(RpClump* clump, RwCameraCallBack callback, void* data);
RpWorld* RpWorldAddCamera(RpWorld* world, RwCamera* camera);
RpWorld* RpWorldAddAtomic(RpWorld* world, RpAtomic* atomic);
RpWorld* RpWorldAddClump(RpWorld* world, RpClump* clump);
RpWorld* RpWorldAddLight(RpWorld* world, RpLight* light);
RpWorld* WorldSyncCamera(RpWorld* world, RwCamera* camera);

void* WorldObjectOpen(void* instance, RwInt32 offset, RwInt32 size) {
    worldObjModule.globalsOffset = offset;

    RWWORLDOBJGLOBAL(tieFreeList) = RwFreeListCreate(sizeof(RpTie), 100, 0);
    if (!RWWORLDOBJGLOBAL(tieFreeList)) {
        return NULL;
    }

    RWWORLDOBJGLOBAL(lightTieFreeList) = RwFreeListCreate(sizeof(RpLightTie), 20, 0);
    if (!RWWORLDOBJGLOBAL(lightTieFreeList)) {
        RwFreeListDestroy(RWWORLDOBJGLOBAL(tieFreeList));
        RWWORLDOBJGLOBAL(tieFreeList) = NULL;
        return NULL;
    }

    RWSRCGLOBAL(renderFrame) = 1;

    worldObjModule.numInstances++;

    return instance;
}

void* WorldObjectClose(void* instance, RwInt32 offset, RwInt32 size) {
    if (RWWORLDOBJGLOBAL(lightTieFreeList)) {
        RwFreeListDestroy(RWWORLDOBJGLOBAL(lightTieFreeList));
        RWWORLDOBJGLOBAL(lightTieFreeList) = NULL;
    }

    if (RWWORLDOBJGLOBAL(tieFreeList)) {
        RwFreeListDestroy(RWWORLDOBJGLOBAL(tieFreeList));
        RWWORLDOBJGLOBAL(tieFreeList) = NULL;
    }

    worldObjModule.numInstances--;

    return instance;
}

RwCamera* WorldCameraBeginUpdate(RwCamera* camera) {
    RpCameraExt* cameraExt = CAMERAEXT(camera);

    RWSRCGLOBAL(curWorld) = cameraExt->world;
    RWSRCGLOBAL(renderFrame)++;

    return cameraExt->oldBeginUpdate(camera);
}

RwCamera* WorldCameraEndUpdate(RwCamera* camera) {
    RpCameraExt* cameraExt = CAMERAEXT(camera);

    RWSRCGLOBAL(curWorld) = NULL;

    return cameraExt->oldEndUpdate(camera);
}

RwObjectHasFrame* WorldCameraSync(RwObjectHasFrame* object) {
    RpCameraExt* cameraExt = CAMERAEXT(object);

    if (cameraExt->oldSync(object)) {
        RpWorld* world = cameraExt->world;

        if (world) {
            RwStandardFunc hint = RWSRCGLOBAL(stdFunc[rwSTANDARDHINTRENDERF2B]);

            WorldSyncCamera(world, (RwCamera*)object);
            hint(NULL, NULL, world->renderOrder == rpWORLDRENDERFRONT2BACK);
        }

        return object;
    }

    return NULL;
}

void* WorldInitCameraExt(void* object, RwInt32 offset, RwInt32 size) {
    RwCamera* camera = (RwCamera*)object;
    RpCameraExt* cameraExt = CAMERAEXT(camera);

    cameraExt->frustumSectors = NULL;
    cameraExt->space = 0;
    cameraExt->position = 0;
    cameraExt->oldBeginUpdate = camera->beginUpdate;
    cameraExt->oldEndUpdate = camera->endUpdate;
    cameraExt->oldSync = camera->object.sync;
    camera->object.sync = WorldCameraSync;
    camera->beginUpdate = WorldCameraBeginUpdate;
    camera->endUpdate = WorldCameraEndUpdate;
    cameraExt->world = NULL;

    return object;
}

void* WorldCopyCameraExt(void* dstObject, const void* srcObject, RwInt32 offset, RwInt32 size) {
    RpCameraExt* dstExt = CAMERAEXT(dstObject);
    const RpCameraExt* srcExt = CAMERAEXT(srcObject);

    dstExt->frustumSectors = NULL;
    dstExt->space = 0;
    dstExt->position = 0;

    if (srcExt->world) {
        RpWorldAddCamera(srcExt->world, (RwCamera*)dstObject);
    }

    return dstObject;
}

void* WorldDeInitCameraExt(void* object, RwInt32 offset, RwInt32 size) {
    RwCamera* camera = (RwCamera*)object;
    RpCameraExt* cameraExt = CAMERAEXT(camera);

    if (cameraExt->frustumSectors) {
        RwFree(cameraExt->frustumSectors);
    }

    cameraExt->frustumSectors = NULL;
    cameraExt->space = 0;
    cameraExt->position = 0;
    camera->beginUpdate = cameraExt->oldBeginUpdate;
    camera->endUpdate = cameraExt->oldEndUpdate;
    camera->object.sync = cameraExt->oldSync;

    return object;
}

RwBool _rpLightTieDestroy(RpLightTie* tie) {
    rwLLLinkRemove(&tie->WorldSectorInLight);
    rwLLLinkRemove(&tie->lightInWorldSector);
    RwFreeListFree(RWWORLDOBJGLOBAL(lightTieFreeList), tie);

    return TRUE;
}

RwBool _rpTieDestroy(RpTie* tie) {
    if (tie->apAtom && tie->worldSector) {
        rwLLLinkRemove(&tie->lWorldSectorInAtomic);
        rwLLLinkRemove(&tie->lAtomicInWorldSector);
        RwFreeListFree(RWWORLDOBJGLOBAL(tieFreeList), tie);
    }

    return TRUE;
}

void WorldAttachAtomicSphere(RpWorld* world, RpAtomic* atomic) {
    RpSector* sectorStack[rpWORLDMAXBSPDEPTH];
    RwV3d inf;
    RwV3d sup;
    const RwSphere* sphere;
    RpSector* sector;
    RwInt32 sectorStackPos = 0;

    sphere = RpAtomicGetWorldBoundingSphere(atomic);
    inf = sphere->center;
    sup = sphere->center;
    inf.x -= sphere->radius;
    inf.y -= sphere->radius;
    inf.z -= sphere->radius;
    sup.x += sphere->radius;
    sup.y += sphere->radius;
    sup.z += sphere->radius;

    sector = world->rootSector;

    do {
        if (sector->type < 0) {
            RpTie* tie = (RpTie*)RwFreeListAlloc(RWWORLDOBJGLOBAL(tieFreeList));

            tie->worldSector = (RpWorldSector*)sector;
            tie->apAtom = atomic;

            if (atomic->object.object.flags & rpATOMICCOLLISIONTEST) {
                rwLinkListAddLLLink(&((RpWorldSector*)sector)->collAtomicsInWorldSector, &tie->lAtomicInWorldSector);
            } else {
                rwLinkListAddLLLink(&((RpWorldSector*)sector)->noCollAtomicsInWorldSector, &tie->lAtomicInWorldSector);
            }

            rwLinkListAddLLLink(&atomic->llWorldSectorsInAtomic, &tie->lWorldSectorInAtomic);

            sector = sectorStack[sectorStackPos--];
        } else {
            RpPlaneSector* planeSector = (RpPlaneSector*)sector;

            if (GETCOORD(inf, planeSector->type) < planeSector->leftValue) {
                sector = planeSector->leftSubTree;

                if (planeSector->rightValue < GETCOORD(sup, planeSector->type)) {
                    sectorStack[++sectorStackPos] = planeSector->rightSubTree;
                }
            } else if (planeSector->rightValue < GETCOORD(sup, planeSector->type)) {
                sector = planeSector->rightSubTree;
            } else {
                sector = sectorStack[sectorStackPos--];
            }
        }
    } while (sectorStackPos >= 0);
}

RwObjectHasFrame* WorldAtomicSync(RwObjectHasFrame* object) {
    RpAtomic* atomic = (RpAtomic*)object;
    RwLLLink* end;
    RwLLLink* cur;
    RpAtomicExt* atomicExt;
    RpWorld* world;

    atomicExt = ATOMICEXT(atomic);
    if (atomicExt->oldSync(object)) {
        world = atomicExt->world;

        if (world) {
            cur = rwLinkListGetFirstLLLink(&atomic->llWorldSectorsInAtomic);
            end = rwLinkListGetTerminator(&atomic->llWorldSectorsInAtomic);
            while (cur != end) {
                RpTie* tie = rwLLLinkGetData(cur, RpTie, lWorldSectorInAtomic);

                cur = rwLLLinkGetNext(cur);
                _rpTieDestroy(tie);
            }

            WorldAttachAtomicSphere(world, atomic);
        }

        return object;
    }

    return NULL;
}

void* WorldInitAtomicExt(void* object, RwInt32 offset, RwInt32 size) {
    RpAtomic* atomic = (RpAtomic*)object;
    RpAtomicExt* atomicExt = ATOMICEXT(atomic);

    atomicExt->world = NULL;
    atomic->renderFrame = RWSRCGLOBAL(renderFrame) - 1;
    atomicExt->oldSync = atomic->object.sync;
    atomic->object.sync = WorldAtomicSync;

    return object;
}

void* WorldCopyAtomicExt(void* dstObject, const void* srcObject, RwInt32 offset, RwInt32 size) {
    return dstObject;
}

void* WorldDeInitAtomicExt(void* object, RwInt32 offset, RwInt32 size) {
    RpAtomic* atomic = (RpAtomic*)object;
    RwLLLink* end;
    RwLLLink* cur;
    RpAtomicExt* atomicExt;

    atomicExt = ATOMICEXT(atomic);
    cur = rwLinkListGetFirstLLLink(&atomic->llWorldSectorsInAtomic);
    end = rwLinkListGetTerminator(&atomic->llWorldSectorsInAtomic);
    while (cur != end) {
        RpTie* tie = rwLLLinkGetData(cur, RpTie, lWorldSectorInAtomic);

        cur = rwLLLinkGetNext(cur);
        _rpTieDestroy(tie);
    }

    atomic->object.sync = atomicExt->oldSync;

    return object;
}

void* WorldInitClumpExt(void* object, RwInt32 offset, RwInt32 size) {
    CLUMPEXT(object)->world = NULL;
    return object;
}

void* WorldCopyClumpExt(void* dstObject, const void* srcObject, RwInt32 offset, RwInt32 size) {
    if (CLUMPEXT(srcObject)->world) {
        RpWorldAddClump(CLUMPEXT(srcObject)->world, (RpClump*)dstObject);
    }

    return dstObject;
}

void* WorldDeInitClumpExt(void* object, RwInt32 offset, RwInt32 size) {
    return object;
}

static inline void LightDestroyTies(RpLight* light) {
    RwLLLink* cur;
    RwLLLink* end;

    cur = rwLinkListGetFirstLLLink(&light->WorldSectorsInLight);
    end = rwLinkListGetTerminator(&light->WorldSectorsInLight);
    while (cur != end) {
        RpLightTie* tie = rwLLLinkGetData(cur, RpLightTie, WorldSectorInLight);

        cur = rwLLLinkGetNext(cur);
        _rpLightTieDestroy(tie);
    }
}

RwObjectHasFrame* WorldLightSync(RwObjectHasFrame* object) {
    RpWorld* world;
    RwFrame* frame;
    RpSector* sector;
    RwInt32 sectorStackPos;
    RpLight* light;
    RpLightExt* lightExt;

    light = (RpLight*)object;
    lightExt = LIGHTEXT(light);

    if (lightExt->oldSync(object)) {
        if ((RwInt32)light->object.object.subType < rpLIGHTPOSITIONINGSTART) {
            return object;
        }

        world = lightExt->world;
        frame = (RwFrame*)light->object.object.parent;

        if (world && frame) {
            RpSector* sectorStack[rpWORLDMAXBSPDEPTH];
            RwV3d inf;
            RwV3d sup;
            RwReal radius = light->radius;
            const RwMatrix* ltm;

            sectorStackPos = 0;
            LightDestroyTies(light);

            ltm = RwFrameGetLTM(frame);
            inf = ltm->pos;
            sup = ltm->pos;
            sup.x += radius;
            sup.y += radius;
            sup.z += radius;
            inf.x -= radius;
            inf.y -= radius;
            inf.z -= radius;

            sector = world->rootSector;

            do {
                if (sector->type < 0) {
                    RpLightTie* tie = (RpLightTie*)RwFreeListAlloc(RWWORLDOBJGLOBAL(lightTieFreeList));

                    tie->sect = (RpWorldSector*)sector;
                    tie->light = light;
                    rwLinkListAddLLLink(&((RpWorldSector*)sector)->lightsInWorldSector, &tie->lightInWorldSector);
                    rwLinkListAddLLLink(&light->WorldSectorsInLight, &tie->WorldSectorInLight);

                    sector = sectorStack[sectorStackPos--];
                } else {
                    RpPlaneSector* planeSector = (RpPlaneSector*)sector;

                    if (GETCOORD(inf, planeSector->type) < planeSector->leftValue) {
                        sector = planeSector->leftSubTree;

                        if (planeSector->rightValue < GETCOORD(sup, planeSector->type)) {
                            sectorStack[++sectorStackPos] = planeSector->rightSubTree;
                        }
                    } else if (planeSector->rightValue < GETCOORD(sup, planeSector->type)) {
                        sector = planeSector->rightSubTree;
                    } else {
                        sector = sectorStack[sectorStackPos--];
                    }
                }
            } while (sectorStackPos >= 0);
        }
    } else {
        RWERROR((E_RW_FRAMESYNC));
    }

    return object;
}

void* WorldInitLightExt(void* object, RwInt32 offset, RwInt32 size) {
    RpLight* light = (RpLight*)object;
    RpLightExt* lightExt = LIGHTEXT(light);

    lightExt->world = NULL;
    lightExt->oldSync = light->object.sync;
    light->object.sync = WorldLightSync;

    return object;
}

void* WorldCopyLightExt(void* dstObject, const void* srcObject, RwInt32 offset, RwInt32 size) {
    if (LIGHTEXT(srcObject)->world) {
        RpWorldAddLight(LIGHTEXT(srcObject)->world, (RpLight*)dstObject);
    }

    return dstObject;
}

void* WorldDeInitLightExt(void* object, RwInt32 offset, RwInt32 size) {
    RpLight* light = (RpLight*)object;
    RwLLLink* end;
    RwLLLink* cur;

    cur = rwLinkListGetFirstLLLink(&light->WorldSectorsInLight);
    end = rwLinkListGetTerminator(&light->WorldSectorsInLight);
    while (cur != end) {
        RpLightTie* tie = rwLLLinkGetData(cur, RpLightTie, WorldSectorInLight);

        cur = rwLLLinkGetNext(cur);
        _rpLightTieDestroy(tie);
    }

    return object;
}

static inline RwBool CameraExtGrowSectors(RpCameraExt* cameraExt) {
    RwUInt32 memSize = (cameraExt->space + rpWORLDSECTORGROWBY) * sizeof(RpWorldSector*);
    RpWorldSector** sectors;

    if (cameraExt->frustumSectors) {
        sectors = (RpWorldSector**)RwRealloc(cameraExt->frustumSectors, memSize);
    } else {
        sectors = (RpWorldSector**)RwMalloc(memSize);
    }

    if (sectors) {
        cameraExt->frustumSectors = sectors;
        cameraExt->space += rpWORLDSECTORGROWBY;
        return TRUE;
    }

    RWERROR((E_RW_NOMEM, memSize));
    return FALSE;
}

RpWorld* WorldSyncCamera(RpWorld* world, RwCamera* camera) {
    RpSector* sector;
    const RwFrustumPlane* frustumPlanes;
    RpCameraExt* cameraExt;
    RwBool backToFront;
    RwInt32 sectorStackPos;
    RwInt32 numSectors;
    RpSector* sectorStack[rpWORLDMAXBSPDEPTH];
    RwV3d cameraPos;
    RwV3d inf;
    RwV3d sup;
    const RwMatrix* ltm;

    cameraExt = CAMERAEXT(camera);
    sectorStackPos = 0;
    numSectors = 0;

    ltm = RwFrameGetLTM((RwFrame*)camera->object.object.parent);
    backToFront = (world->renderOrder == rpWORLDRENDERBACK2FRONT);
    frustumPlanes = camera->frustumPlanes;
    cameraPos = ltm->pos;
    inf = camera->frustumBoundBox.inf;
    sup = camera->frustumBoundBox.sup;
    sector = cameraExt->world->rootSector;

    do {
        if (sector->type < 0) {
            const RwFrustumPlane* plane = frustumPlanes;
            RwInt32 i;
            RwBool outside;
            const RwV3d* corners = (const RwV3d*)&((RpWorldSector*)sector)->boundingBox;

            for (i = 0; i < 6; i++) {
                RwSplitBits dot;

                RwV3d vec;

                vec.x = corners[plane->closestX].x;
                vec.y = corners[plane->closestY].y;
                vec.z = corners[plane->closestZ].z;
                dot.nReal = vec.x * plane->plane.normal.x + vec.y * plane->plane.normal.y +
                            vec.z * plane->plane.normal.z - plane->plane.distance;

                outside = (dot.nInt > 0);
                if (outside) {
                    break;
                }

                plane++;
            }

            if (!outside) {
                if (numSectors >= cameraExt->space) {
                    if (!CameraExtGrowSectors(cameraExt)) {
                        cameraExt->position = numSectors;
                        return world;
                    }
                }

                cameraExt->frustumSectors[numSectors++] = (RpWorldSector*)sector;
            }

            sector = sectorStack[sectorStackPos--];
        } else {
            RpPlaneSector* planeSector = (RpPlaneSector*)sector;
            RwSplitBits leftDist;
            RwSplitBits rightDist;

            leftDist.nReal = GETCOORD(inf, planeSector->type) - planeSector->leftValue;
            rightDist.nReal = planeSector->rightValue - GETCOORD(sup, planeSector->type);

            if (leftDist.nInt < 0 && rightDist.nInt < 0) {
                RwBool farSide = GETCOORD(cameraPos, planeSector->type) > planeSector->value;

                if ((backToFront && farSide) || (!backToFront && !farSide)) {
                    sectorStack[++sectorStackPos] = planeSector->rightSubTree;
                    sector = planeSector->leftSubTree;
                } else {
                    sectorStack[++sectorStackPos] = planeSector->leftSubTree;
                    sector = planeSector->rightSubTree;
                }
            } else {
                sector = (leftDist.nInt < 0) ? planeSector->leftSubTree : planeSector->rightSubTree;
            }
        }
    } while (sectorStackPos >= 0);

    cameraExt->position = numSectors;
    return world;
}

RpAtomic* WorldAddClumpAtomic(RpAtomic* atomic, void* data) {
    RpWorldAddAtomic((RpWorld*)data, atomic);
    return atomic;
}

RpLight* WorldAddClumpLight(RpLight* light, void* data) {
    RpWorldAddLight((RpWorld*)data, light);
    return light;
}

RwCamera* WorldAddClumpCamera(RwCamera* camera, void* data) {
    RpWorldAddCamera((RpWorld*)data, camera);
    return camera;
}

RwStream* writeGeometryMesh(RwStream* stream, RwInt32 binaryLength, const void* object, RwInt32 offsetInObject,
                            RwInt32 sizeInObject) {
    const RpGeometry* geometry = (const RpGeometry*)object;

    return _rpMeshWrite(geometry->mesh, geometry, stream, &geometry->matList);
}

RwStream* readGeometryMesh(RwStream* stream, RwInt32 binaryLength, void* object, RwInt32 offsetInObject,
                           RwInt32 sizeInObject) {
    RpGeometry* geometry = (RpGeometry*)object;

    geometry->mesh = _rpMeshRead(stream, geometry, &geometry->matList);
    if (!geometry->mesh) {
        return NULL;
    }

    return stream;
}

RwInt32 sizeGeometryMesh(const void* object, RwInt32 offsetInObject, RwInt32 sizeInObject) {
    const RpGeometry* geometry = (const RpGeometry*)object;

    return _rpMeshSize(geometry->mesh, geometry);
}

RwStream* writeGeometryNative(RwStream* stream, RwInt32 binaryLength, const void* object, RwInt32 offsetInObject,
                              RwInt32 sizeInObject) {
    return _rpGeometryNativeWrite(stream, (const RpGeometry*)object);
}

RwStream* readGeometryNative(RwStream* stream, RwInt32 binaryLength, void* object, RwInt32 offsetInObject,
                             RwInt32 sizeInObject) {
    if (!_rpGeometryNativeRead(stream, (RpGeometry*)object)) {
        return NULL;
    }

    return stream;
}

RwInt32 sizeGeometryNative(const void* object, RwInt32 offsetInObject, RwInt32 sizeInObject) {
    return _rpGeometryNativeSize((const RpGeometry*)object);
}

RwStream* writeWorldSectorNative(RwStream* stream, RwInt32 binaryLength, const void* object,
                                 RwInt32 offsetInObject, RwInt32 sizeInObject) {
    return _rpWorldSectorNativeWrite(stream, (const RpWorldSector*)object);
}

RwStream* readWorldSectorNative(RwStream* stream, RwInt32 binaryLength, void* object, RwInt32 offsetInObject,
                                RwInt32 sizeInObject) {
    if (!_rpWorldSectorNativeRead(stream, (RpWorldSector*)object)) {
        return NULL;
    }

    return stream;
}

RwInt32 sizeWorldSectorNative(const void* object, RwInt32 offsetInObject, RwInt32 sizeInObject) {
    return _rpWorldSectorNativeSize((const RpWorldSector*)object);
}

RwStream* writeSectorMesh(RwStream* stream, RwInt32 binaryLength, const void* object, RwInt32 offsetInObject,
                          RwInt32 sizeInObject) {
    const RpWorldSector* sector = (const RpWorldSector*)object;
    const RpWorld* world = RpWorldSectorGetWorld(sector);

    return _rpMeshWrite(sector->mesh, world, stream, &world->matList);
}

RwStream* readSectorMesh(RwStream* stream, RwInt32 binaryLength, void* object, RwInt32 offsetInObject,
                         RwInt32 sizeInObject) {
    RpWorldSector* sector = (RpWorldSector*)object;
    const RpWorld* world = RpWorldSectorGetWorld(sector);

    sector->mesh = _rpMeshRead(stream, world, &world->matList);
    if (!sector->mesh) {
        return NULL;
    }

    return stream;
}

RwInt32 sizeSectorMesh(const void* object, RwInt32 offsetInObject, RwInt32 sizeInObject) {
    const RpWorldSector* sector = (const RpWorldSector*)object;

    return _rpMeshSize(sector->mesh, RpWorldSectorGetWorld(sector));
}

RwBool _rpWorldObjRegisterExtensions(void) {
    RwInt32 success;

    success = RwEngineRegisterPlugin(sizeof(rpWorldObjGlobals), rwID_WORLDOBJPLUGIN, WorldObjectOpen,
                                     WorldObjectClose);

    cameraExtOffset = RwCameraRegisterPlugin(sizeof(RpCameraExt), rwID_WORLDOBJPLUGIN, WorldInitCameraExt,
                                             WorldDeInitCameraExt, WorldCopyCameraExt);
    success |= cameraExtOffset;

    atomicExtOffset = RpAtomicRegisterPlugin(sizeof(RpAtomicExt), rwID_WORLDOBJPLUGIN, WorldInitAtomicExt,
                                             WorldDeInitAtomicExt, WorldCopyAtomicExt);
    success |= atomicExtOffset;

    clumpExtOffset = RpClumpRegisterPlugin(sizeof(RpClumpExt), rwID_WORLDOBJPLUGIN, WorldInitClumpExt,
                                           WorldDeInitClumpExt, WorldCopyClumpExt);
    success |= clumpExtOffset;

    lightExtOffset = RpLightRegisterPlugin(sizeof(RpLightExt), rwID_WORLDOBJPLUGIN, WorldInitLightExt,
                                           WorldDeInitLightExt, WorldCopyLightExt);
    success |= lightExtOffset;

    success |= RpGeometryRegisterPlugin(0, rwID_BINMESHPLUGIN, NULL, NULL, NULL);
    success |= RpWorldSectorRegisterPlugin(0, rwID_BINMESHPLUGIN, NULL, NULL, NULL);
    success |= RpGeometryRegisterPluginStream(rwID_BINMESHPLUGIN, readGeometryMesh, writeGeometryMesh,
                                              sizeGeometryMesh);
    success |= RpWorldSectorRegisterPluginStream(rwID_BINMESHPLUGIN, readSectorMesh, writeSectorMesh,
                                                 sizeSectorMesh);

    success |= RpGeometryRegisterPlugin(0, rwID_NATIVEDATAPLUGIN, NULL, NULL, NULL);
    success |= RpWorldSectorRegisterPlugin(0, rwID_NATIVEDATAPLUGIN, NULL, NULL, NULL);
    success |= RpGeometryRegisterPluginStream(rwID_NATIVEDATAPLUGIN, readGeometryNative, writeGeometryNative,
                                              sizeGeometryNative);
    success |= RpWorldSectorRegisterPluginStream(rwID_NATIVEDATAPLUGIN, readWorldSectorNative,
                                                 writeWorldSectorNative, sizeWorldSectorNative);

    success |= RpAtomicRegisterPlugin(0, rwID_RIGHTTORENDER, NULL, NULL, NULL);
    success |= RpAtomicRegisterPluginStream(rwID_RIGHTTORENDER, _rpReadAtomicRights, _rpWriteAtomicRights,
                                            _rpSizeAtomicRights);
    success |= RpWorldRegisterPlugin(0, rwID_RIGHTTORENDER, NULL, NULL, NULL);
    success |= RpWorldRegisterPluginStream(rwID_RIGHTTORENDER, _rpReadWorldRights, _rpWriteWorldRights,
                                           _rpSizeWorldRights);
    success |= RpWorldSectorRegisterPlugin(0, rwID_RIGHTTORENDER, NULL, NULL, NULL);
    success |= RpWorldSectorRegisterPluginStream(rwID_RIGHTTORENDER, _rpReadSectRights, _rpWriteSectRights,
                                                 _rpSizeSectRights);

    if (success < 0) {
        return FALSE;
    }

    return fn_8015470C() ? TRUE : FALSE;
}

RpWorld* RpWorldAddCamera(RpWorld* world, RwCamera* camera) {
    RpCameraExt* cameraExt = CAMERAEXT(camera);

    if (camera->object.object.parent) {
        RwFrameUpdateObjects((RwFrame*)camera->object.object.parent);
    }

    cameraExt->world = world;

    return world;
}

RpWorld* RpWorldRemoveCamera(RpWorld* world, RwCamera* camera) {
    RpCameraExt* cameraExt = CAMERAEXT(camera);

    if (cameraExt->world) {
        if (cameraExt->frustumSectors) {
            RwFree(cameraExt->frustumSectors);
        }

        cameraExt->frustumSectors = NULL;
        cameraExt->space = 0;
        cameraExt->position = 0;
        cameraExt->world = NULL;

        return world;
    }

    return NULL;
}

RpWorld* RpWorldAddAtomic(RpWorld* world, RpAtomic* atomic) {
    RpAtomicExt* atomicExt = ATOMICEXT(atomic);
    RwFrame* frame = (RwFrame*)atomic->object.object.parent;

    if (frame) {
        RwFrameUpdateObjects(frame);
    }

    atomicExt->world = world;

    return world;
}

RpWorld* RpAtomicGetWorld(const RpAtomic* atomic) {
    return ATOMICEXT(atomic)->world;
}

RpAtomic* RpAtomicForAllWorldSectors(RpAtomic* atomic, RpWorldSectorCallBack callback, void* data) {
    RwLLLink* end;
    RwLLLink* next;
    RwLLLink* cur;

    cur = rwLinkListGetFirstLLLink(&atomic->llWorldSectorsInAtomic);
    end = rwLinkListGetTerminator(&atomic->llWorldSectorsInAtomic);
    while (cur != end) {
        RpTie* tie = rwLLLinkGetData(cur, RpTie, lWorldSectorInAtomic);

        next = rwLLLinkGetNext(cur);
        if (!callback(tie->worldSector, data)) {
            return atomic;
        }

        cur = next;
    }

    return atomic;
}

RpWorld* RpWorldAddClump(RpWorld* world, RpClump* clump) {
    RpClumpExt* clumpExt = CLUMPEXT(clump);

    if (!clumpExt->world) {
        RwFrame* frame = (RwFrame*)clump->object.parent;

        rwLinkListAddLLLink(&world->clumpList, &clump->inWorldLink);
        world->numClumpsInWorld++;
        clumpExt->world = world;

        RpClumpForAllAtomics(clump, WorldAddClumpAtomic, world);
        RpClumpForAllLights(clump, WorldAddClumpLight, world);
        RpClumpForAllCameras(clump, WorldAddClumpCamera, world);

        if (frame) {
            RwMatrixOptimize(&frame->modelling, NULL);
            RwFrameUpdateObjects(frame);
        }

        return world;
    }

    RWERROR((E_RP_WORLD_CLUMPINWORLD));
    return NULL;
}

RwCamera* RwCameraForAllSectorsInFrustum(RwCamera* camera, RpWorldSectorCallBack callBack, void* data) {
    RpCameraExt* cameraExt = CAMERAEXT(camera);
    RpWorldSector** frustumSectors = cameraExt->frustumSectors;
    RwInt32 numSectors = cameraExt->position;

    while (numSectors) {
        if (!callBack(*frustumSectors, data)) {
            return camera;
        }

        frustumSectors++;
        numSectors--;
    }

    return camera;
}

RpWorld* RpWorldAddLight(RpWorld* world, RpLight* light) {
    LIGHTEXT(light)->world = world;

    if (light->object.object.subType < rpLIGHTPOSITIONINGSTART) {
        rwLinkListAddLLLink(&world->directionalLightList, &light->inWorld);
    } else {
        if (light->object.object.parent) {
            RwFrameUpdateObjects((RwFrame*)light->object.object.parent);
        }

        rwLinkListAddLLLink(&world->lightList, &light->inWorld);
    }

    return world;
}

RpWorld* RpWorldRemoveLight(RpWorld* world, RpLight* light) {
    RwLLLink* end;
    RwLLLink* cur;

    LIGHTEXT(light)->world = NULL;

    cur = rwLinkListGetFirstLLLink(&light->WorldSectorsInLight);
    end = rwLinkListGetTerminator(&light->WorldSectorsInLight);
    while (cur != end) {
        RpLightTie* tie = rwLLLinkGetData(cur, RpLightTie, WorldSectorInLight);

        cur = rwLLLinkGetNext(cur);
        _rpLightTieDestroy(tie);
    }

    rwLLLinkRemove(&light->inWorld);

    return world;
}
