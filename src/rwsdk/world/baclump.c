/*
 * Clumps and atomics: creation, destruction, frame/geometry attachment,
 * bounding spheres and stream reading of whole clumps.
 */

#include "rwsdk/rpworld_int.h"

#define rwPLUGIN_ID 2 /* world */

#define rwID_CAMERA 0x05
#define rwID_FRAMELIST 0x0E
#define rwID_GEOMETRY 0x0F
#define rwID_LIGHT 0x12
#define rwID_ATOMIC 0x14
#define rwID_GEOMETRYLIST 0x1A
#define rwID_CLUMPPLUGIN 0x10

#define E_RW_BADVERSION 0x80000004
#define E_RW_READ 0x8000001A

#define rpATOMIC 1
#define rpCLUMP 2

#define rpATOMICPRIVATEWORLDBOUNDDIRTY 0x01
#define rpATOMICSAMEBOUNDINGSPHERE 0x01

#define rpINTERPOLATORDIRTYINSTANCE 0x01
#define rpINTERPOLATORDIRTYSPHERE 0x02

#define rwMATRIXTYPEMASK 0x03

#define RwRealMax2(a, b) (((a) >= (b)) ? (a) : (b))

typedef RpClump* (*RpClumpCallBack)(RpClump* clump, void* data);
typedef RpAtomic* (*RpAtomicCallBack)(RpAtomic* atomic, void* data);
typedef RwCamera* (*RwCameraCallBack)(RwCamera* camera, void* data);

typedef struct rwFrameList {
    RwFrame** frames;
    RwInt32 numFrames;
} rwFrameList;

typedef struct rpGeometryList {
    RpGeometry** geometries;
    RwInt32 numGeoms;
} rpGeometryList;

typedef struct rpClumpGlobals {
    RwFreeList* atomicFreeList;
    RwFreeList* clumpFreeList;
} rpClumpGlobals;

/* Per-light and per-camera clump membership. */
typedef struct rpClumpExt {
    RpClump* clump;
    RwLLLink inClumpLink;
} rpClumpExt;

typedef struct _rpClump {
    RwInt32 numAtomics;
    RwInt32 numLights;
    RwInt32 numCameras;
} _rpClump;

typedef struct _rpAtomic {
    RwInt32 frameIndex;
    RwInt32 geomIndex;
    RwInt32 flags;
    RwInt32 unused;
} _rpAtomic;

typedef struct _rpOldAtomic {
    RwInt32 frameIndex;
    RwInt32 flags;
    RwInt32 unused;
} _rpOldAtomic;

#define RWCLUMPGLOBAL(var) (RWPLUGINOFFSET(rpClumpGlobals, RwEngineInstance, clumpModule.globalsOffset)->var)
#define CLUMPCAMERAEXT(camera) (RWPLUGINOFFSET(rpClumpExt, camera, clumpCameraExtOffset))
#define CLUMPLIGHTEXT(light) (RWPLUGINOFFSET(rpClumpExt, light, clumpLightExtOffset))

#define RpAtomicSetRenderCallBackMacro(_atomic, _callback)           \
    do {                                                              \
        (_atomic)->renderCallBack = (_callback);                      \
        if (!(_atomic)->renderCallBack) {                             \
            (_atomic)->renderCallBack = AtomicDefaultRenderCallBack;  \
        }                                                             \
    } while (0)

RwPluginRegistry atomicTKList = { sizeof(RpAtomic), sizeof(RpAtomic), 0, 0, NULL, NULL };
RwPluginRegistry clumpTKList = { 0x30, 0x30, 0, 0, NULL, NULL };

/* .sbss is laid out in reverse declaration order. */
RwModuleInfo clumpModule;
static RwUInt32 atomicRightsPluginID;
static RwUInt32 atomicRightsExtraData;
static RwInt32 clumpLightExtOffset;
static RwInt32 clumpCameraExtOffset;

RwUInt32 RwStreamRead(RwStream* stream, void* buffer, RwUInt32 length);
void* RwMemNative32(void* mem, RwUInt32 size);
void* memset(void* dst, int val, unsigned long n);
const RwPluginRegistry* _rwPluginRegistryReadDataChunks(const RwPluginRegistry* reg, RwStream* stream,
                                                        void* object);
const RwPluginRegistry* _rwPluginRegistryInvokeRights(const RwPluginRegistry* reg, RwUInt32 id, void* obj,
                                                      RwUInt32 extraData);
RwInt32 _rwPluginRegistryAddPluginStream(RwPluginRegistry* reg, RwUInt32 pluginID,
                                         RwPluginDataChunkReadCallBack readCB,
                                         RwPluginDataChunkWriteCallBack writeCB,
                                         RwPluginDataChunkGetSizeCallBack getSizeCB);
