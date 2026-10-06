/*
 * Collision plugin: registers per-world-sector and per-geometry collision data
 * (a BSP tree over the triangles plus a triangle remap table) and streams it.
 */

#include "rwsdk/rwcore.h"
#include "rwsdk/rpworld.h"
#include "rwsdk/rpcollis_int.h"

#define rwPLUGIN_ID 0x11D /* rwID_COLLISPLUGIN */

#define rpCOLLISIONDATAFLAG_SINGLEBLOCK 0x1

// Declared in reverse: .sbss holds them in the opposite order.
RwInt32 _rpCollisionWorldSectorDataOffset;
RwInt32 _rpCollisionGeometryDataOffset;
RwInt32 _rpCollisionAtomicDataOffset;
RwInt32 _rpCollisionGlobalsOffset;
RwInt32 _rpCollisionNumInstances;

#define COLLISIONGLOBAL(var) (*(void**)((RwUInt8*)RwEngineInstance + _rpCollisionGlobalsOffset))
#define COLLISIONDATAFROMOBJECT(object, offset) (*(RpCollisionData**)((RwUInt8*)(object) + (offset)))

static void* CollisionOpen(void* instance, RwInt32 offset, RwInt32 size) {
    _rpCollisionNumInstances++;
    COLLISIONGLOBAL(unused) = NULL;
    return instance;
}

static void* CollisionClose(void* instance, RwInt32 offset, RwInt32 size) {
    _rpCollisionNumInstances--;
    return instance;
}

static inline void _rpCollisionDataDestroy(RpCollisionData* data) {
    if (data) {
        if (!(data->flags & rpCOLLISIONDATAFLAG_SINGLEBLOCK)) {
            _rpCollBSPTreeDestroy(data->tree);
        }
        RwFree(data);
    }
}

static void* CollisionDataDestroy(void* object, RwInt32 offset, RwInt32 size) {
    RpCollisionData** extData = &COLLISIONDATAFROMOBJECT(object, offset);

    if (*extData) {
        _rpCollisionDataDestroy(*extData);
        *extData = NULL;
    }

    return object;
}

static void* CollisionDataCreate(void* object, RwInt32 offset, RwInt32 size) {
    COLLISIONDATAFROMOBJECT(object, offset) = NULL;
    return object;
}

static void* CollisionAtomicInit(void* object, RwInt32 offset, RwInt32 size) {
    COLLISIONDATAFROMOBJECT(object, _rpCollisionAtomicDataOffset) = NULL;
    return object;
}

static RwStream* CollisionDataStreamWrite(RwStream* stream, RwInt32 binaryLength, const void* object, RwInt32 offset,
                                          RwInt32 size) {
    RwInt32 i;
    RwUInt16* triIndex;
    RwInt32 numTri;
    RpCollisionData* data = COLLISIONDATAFROMOBJECT(object, offset);

    if (!data) {
        return stream;
    }

    if (!RwStreamWriteInt32(stream, (RwInt32*)&data->tree->numLeafNodes, sizeof(RwInt32)) ||
        !RwStreamWriteInt32(stream, &data->numTriangles, sizeof(RwInt32))) {
        return NULL;
    }

    if (!_rpCollBSPTreeStreamWrite(data->tree, stream)) {
        return NULL;
    }

    numTri = data->numTriangles;
    triIndex = data->triangleMap;
    while (numTri--) {
        i = *triIndex++;
        if (!RwStreamWriteInt32(stream, &i, sizeof(RwInt32))) {
            return NULL;
        }
    }

    return stream;
}

