/*
 * Meshes: mesh headers (one mesh per material), the build meshes used while
 * sorting triangles, and the mesh chunk read and write.
 */

#include "rwsdk/rwcore.h"

#define rwPLUGIN_ID 2 /* world */

#define rpWORLD 7
#define rpGEOMETRY 8
#define rpGEOMETRYNATIVE 0x01000000

typedef RwUInt16 RxVertexIndex;
typedef struct RpMaterial RpMaterial;
typedef struct RpMaterialList RpMaterialList;

typedef struct RpMeshHeader {
    RwUInt32 flags;
    RwUInt16 numMeshes;
    RwUInt16 serialNum;
    RwUInt32 totalIndicesInMesh;
    RwUInt32 firstMeshOffset;
} RpMeshHeader;

typedef struct RpMesh {
    RxVertexIndex* indices;
    RwUInt32 numIndices;
    RpMaterial* material;
} RpMesh;

typedef struct RpBuildMeshTriangle {
    RwUInt16 vertIndex[3];
    RpMaterial* material;
} RpBuildMeshTriangle;

typedef struct RpBuildMesh {
    RwUInt32 triangleBufferSize;
    RwUInt32 numTriangles;
    RpBuildMeshTriangle* meshTriangles;
} RpBuildMesh;

typedef RpMesh* (*RpMeshCallBack)(RpMesh* mesh, RpMeshHeader* meshHeader, void* pData);

typedef struct rpMeshGlobals {
    RwInt16 nextSerialNum;
    RwInt16 pad;
    RwUInt32 unused;
    RwUInt8 meshFlagsToPrimType[32];
    RwUInt8 primTypeToMeshFlags[8];
} rpMeshGlobals;

/* Geometry and world flags both sit at offset 8; native data has no indices. */
#define rpMeshObjectFlags(object) (((const RwUInt32*)(object))[2])
#define rpMeshObjectHasIndices(object)                                                                     \
    ((((const RwObject*)(object))->type == rpGEOMETRY && !(rpMeshObjectFlags(object) & rpGEOMETRYNATIVE)) || \
     (((const RwObject*)(object))->type == rpWORLD && !(rpMeshObjectFlags(object) & rpGEOMETRYNATIVE)))

#pragma explicit_zero_data on
static RwFreeList* RpBuildMeshFreeList = NULL;
#pragma explicit_zero_data reset

RwModuleInfo meshModule;

#define RWMESHGLOBAL(var) (RWPLUGINOFFSET(rpMeshGlobals, RwEngineInstance, meshModule.globalsOffset)->var)
#define RwRealloc(_p, _s) ((RWSRCGLOBAL(memoryFuncs).rwrealloc)((_p), (_s)))

RwStream* RwStreamReadInt32(RwStream* stream, RwInt32* ints, RwUInt32 numBytes);
RwStream* RwStreamWriteInt32(RwStream* stream, const RwInt32* ints, RwUInt32 numBytes);
RpMaterial* _rpMaterialListGetMaterial(const RpMaterialList* matList, RwInt32 matIndex);
RwInt32 _rpMaterialListFindMaterialIndex(const RpMaterialList* matList, const RpMaterial* material);

RpMeshHeader* _rpMeshHeaderCreate(RwUInt32 size) {
    return (RpMeshHeader*)RwMalloc(size);
}

void* _rpMeshClose(void* instance, RwInt32 offset, RwInt32 size) {
    if (--meshModule.numInstances == 0) {
        if (RpBuildMeshFreeList) {
            RwFreeListDestroy(RpBuildMeshFreeList);
            RpBuildMeshFreeList = NULL;
        }
    }

    return instance;
}