RwInt32 _rwPluginRegistryAddPlgnStrmlwysCB(RwPluginRegistry* reg, RwUInt32 pluginID, void* alwaysCB);
RwInt32 _rwPluginRegistryAddPlgnStrmRightsCB(RwPluginRegistry* reg, RwUInt32 pluginID, void* rightsCB);
RwInt32 _rwPluginRegistryGetPluginOffset(const RwPluginRegistry* reg, RwUInt32 pluginID);
RxPipeline* RxPipelineExecute(RxPipeline* pipeline, void* data, RwBool heapReset);
RwBool RwResourcesFreeResEntry(RwResEntry* entry);
RwBool RwFrameDirty(const RwFrame* frame);
RwMatrix* RwFrameGetLTM(RwFrame* frame);
RwFrame* RwFrameUpdateObjects(RwFrame* frame);
RwBool RwFrameDestroyHierarchy(RwFrame* frame);
RwV3d* RwV3dTransformPoints(RwV3d* pointsOut, const RwV3d* pointsIn, RwInt32 numPoints, const RwMatrix* matrix);
RwReal _rwSqrt(const RwReal num);
void _rwObjectHasFrameSetFrame(void* object, RwFrame* frame);
void _rwObjectHasFrameReleaseFrame(void* object);
rwFrameList* _rwFrameListDeinitialize(rwFrameList* frameList);
rwFrameList* _rwFrameListStreamRead(RwStream* stream, rwFrameList* frameList);
RpGeometry* RpGeometryStreamRead(RwStream* stream);
RwBool RpGeometryDestroy(RpGeometry* geometry);
RpGeometry* _rpGeometryAddRef(RpGeometry* geometry);
RpWorld* RpAtomicGetWorld(const RpAtomic* atomic);
RwBool RpLightDestroy(RpLight* light);
RpLight* RpLightStreamRead(RwStream* stream);
RwBool RwCameraDestroy(RwCamera* camera);
RwCamera* RwCameraStreamRead(RwStream* stream);
RwInt32 RwCameraRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB,
                               RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);
RwInt32 RpLightRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB,
                              RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);
RpAtomic* RpAtomicSetFrame(RpAtomic* atomic, RwFrame* frame);
RpAtomic* RpAtomicCreate(void);
RpAtomic* RpAtomicSetGeometry(RpAtomic* atomic, RpGeometry* geometry, RwUInt32 flags);
RwBool RpAtomicDestroy(RpAtomic* atomic);
RwBool RpClumpDestroy(RpClump* clump);
void _rpAtomicResyncInterpolatedSphere(RpAtomic* atomic);

void ClumpTidyDestroyClump(void* clump, void* data) {
    RpClumpDestroy((RpClump*)clump);
}

void ClumpTidyDestroyAtomic(void* atomic, void* data) {
    RpAtomicDestroy((RpAtomic*)atomic);
}

RwStream* _rpReadAtomicRights(RwStream* stream, RwInt32 binaryLength, void* object, RwInt32 offsetInObject,
                              RwInt32 sizeInObject) {
    if (!RwStreamReadInt32(stream, (RwInt32*)&atomicRightsPluginID, sizeof(RwInt32))) {
        return NULL;
    }

    if (binaryLength == 8) {
        if (!RwStreamReadInt32(stream, (RwInt32*)&atomicRightsExtraData, sizeof(RwInt32))) {
            return NULL;
        }
    }

    return stream;
}

RwStream* _rpWriteAtomicRights(RwStream* stream, RwInt32 binaryLength, const void* object,
                               RwInt32 offsetInObject, RwInt32 sizeInObject) {
    const RpAtomic* atomic = (const RpAtomic*)object;

    if (!RwStreamWriteInt32(stream, (const RwInt32*)&atomic->pipeline->pluginId, sizeof(RwInt32))) {
        return NULL;
    }

    if (!RwStreamWriteInt32(stream, (const RwInt32*)&atomic->pipeline->pluginData, sizeof(RwInt32))) {
        return NULL;
    }

    return stream;
}

RwInt32 _rpSizeAtomicRights(const void* object, RwInt32 offsetInObject, RwInt32 sizeInObject) {
    const RpAtomic* atomic = (const RpAtomic*)object;

    if (atomic->pipeline && atomic->pipeline->pluginId) {
        return sizeof(RwUInt32) * 2;
    }

    return 0;
}

RwObjectHasFrame* AtomicSync(RwObjectHasFrame* object) {
    RpAtomic* atomic = (RpAtomic*)object;

    if (atomic->interpolator.flags & rpINTERPOLATORDIRTYSPHERE) {
        _rpAtomicResyncInterpolatedSphere(atomic);
    }

    atomic->object.object.privateFlags |= rpATOMICPRIVATEWORLDBOUNDDIRTY;

    return object;
}

RpAtomic* AtomicDefaultRenderCallBack(RpAtomic* atomic) {
    RxPipeline* pipeline = atomic->pipeline;

    if (!pipeline) {
        pipeline = RXPIPELINEGLOBAL(currentAtomicPipeline);
    }

    if (RxPipelineExecute(pipeline, atomic, TRUE)) {
        return atomic;
    }

    return NULL;
}

