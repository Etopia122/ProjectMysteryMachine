/*
 * GameCube skin platform code: skinned atomics keep their bind pose arrays in
 * the skin and render from a separate resource entry that is reskinned every
 * time the all-in-one node asks for a reinstance. The vertex blend itself is a
 * hand-scheduled paired-single loop working on quantized vertex data.
 */

#include "rwsdk/rpworld_int.h"

#define rwPLUGIN_ID rwID_SKINPLUGIN

#define rwID_SKINPLUGIN 0x116

#define E_RW_BADVERSION 0x80000004

#define rpATOMIC 1

#define rpGEOMETRYNORMALS 0x00000010
#define rpGEOMETRYNATIVE 0x01000000

#define rpGEOMETRYLOCKVERTICES 0x02
#define rpGEOMETRYLOCKNORMALS 0x04

#define rpHANIMHIERARCHYNOMATRICES 0x0002
#define rpHANIMHIERARCHYLOCALSPACEMATRICES 0x4000

#define rpSKINTYPEGENERIC 1

#define rpSKINPIPELINEGENERIC 0x01

#define rxGCNATIVEPLATFORM 6

struct RwResEntry {
    /* 0x00 */ RwLLLink link;
    /* 0x08 */ RwInt32 size;
    /* 0x0C */ void* owner;
    /* 0x10 */ RwResEntry** ownerRef;
    /* 0x14 */ void (*destroyNotify)(RwResEntry* resEntry);
};

typedef struct rxGCInstanceData {
    /* 0x00 */ RwUInt32 pad00;
    /* 0x04 */ RwUInt16 serialNum;
    /* 0x06 */ RwUInt8 posStride;
    /* 0x07 */ RwUInt8 pad07;
    /* 0x08 */ RwUInt8 pad08[0x18 - 0x08];
    /* 0x18 */ RwUInt8* positions;
    /* 0x1C */ RwUInt8* normals;
} rxGCInstanceData;

typedef struct rxGCVtxFmt {
    /* 0x00 */ RwUInt8 posType;
    /* 0x01 */ RwUInt8 nrmType;
    /* 0x02 */ RwUInt8 pad02[0x0C - 0x02];
    /* 0x0C */ RwUInt8 posFrac;
    /* 0x0D */ RwUInt8 nbt;
} rxGCVtxFmt;

typedef struct RpHAnimNodeInfo {
    RwInt32 nodeID;
    RwInt32 nodeIndex;
    RwInt32 flags;
    RwFrame* pFrame;
} RpHAnimNodeInfo;

typedef struct RpHAnimHierarchy {
    /* 0x00 */ RwInt32 flags;
    /* 0x04 */ RwInt32 numNodes;
    /* 0x08 */ RwUInt8 pad08[0x28 - 0x08];
    /* 0x28 */ RwMatrix* pMatrixArray;
    /* 0x2C */ void* pMatrixArrayUnaligned;
    /* 0x30 */ RpHAnimNodeInfo* pNodeInfo;
} RpHAnimHierarchy;

typedef struct RwMatrixWeights {
    RwReal w0;
    RwReal w1;
    RwReal w2;
    RwReal w3;
} RwMatrixWeights;

typedef struct RpSkin {
    /* 0x00 */ RwUInt32 numBones;
    /* 0x04 */ RwMatrix* skinToBoneMatrices;
    /* 0x08 */ RwUInt32* vertexBoneIndices;
    /* 0x0C */ RwMatrixWeights* vertexBoneWeights;
    /* 0x10 */ RwUInt8* positions;
    /* 0x14 */ RwUInt8* normals;
    /* 0x18 */ void* unaligned;
} RpSkin;

typedef struct SkinGlobals {
    /* 0x00 */ RwInt32 engineOffset;
    /* 0x04 */ RwInt32 atomicOffset;
    /* 0x08 */ RwInt32 geometryOffset;
    /* 0x0C */ RwMatrix* alignedMatrixCache;
    /* 0x10 */ void* unalignedMatrixCache;
    /* 0x14 */ RwFreeList* freeList;
    /* 0x18 */ RwModuleInfo module;
    /* 0x20 */ RxPipeline* pipelines[6];
} SkinGlobals;

typedef void* (*SkinReinstanceCallBack)(void* object, RwResEntry** resEntryRef);

