/*
 * Skin plugin: per geometry bone weights, bone indices and inverse bind
 * matrices, plus the atomic's hierarchy pointer. Rendering lives in the
 * GameCube skin pipelines; native (pre-instanced) skins stream through them.
 */

#include "rwsdk/rpworld_int.h"
#include "rwsdk/batkreg.h"

#define rwID_SKINPLUGIN 0x116
#define rwID_MATERIALEFFECTSPLUGIN 0x120
#define rwID_TOONPLUGIN 0x12E

#define rpGEOMETRYNATIVE 0x01000000

#define rpSKINTYPEGENERIC 1
#define rpSKINTYPEMATFX 2
#define rpSKINTYPETOON 3

#define rpSKINPIPELINEGENERIC 0x01
#define rpSKINPIPELINEMATFX 0x02
#define rpSKINPIPELINETOON 0x04

#define rpSKINBONEMARKER 0xDEADDEAD

#define rpSKINMATRIXCACHESIZE (256 * sizeof(RwMatrix) + 15)

#define ALIGN16(p) ((void*)(((RwUInt32)(p) + 15) & ~15))

typedef struct RpHAnimHierarchy RpHAnimHierarchy;

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
    /* 0x10 */ RwUInt32 pad10[2];
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
    /* 0x20 */ RwUInt32 pad20[6];
} SkinGlobals;

#define RPSKINATOMICGETDATA(atomic) (*(RpHAnimHierarchy**)((RwUInt8*)(atomic) + _rpSkinGlobals.atomicOffset))
#define RPSKINGEOMETRYGETDATA(geometry) (*(RpSkin**)((RwUInt8*)(geometry) + _rpSkinGlobals.geometryOffset))

RwInt32 RwEngineGetPluginOffset(RwUInt32 pluginID);
RwInt32 RpAtomicGetPluginOffset(RwUInt32 pluginID);
RwInt32 RpAtomicRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB,
                                     RwPluginDataChunkWriteCallBack writeCB,
                                     RwPluginDataChunkGetSizeCallBack getSizeCB);
RwInt32 RpAtomicSetStreamAlwaysCallBack(RwUInt32 pluginID, RwPluginDataChunkAlwaysCallBack alwaysCB);
RwInt32 RpAtomicSetStreamRightsCallBack(RwUInt32 pluginID, RwPluginDataChunkRightsCallBack rightsCB);
RwStream* RwStreamSkip(RwStream* stream, RwUInt32 offset);
void* memset(void* dst, int c, unsigned long n);

RpAtomic* _rpSkinPipelinesAttach(RpAtomic* atomic, RwInt32 type);
void _rpSkinPipelinesDestroy(void);
RwBool _rpSkinPipelinesCreate(RwUInt32 pipes);
RpGeometry* _rpSkinInitialize(RpGeometry* geometry);
RpGeometry* _rpSkinDeinitialize(RpGeometry* geometry);
RwInt32 _rpSkinGeometryNativeSize(const RpGeometry* geometry);
RwStream* _rpSkinGeometryNativeWrite(RwStream* stream, const RpGeometry* geometry);
RwStream* _rpSkinGeometryNativeRead(RwStream* stream, RpGeometry* geometry);

RpSkin* RpSkinGeometryGetSkin(RpGeometry* geometry);
RpGeometry* RpSkinGeometrySetSkin(RpGeometry* geometry, RpSkin* skin);
RpSkin* RpSkinDestroy(RpSkin* skin);

SkinGlobals _rpSkinGlobals = { 0 };

void* SkinOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    if (_rpSkinGlobals.module.numInstances == 0) {
        RwUInt32 pipes = rpSKINPIPELINEGENERIC;

        if (RwEngineGetPluginOffset(rwID_MATERIALEFFECTSPLUGIN) != -1) {
            pipes |= rpSKINPIPELINEMATFX;
        }
        if (RwEngineGetPluginOffset(rwID_TOONPLUGIN) != -1) {
            pipes |= rpSKINPIPELINETOON;
        }
        _rpSkinPipelinesCreate(pipes);

        _rpSkinGlobals.freeList = RwFreeListCreate(sizeof(RpSkin), 20, 0);

        _rpSkinGlobals.unalignedMatrixCache = RwMalloc(rpSKINMATRIXCACHESIZE);
        memset(_rpSkinGlobals.unalignedMatrixCache, 0, rpSKINMATRIXCACHESIZE);
        _rpSkinGlobals.alignedMatrixCache = (RwMatrix*)ALIGN16(_rpSkinGlobals.unalignedMatrixCache);
    }

    _rpSkinGlobals.module.numInstances++;
    return instance;
}