rpGeometryList* GeometryListStreamRead(RwStream* stream, rpGeometryList* geomList) {
    RwInt32 numGeoms;
    RwUInt32 size;
    RwUInt32 version;
    RwInt32 i;

    if (!RwStreamFindChunk(stream, rwID_STRUCT, &size, &version)) {
        return NULL;
    }

    if (version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION) {
        if (RwStreamRead(stream, &numGeoms, sizeof(numGeoms)) != sizeof(numGeoms)) {
            return NULL;
        }

        RwMemNative32(&numGeoms, sizeof(numGeoms));

        geomList->numGeoms = numGeoms;
        if (geomList->numGeoms > 0) {
            geomList->geometries = (RpGeometry**)RwMalloc(numGeoms * sizeof(RpGeometry*));
            if (!geomList->geometries) {
                RWERROR((E_RW_NOMEM, numGeoms * sizeof(RpGeometry*)));
                return NULL;
            }
        } else {
            geomList->geometries = NULL;
        }

        for (i = 0; i < numGeoms; i++) {
            if (!RwStreamFindChunk(stream, rwID_GEOMETRY, NULL, &version)) {
                return NULL;
            }

            if (version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION) {
                if (!(geomList->geometries[i] = RpGeometryStreamRead(stream))) {
                    return NULL;
                }
            } else {
                return NULL;
            }
        }

    } else {
        RWERROR((E_RW_BADVERSION));
        return NULL;
    }

    return geomList;
}

void* ClumpInitCameraExt(void* object, RwInt32 offset, RwInt32 size) {
    rpClumpExt* ext = CLUMPCAMERAEXT(object);

    ext->inClumpLink.prev = NULL;
    ext->inClumpLink.next = NULL;
    ext->clump = NULL;

    return object;
}

void* ClumpDeInitCameraExt(void* object, RwInt32 offset, RwInt32 size) {
    return object;
}

void* ClumpInitLightExt(void* object, RwInt32 offset, RwInt32 size) {
    rpClumpExt* ext = CLUMPLIGHTEXT(object);

    ext->inClumpLink.prev = NULL;
    ext->inClumpLink.next = NULL;
    ext->clump = NULL;

    return object;
}

void* ClumpDeInitLightExt(void* object, RwInt32 offset, RwInt32 size) {
    return object;
}

RpClump* ClumpCallBack(RpClump* clump, void* data) {
    return clump;
}

RpAtomic* ClumpAtomicStreamRead(RwStream* stream, rwFrameList* frameList, rpGeometryList* geomList) {
    _rpOldAtomic oldAtom;
    _rpAtomic a;
    RpAtomic* atomic;
    RpGeometry* geometry;
    RwUInt32 size;
    RwUInt32 version;

    if (!RwStreamFindChunk(stream, rwID_STRUCT, &size, &version)) {
        RWERROR((E_RW_READ));
        return NULL;
    }

    if (version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION) {
        if (version >= 0x30400) {
            RwBool status;

            memset(&a, 0, sizeof(a));
            status = (RwStreamRead(stream, &a, size) == size);
            if (!status) {
                RWERROR((E_RW_READ));
                return NULL;
            }

            RwMemNative32(&a, sizeof(a));

            atomic = RpAtomicCreate();
            if (!atomic) {
                return NULL;
            }

            atomic->object.object.flags = (RwUInt8)a.flags;

            if (frameList->numFrames) {
                RpAtomicSetFrame(atomic, frameList->frames[a.frameIndex]);
            }

            if (geomList->numGeoms) {
                RpAtomicSetGeometry(atomic, geomList->geometries[a.geomIndex], 0);
            } else {
                if (!RwStreamFindChunk(stream, rwID_GEOMETRY, NULL, &version)) {
                    RpAtomicDestroy(atomic);
                    RWERROR((E_RW_READ));
                    return NULL;
                }

                if (version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION) {
                    geometry = RpGeometryStreamRead(stream);
                    if (!geometry) {
                        RpAtomicDestroy(atomic);
                        RWERROR((E_RW_READ));
                        return NULL;
                    }
                } else {
                    RpAtomicDestroy(atomic);
                    RWERROR((E_RW_BADVERSION));
                    return NULL;
                }

                RpAtomicSetGeometry(atomic, geometry, 0);
                RpGeometryDestroy(geometry);
            }
        } else {
            RwBool status;

            memset(&oldAtom, 0, sizeof(oldAtom));
            status = (RwStreamRead(stream, &oldAtom, size) == size);
            if (!status) {
                RWERROR((E_RW_READ));
                return NULL;
            }

            RwMemNative32(&oldAtom, sizeof(oldAtom));

            atomic = RpAtomicCreate();
            if (!atomic) {
                return NULL;
            }

            atomic->object.object.flags = (RwUInt8)oldAtom.flags;

            if (frameList->numFrames) {
                RpAtomicSetFrame(atomic, frameList->frames[oldAtom.frameIndex]);
            }

            if (!RwStreamFindChunk(stream, rwID_GEOMETRY, NULL, &version)) {
                RpAtomicDestroy(atomic);
                RWERROR((E_RW_READ));
                return NULL;
            }

            if (version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION) {
                geometry = RpGeometryStreamRead(stream);
                if (!geometry) {
                    RpAtomicDestroy(atomic);
                    RWERROR((E_RW_READ));
                    return NULL;
                }
            } else {
                RpAtomicDestroy(atomic);
                RWERROR((E_RW_BADVERSION));
                return NULL;
            }

            RpAtomicSetGeometry(atomic, geometry, 0);
            RpGeometryDestroy(geometry);
        }

        atomicRightsPluginID = 0;
        atomicRightsExtraData = 0;

        {
            RwBool status = (_rwPluginRegistryReadDataChunks(&atomicTKList, stream, atomic) != NULL);

            if (!status) {
                RWERROR((E_RW_READ));
                return NULL;
            }
        }

        if (atomicRightsPluginID) {
            _rwPluginRegistryInvokeRights(&atomicTKList, atomicRightsPluginID, atomic, atomicRightsExtraData);
        }

        return atomic;
    }

    RWERROR((E_RW_BADVERSION));
    return NULL;
}

