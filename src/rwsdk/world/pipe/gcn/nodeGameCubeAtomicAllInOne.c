/*
 * GameCubeAtomicAllInOne.csl: the whole atomic pipeline in one node. The
 * geometry is instanced into display lists and GX vertex arrays held in a
 * resource entry, reinstanced in place when locked or morphing, lit and then
 * drawn, each step through a replaceable callback.
 */

#include "rwsdk/gcallinone_int.h"

#define rpGEOMETRYNATIVE 0x01000000
#define rpGEOMETRYNATIVEINSTANCE 0x02000000

#define rpGEOMETRYLOCKVERTICES 0x0002
#define rpGEOMETRYLOCKNORMALS 0x0004
#define rpGEOMETRYLOCKPRELIGHT 0x0008
#define rpGEOMETRYLOCKTEXCOORDS1 0x0010
#define rpGEOMETRYLOCKTEXCOORDSALL 0x0FF0

#define rpINTERPOLATORDIRTYINSTANCE 0x01

#define GX_INDEX8 2
#define GX_INDEX16 3

struct RwResEntry {
    /* 0x00 */ RwLLLink link;
    /* 0x08 */ RwInt32 size;
    /* 0x0C */ void* owner;
    /* 0x10 */ RwResEntry** ownerRef;
    /* 0x14 */ void (*destroyNotify)(RwResEntry* resEntry);
};

typedef struct rwResourcesGlobals {
    /* 0x00 */ RwUInt32 maxSize;
    /* 0x04 */ RwUInt32 currentSize;
    /* 0x08 */ RwUInt32 reusageSize;
    /* 0x0C */ void* memHeap;
    /* 0x10 */ RwLinkList entriesA;
    /* 0x18 */ RwLinkList entriesB;
    /* 0x20 */ RwLinkList* freeEntries;
    /* 0x24 */ RwLinkList* usedEntries;
} rwResourcesGlobals;

extern RwModuleInfo resourcesModule;

#define RWRESOURCESGLOBAL(var) \
    (RWPLUGINOFFSET(rwResourcesGlobals, RwEngineInstance, resourcesModule.globalsOffset)->var)

#define RwResourcesUseResEntry(_ntry)                                          \
    do {                                                                       \
        if ((_ntry)->link.next) {                                              \
            rwLLLinkRemove(&(_ntry)->link);                                    \
            rwLinkListAddLLLink(RWRESOURCESGLOBAL(usedEntries), &(_ntry)->link); \
        }                                                                      \
    } while (0)

#define RwV3dSub(o, a, b)          \
    do {                           \
        (o)->x = (a)->x - (b)->x;  \
        (o)->y = (a)->y - (b)->y;  \
        (o)->z = (a)->z - (b)->z;  \
    } while (0)

#define RwV3dAdd(o, a, b)          \
    do {                           \
        (o)->x = (a)->x + (b)->x;  \
        (o)->y = (a)->y + (b)->y;  \
        (o)->z = (a)->z + (b)->z;  \
    } while (0)

#define RwV3dScale(o, a, s)        \
    do {                           \
        (o)->x = (a)->x * (s);     \
        (o)->y = (a)->y * (s);     \
        (o)->z = (a)->z * (s);     \
    } while (0)

extern volatile RwUInt16 lbl_8025E800;
extern volatile RwUInt16 lbl_8025F890;

RwBool _rwDlTokenDone(RwUInt16 token);
RpGameCubeVtxFmt* RpGameCubeGeometryGetVtxFmt(RpGeometry* geometry);
RwResEntry* RwResourcesAllocateResEntry(void* owner, RwResEntry** ownerRef, RwInt32 size,
                                        void (*destroyNotify)(RwResEntry* resEntry));
RwBool RwResourcesFreeResEntry(RwResEntry* entry);
RwUInt32 _rwGCNVtxFmtInstPos(RpGameCubeVtxFmt* fmt, void* dst, const RwV3d* src, RwInt32 numVerts, RwUInt8 stride);
RwUInt32 _rwGCNVtxFmtInstNrm(RpGameCubeVtxFmt* fmt, void* dst, const RwV3d* src, RwInt32 numVerts, RwUInt8 stride);
RwUInt32 _rwGCNVtxFmtInstNBT(RpGameCubeVtxFmt* fmt, void* dst, const RwRGBA* src, RwInt32 numVerts, RwUInt8 stride);
RwUInt32 _rwGCNVtxFmtInstTex(RpGameCubeVtxFmt* fmt, RwInt32 index, void* dst, const RwTexCoords* src,
                             RwInt32 numVerts, RwUInt8 stride);