void* _rpMeshOpen(void* instance, RwInt32 offset, RwInt32 size) {
    meshModule.globalsOffset = offset;

    if (meshModule.numInstances == 0) {
        RpBuildMeshFreeList = RwFreeListCreate(sizeof(RpBuildMesh), 50, 0);
        if (!RpBuildMeshFreeList) {
            if (RpBuildMeshFreeList) {
                RwFreeListDestroy(RpBuildMeshFreeList);
                RpBuildMeshFreeList = NULL;
            }

            return NULL;
        }
    }

    RWMESHGLOBAL(nextSerialNum) = 1;
    meshModule.numInstances++;

    RWMESHGLOBAL(meshFlagsToPrimType)[0] = 3;
    RWMESHGLOBAL(meshFlagsToPrimType)[1] = 4;
    RWMESHGLOBAL(meshFlagsToPrimType)[2] = 5;
    RWMESHGLOBAL(meshFlagsToPrimType)[4] = 1;
    RWMESHGLOBAL(meshFlagsToPrimType)[8] = 2;
    RWMESHGLOBAL(meshFlagsToPrimType)[16] = 6;

    RWMESHGLOBAL(primTypeToMeshFlags)[0] = 4;
    RWMESHGLOBAL(primTypeToMeshFlags)[1] = 8;
    RWMESHGLOBAL(primTypeToMeshFlags)[2] = 0;
    RWMESHGLOBAL(primTypeToMeshFlags)[3] = 1;
    RWMESHGLOBAL(primTypeToMeshFlags)[4] = 2;
    RWMESHGLOBAL(primTypeToMeshFlags)[5] = 16;

    return instance;
}

RpBuildMesh* _rpBuildMeshCreate(RwUInt32 bufferSize) {
    RpBuildMesh* mesh = (RpBuildMesh*)RwFreeListAlloc(RpBuildMeshFreeList);

    if (mesh) {
        mesh->numTriangles = 0;

        if (bufferSize) {
            RwUInt32 size = bufferSize * sizeof(RpBuildMeshTriangle);

            mesh->meshTriangles = (RpBuildMeshTriangle*)RwMalloc(size);
            if (!mesh->meshTriangles) {
                RwFreeListFree(RpBuildMeshFreeList, mesh);
                RWERROR((E_RW_NOMEM, size));
                return NULL;
            }

            mesh->triangleBufferSize = bufferSize;
        } else {
            mesh->meshTriangles = NULL;
            mesh->triangleBufferSize = 0;
        }

        return mesh;
    }

    RWERROR((E_RW_NOMEM, sizeof(RpBuildMesh)));
    return NULL;
}

RwBool _rpBuildMeshDestroy(RpBuildMesh* mesh) {
    if (mesh->meshTriangles) {
        RwFree(mesh->meshTriangles);
        mesh->meshTriangles = NULL;
    }

    RwFreeListFree(RpBuildMeshFreeList, mesh);

    return TRUE;
}

RwBool _rpMeshDestroy(RpMeshHeader* meshHeader) {
    if (meshHeader->flags || meshHeader->numMeshes || meshHeader->serialNum || meshHeader->totalIndicesInMesh ||
        meshHeader->firstMeshOffset) {
        RwFree(meshHeader);
    }

    return TRUE;
}

RpBuildMesh* _rpBuildMeshAddTriangle(RpBuildMesh* mesh, RpMaterial* material, RwInt32 vert1, RwInt32 vert2,
                                     RwInt32 vert3) {
    if (mesh->numTriangles >= mesh->triangleBufferSize) {
        RwUInt32 size = (mesh->numTriangles + 1) * sizeof(RpBuildMeshTriangle);
        RpBuildMeshTriangle* triangles;

        if (mesh->numTriangles) {
            triangles = (RpBuildMeshTriangle*)RwRealloc(mesh->meshTriangles, size);
        } else {
            triangles = (RpBuildMeshTriangle*)RwMalloc(size);
        }

        if (!triangles) {
            RWERROR((E_RW_NOMEM, size));
            return NULL;
        }

        mesh->meshTriangles = triangles;
        mesh->triangleBufferSize = mesh->numTriangles + 1;
    }

    mesh->meshTriangles[mesh->numTriangles].material = material;
    mesh->meshTriangles[mesh->numTriangles].vertIndex[0] = (RwUInt16)vert1;
    mesh->meshTriangles[mesh->numTriangles].vertIndex[1] = (RwUInt16)vert2;
    mesh->meshTriangles[mesh->numTriangles].vertIndex[2] = (RwUInt16)vert3;
    mesh->numTriangles++;

    return mesh;
}

RpMeshHeader* _rpMeshHeaderForAllMeshes(RpMeshHeader* meshHeader, RpMeshCallBack fpCallBack, void* pData) {
    RwInt32 numMeshes = meshHeader->numMeshes;
    RpMesh* mesh = (RpMesh*)((RwUInt8*)(meshHeader + 1) + meshHeader->firstMeshOffset);

    while (numMeshes--) {
        if (!fpCallBack(mesh, meshHeader, pData)) {
            return meshHeader;
        }

        mesh++;
    }

    return meshHeader;
}