#define RPSKINATOMICGETDATA(atomic) (*(RpHAnimHierarchy**)((RwUInt8*)(atomic) + _rpSkinGlobals.atomicOffset))
#define RPSKINGEOMETRYGETDATA(geometry) (*(RpSkin**)((RwUInt8*)(geometry) + _rpSkinGlobals.geometryOffset))

#define GCRESENTRYDATA(resEntry) ((rxGCInstanceData*)((resEntry) + 1))
#define GCRESENTRYTOKEN(resEntry) (*(RwUInt16*)((resEntry) + 1))
#define SKINBUFFERRESENTRY(buffer) (((RwResEntry**)(buffer))[-1])

#define ALIGN32(x) (((RwUInt32)(x) + 31) & ~31)

#define RwResourcesUseResEntry(_ntry)                                              \
    do {                                                                           \
        if ((_ntry)->link.next) {                                                  \
            rwLLLinkRemove(&(_ntry)->link);                                        \
            rwLinkListAddLLLink(RWRESOURCESGLOBAL(usedEntries), &(_ntry)->link);   \
        }                                                                          \
    } while (0)

#define GCDRAWSYNCWAIT(_token)                                      \
    do {                                                            \
        if ((_token) == lbl_8025E800) {                             \
            GXSetDrawSync(lbl_8025E800);                            \
            lbl_8025E800 = (lbl_8025E800 + 1) % 0xE000;             \
        }                                                           \
        while (!_rwDlTokenDone(_token)) {                           \
        }                                                           \
    } while (0)

typedef struct rwResourcesGlobals {
    RwUInt32 maxSize;
    RwUInt32 currentSize;
    RwUInt32 reusageSize;
    void* memHeap;
    RwLinkList entriesA;
    RwLinkList entriesB;
    RwLinkList* freeEntries;
    RwLinkList* usedEntries;
} rwResourcesGlobals;

extern RwModuleInfo resourcesModule;

#define RWRESOURCESGLOBAL(var) (RWPLUGINOFFSET(rwResourcesGlobals, RwEngineInstance, resourcesModule.globalsOffset)->var)

extern SkinGlobals _rpSkinGlobals;
extern volatile RwUInt16 lbl_8025E800;

RwResEntry* RwResourcesAllocateResEntry(void* owner, RwResEntry** ownerRef, RwInt32 size,
                                        void (*destroyNotify)(RwResEntry* resEntry));
RwBool RwResourcesFreeResEntry(RwResEntry* entry);
RwMatrix* RwFrameGetLTM(RwFrame* frame);
RwMatrix* RwMatrixMultiply(RwMatrix* matrixOut, const RwMatrix* matrixIn1, const RwMatrix* matrixIn2);
RwMatrix* RwMatrixInvert(RwMatrix* matrixOut, const RwMatrix* matrixIn);
RwUInt32 RwStreamRead(RwStream* stream, void* buffer, RwUInt32 length);
RwStream* RwStreamWrite(RwStream* stream, const void* buffer, RwUInt32 length);
void* memset(void* dst, int c, unsigned long n);
RwMatrix* RpSkinGetSkinToBoneMatrices(RpSkin* skin);
RpGeometry* RpSkinGeometrySetSkin(RpGeometry* geometry, RpSkin* skin);
rxGCVtxFmt* RpGameCubeGeometryGetVtxFmt(const RpGeometry* geometry);
RxNodeDefinition* RxNodeDefinitionGetGameCubeAtomicAllInOne(void);
RxPipelineNode* RxPipelineFindNodeByName(RxPipeline* pipeline, const RwChar* name, RxPipelineNode* start,
                                         RwInt32* nodeIndex);
void _rxGameCubeAllInOneSetInstanceCallBack(RxPipelineNode* node, void* callBack);
SkinReinstanceCallBack _rxGameCubeAllInOneGetReinstanceCallBack(RxPipelineNode* node);
void _rxGameCubeAllInOneSetReinstanceCallBack(RxPipelineNode* node, void* callBack);
void RxGameCubeAllInOneSetRenderCallBack(RxPipelineNode* node, void* callBack);
void _rxGCResEntryWaitDone(RwResEntry* resEntry);
RwBool _rwDlTokenDone(RwUInt16 token);
void GXSetDrawSync(RwUInt16 token);
void GXInvalidateVtxCache(void);
void DCFlushRange(void* addr, RwUInt32 nBytes);

static SkinReinstanceCallBack SkinDefaultReinstance;