void fn_8015F0A4(RwInt32 reset);
void fn_8015F264(RpLight* light, RxGameCubeRenderData* data);
RwUInt32 fn_8015ED40(RpMeshHeader* meshHeader, RwInt32 numTexCoords, RwUInt32 indexSize, RwUInt32 flags);
void fn_8015EDD0(RxGameCubeDisplayList* displayLists, RwUInt8* dst, RpMeshHeader* meshHeader, RwInt32 numTexCoords,
                 RwUInt32 indexSize, RwUInt32 flags);

void GXSetDrawSync(RwUInt16 token);
void GXInvalidateVtxCache(void);
void DCFlushRange(void* addr, RwUInt32 nBytes);
void* memset(void* dst, RwInt32 c, RwUInt32 n);

/* Wait until the GPU has finished with the entry's vertex arrays. */
static inline void GCWaitToken(RxGameCubeInstanceData* inst)
{
    if (inst->token == lbl_8025E800) {
        GXSetDrawSync(lbl_8025E800);
        lbl_8025E800 = (lbl_8025E800 + 1) % 0xE000;
    }

    while (!_rwDlTokenDone(inst->token)) {
    }
}

RpAtomic* fn_8015DA50(RpAtomic* atomic, RxGameCubeRenderData* data)
{
    RwUInt32 flags = atomic->geometry->flags;

    data->lightMask0 = 0;
    data->lightMask1 = 0;
    data->ambient[0] = 0.0f;
    data->ambient[1] = 0.0f;
    data->ambient[2] = 0.0f;
    data->ambient[3] = 1.0f;
    data->ambientLight = FALSE;
    data->numLights = 0;

    if ((flags & rpGEOMETRYLIGHT) && RWSRCGLOBAL(curWorld)) {
        RwLLLink* cur;
        RwLLLink* end;

        fn_8015F0A4(1);
        RWSRCGLOBAL(lightFrame)++;

        end = rwLinkListGetTerminator(&atomic->llWorldSectorsInAtomic);
        cur = rwLinkListGetFirstLLLink(&atomic->llWorldSectorsInAtomic);
        while (cur != end) {
            RpWorldSector* sector = rwLLLinkGetData(cur, RpTie, lWorldSectorInAtomic)->worldSector;
            RwLLLink* lcur;
            RwLLLink* lend;

            lend = rwLinkListGetTerminator(&sector->lightsInWorldSector);
            lcur = rwLinkListGetFirstLLLink(&sector->lightsInWorldSector);
            while (lcur != lend) {
                RpLightTie* tie = rwLLLinkGetData(lcur, RpLightTie, lightInWorldSector);

                if (tie->light && tie->light->lightFrame != RWSRCGLOBAL(lightFrame) &&
                    (tie->light->object.object.flags & rpLIGHTLIGHTATOMICS)) {
                    tie->light->lightFrame = RWSRCGLOBAL(lightFrame);
                    fn_8015F264(tie->light, data);
                }
                lcur = rwLLLinkGetNext(lcur);
            }
            cur = rwLLLinkGetNext(cur);
        }
    }

    if (data->ambientLight) {
        if (data->ambient[0] > 1.0f) {
            data->ambient[0] = 1.0f;
        }
        if (data->ambient[1] > 1.0f) {
            data->ambient[1] = 1.0f;
        }
        if (data->ambient[2] > 1.0f) {
            data->ambient[2] = 1.0f;
        }
    }

    return atomic;
}