static RwStream* CollisionDataStreamRead(RwStream* stream, RwInt32 binaryLength, void* object, RwInt32 offset,
                                         RwInt32 size) {
    if (binaryLength) {
        RpCollisionData* data;
        RwInt32 numLeaves;
        RwInt32 numTris;
        RwInt32 treeSize;
        RwUInt16* triIndex;
        RwInt32 numTri;
        RwInt32 i;

        if (!RwStreamReadInt32(stream, &numLeaves, sizeof(RwInt32)) ||
            !RwStreamReadInt32(stream, &numTris, sizeof(RwInt32))) {
            return NULL;
        }

        treeSize = _rpCollBSPTreeMemGetSize(numLeaves);
        size = sizeof(RpCollisionData) + treeSize + numTris * sizeof(RwUInt16);
        data = (RpCollisionData*)RwMalloc(size);
        if (!data) {
            RWERROR((E_RW_NOMEM, size));
            return NULL;
        }

        data->flags = rpCOLLISIONDATAFLAG_SINGLEBLOCK;
        data->tree = (RpCollBSPTree*)(data + 1);
        data->numTriangles = numTris;
        data->triangleMap = (RwUInt16*)((RwUInt8*)data->tree + treeSize);

        _rpCollBSPTreeInit(data->tree, numLeaves);
        if (!_rpCollBSPTreeStreamRead(data->tree, stream)) {
            RwFree(data);
            return NULL;
        }

        numTri = data->numTriangles;
        triIndex = data->triangleMap;
        while (numTri--) {
            if (!RwStreamReadInt32(stream, &i, sizeof(RwInt32))) {
                RwFree(data);
                return NULL;
            }
            *triIndex++ = (RwUInt16)i;
        }

        COLLISIONDATAFROMOBJECT(object, offset) = data;
    }

    return stream;
}

static RwInt32 CollisionDataStreamGetSize(const void* object, RwInt32 offset, RwInt32 size) {
    RwInt32 binarySize;
    RpCollisionData* data = COLLISIONDATAFROMOBJECT(object, offset);

    if (data) {
        binarySize = _rpCollBSPTreeStreamGetSize(data->tree) + sizeof(RwInt32) * 2;
        binarySize += data->numTriangles * sizeof(RwInt32);
        return binarySize;
    }

    return 0;
}

static inline RwBool CollisionWorldAttach(void) {
    RwBool result;

    _rpCollisionGlobalsOffset = RwEngineRegisterPlugin(sizeof(void*), rwPLUGIN_ID, CollisionOpen, CollisionClose);
    result = (_rpCollisionGlobalsOffset >= 0);

    if (result) {
        _rpCollisionWorldSectorDataOffset = RpWorldSectorRegisterPlugin(
            sizeof(RpCollisionData*), rwPLUGIN_ID, CollisionDataCreate, CollisionDataDestroy, NULL);
        result = (_rpCollisionWorldSectorDataOffset >= 0);

        if (result) {
            result = (RpWorldSectorRegisterPluginStream(rwPLUGIN_ID, CollisionDataStreamRead,
                                                        CollisionDataStreamWrite, CollisionDataStreamGetSize) >= 0);
        }
    }

    return result;
}

static inline RwBool CollisionGeometryAttach(void) {
    RwBool result;

    _rpCollisionAtomicDataOffset =
        RpAtomicRegisterPlugin(sizeof(RpCollisionData*), rwPLUGIN_ID, CollisionAtomicInit, NULL, NULL);
    result = (_rpCollisionAtomicDataOffset >= 0);

    if (result) {
        _rpCollisionGeometryDataOffset = RpGeometryRegisterPlugin(
            sizeof(RpCollisionData*), rwPLUGIN_ID, CollisionDataCreate, CollisionDataDestroy, NULL);
        result = (_rpCollisionGeometryDataOffset >= 0);

        if (result) {
            result = (RpGeometryRegisterPluginStream(rwPLUGIN_ID, CollisionDataStreamRead,
                                                     CollisionDataStreamWrite, CollisionDataStreamGetSize) >= 0);
        }
    }

    return result;
}

RwBool RpCollisionPluginAttach(void) {
    RwBool result = CollisionWorldAttach();

    if (result) {
        result = CollisionGeometryAttach();
    }

    return result;
}