static inline void SkinSetQuantization(register RwUInt32 gqr6, register RwUInt32 gqr7)
{
    asm {
        mtspr GQR6, gqr6
        mtspr GQR7, gqr7
    }
}

asm void _rpSkinGameCubeBlend(const RwMatrixWeights* weights, const RwUInt8* posIn, const RwUInt8* nrmIn,
                              RwUInt8* posOut, RwUInt8* nrmOut, const RwMatrix* matrices,
                              const RwUInt32* indices, RwInt32 numVertices, RwInt32 posStride,
                              RwInt32 nrmStride, RwInt32 nrmExtra)
{
    nofralloc
    stwu r1, -0xb8(r1)
    stfd f14, 0x8(r1)
    stfd f15, 0x10(r1)
    stfd f16, 0x18(r1)
    stfd f17, 0x20(r1)
    stfd f18, 0x28(r1)
    stfd f19, 0x30(r1)
    stfd f20, 0x38(r1)
    stfd f21, 0x40(r1)
    stfd f22, 0x48(r1)
    stfd f23, 0x50(r1)
    stfd f24, 0x58(r1)
    stfd f25, 0x60(r1)
    stfd f26, 0x68(r1)
    stfd f27, 0x70(r1)
    stfd f28, 0x78(r1)
    stfd f29, 0x80(r1)
    stfd f30, 0x88(r1)
    stfd f31, 0x90(r1)
    stw r14, 0x98(r1)
    stw r15, 0x9c(r1)
    stw r16, 0xa0(r1)
    stw r17, 0xa4(r1)
    stw r18, 0xa8(r1)
    stw r19, 0xac(r1)
    stw r20, 0xb0(r1)
    lwz r18, 0xc0(r1)
    lwz r19, 0xc4(r1)
    lwz r20, 0xc8(r1)
    cmpwi r10, 0
    mtctr r10
    addi r3, r3, -4
    addi r9, r9, -4
    subf r4, r18, r4
    subf r5, r19, r5
    subf r6, r18, r6
    subf r7, r19, r7
    ble done
    cmpw r4, r5
vertexLoop:
    ps_sub f0, f0, f0
    lfsu f28, 4(r3)
    lfsu f29, 4(r3)
    lfsu f30, 4(r3)
    lfsu f31, 4(r3)
    lwzu r11, 4(r9)
    ps_cmpo0 cr5, f29, f0
    ps_cmpo0 cr6, f30, f0
    ps_cmpo0 cr7, f31, f0
    rlwinm r14, r11, 6, 18, 25
    rlwinm r15, r11, 30, 18, 25
    rlwinm r16, r11, 22, 18, 25
    rlwinm r17, r11, 14, 18, 25
    add r14, r14, r8
    add r15, r15, r8
    add r16, r16, r8
    add r17, r17, r8
    ble cr5, oneBone
    ble cr6, twoBones
    ble cr7, threeBones
    b blend
oneBone:
    addi r15, r14, 0
twoBones:
    addi r16, r14, 0
threeBones:
    addi r17, r14, 0
blend:
    psq_lux f16, r4, r18, 1, 6
    psq_lux f17, r4, r18, 1, 6
    psq_lux f18, r4, r18, 1, 6
    psq_lux f21, r5, r19, 1, 7
    psq_lux f22, r5, r19, 1, 7
    psq_lux f23, r5, r19, 1, 7
    add r5, r5, r20
    psq_l f0, 0x0(r14), 0, 0
    psq_l f1, 0x8(r14), 1, 0
    psq_l f2, 0x10(r14), 0, 0
    psq_l f3, 0x18(r14), 1, 0
    psq_l f4, 0x20(r14), 0, 0
    psq_l f5, 0x28(r14), 1, 0
    psq_l f6, 0x30(r14), 0, 0
    psq_l f7, 0x38(r14), 1, 0
    ps_madds0 f6, f0, f16, f6
    psq_l f8, 0x0(r15), 0, 0
    ps_madds0 f7, f1, f16, f7
    ps_muls0 f24, f0, f21
    psq_l f9, 0x8(r15), 1, 0
    ps_muls0 f25, f1, f21
    ps_madds0 f6, f2, f17, f6
    psq_l f10, 0x10(r15), 0, 0
    ps_madds0 f7, f3, f17, f7
    ps_madds0 f24, f2, f22, f24
    psq_l f11, 0x18(r15), 1, 0
    ps_madds0 f25, f3, f22, f25
    ps_madds0 f6, f4, f18, f6
    psq_l f12, 0x20(r15), 0, 0
    ps_madds0 f7, f5, f18, f7
    ps_madds0 f24, f4, f23, f24
    psq_l f13, 0x28(r15), 1, 0
    ps_madds0 f25, f5, f23, f25
    ps_muls0 f19, f6, f28
    psq_l f14, 0x30(r15), 0, 0
    ps_muls0 f20, f7, f28
    ps_muls0 f26, f24, f28
    psq_l f15, 0x38(r15), 1, 0
    ps_muls0 f27, f25, f28
    ble cr5, store
    nop
    ps_madds0 f14, f8, f16, f14
    psq_l f0, 0x0(r16), 0, 0
    ps_madds0 f15, f9, f16, f15
    ps_muls0 f24, f8, f21
    psq_l f1, 0x8(r16), 1, 0
    ps_muls0 f25, f9, f21
    ps_madds0 f14, f10, f17, f14
    psq_l f2, 0x10(r16), 0, 0
    ps_madds0 f15, f11, f17, f15
    ps_madds0 f24, f10, f22, f24
    psq_l f3, 0x18(r16), 1, 0
    ps_madds0 f25, f11, f22, f25
    ps_madds0 f14, f12, f18, f14
    psq_l f4, 0x20(r16), 0, 0
    ps_madds0 f15, f13, f18, f15
    ps_madds0 f24, f12, f23, f24
    psq_l f5, 0x28(r16), 1, 0
    ps_madds0 f25, f13, f23, f25
    ps_madds0 f19, f14, f29, f19
    psq_l f6, 0x30(r16), 0, 0
    ps_madds0 f20, f15, f29, f20
    ps_madds0 f26, f24, f29, f26
    psq_l f7, 0x38(r16), 1, 0
    ps_madds0 f27, f25, f29, f27
    ble cr6, store
    nop
    ps_madds0 f6, f0, f16, f6
    psq_l f8, 0x0(r17), 0, 0
    ps_madds0 f7, f1, f16, f7
    ps_muls0 f24, f0, f21
    psq_l f9, 0x8(r17), 1, 0
    ps_muls0 f25, f1, f21
    ps_madds0 f6, f2, f17, f6
    psq_l f10, 0x10(r17), 0, 0
    ps_madds0 f7, f3, f17, f7
    ps_madds0 f24, f2, f22, f24
    psq_l f11, 0x18(r17), 1, 0
    ps_madds0 f25, f3, f22, f25
    ps_madds0 f6, f4, f18, f6
    psq_l f12, 0x20(r17), 0, 0
    ps_madds0 f7, f5, f18, f7
    ps_madds0 f24, f4, f23, f24
    psq_l f13, 0x28(r17), 1, 0
    ps_madds0 f25, f5, f23, f25
    ps_madds0 f19, f6, f30, f19
    psq_l f14, 0x30(r17), 0, 0
    ps_madds0 f20, f7, f30, f20
    ps_madds0 f26, f24, f30, f26
    psq_l f15, 0x38(r17), 1, 0
    ps_madds0 f27, f25, f30, f27
    ble cr7, store
    nop
    ps_madds0 f14, f8, f16, f14
    ps_madds0 f15, f9, f16, f15
    ps_muls0 f24, f8, f21
    ps_muls0 f25, f9, f21
    ps_madds0 f14, f10, f17, f14
    ps_madds0 f15, f11, f17, f15
    ps_madds0 f24, f10, f22, f24
    ps_madds0 f25, f11, f22, f25
    ps_madds0 f14, f12, f18, f14
    ps_madds0 f15, f13, f18, f15
    ps_madds0 f24, f12, f23, f24
    ps_madds0 f25, f13, f23, f25
    ps_madds0 f19, f14, f31, f19
    ps_madds0 f20, f15, f31, f20
    ps_madds0 f26, f24, f31, f26
    ps_madds0 f27, f25, f31, f27
store:
    ps_merge11 f14, f19, f19
    psq_stux f19, r6, r18, 1, 6
    psq_stux f14, r6, r18, 1, 6
    psq_stux f20, r6, r18, 1, 6
    beq next
    ps_merge11 f24, f26, f26
    psq_stux f26, r7, r19, 1, 7
    psq_stux f24, r7, r19, 1, 7
    psq_stux f27, r7, r19, 1, 7
    add r7, r7, r20
next:
    bdnz vertexLoop
    nop
done:
    lfd f14, 0x8(r1)
    lfd f15, 0x10(r1)
    lfd f16, 0x18(r1)
    lfd f17, 0x20(r1)
    lfd f18, 0x28(r1)
    lfd f19, 0x30(r1)
    lfd f20, 0x38(r1)
    lfd f21, 0x40(r1)
    lfd f22, 0x48(r1)
    lfd f23, 0x50(r1)
    lfd f24, 0x58(r1)
    lfd f25, 0x60(r1)
    lfd f26, 0x68(r1)
    lfd f27, 0x70(r1)
    lfd f28, 0x78(r1)
    lfd f29, 0x80(r1)
    lfd f30, 0x88(r1)
    lfd f31, 0x90(r1)
    lwz r14, 0x98(r1)
    lwz r15, 0x9c(r1)
    lwz r16, 0xa0(r1)
    lwz r17, 0xa4(r1)
    lwz r18, 0xa8(r1)
    lwz r19, 0xac(r1)
    lwz r20, 0xb0(r1)
    addi r1, r1, 0xb8
    blr
}