void _rpAtomicResyncInterpolatedSphere(RpAtomic* atomic) {
    RpGeometry* geometry = atomic->geometry;
    RpInterpolator* interp;

    if (!geometry) {
        return;
    }

    interp = &atomic->interpolator;

    if (interp->startMorphTarget == interp->endMorphTarget || interp->startMorphTarget >= geometry->numMorphTargets ||
        interp->endMorphTarget >= geometry->numMorphTargets) {
        if (interp->startMorphTarget >= geometry->numMorphTargets || interp->endMorphTarget >= geometry->numMorphTargets) {
            atomic->boundingSphere = geometry->morphTarget[0].boundingSphere;
        } else {
            atomic->boundingSphere = geometry->morphTarget[interp->startMorphTarget].boundingSphere;
        }
    } else {
        RwReal scale = interp->recipTime * interp->position;
        RpMorphTarget* morphTarget1 = &geometry->morphTarget[interp->startMorphTarget];
        RpMorphTarget* morphTarget2 = &geometry->morphTarget[interp->endMorphTarget];

        atomic->boundingSphere.radius =
            morphTarget1->boundingSphere.radius +
            scale * (morphTarget2->boundingSphere.radius - morphTarget1->boundingSphere.radius);

        atomic->boundingSphere.center.x = morphTarget2->boundingSphere.center.x - morphTarget1->boundingSphere.center.x;
        atomic->boundingSphere.center.y = morphTarget2->boundingSphere.center.y - morphTarget1->boundingSphere.center.y;
        atomic->boundingSphere.center.z = morphTarget2->boundingSphere.center.z - morphTarget1->boundingSphere.center.z;
        atomic->boundingSphere.center.x = atomic->boundingSphere.center.x * scale;
        atomic->boundingSphere.center.y = atomic->boundingSphere.center.y * scale;
        atomic->boundingSphere.center.z = atomic->boundingSphere.center.z * scale;
        atomic->boundingSphere.center.x = atomic->boundingSphere.center.x + morphTarget1->boundingSphere.center.x;
        atomic->boundingSphere.center.y = atomic->boundingSphere.center.y + morphTarget1->boundingSphere.center.y;
        atomic->boundingSphere.center.z = atomic->boundingSphere.center.z + morphTarget1->boundingSphere.center.z;
    }

    interp->flags &= ~rpINTERPOLATORDIRTYSPHERE;
    atomic->object.object.privateFlags |= rpATOMICPRIVATEWORLDBOUNDDIRTY;
}

const RwSphere* RpAtomicGetWorldBoundingSphere(RpAtomic* atomic) {
    RwFrame* frame = (RwFrame*)atomic->object.object.parent;

    if (atomic->interpolator.flags & rpINTERPOLATORDIRTYSPHERE) {
        _rpAtomicResyncInterpolatedSphere(atomic);
    }

    if (RwFrameDirty(frame) || (atomic->object.object.privateFlags & rpATOMICPRIVATEWORLDBOUNDDIRTY)) {
        const RwMatrix* ltm = RwFrameGetLTM(frame);

        RwV3dTransformPoints(&atomic->worldBoundingSphere.center, &atomic->boundingSphere.center, 1, ltm);

        if ((ltm->flags & rwMATRIXTYPEMASK) != rwMATRIXTYPEORTHONORMAL) {
            RwReal rightLen2 = ltm->right.x * ltm->right.x + ltm->right.y * ltm->right.y + ltm->right.z * ltm->right.z;
            RwReal upLen2 = ltm->up.x * ltm->up.x + ltm->up.y * ltm->up.y + ltm->up.z * ltm->up.z;
            RwReal atLen2 = ltm->at.x * ltm->at.x + ltm->at.y * ltm->at.y + ltm->at.z * ltm->at.z;

            atomic->worldBoundingSphere.radius =
                atomic->boundingSphere.radius * _rwSqrt(RwRealMax2(rightLen2, RwRealMax2(upLen2, atLen2)));
        } else {
            atomic->worldBoundingSphere.radius = atomic->boundingSphere.radius;
        }

        atomic->object.object.privateFlags &= ~rpATOMICPRIVATEWORLDBOUNDDIRTY;
    }

    return &atomic->worldBoundingSphere;
}