void* SkinClose(void* instance, RwInt32 offset, RwInt32 size)
{
    if (--_rpSkinGlobals.module.numInstances == 0) {
        _rpSkinPipelinesDestroy();

        RwFreeListDestroy(_rpSkinGlobals.freeList);
        _rpSkinGlobals.freeList = NULL;

        RwFree(_rpSkinGlobals.unalignedMatrixCache);
        _rpSkinGlobals.unalignedMatrixCache = NULL;
    }
    return instance;
}

void* SkinGeometryConstructor(void* object, RwInt32 offset, RwInt32 size)
{
    RPSKINGEOMETRYGETDATA(object) = NULL;
    return object;
}

void* SkinGeometryDestructor(void* object, RwInt32 offset, RwInt32 size)
{
    RpSkin* skin = RPSKINGEOMETRYGETDATA(object);

    if (skin != NULL) {
        _rpSkinDeinitialize((RpGeometry*)object);
        RPSKINGEOMETRYGETDATA(object) = RpSkinDestroy(skin);
    }
    return object;
}

void* SkinGeometryCopy(void* dstObject, const void* srcObject, RwInt32 offset, RwInt32 size)
{
    return dstObject;
}

void* SkinAtomicConstructor(void* object, RwInt32 offset, RwInt32 size)
{
    RPSKINATOMICGETDATA(object) = NULL;
    return object;
}

void* SkinAtomicDestructor(void* object, RwInt32 offset, RwInt32 size)
{
    if (RPSKINATOMICGETDATA(object) != NULL) {
        RPSKINATOMICGETDATA(object) = NULL;
    }
    return object;
}

void* SkinAtomicCopy(void* dstObject, const void* srcObject, RwInt32 offset, RwInt32 size)
{
    RPSKINATOMICGETDATA(dstObject) = RPSKINATOMICGETDATA(srcObject);
    return dstObject;
}

static inline RpAtomic* SkinAtomicSetType(RpAtomic* atomic, RwInt32 type)
{
    RwInt32 pipeType;

    if (RwEngineGetPluginOffset(rwID_MATERIALEFFECTSPLUGIN) == -1 && type == rpSKINTYPEMATFX) {
        pipeType = rpSKINTYPEGENERIC;
    } else if (RwEngineGetPluginOffset(rwID_TOONPLUGIN) == -1 && type == rpSKINTYPETOON) {
        pipeType = rpSKINTYPEGENERIC;
    } else {
        pipeType = type;
    }
    return _rpSkinPipelinesAttach(atomic, pipeType);
}

RwBool SkinAtomicAlways(void* object, RwInt32 offset, RwInt32 size)
{
    RwInt32 type = rpSKINTYPEGENERIC;
    RpAtomic* atomic = (RpAtomic*)object;

    if (RwEngineGetPluginOffset(rwID_MATERIALEFFECTSPLUGIN) != -1 &&
        *((RwUInt8*)atomic + RpAtomicGetPluginOffset(rwID_MATERIALEFFECTSPLUGIN))) {
        type = rpSKINTYPEMATFX;
    }

    if (atomic->geometry != NULL && RpSkinGeometryGetSkin(atomic->geometry) != NULL) {
        SkinAtomicSetType(atomic, type);
    }
    return TRUE;
}

RwBool SkinAtomicRights(void* object, RwInt32 offset, RwInt32 size, RwUInt32 extraData)
{
    RpAtomic* atomic = (RpAtomic*)object;

    if (atomic->geometry != NULL && RpSkinGeometryGetSkin(atomic->geometry) != NULL) {
        SkinAtomicSetType(atomic, extraData);
    }
    return TRUE;
}

RwInt32 SkinGeometrySize(const void* object, RwInt32 offset, RwInt32 size)
{
    const RpGeometry* geometry = (const RpGeometry*)object;
    const RpSkin* skin = RPSKINGEOMETRYGETDATA(geometry);
    RwInt32 skinSize = 0;

    if (skin != NULL) {
        if (!(geometry->flags & rpGEOMETRYNATIVE)) {
            skinSize = sizeof(RwInt32);
            skinSize += geometry->numVertices * sizeof(RwUInt32);
            skinSize += geometry->numVertices * sizeof(RwMatrixWeights);
            skinSize += skin->numBones * sizeof(RwMatrix);
            skinSize += skin->numBones * sizeof(RwInt32);
        } else {
            skinSize = _rpSkinGeometryNativeSize(geometry);
        }
    }
    return skinSize;
}