void SkinAtomicResEntryDestroy(RwResEntry* resEntry)
{
    RpSkin* skin;

    GCDRAWSYNCWAIT(GCRESENTRYDATA(resEntry)->serialNum);

    if (GCRESENTRYDATA(resEntry)->positions) {
        RwResourcesFreeResEntry(SKINBUFFERRESENTRY(GCRESENTRYDATA(resEntry)->positions));
    }

    if (((RwObject*)resEntry->owner)->type == rpATOMIC) {
        skin = RPSKINGEOMETRYGETDATA(((RpAtomic*)resEntry->owner)->geometry);
    } else {
        skin = RPSKINGEOMETRYGETDATA(resEntry->owner);
    }

    GCRESENTRYDATA(resEntry)->positions = skin->positions;
    GCRESENTRYDATA(resEntry)->normals = skin->normals;
    skin->positions = NULL;
    skin->normals = NULL;

    GXInvalidateVtxCache();
}

void SkinBufferResEntryDestroy(RwResEntry* resEntry)
{
    GCDRAWSYNCWAIT(GCRESENTRYTOKEN(resEntry));

    GXInvalidateVtxCache();
}

RwBool SkinBufferInstance(RpSkin* skin, RpAtomic* atomic, rxGCInstanceData* inst, RwResEntry** resEntryRef)
{
    RpGeometry* geometry = atomic->geometry;

    RwResourcesUseResEntry(*resEntryRef);
    (*resEntryRef)->destroyNotify = SkinAtomicResEntryDestroy;

    if (skin->positions == NULL) {
        skin->positions = inst->positions;
        skin->normals = inst->normals;
        inst->positions = NULL;
        inst->normals = NULL;
    }

    if (inst->positions) {
        if (!_rwDlTokenDone(GCRESENTRYTOKEN(SKINBUFFERRESENTRY(inst->positions)))) {
            SKINBUFFERRESENTRY(inst->positions)->ownerRef = NULL;
            inst->positions = NULL;
            inst->normals = NULL;
        } else {
            GXInvalidateVtxCache();
        }
    }

    if (inst->positions == NULL) {
        RwResEntry* resEntry;
        RwResEntry* repEntry;
        rxGCVtxFmt* vtxFmt;
        RwUInt32 size;

        vtxFmt = RpGameCubeGeometryGetVtxFmt(geometry);
        if (vtxFmt) {
            RwUInt32 compSizes[5] = { 3, 3, 6, 6, 12 };

            size = ALIGN32(geometry->numVertices * compSizes[vtxFmt->posType]) + 6;
            if (geometry->flags & rpGEOMETRYNORMALS) {
                size += ALIGN32(geometry->numVertices * (compSizes[vtxFmt->nrmType] * (vtxFmt->nbt ? 3 : 1)));
            }
        } else {
            RwUInt32 arraySize = ALIGN32(geometry->numVertices * sizeof(RwV3d));

            size = arraySize + 6;
            if (geometry->flags & rpGEOMETRYNORMALS) {
                size += arraySize;
            }
        }

        resEntry = RwResourcesAllocateResEntry(atomic, (RwResEntry**)&inst->positions, size, SkinBufferResEntryDestroy);

        if (geometry->numMorphTargets == 1) {
            repEntry = geometry->repEntry;
        } else {
            repEntry = atomic->repEntry;
        }

        if (resEntry == NULL || repEntry == NULL) {
            if (resEntry) {
                RwResourcesFreeResEntry(resEntry);
            }
            return FALSE;
        }

        inst->positions = (RwUInt8*)ALIGN32(inst->positions + sizeof(RwResEntry) + 6);
        if (geometry->flags & rpGEOMETRYNORMALS) {
            inst->normals = inst->positions + ALIGN32(geometry->numVertices * inst->posStride);
        }
        SKINBUFFERRESENTRY(inst->positions) = resEntry;
    } else {
        RwResourcesUseResEntry(SKINBUFFERRESENTRY(inst->positions));
    }

    GCRESENTRYTOKEN(SKINBUFFERRESENTRY(inst->positions)) = lbl_8025E800;
    return TRUE;
}