void* _rpClumpClose(void* instance, RwInt32 offset, RwInt32 size) {
    RwFreeListForAllUsed(RWCLUMPGLOBAL(clumpFreeList), ClumpTidyDestroyClump, NULL);
    RwFreeListForAllUsed(RWCLUMPGLOBAL(atomicFreeList), ClumpTidyDestroyAtomic, NULL);

    RwFreeListDestroy(RWCLUMPGLOBAL(atomicFreeList));
    RwFreeListDestroy(RWCLUMPGLOBAL(clumpFreeList));
    RWCLUMPGLOBAL(atomicFreeList) = NULL;
    RWCLUMPGLOBAL(clumpFreeList) = NULL;

    clumpModule.numInstances--;

    return instance;
}

void* _rpClumpOpen(void* instance, RwInt32 offset, RwInt32 size) {
    clumpModule.globalsOffset = offset;

    RWCLUMPGLOBAL(atomicFreeList) = RwFreeListCreate(atomicTKList.sizeOfStruct, 20, 0);
    if (RWCLUMPGLOBAL(atomicFreeList)) {
        RWCLUMPGLOBAL(clumpFreeList) = RwFreeListCreate(clumpTKList.sizeOfStruct, 10, 0);
        if (RWCLUMPGLOBAL(clumpFreeList)) {
            clumpModule.numInstances++;
            return instance;
        }

        RwFreeListDestroy(RWCLUMPGLOBAL(atomicFreeList));
        RWCLUMPGLOBAL(atomicFreeList) = NULL;
    }

    return NULL;
}

RwBool _rpClumpRegisterExtensions(void) {
    clumpCameraExtOffset = RwCameraRegisterPlugin(sizeof(rpClumpExt), rwID_CLUMPPLUGIN, ClumpInitCameraExt,
                                                  ClumpDeInitCameraExt, NULL);
    if (clumpCameraExtOffset < 0) {
        return FALSE;
    }

    clumpLightExtOffset = RpLightRegisterPlugin(sizeof(rpClumpExt), rwID_CLUMPPLUGIN, ClumpInitLightExt,
                                                ClumpDeInitLightExt, NULL);
    if (clumpLightExtOffset < 0) {
        return FALSE;
    }

    return TRUE;
}

RpClump* RpClumpForAllAtomics(RpClump* clump, RpAtomicCallBack callback, void* data) {
    RwLLLink* cur = rwLinkListGetFirstLLLink(&clump->atomicList);
    RwLLLink* end = rwLinkListGetTerminator(&clump->atomicList);

    while (cur != end) {
        RpAtomic* atomic = rwLLLinkGetData(cur, RpAtomic, inClumpLink);
        RwLLLink* next = rwLLLinkGetNext(cur);

        if (!callback(atomic, data)) {
            return clump;
        }

        cur = next;
    }

    return clump;
}

RpClump* RpClumpForAllCameras(RpClump* clump, RwCameraCallBack callback, void* data) {
    RwLLLink* cur = rwLinkListGetFirstLLLink(&clump->cameraList);
    RwLLLink* end = rwLinkListGetTerminator(&clump->cameraList);

    while (cur != end) {
        RwCamera* camera = (RwCamera*)((RwUInt8*)cur - offsetof(rpClumpExt, inClumpLink) - clumpCameraExtOffset);
        RwLLLink* next = rwLLLinkGetNext(cur);

        if (!callback(camera, data)) {
            return clump;
        }

        cur = next;
    }

    return clump;
}

RpClump* RpClumpForAllLights(RpClump* clump, RpLightCallBack callback, void* data) {
    RwLLLink* cur = rwLinkListGetFirstLLLink(&clump->lightList);
    RwLLLink* end = rwLinkListGetTerminator(&clump->lightList);

    while (cur != end) {
        RpLight* light = (RpLight*)((RwUInt8*)cur - offsetof(rpClumpExt, inClumpLink) - clumpLightExtOffset);
        RwLLLink* next = rwLLLinkGetNext(cur);

        if (!callback(light, data)) {
            return clump;
        }

        cur = next;
    }

    return clump;
}