void fn_8015DBA4(RpGeometry* geom, RxGameCubeInstanceData* inst, RpInterpolator* interp)
{
    RwInt32 numVerts = geom->numVertices;
    RwInt32 start = interp->startMorphTarget;
    RwInt32 end = interp->endMorphTarget;
    RwV3d* pos = (RwV3d*)inst->posData;
    RwV3d* nrm = (RwV3d*)inst->nrmData;

    if (start >= geom->numMorphTargets || end >= geom->numMorphTargets) {
        end = 0;
        start = 0;
    }

    if (start == end) {
        RpMorphTarget* mt = &geom->morphTarget[start];
        RwV3d* srcPos = mt->verts;

        if (geom->flags & rxGEOMETRY_NORMALS) {
            RwV3d* srcNrm = mt->normals;
            while (numVerts--) {
                *pos++ = *srcPos++;
                *nrm = *srcNrm++;
                nrm = (RwV3d*)((RwUInt8*)nrm + inst->nrmStride);
            }
            DCFlushRange(inst->posData, geom->numVertices * sizeof(RwV3d));
            DCFlushRange(inst->nrmData, inst->nrmStride * geom->numVertices);
        } else {
            while (numVerts--) {
                *pos++ = *srcPos++;
            }
            DCFlushRange(inst->posData, geom->numVertices * sizeof(RwV3d));
        }
    } else {
        RwReal scale = interp->recipTime * interp->position;
        RpMorphTarget* mt1 = &geom->morphTarget[start];
        RpMorphTarget* mt2 = &geom->morphTarget[end];
        RwV3d* pos1 = mt1->verts;
        RwV3d* pos2 = mt2->verts;

        if (geom->flags & rxGEOMETRY_NORMALS) {
            RwV3d* nrm1 = mt1->normals;
            RwV3d* nrm2 = mt2->normals;

            while (numVerts--) {
                RwV3dSub(pos, pos2, pos1);
                pos2++;
                RwV3dScale(pos, pos, scale);
                RwV3dAdd(pos, pos, pos1);
                pos1++;
                pos++;
                RwV3dSub(nrm, nrm2, nrm1);
                nrm2++;
                RwV3dScale(nrm, nrm, scale);
                RwV3dAdd(nrm, nrm, nrm1);
                nrm1++;
                nrm = (RwV3d*)((RwUInt8*)nrm + inst->nrmStride);
            }
            DCFlushRange(inst->posData, geom->numVertices * sizeof(RwV3d));
            DCFlushRange(inst->nrmData, inst->nrmStride * geom->numVertices);
        } else {
            while (numVerts--) {
                RwV3dSub(pos, pos2, pos1);
                pos2++;
                RwV3dScale(pos, pos, scale);
                RwV3dAdd(pos, pos, pos1);
                pos1++;
                pos++;
            }
            DCFlushRange(inst->posData, geom->numVertices * sizeof(RwV3d));
        }
    }

    GXInvalidateVtxCache();
}

RpAtomic* fn_8015E054(RpAtomic* atomic, RxGameCubeRenderData* data)
{
    RpGeometry* geom = atomic->geometry;
    RxGameCubeInstanceData* inst = RESENTRYDATA(data->repEntry);

    if (geom->lockedSinceLastInst) {
        RwUInt32 locked;
        RwUInt32 flags;
        RwInt32 numVerts;

        GCWaitToken(inst);

        locked = geom->lockedSinceLastInst;
        flags = (geom->flags | rpGEOMETRYPOSITIONS) & ~data->pvtData->unk10;
        numVerts = geom->numVertices;

        if ((locked & rpGEOMETRYLOCKVERTICES) && (flags & rpGEOMETRYPOSITIONS)) {
            DCFlushRange(inst->posData, _rwGCNVtxFmtInstPos(RpGameCubeGeometryGetVtxFmt(geom), inst->posData,
                                                            geom->morphTarget->verts, numVerts, inst->posStride));
        }
        if ((locked & rpGEOMETRYLOCKNORMALS) && (flags & rxGEOMETRY_NORMALS)) {
            DCFlushRange(inst->nrmData,
                         _rwGCNVtxFmtInstNrm(RpGameCubeGeometryGetVtxFmt(geom), inst->nrmData,
                                             geom->morphTarget->normals, geom->numVertices, inst->nrmStride));
        }
        if ((locked & rpGEOMETRYLOCKPRELIGHT) && (flags & rxGEOMETRY_PRELIT)) {
            DCFlushRange(inst->clrData, _rwGCNVtxFmtInstNBT(RpGameCubeGeometryGetVtxFmt(geom), inst->clrData,
                                                            geom->preLitLum, geom->numVertices, inst->clrStride));
        }
        if ((locked & rpGEOMETRYLOCKTEXCOORDSALL) && (flags & (rxGEOMETRY_TEXTURED | rxGEOMETRY_TEXTURED2))) {
            RwInt32 i;

            for (i = 0; i < geom->numTexCoordSets; i++) {
                if ((rpGEOMETRYLOCKTEXCOORDS1 << i) & locked) {
                    DCFlushRange(inst->texData[i],
                                 _rwGCNVtxFmtInstTex(RpGameCubeGeometryGetVtxFmt(geom), i, inst->texData[i],
                                                     geom->texCoords[i], geom->numVertices, inst->texStride[i]));
                }
            }
        }

        GXInvalidateVtxCache();
        geom->lockedSinceLastInst = 0;
    }

    if (geom->numMorphTargets != 1 && (atomic->interpolator.flags & rpINTERPOLATORDIRTYINSTANCE)) {
        GCWaitToken(inst);
        fn_8015DBA4(geom, inst, &atomic->interpolator);
        atomic->interpolator.flags &= ~rpINTERPOLATORDIRTYINSTANCE;
    }

    return atomic;
}