static inline void SkinMatricesUpdate(RpAtomic* atomic, RpSkin* skin)
{
    RpHAnimHierarchy* hierarchy;
    RwMatrix* ltm;
    RwMatrix* skinToBone;
    RwMatrix* matrices;
    RwInt32 i;

    hierarchy = RPSKINATOMICGETDATA(atomic);
    ltm = RwFrameGetLTM((RwFrame*)rwObjectGetParent(atomic));
    skinToBone = RpSkinGetSkinToBoneMatrices(skin);
    matrices = _rpSkinGlobals.alignedMatrixCache;

    if (hierarchy) {
        if (hierarchy->flags & rpHANIMHIERARCHYNOMATRICES) {
            RwMatrix inverseLTM;
            RwMatrix tmp;

            RwMatrixInvert(&inverseLTM, ltm);
            for (i = 0; i < hierarchy->numNodes; i++) {
                RwMatrixMultiply(&tmp, &skinToBone[i], RwFrameGetLTM(hierarchy->pNodeInfo[i].pFrame));
                RwMatrixMultiply(&matrices[i], &tmp, &inverseLTM);
            }
        } else if (hierarchy->flags & rpHANIMHIERARCHYLOCALSPACEMATRICES) {
            for (i = 0; i < hierarchy->numNodes; i++) {
                RwMatrixMultiply(&matrices[i], &skinToBone[i], &hierarchy->pMatrixArray[i]);
            }
        } else {
            RwMatrix inverseLTM;
            RwMatrix tmp;

            RwMatrixInvert(&inverseLTM, ltm);
            for (i = 0; i < hierarchy->numNodes; i++) {
                RwMatrixMultiply(&tmp, &skinToBone[i], &hierarchy->pMatrixArray[i]);
                RwMatrixMultiply(&matrices[i], &tmp, &inverseLTM);
            }
        }
    }
}