RpAtomic* RpAtomicCreate(void) {
    RpAtomic* atomic = (RpAtomic*)RwFreeListAlloc(RWCLUMPGLOBAL(atomicFreeList));

    if (!atomic) {
        return NULL;
    }

    rwObjectInitialize(atomic, rpATOMIC, 0);
    atomic->object.sync = AtomicSync;
    atomic->repEntry = NULL;
    atomic->object.object.flags = rpATOMICCOLLISIONTEST | rpATOMICRENDER;
    atomic->object.object.privateFlags = rpATOMICPRIVATEWORLDBOUNDDIRTY;

    RpAtomicSetFrame(atomic, NULL);

    atomic->geometry = NULL;

    atomic->boundingSphere.radius = 0.0f;
    atomic->boundingSphere.center.x = 0.0f;
    atomic->boundingSphere.center.y = 0.0f;
    atomic->boundingSphere.center.z = 0.0f;
    atomic->worldBoundingSphere.radius = 0.0f;
    atomic->worldBoundingSphere.center.x = 0.0f;
    atomic->worldBoundingSphere.center.y = 0.0f;
    atomic->worldBoundingSphere.center.z = 0.0f;

    RpAtomicSetRenderCallBackMacro(atomic, AtomicDefaultRenderCallBack);

    atomic->interpolator.startMorphTarget = 0;
    atomic->interpolator.endMorphTarget = 0;
    atomic->interpolator.time = 1.0f;
    atomic->interpolator.recipTime = 1.0f;
    atomic->interpolator.position = 0.0f;
    atomic->interpolator.flags = rpINTERPOLATORDIRTYINSTANCE | rpINTERPOLATORDIRTYSPHERE;

    atomic->inClumpLink.prev = NULL;
    atomic->inClumpLink.next = NULL;
    atomic->clump = NULL;
    atomic->pipeline = NULL;

    rwLinkListInitialize(&atomic->llWorldSectorsInAtomic);

    _rwPluginRegistryInitObject(&atomicTKList, atomic);

    return atomic;
}

RpAtomic* RpAtomicSetGeometry(RpAtomic* atomic, RpGeometry* geometry, RwUInt32 flags) {
    if (geometry != atomic->geometry) {
        if (geometry) {
            _rpGeometryAddRef(geometry);
        }

        if (atomic->geometry) {
            RpGeometryDestroy(atomic->geometry);
        }

        atomic->geometry = geometry;

        if (!(flags & rpATOMICSAMEBOUNDINGSPHERE)) {
            RwFrame* frame;

            if (geometry) {
                atomic->boundingSphere = geometry->morphTarget[0].boundingSphere;
            }

            frame = (RwFrame*)atomic->object.object.parent;
            if (frame && RpAtomicGetWorld(atomic)) {
                RwFrameUpdateObjects(frame);
            }
        }
    }

    return atomic;
}

RwBool RpAtomicDestroy(RpAtomic* atomic) {
    _rwPluginRegistryDeInitObject(&atomicTKList, atomic);

    if (atomic->repEntry) {
        RwResourcesFreeResEntry(atomic->repEntry);
    }

    if (atomic->geometry) {
        RwFrame* frame;

        if (atomic->geometry) {
            RpGeometryDestroy(atomic->geometry);
        }

        atomic->geometry = NULL;

        frame = (RwFrame*)atomic->object.object.parent;
        if (frame && RpAtomicGetWorld(atomic)) {
            RwFrameUpdateObjects(frame);
        }
    }

    _rwObjectHasFrameReleaseFrame(atomic);

    RwFreeListFree(RWCLUMPGLOBAL(atomicFreeList), atomic);

    return TRUE;
}

RpClump* RpClumpRemoveLight(RpClump* clump, RpLight* light);
RpClump* RpClumpRemoveCamera(RpClump* clump, RwCamera* camera);

static inline RpAtomic* ClumpDestroyAtomic(RpAtomic* atomic, void* data) {
    RpAtomicDestroy(atomic);
    return atomic;
}

static inline RpLight* ClumpDestroyLight(RpLight* light, void* data) {
    RpClumpRemoveLight(CLUMPLIGHTEXT(light)->clump, light);
    RpLightDestroy(light);
    return light;
}

static inline RwCamera* ClumpDestroyCamera(RwCamera* camera, void* data) {
    RpClumpRemoveCamera(CLUMPCAMERAEXT(camera)->clump, camera);
    RwCameraDestroy(camera);
    return camera;
}

RwBool RpClumpDestroy(RpClump* clump) {
    RwFrame* frame;

    _rwPluginRegistryDeInitObject(&clumpTKList, clump);

    RpClumpForAllAtomics(clump, ClumpDestroyAtomic, NULL);
    RpClumpForAllLights(clump, ClumpDestroyLight, NULL);
    RpClumpForAllCameras(clump, ClumpDestroyCamera, NULL);

    frame = (RwFrame*)clump->object.parent;
    if (frame) {
        RwFrameDestroyHierarchy(frame);
    }

    RwFreeListFree(RWCLUMPGLOBAL(clumpFreeList), clump);

    return TRUE;
}