void* fn_8015E2E8(void* object, RxGameCubeRenderData* data)
{
    RwUInt8* dst;
    void* owner;
    RwUInt32 size;
    RpGeometry* geom;
    RwResEntry** ownerRef;
    RwUInt32 dlSize;
    RxGameCubeInstanceData* inst;
    RwUInt32 flags;
    RwUInt32 numVerts;

    RpAtomic* atomic;
    RwUInt32 arrayFlags;
    RwUInt32 numMeshes;
    RpMeshHeader* meshHeader;
    atomic = (RpAtomic*)object;
    geom = atomic->geometry;
    meshHeader = geom->mesh;
    numVerts = geom->numVertices;
    numMeshes = meshHeader->numMeshes;

    if (geom->numMorphTargets != 1) {
        owner = atomic;
        ownerRef = &atomic->repEntry;
    } else {
        owner = geom;
        ownerRef = &geom->repEntry;
    }

    flags = geom->flags | rpGEOMETRYPOSITIONS;
    arrayFlags = flags & ~data->pvtData->unk10;
    dlSize = fn_8015ED40(meshHeader, geom->numTexCoordSets, (numVerts < 0xFF) ? 1 : 2, flags);

    size = (numMeshes * sizeof(RxGameCubeDisplayList) + (sizeof(RxGameCubeInstanceData) - sizeof(RxGameCubeDisplayList)) +
            31) & ~31;
    size += dlSize;
    size = ((size + 31) & ~31) +
           _rxGCVertexArraySize(RpGameCubeGeometryGetVtxFmt(geom), arrayFlags, numVerts, geom->numTexCoordSets);

    if (geom->flags & rpGEOMETRYNATIVEINSTANCE) {
        data->repEntry = (RwResEntry*)RwMalloc(size + sizeof(RwResEntry) + 32);
        data->repEntry->link.next = NULL;
        data->repEntry->link.prev = NULL;
        data->repEntry->owner = owner;
        data->repEntry->size = size + 32;
        data->repEntry->ownerRef = ownerRef;
        data->repEntry->destroyNotify = _rxGCResEntryWaitDone;
        *ownerRef = data->repEntry;
    } else {
        data->repEntry = RwResourcesAllocateResEntry(owner, ownerRef, size + 32, _rxGCResEntryWaitDone);
    }

    inst = RESENTRYDATA(data->repEntry);
    memset(inst, 0, sizeof(RxGameCubeInstanceData) - sizeof(RxGameCubeDisplayList));
    inst->serialNum = meshHeader->serialNum;
    inst->descType = (numVerts < 0xFF) ? GX_INDEX8 : GX_INDEX16;
    inst->token = lbl_8025F890;
    inst->unk14 = data->pvtData->unk10;

    dst = (RwUInt8*)(((RwUInt32)((RwUInt8*)inst + numMeshes * sizeof(RxGameCubeDisplayList)) +
                      (sizeof(RxGameCubeInstanceData) - sizeof(RxGameCubeDisplayList)) + 31) &
                     ~31);
    fn_8015EDD0(inst->displayLists, dst, meshHeader, geom->numTexCoordSets, (numVerts < 0xFF) ? 1 : 2, flags);
    dst = (RwUInt8*)(((RwUInt32)dst + dlSize + 31) & ~31);
    _rxGCInstanceGeometryArrays(geom, inst, RpGameCubeGeometryGetVtxFmt(geom), arrayFlags, geom->numTexCoordSets, dst);

    DCFlushRange(RESENTRYDATA(data->repEntry), data->repEntry->size);
    geom->lockedSinceLastInst = 0;

    return atomic;
}