void* SkinAtomicReinstance(void* object, RwResEntry** resEntryRef)
{
    RpAtomic* atomic;
    RpGeometry* geometry;
    RpSkin* skin;
    rxGCInstanceData* inst;
    rxGCVtxFmt* vtxFmt;
    RwInt32 numVertices;
    RwUInt32 gqr6;
    RwUInt32 gqr7;
    RwInt32 posStride;
    RwInt32 nrmStride;
    RwInt32 nrmExtra;
    const RwMatrix* matrices;
    const RwMatrixWeights* weights;
    const RwUInt32* indices;
    const RwUInt8* posIn;
    const RwUInt8* nrmIn;
    RwUInt8* posOut;
    RwUInt8* nrmOut;
    atomic = (RpAtomic*)object;
    geometry = ((RpAtomic*)object)->geometry;
    skin = RPSKINGEOMETRYGETDATA(geometry);
    inst = GCRESENTRYDATA(*resEntryRef);

    if (skin->positions == NULL) {
        if (!(geometry->flags & rpGEOMETRYNATIVE)) {
            SkinDefaultReinstance(object, resEntryRef);
        }
        skin->positions = inst->positions;
        skin->normals = inst->normals;
        inst->positions = NULL;
        inst->normals = NULL;
    } else if (!(geometry->flags & rpGEOMETRYNATIVE)) {
        RwUInt8* positions = inst->positions;
        RwUInt8* normals = inst->normals;

        inst->positions = skin->positions;
        inst->normals = skin->normals;
        SkinDefaultReinstance(object, resEntryRef);
        inst->positions = positions;
        inst->normals = normals;
    }

    if (!SkinBufferInstance(skin, atomic, inst, resEntryRef)) {
        return NULL;
    }

    SkinMatricesUpdate(atomic, skin);

    numVertices = geometry->numVertices;
    vtxFmt = RpGameCubeGeometryGetVtxFmt(geometry);

    gqr6 = 0;
    gqr7 = 0;
    posStride = 4;
    nrmStride = 4;
    nrmExtra = 0;
    matrices = _rpSkinGlobals.alignedMatrixCache;
    weights = skin->vertexBoneWeights;
    indices = skin->vertexBoneIndices;
    posIn = skin->positions;
    nrmIn = skin->normals;
    posOut = inst->positions;
    nrmOut = inst->normals;

    if (vtxFmt) {
        RwUInt8 nrmFrac[5] = { 0, 6, 0, 14, 0 };
        RwUInt8 compSize[5] = { 1, 1, 2, 2, 4 };
        RwUInt8 gqrType[5] = { 4, 6, 5, 7, 0 };

        posStride = compSize[vtxFmt->posType];
        nrmStride = compSize[vtxFmt->nrmType];
        nrmExtra = vtxFmt->nbt ? nrmStride * 6 : 0;
        gqr6 = gqrType[vtxFmt->posType] | (vtxFmt->posFrac << 8);
        gqr7 = gqrType[vtxFmt->nrmType] | (nrmFrac[vtxFmt->nrmType] << 8);
        gqr6 |= gqr6 << 16;
        gqr7 |= gqr7 << 16;
    }

    SkinSetQuantization(gqr6, gqr7);

    _rpSkinGameCubeBlend(weights, posIn, nrmOut ? nrmIn : posIn, posOut, nrmOut, matrices, indices, numVertices,
                         posStride, nrmStride, nrmExtra);

    DCFlushRange(SKINBUFFERRESENTRY(posOut) + 1, SKINBUFFERRESENTRY(posOut)->size);

    return object;
}