RpClump* RpClumpAddAtomic(RpClump* clump, RpAtomic* atomic) {
    rwLinkListAddLLLink(&clump->atomicList, &atomic->inClumpLink);
    atomic->clump = clump;
    return clump;
}

RpClump* RpClumpRemoveAtomic(RpClump* clump, RpAtomic* atomic) {
    rwLLLinkRemove(&atomic->inClumpLink);
    atomic->clump = NULL;
    return clump;
}

RpClump* RpClumpRemoveLight(RpClump* clump, RpLight* light) {
    rpClumpExt* ext = CLUMPLIGHTEXT(light);

    rwLLLinkRemove(&ext->inClumpLink);
    ext->inClumpLink.prev = NULL;
    ext->inClumpLink.next = NULL;
    ext->clump = NULL;

    return clump;
}

RpClump* RpClumpRemoveCamera(RpClump* clump, RwCamera* camera) {
    rpClumpExt* ext = CLUMPCAMERAEXT(camera);

    rwLLLinkRemove(&ext->inClumpLink);
    ext->inClumpLink.prev = NULL;
    ext->inClumpLink.next = NULL;
    ext->clump = NULL;

    return clump;
}

static inline RpClump* RpClumpCreate(void) {
    RpClump* clump = (RpClump*)RwFreeListAlloc(RWCLUMPGLOBAL(clumpFreeList));

    if (!clump) {
        return clump;
    }

    rwObjectInitialize(clump, rpCLUMP, 0);
    clump->object.parent = NULL;
    rwLinkListInitialize(&clump->atomicList);
    rwLinkListInitialize(&clump->lightList);
    rwLinkListInitialize(&clump->cameraList);
    clump->inWorldLink.prev = NULL;
    clump->inWorldLink.next = NULL;
    clump->callback = ClumpCallBack;

    _rwPluginRegistryInitObject(&clumpTKList, clump);

    return clump;
}

static inline void GeometryListDeinitialize(rpGeometryList* geomList) {
    if (geomList->numGeoms) {
        RwInt32 i;

        for (i = 0; i < geomList->numGeoms; i++) {
            RpGeometryDestroy(geomList->geometries[i]);
        }

        RwFree(geomList->geometries);
    }
}

static inline RpClump* RpClumpAddLight(RpClump* clump, RpLight* light) {
    rpClumpExt* ext = CLUMPLIGHTEXT(light);

    rwLinkListAddLLLink(&clump->lightList, &ext->inClumpLink);
    ext->clump = clump;
    return clump;
}

static inline RpClump* RpClumpAddCamera(RpClump* clump, RwCamera* camera) {
    rpClumpExt* ext = CLUMPCAMERAEXT(camera);

    rwLinkListAddLLLink(&clump->cameraList, &ext->inClumpLink);
    ext->clump = clump;
    return clump;
}