RwBool fn_8015E514(RxPipelineNodeInstance* self, const RxPipelineNodeParam* params)
{
    RpAtomic* atomic = (RpAtomic*)params->dataParam;
    RxGameCubeAllInOnePvtData* pvt = (RxGameCubeAllInOnePvtData*)self->privateData;
    RpGeometry* geom = atomic->geometry;
    RxGameCubeRenderData data;

    data.pvtData = pvt;
    if (!(geom->flags & rpGEOMETRYNATIVE)) {
        RpMeshHeader* meshHeader;

        if (!(RwUInt32)geom->numVertices) {
            return TRUE;
        }
        meshHeader = geom->mesh;
        if (!meshHeader->numMeshes) {
            return TRUE;
        }

        data.repEntry = (geom->numMorphTargets != 1) ? atomic->repEntry : geom->repEntry;
        data.meshHeader = geom->mesh;
        data.flags = geom->flags;

        if (data.repEntry && RESENTRYDATA(data.repEntry)->serialNum != meshHeader->serialNum) {
            RwResourcesFreeResEntry(data.repEntry);
            data.repEntry = NULL;
        }

        if (data.repEntry) {
            if (pvt->reinstanceCB && pvt->reinstanceCB(atomic, &data) != atomic) {
                return FALSE;
            }
            RwResourcesUseResEntry(data.repEntry);
        } else {
            if (pvt->instanceCB && pvt->instanceCB(atomic, &data) != atomic) {
                return FALSE;
            }
            if (pvt->reinstanceCB && pvt->reinstanceCB(atomic, &data) != atomic) {
                return FALSE;
            }
        }
    } else {
        data.repEntry = geom->repEntry;
        data.meshHeader = geom->mesh;
        data.flags = geom->flags;
        if (pvt->reinstanceCB && pvt->reinstanceCB(atomic, &data) != atomic) {
            return FALSE;
        }
    }

    if (pvt->lightingCB && pvt->lightingCB(atomic, &data) != atomic) {
        return FALSE;
    }
    if (pvt->renderCB && pvt->renderCB(atomic, &data) != atomic) {
        return FALSE;
    }

    return TRUE;
}

RwBool fn_8015E798(RxPipelineNode* node)
{
    RxGameCubeAllInOnePvtData* pvt = (RxGameCubeAllInOnePvtData*)node->privateData;

    pvt->instanceCB = fn_8015E2E8;
    pvt->reinstanceCB = (RxGameCubeAllInOneCallBack)fn_8015E054;
    pvt->lightingCB = (RxGameCubeAllInOneCallBack)fn_8015DA50;
    pvt->renderCB = _rxGCDefaultRender;
    pvt->unk10 = 0;

    return TRUE;
}

RxNodeDefinition* RxNodeDefinitionGetGameCubeAtomicAllInOne(void)
{
    static RwChar _GameCubeAtomicAllInOne_csl[] = "GameCubeAtomicAllInOne.csl";

    static RxNodeDefinition nodeGameCubeAtomicAllInOneCSL = {
        _GameCubeAtomicAllInOne_csl,
        { fn_8015E514, NULL, NULL, fn_8015E798, NULL, NULL, NULL },
        { 0, NULL, NULL, 0, NULL },
        sizeof(RxGameCubeAllInOnePvtData),
        0,
        0,
    };

    return &nodeGameCubeAtomicAllInOneCSL;
}