RwStream* _rpMeshWrite(const RpMeshHeader* meshHeader, const void* object, RwStream* stream,
                       const RpMaterialList* matList) {
    RwInt32 header[3];
    RwUInt32 count;
    RwUInt32 numMeshes;
    const RpMesh* mesh;
    RwUInt32 remaining;
    const RxVertexIndex* indices;

    header[0] = meshHeader->flags;
    header[1] = meshHeader->numMeshes;
    header[2] = meshHeader->totalIndicesInMesh;

    if (!RwStreamWriteInt32(stream, header, sizeof(header))) {
        return NULL;
    }

    numMeshes = meshHeader->numMeshes;
    mesh = (const RpMesh*)(meshHeader + 1);

    while (numMeshes--) {
        RwInt32 meshInfo[2];

        meshInfo[0] = mesh->numIndices;
        meshInfo[1] = _rpMaterialListFindMaterialIndex(matList, mesh->material);
        if (meshInfo[1] < 0) {
            meshInfo[1] = 0;
        }

        if (!RwStreamWriteInt32(stream, meshInfo, sizeof(meshInfo))) {
            return NULL;
        }

        if (rpMeshObjectHasIndices(object)) {
            remaining = mesh->numIndices;
            indices = mesh->indices;

            while (remaining) {
                RwInt32 buffer[256];
                RwUInt32 i;

                count = 256;

                if (remaining < 256) {
                    count = remaining;
                }

                for (i = 0; i < count; i++) {
                    buffer[i] = *indices++;
                }

                if (!RwStreamWriteInt32(stream, buffer, count * sizeof(RwInt32))) {
                    return NULL;
                }

                remaining -= count;
            }
        }

        mesh++;
    }

    return stream;
}

RpMeshHeader* _rpMeshRead(RwStream* stream, const void* object, const RpMaterialList* matList) {
    RpMeshHeader* meshHeader;
    RpMesh* mesh;
    RxVertexIndex* indices;
    RwUInt32 count;
    RwUInt32 numMeshes;
    RwUInt32 remaining;
    RwInt32* src;
    RwInt32 header[3];
    RwUInt32 size;

    if (!RwStreamReadInt32(stream, header, sizeof(header))) {
        return NULL;
    }

    size = header[1] * 16 + sizeof(RpMeshHeader);
    if (rpMeshObjectHasIndices(object)) {
        size += header[2] * sizeof(RxVertexIndex);
    }

    meshHeader = _rpMeshHeaderCreate(size);
    if (meshHeader) {
        mesh = (RpMesh*)(meshHeader + 1);
        indices = (RxVertexIndex*)(mesh + header[1]);

        meshHeader->flags = header[0];
        meshHeader->numMeshes = (RwUInt16)header[1];
        meshHeader->serialNum = RWMESHGLOBAL(nextSerialNum);
        meshHeader->totalIndicesInMesh = header[2];
        meshHeader->firstMeshOffset = 0;
        RWMESHGLOBAL(nextSerialNum)++;

        numMeshes = meshHeader->numMeshes;
        while (numMeshes--) {
            RwInt32 meshInfo[2];

            if (!RwStreamReadInt32(stream, meshInfo, sizeof(meshInfo))) {
                return NULL;
            }

            mesh->numIndices = meshInfo[0];
            mesh->material = _rpMaterialListGetMaterial(matList, meshInfo[1]);
            mesh->indices = indices;

            if (rpMeshObjectHasIndices(object)) {
                remaining = mesh->numIndices;

                while (remaining) {
                    RwInt32 buffer[256];

                    src = buffer;
                    count = (remaining < 256) ? remaining : 256;

                    if (!RwStreamReadInt32(stream, src, count * sizeof(RwInt32))) {
                        return NULL;
                    }

                    remaining -= count;
                    while (count--) {
                        *indices++ = (RxVertexIndex)*src++;
                    }
                }
            }

            mesh++;
        }
    }

    return meshHeader;
}

RwInt32 _rpMeshSize(const RpMeshHeader* meshHeader, const void* object) {
    if ((((const RwObject*)object)->type == rpGEOMETRY && (rpMeshObjectFlags(object) & rpGEOMETRYNATIVE)) ||
        (((const RwObject*)object)->type == rpWORLD && (rpMeshObjectFlags(object) & rpGEOMETRYNATIVE))) {
        return meshHeader->numMeshes * 8 + 12;
    }

    return 12 + meshHeader->numMeshes * 8 + meshHeader->totalIndicesInMesh * 4;
}