RpClump* RpClumpStreamRead(RwStream* stream) {
    _rpClump cl;
    RwCamera* camera;
    RwInt32 j;
    RpAtomic* atomic;
    RwInt32 k;
    RwInt32 i;
    RpClump* clump;
    RpLight* light;
    RwUInt32 size;
    RwUInt32 version;
    RwUInt32 chunkVersion;
    RwInt32 numAtomics;
    RwInt32 lightFrameIndex;
    RwInt32 cameraFrameIndex;

    if (!RwStreamFindChunk(stream, rwID_STRUCT, &size, &version)) {
        RWERROR((E_RW_READ));
        return NULL;
    }

    if (version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION) {
        rwFrameList frameList;
        rpGeometryList geomList;

        if (version <= 0x33000) {
            RwBool status = (RwStreamRead(stream, &numAtomics, sizeof(numAtomics)) == sizeof(numAtomics));

            if (!status) {
                RWERROR((E_RW_READ));
                return NULL;
            }

            RwMemNative32(&numAtomics, sizeof(numAtomics));
            cl.numAtomics = numAtomics;
            cl.numLights = 0;
            cl.numCameras = 0;
        } else {
            RwBool status = (RwStreamRead(stream, &cl, sizeof(cl)) == sizeof(cl));

            if (!status) {
                RWERROR((E_RW_READ));
                return NULL;
            }

            RwMemNative32(&cl, sizeof(cl));
        }

        clump = RpClumpCreate();
        if (!clump) {
            return NULL;
        }

        if (!RwStreamFindChunk(stream, rwID_FRAMELIST, NULL, &chunkVersion)) {
            RpClumpDestroy(clump);
            RWERROR((E_RW_READ));
            return NULL;
        }

        {
            RwBool status = (_rwFrameListStreamRead(stream, &frameList) != NULL);

            if (!status) {
                RpClumpDestroy(clump);
                RWERROR((E_RW_READ));
                return NULL;
            }
        }

        if (!RwStreamFindChunk(stream, rwID_GEOMETRYLIST, NULL, &chunkVersion)) {
            RWERROR((E_RW_READ));
            return NULL;
        }

        {
            RwBool status = (GeometryListStreamRead(stream, &geomList) != NULL);

            if (!status) {
                _rwFrameListDeinitialize(&frameList);
                RpClumpDestroy(clump);
                RWERROR((E_RW_READ));
                return NULL;
            }
        }

        clump->object.parent = frameList.frames[0];

        for (i = 0; i < cl.numAtomics; i++) {
            RwBool status = RwStreamFindChunk(stream, rwID_ATOMIC, NULL, &version);

            if (status) {
                atomic = ClumpAtomicStreamRead(stream, &frameList, &geomList);
                status = (atomic != NULL);
            }

            if (!status) {
                GeometryListDeinitialize(&geomList);
                _rwFrameListDeinitialize(&frameList);
                RpClumpDestroy(clump);
                RWERROR((E_RW_READ));
                return NULL;
            }

            RpClumpAddAtomic(clump, atomic);
        }

        for (j = 0; j < cl.numLights; j++) {
            RwBool status = FALSE;

            if (RwStreamFindChunk(stream, rwID_STRUCT, NULL, NULL)) {
                if (RwStreamReadInt32(stream, &lightFrameIndex, sizeof(RwInt32))) {
                    status = TRUE;
                }
            }

            if (status) {
                status = RwStreamFindChunk(stream, rwID_LIGHT, NULL, NULL);
            }

            if (status) {
                light = RpLightStreamRead(stream);
                status = (light != NULL);
            }

            if (!status) {
                GeometryListDeinitialize(&geomList);
                _rwFrameListDeinitialize(&frameList);
                RpClumpDestroy(clump);
                RWERROR((E_RW_READ));
                return NULL;
            }

            _rwObjectHasFrameSetFrame(light, frameList.frames[lightFrameIndex]);
            RpClumpAddLight(clump, light);
        }

        for (k = 0; k < cl.numCameras; k++) {
            RwBool status = FALSE;

            if (RwStreamFindChunk(stream, rwID_STRUCT, NULL, NULL)) {
                if (RwStreamReadInt32(stream, &cameraFrameIndex, sizeof(RwInt32))) {
                    status = TRUE;
                }
            }

            if (status) {
                status = RwStreamFindChunk(stream, rwID_CAMERA, NULL, NULL);
            }

            if (status) {
                camera = RwCameraStreamRead(stream);
                status = (camera != NULL);
            }

            if (!status) {
                GeometryListDeinitialize(&geomList);
                _rwFrameListDeinitialize(&frameList);
                RpClumpDestroy(clump);
                RWERROR((E_RW_READ));
                return NULL;
            }

            _rwObjectHasFrameSetFrame(camera, frameList.frames[cameraFrameIndex]);
            RpClumpAddCamera(clump, camera);
        }

        GeometryListDeinitialize(&geomList);
        _rwFrameListDeinitialize(&frameList);

        {
            RwBool status = (_rwPluginRegistryReadDataChunks(&clumpTKList, stream, clump) != NULL);

            if (!status) {
                RpClumpDestroy(clump);
                RWERROR((E_RW_READ));
                return NULL;
            }
        }

        return clump;
    }

    RWERROR((E_RW_BADVERSION));
    return NULL;
}

RwInt32 RpAtomicRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB,
                               RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB) {
    return _rwPluginRegistryAddPlugin(&atomicTKList, size, pluginID, constructCB, destructCB, copyCB);
}

RwInt32 RpClumpRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB,
                              RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB) {
    return _rwPluginRegistryAddPlugin(&clumpTKList, size, pluginID, constructCB, destructCB, copyCB);
}

RwInt32 RpAtomicRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB,
                                     RwPluginDataChunkWriteCallBack writeCB,
                                     RwPluginDataChunkGetSizeCallBack getSizeCB) {
    return _rwPluginRegistryAddPluginStream(&atomicTKList, pluginID, readCB, writeCB, getSizeCB);
}

RwInt32 RpAtomicSetStreamAlwaysCallBack(RwUInt32 pluginID, void* alwaysCB) {
    return _rwPluginRegistryAddPlgnStrmlwysCB(&atomicTKList, pluginID, alwaysCB);
}

RwInt32 RpAtomicSetStreamRightsCallBack(RwUInt32 pluginID, void* rightsCB) {
    return _rwPluginRegistryAddPlgnStrmRightsCB(&atomicTKList, pluginID, rightsCB);
}

RwInt32 RpAtomicGetPluginOffset(RwUInt32 pluginID) {
    return _rwPluginRegistryGetPluginOffset(&atomicTKList, pluginID);
}

RpAtomic* RpAtomicSetFrame(RpAtomic* atomic, RwFrame* frame) {
    _rwObjectHasFrameSetFrame(atomic, frame);
    atomic->object.object.privateFlags |= rpATOMICPRIVATEWORLDBOUNDDIRTY;
    return atomic;
}