RpGeometry* _rpSkinInitialize(RpGeometry* geometry)
{
    RpSkin* skin = RPSKINGEOMETRYGETDATA(geometry);

    if (skin) {
        if (geometry->flags & rpGEOMETRYNATIVE) {
            if (geometry->flags & rpGEOMETRYNORMALS) {
                geometry->lockedSinceLastInst |= rpGEOMETRYLOCKVERTICES | rpGEOMETRYLOCKNORMALS;
            } else {
                geometry->lockedSinceLastInst |= rpGEOMETRYLOCKVERTICES;
            }
        }
        skin->positions = NULL;
        skin->normals = NULL;
    }
    return geometry;
}

RpGeometry* _rpSkinDeinitialize(RpGeometry* geometry)
{
    RpSkin* skin = RPSKINGEOMETRYGETDATA(geometry);

    if (skin) {
        skin->positions = NULL;
        skin->normals = NULL;
    }
    return geometry;
}

#pragma dont_inline on
RxPipeline* SkinCreateAtomicPipeline(RwUInt32 type, void* instanceCB, void* reinstanceCB, void* renderCB)
{
    RxPipeline* pipe;
    RxLockedPipe* lpipe;
    RxNodeDefinition* nodeDef;
    RxPipelineNode* node;

    pipe = RxPipelineCreate();
    pipe->pluginId = rwID_SKINPLUGIN;
    pipe->pluginData = type;

    lpipe = RxPipelineLock(pipe);
    nodeDef = RxNodeDefinitionGetGameCubeAtomicAllInOne();
    lpipe = RxLockedPipeAddFragment(lpipe, NULL, nodeDef, NULL);
    RxLockedPipeUnlock(lpipe);

    node = RxPipelineFindNodeByName(pipe, nodeDef->name, NULL, NULL);

    if (instanceCB) {
        _rxGameCubeAllInOneSetInstanceCallBack(node, instanceCB);
    }
    if (reinstanceCB) {
        SkinDefaultReinstance = _rxGameCubeAllInOneGetReinstanceCallBack(node);
        _rxGameCubeAllInOneSetReinstanceCallBack(node, reinstanceCB);
    }
    if (renderCB) {
        RxGameCubeAllInOneSetRenderCallBack(node, renderCB);
    }
    return pipe;
}
#pragma dont_inline reset