RwStream* SkinGeometryWrite(RwStream* stream, RwInt32 length, const void* object, RwInt32 offset, RwInt32 size)
{
    const RpGeometry* geometry = (const RpGeometry*)object;
    RwUInt32 matrixOffset;
    RwInt32 numVertices;
    const RpSkin* skin = RPSKINGEOMETRYGETDATA(geometry);
    RwUInt32 i;

    if (skin != NULL) {
        if (!(geometry->flags & rpGEOMETRYNATIVE)) {
            numVertices = geometry->numVertices;

            if (!RwStreamWriteInt32(stream, (RwInt32*)&skin->numBones, sizeof(RwInt32))) {
                return NULL;
            }
            if (!RwStreamWriteInt32(stream, (RwInt32*)skin->vertexBoneIndices, numVertices * sizeof(RwUInt32))) {
                return NULL;
            }
            if (!RwStreamWriteReal(stream, (RwReal*)skin->vertexBoneWeights, numVertices * sizeof(RwMatrixWeights))) {
                return NULL;
            }

            for (i = 0, matrixOffset = 0; i < skin->numBones; matrixOffset += sizeof(RwMatrix), i++) {
                RwInt32 marker = rpSKINBONEMARKER;
                RwReal* matrix = (RwReal*)((RwUInt8*)skin->skinToBoneMatrices + matrixOffset);

                if (!RwStreamWriteInt32(stream, &marker, sizeof(RwInt32))) {
                    return NULL;
                }
                if (!RwStreamWriteReal(stream, matrix, sizeof(RwMatrix))) {
                    return NULL;
                }
            }
        } else if (!_rpSkinGeometryNativeWrite(stream, geometry)) {
            return NULL;
        }
    }
    return stream;
}

static inline RpSkin* SkinCreate(RwUInt32 numBones, RwInt32 numVertices)
{
    RpSkin* skin = (RpSkin*)RwFreeListAlloc(_rpSkinGlobals.freeList);
    RwUInt32 size;

    memset(skin, 0, sizeof(RpSkin));

    size = numVertices * (sizeof(RwUInt32) + sizeof(RwMatrixWeights)) + numBones * sizeof(RwMatrix) + 15;
    skin->unaligned = RwMalloc(size);
    memset(skin->unaligned, 0, size);

    skin->skinToBoneMatrices = (RwMatrix*)ALIGN16(skin->unaligned);
    skin->vertexBoneIndices = (RwUInt32*)(skin->skinToBoneMatrices + numBones);
    skin->vertexBoneWeights = (RwMatrixWeights*)(skin->vertexBoneIndices + numVertices);
    skin->numBones = numBones;
    return skin;
}

RwStream* SkinGeometryRead(RwStream* stream, RwInt32 length, void* object, RwInt32 offset, RwInt32 size)
{
    RpGeometry* geometry = (RpGeometry*)object;

    if (!(geometry->flags & rpGEOMETRYNATIVE)) {
        RwUInt32 numBones;
        RwInt32 numVertices;
        RpSkin* skin;
        RwUInt32 matrixOffset;
        RwUInt32 i;

        if (!RwStreamReadInt32(stream, (RwInt32*)&numBones, sizeof(RwInt32))) {
            return NULL;
        }

        numVertices = geometry->numVertices;
        skin = SkinCreate(numBones, numVertices);

        if (!RwStreamReadInt32(stream, (RwInt32*)skin->vertexBoneIndices, numVertices * sizeof(RwUInt32))) {
            return NULL;
        }
        if (!RwStreamReadReal(stream, (RwReal*)skin->vertexBoneWeights, numVertices * sizeof(RwMatrixWeights))) {
            return NULL;
        }

        for (i = 0, matrixOffset = 0; i < skin->numBones; matrixOffset += sizeof(RwMatrix), i++) {
            if (!RwStreamSkip(stream, sizeof(RwInt32))) {
                return NULL;
            }
            if (!RwStreamReadReal(stream, (RwReal*)((RwUInt8*)skin->skinToBoneMatrices + matrixOffset),
                                  sizeof(RwMatrix))) {
                return NULL;
            }
        }

        RpSkinGeometrySetSkin(geometry, skin);
    } else if (!_rpSkinGeometryNativeRead(stream, geometry)) {
        return NULL;
    }
    return stream;
}