RwInt32 _rpSkinGeometryNativeSize(const RpGeometry* geometry)
{
    RwInt32 size = geometry->numVertices * (sizeof(RwUInt32) + sizeof(RwMatrixWeights)) + 20;
    RpSkin* skin = RPSKINGEOMETRYGETDATA(geometry);

    size += skin->numBones * sizeof(RwMatrix);

    if (skin->positions) {
        rxGCInstanceData* inst = GCRESENTRYDATA(geometry->repEntry);

        SKINBUFFERRESENTRY(inst->positions)->ownerRef = NULL;
        inst->positions = skin->positions;
        inst->normals = skin->normals;
        geometry->repEntry->destroyNotify = _rxGCResEntryWaitDone;
        skin->positions = NULL;
        skin->normals = NULL;
    }
    return size;
}

RwStream* _rpSkinGeometryNativeWrite(RwStream* stream, const RpGeometry* geometry)
{
    RwInt32 platform = rxGCNATIVEPLATFORM;
    RpSkin* skin;
    RwInt32 numVertices;

    if (!RwStreamWriteChunkHeader(stream, rwID_STRUCT, _rpSkinGeometryNativeSize(geometry) - 12)) {
        return NULL;
    }
    if (!RwStreamWriteInt32(stream, &platform, sizeof(platform))) {
        return NULL;
    }

    skin = RPSKINGEOMETRYGETDATA(geometry);
    numVertices = geometry->numVertices;

    if (!RwStreamWriteInt32(stream, (RwInt32*)&skin->numBones, sizeof(skin->numBones))) {
        return NULL;
    }
    if (!RwStreamWrite(stream, skin->vertexBoneIndices, numVertices * sizeof(RwUInt32))) {
        return NULL;
    }
    if (!RwStreamWrite(stream, skin->vertexBoneWeights, numVertices * sizeof(RwMatrixWeights))) {
        return NULL;
    }
    if (!RwStreamWrite(stream, skin->skinToBoneMatrices, skin->numBones * sizeof(RwMatrix))) {
        return NULL;
    }
    return stream;
}

RwStream* _rpSkinGeometryNativeRead(RwStream* stream, RpGeometry* geometry)
{
    RwUInt32 version;
    RwUInt32 length;
    RwInt32 platform;
    RwInt32 numVertices;
    RpSkin* skin;

    if (!RwStreamFindChunk(stream, rwID_STRUCT, &length, &version)) {
        return NULL;
    }

    if (version < rwLIBRARYBASEVERSION || version > rwLIBRARYCURRENTVERSION) {
        RWERROR((E_RW_BADVERSION));
        return NULL;
    }

    if (!RwStreamReadInt32(stream, &platform, sizeof(platform))) {
        return NULL;
    }
    if (platform != rxGCNATIVEPLATFORM) {
        return NULL;
    }

    skin = (RpSkin*)RwFreeListAlloc(_rpSkinGlobals.freeList);
    memset(skin, 0, sizeof(RpSkin));

    numVertices = geometry->numVertices;
    length -= 8;
    skin->unaligned = RwMalloc(length);

    skin->vertexBoneIndices = (RwUInt32*)skin->unaligned;
    skin->vertexBoneWeights = (RwMatrixWeights*)(skin->vertexBoneIndices + numVertices);
    skin->skinToBoneMatrices = (RwMatrix*)(skin->vertexBoneWeights + numVertices);

    if (!RwStreamReadInt32(stream, (RwInt32*)&skin->numBones, sizeof(skin->numBones))) {
        return NULL;
    }
    if (RwStreamRead(stream, skin->unaligned, length) != length) {
        return NULL;
    }

    RpSkinGeometrySetSkin(geometry, skin);
    return stream;
}

RwBool _rpSkinPipelinesCreate(RwUInt32 pipes)
{
    SkinGlobals* globals = &_rpSkinGlobals;

    if (pipes & rpSKINPIPELINEGENERIC) {
        globals->pipelines[rpSKINTYPEGENERIC] = SkinCreateAtomicPipeline(rpSKINTYPEGENERIC, NULL, SkinAtomicReinstance, NULL);
    }
    return TRUE;
}

RwBool _rpSkinPipelinesDestroy(void)
{
    if (_rpSkinGlobals.pipelines[rpSKINTYPEGENERIC]) {
        _rxPipelineDestroy(_rpSkinGlobals.pipelines[rpSKINTYPEGENERIC]);
        _rpSkinGlobals.pipelines[rpSKINTYPEGENERIC] = NULL;
    }
    return TRUE;
}

RpAtomic* _rpSkinPipelinesAttach(RpAtomic* atomic, RwInt32 type)
{
    atomic->pipeline = _rpSkinGlobals.pipelines[rpSKINTYPEGENERIC];
    return atomic;
}