RwStream* SkinAtomicRead(RwStream* stream, RwInt32 length, void* object, RwInt32 offset, RwInt32 size)
{
    RpGeometry* geometry = ((RpAtomic*)object)->geometry;

    if (RpSkinGeometryGetSkin(geometry) == NULL) {
        RwUInt32 numBones;
        RwInt32 numVertices;
        RpSkin* skin;
        RwUInt32 matrixOffset;
        RwUInt32 i;

        if (!RwStreamReadInt32(stream, (RwInt32*)&numBones, sizeof(RwInt32))) {
            return NULL;
        }

        numVertices = geometry->numVertices;
        skin = SkinCreate(numBones, numVertices);

        if (!RwStreamSkip(stream, sizeof(RwInt32))) {
            return NULL;
        }
        if (!RwStreamReadInt32(stream, (RwInt32*)skin->vertexBoneIndices, numVertices * sizeof(RwUInt32))) {
            return NULL;
        }
        if (!RwStreamReadReal(stream, (RwReal*)skin->vertexBoneWeights, numVertices * sizeof(RwMatrixWeights))) {
            return NULL;
        }

        for (i = 0, matrixOffset = 0; i < skin->numBones; matrixOffset += sizeof(RwMatrix), i++) {
            if (!RwStreamSkip(stream, 3 * sizeof(RwInt32))) {
                return NULL;
            }
            if (!RwStreamReadReal(stream, (RwReal*)((RwUInt8*)skin->skinToBoneMatrices + matrixOffset),
                                  sizeof(RwMatrix))) {
                return NULL;
            }
        }

        RpSkinGeometrySetSkin(geometry, skin);
    } else if (!RwStreamSkip(stream, size)) {
        return NULL;
    }
    return stream;
}

RwStream* SkinAtomicWrite(RwStream* stream, RwInt32 length, const void* object, RwInt32 offset, RwInt32 size)
{
    return stream;
}

RwInt32 SkinAtomicGetSize(const void* object, RwInt32 offset, RwInt32 size)
{
    return 0;
}

RwBool RpSkinPluginAttach(void)
{
    _rpSkinGlobals.engineOffset = RwEngineRegisterPlugin(0, rwID_SKINPLUGIN, SkinOpen, SkinClose);

    _rpSkinGlobals.atomicOffset = RpAtomicRegisterPlugin(sizeof(RpHAnimHierarchy*), rwID_SKINPLUGIN,
                                                         SkinAtomicConstructor, SkinAtomicDestructor, SkinAtomicCopy);
    RpAtomicRegisterPluginStream(rwID_SKINPLUGIN, SkinAtomicRead, SkinAtomicWrite, SkinAtomicGetSize);
    RpAtomicSetStreamAlwaysCallBack(rwID_SKINPLUGIN, SkinAtomicAlways);
    RpAtomicSetStreamRightsCallBack(rwID_SKINPLUGIN, SkinAtomicRights);

    _rpSkinGlobals.geometryOffset = RpGeometryRegisterPlugin(sizeof(RpSkin*), rwID_SKINPLUGIN,
                                                             SkinGeometryConstructor, SkinGeometryDestructor,
                                                             SkinGeometryCopy);
    RpGeometryRegisterPluginStream(rwID_SKINPLUGIN, SkinGeometryRead, SkinGeometryWrite, SkinGeometrySize);

    return TRUE;
}

RpAtomic* RpSkinAtomicSetHAnimHierarchy(RpAtomic* atomic, RpHAnimHierarchy* hierarchy)
{
    RPSKINATOMICGETDATA(atomic) = hierarchy;
    return atomic;
}

RpSkin* RpSkinGeometryGetSkin(RpGeometry* geometry)
{
    return RPSKINGEOMETRYGETDATA(geometry);
}

RpGeometry* RpSkinGeometrySetSkin(RpGeometry* geometry, RpSkin* skin)
{
    if (skin != RPSKINGEOMETRYGETDATA(geometry)) {
        if (RPSKINGEOMETRYGETDATA(geometry) != NULL) {
            _rpSkinDeinitialize(geometry);
        }

        RPSKINGEOMETRYGETDATA(geometry) = skin;

        if (skin != NULL) {
            _rpSkinInitialize(geometry);
        }
    }
    return geometry;
}

RpSkin* RpSkinDestroy(RpSkin* skin)
{
    if (skin->unaligned != NULL) {
        RwFree(skin->unaligned);
    }

    skin->unaligned = NULL;
    skin->skinToBoneMatrices = NULL;
    skin->vertexBoneIndices = NULL;
    skin->vertexBoneWeights = NULL;
    skin->numBones = 0;

    RwFreeListFree(_rpSkinGlobals.freeList, skin);
    return NULL;
}

RwUInt32 RpSkinGetNumBones(RpSkin* skin)
{
    return skin->numBones;
}

const RwMatrixWeights* RpSkinGetVertexBoneWeights(RpSkin* skin)
{
    return skin->vertexBoneWeights;
}

const RwUInt32* RpSkinGetVertexBoneIndices(RpSkin* skin)
{
    return skin->vertexBoneIndices;
}

const RwMatrix* RpSkinGetSkinToBoneMatrices(RpSkin* skin)
{
    return skin->skinToBoneMatrices;
}

RpAtomic* RpSkinAtomicSetType(RpAtomic* atomic, RwInt32 type)
{
    return SkinAtomicSetType(atomic, type);
}
