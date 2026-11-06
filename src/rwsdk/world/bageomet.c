/*
 * Geometry: creation and destruction, morph target allocation, locking,
 * mesh building on unlock and stream reading.
 */

#include "rwsdk/rpworld_int.h"

#define rwPLUGIN_ID 2 /* world */

#define rpGEOMETRY 8

#define E_RW_BADVERSION 0x80000004
#define E_RP_WORLD_TOOMANYVERTICES 6

#define rwID_MATLIST 0x08

#define rpGEOMETRYTRISTRIP 0x00000001
#define rpGEOMETRYTEXTURED 0x00000004
#define rpGEOMETRYPRELIT 0x00000008
#define rpGEOMETRYNORMALS 0x00000010
#define rpGEOMETRYTEXTURED2 0x00000080
#define rpGEOMETRYNATIVE 0x01000000
#define rpGEOMETRYNATIVEFLAGSMASK 0x0F000000

#define rpGEOMETRYLOCKPOLYGONS 0x0001
#define rpGEOMETRYLOCKALL 0x0FFF

typedef struct _rpGeometry {
    /* 0x00 */ RwUInt32 format;
    /* 0x04 */ RwInt32 numTriangles;
    /* 0x08 */ RwInt32 numVertices;
    /* 0x0C */ RwInt32 numMorphTargets;
    /* 0x10 */ RwSurfaceProperties surfaceProps;
} _rpGeometry;

typedef struct _rpMorphTarget {
    RwSphere boundingSphere;
    RwBool pointsPresent;
    RwBool normalsPresent;
} _rpMorphTarget;

typedef RpMaterial* (*RpMaterialCallBack)(RpMaterial* material, void* data);

RwPluginRegistry geometryTKList = { sizeof(RpGeometry), sizeof(RpGeometry), 0, 0, NULL, NULL };

RwModuleInfo geometryModule;

#define RwRealloc(_p, _s) ((RWSRCGLOBAL(memoryFuncs).rwrealloc)((_p), (_s)))

RwUInt32 RwStreamRead(RwStream* stream, void* buffer, RwUInt32 length);
void* RwMemNative32(void* mem, RwUInt32 size);
void* RwMemFloat32ToReal(void* mem, RwUInt32 size);
void* memset(void* dst, int val, unsigned long n);
const RwPluginRegistry* _rwPluginRegistryReadDataChunks(const RwPluginRegistry* reg, RwStream* stream,
                                                        void* object);
RwInt32 _rwPluginRegistryAddPluginStream(RwPluginRegistry* reg, RwUInt32 pluginID,
                                         RwPluginDataChunkReadCallBack readCB,
                                         RwPluginDataChunkWriteCallBack writeCB,
                                         RwPluginDataChunkGetSizeCallBack getSizeCB);
RwBool RwResourcesFreeResEntry(RwResEntry* entry);
RpMaterialList* _rpMaterialListInitialize(RpMaterialList* matList);
RpMaterialList* _rpMaterialListDeinitialize(RpMaterialList* matList);
RpMaterialList* _rpMaterialListStreamRead(RwStream* stream, RpMaterialList* matList);
RpMaterial* _rpMaterialListGetMaterial(const RpMaterialList* matList, RwInt32 matIndex);
void RpMaterialSetDefaultSurfaceProperties(const RwSurfaceProperties* surfaceProps);
RpBuildMesh* _rpBuildMeshCreate(RwUInt32 bufferSize);
RwBool _rpBuildMeshDestroy(RpBuildMesh* mesh);
RpBuildMesh* _rpBuildMeshAddTriangle(RpBuildMesh* mesh, RpMaterial* material, RwInt32 vert1, RwInt32 vert2,
                                     RwInt32 vert3);
RpMeshHeader* _rpMeshOptimise(RpBuildMesh* buildMesh, RwUInt32 flags);
RwBool _rpMeshDestroy(RpMeshHeader* meshHeader);

void* _rpGeometryOpen(void* instance, RwInt32 offset, RwInt32 size) {
    geometryModule.globalsOffset = offset;
    geometryModule.numInstances++;
    return instance;
}

void* _rpGeometryClose(void* instance, RwInt32 offset, RwInt32 size) {
    geometryModule.numInstances--;
    return instance;
}

RwInt32 RpGeometryAddMorphTargets(RpGeometry* geometry, RwInt32 mtcount) {
    RpMorphTarget* morphTarget;
    RwUInt32 mtSize;
    RwUInt32 memSize;
    RwInt32 i;

    if (geometry->flags & rpGEOMETRYNATIVE) {
        mtSize = sizeof(RpMorphTarget);
    } else {
        mtSize = sizeof(RpMorphTarget) + geometry->numVertices * sizeof(RwV3d);
        if (geometry->flags & rpGEOMETRYNORMALS) {
            mtSize += geometry->numVertices * sizeof(RwV3d);
        }
    }

    memSize = mtSize * (geometry->numMorphTargets + mtcount);

    if (geometry->morphTarget) {
        morphTarget = (RpMorphTarget*)RwRealloc(geometry->morphTarget, memSize);
        if (!morphTarget) {
            RWERROR((E_RW_NOMEM, memSize));
            return -1;
        } else {
            RwUInt8* dst;
            RwUInt8* src;
            RwUInt32 count;

            src = (RwUInt8*)morphTarget - 1 + mtSize * geometry->numMorphTargets;
            dst = src + mtcount * sizeof(RpMorphTarget);
            count = mtSize * geometry->numMorphTargets - geometry->numMorphTargets * sizeof(RpMorphTarget);

            while (count--) {
                *dst-- = *src--;
            }
        }
    } else {
        morphTarget = (RpMorphTarget*)RwMalloc(memSize);
        if (!morphTarget) {
            RWERROR((E_RW_NOMEM, memSize));
            return -1;
        }
    }

    geometry->numMorphTargets += mtcount;
    geometry->morphTarget = morphTarget;

    {
        RwV3d* vertData = (RwV3d*)(morphTarget + geometry->numMorphTargets);

        for (i = 0; i < geometry->numMorphTargets; i++) {
            RpMorphTarget* mt = &geometry->morphTarget[i];

            mt->verts = NULL;
            mt->normals = NULL;

            if (!(geometry->flags & rpGEOMETRYNATIVE) && geometry->numVertices) {
                mt->verts = vertData;
                vertData += geometry->numVertices;

                if (geometry->flags & rpGEOMETRYNORMALS) {
                    mt->normals = vertData;
                    vertData += geometry->numVertices;
                }
            }
        }
    }

    for (i = geometry->numMorphTargets - mtcount; i < geometry->numMorphTargets; i++) {
        RpMorphTarget* mt = &geometry->morphTarget[i];

        mt->boundingSphere.center.x = 0.0f;
        mt->boundingSphere.center.y = 0.0f;
        mt->boundingSphere.center.z = 0.0f;
        mt->boundingSphere.radius = 0.0f;
        mt->parentGeom = geometry;
    }

    return geometry->numMorphTargets - mtcount;
}

RpGeometry* RpGeometryForAllMaterials(RpGeometry* geometry, RpMaterialCallBack fpCallBack, void* data) {
    RwInt32 numMaterials = geometry->matList.numMaterials;
    RwInt32 i;

    for (i = 0; i < numMaterials; i++) {
        if (!fpCallBack(geometry->matList.materials[i], data)) {
            return geometry;
        }
    }

    return geometry;
}

RpGeometry* RpGeometryLock(RpGeometry* geometry, RwInt32 lockMode) {
    geometry->lockedSinceLastInst |= lockMode;

    if (lockMode & rpGEOMETRYLOCKPOLYGONS) {
        if (geometry->mesh) {
            _rpMeshDestroy(geometry->mesh);
            geometry->mesh = NULL;
        }
    }

    return geometry;
}

RpGeometry* RpGeometryUnlock(RpGeometry* geometry) {
    if (!geometry->mesh) {
        RpBuildMesh* buildMesh = _rpBuildMeshCreate(geometry->numTriangles);
        RpMeshHeader* mesh;
        RwInt32 i;

        if (buildMesh) {
            for (i = 0; i < geometry->numTriangles; i++) {
                RpTriangle* triangle = &geometry->triangles[i];
                RpMaterial* material;

                if (triangle->matIndex != -1) {
                    material = _rpMaterialListGetMaterial(&geometry->matList, triangle->matIndex);
                } else {
                    material = NULL;
                }

                _rpBuildMeshAddTriangle(buildMesh, material, triangle->vertIndex[0], triangle->vertIndex[1],
                                        triangle->vertIndex[2]);
            }

            if (geometry->flags & rpGEOMETRYTRISTRIP) {
                mesh = _rpMeshOptimise(buildMesh, 1);
            } else {
                mesh = _rpMeshOptimise(buildMesh, 0);
            }

            if (mesh) {
                geometry->mesh = mesh;
                return geometry;
            }

            _rpBuildMeshDestroy(buildMesh);
            return NULL;
        }

        return NULL;
    }

    return geometry;
}

RpGeometry* RpGeometryCreate(RwInt32 numVerts, RwInt32 numTriangles, RwUInt32 format) {
    RwUInt32 numTexCoordSets;
    RwUInt32 native;
    RwUInt32 flags;
    RpGeometry* geometry;
    RwUInt32 formatFlags;
    RwUInt32 size;

    if (!(numVerts >= 0 && numVerts < 65536 && numTriangles >= 0)) {
        if (numVerts >= 0 && !(numVerts < 65536)) {
            RWERROR((E_RP_WORLD_TOOMANYVERTICES));
        }

        return NULL;
    }

    size = geometryTKList.sizeOfStruct;
    formatFlags = format & 0xFF;

    if (format & 0x00FF0000) {
        numTexCoordSets = (format & 0x00FF0000) >> 16;
    } else if (format & rpGEOMETRYTEXTURED2) {
        numTexCoordSets = 2;
    } else {
        numTexCoordSets = (format & rpGEOMETRYTEXTURED) ? 1 : 0;
    }

    flags = (formatFlags & ~(rpGEOMETRYTEXTURED | rpGEOMETRYTEXTURED2)) |
            ((numTexCoordSets == 1) ? rpGEOMETRYTEXTURED : ((numTexCoordSets > 1) ? rpGEOMETRYTEXTURED2 : 0));

    native = format & rpGEOMETRYNATIVE;

    if (!native) {
        if (flags & rpGEOMETRYPRELIT) {
            size += numVerts * sizeof(RwRGBA);
        }

        if (numTexCoordSets) {
            size += numTexCoordSets * (numVerts * sizeof(RwTexCoords));
        }

        size += numTriangles * sizeof(RpTriangle);
    }

    geometry = (RpGeometry*)RwMalloc(size);
    if (!geometry) {
        return NULL;
    }

    if (!_rpMaterialListInitialize(&geometry->matList)) {
        return NULL;
    }

    geometry->morphTarget = NULL;
    geometry->numMorphTargets = 0;
    rwObjectInitialize(geometry, rpGEOMETRY, 0);
    geometry->repEntry = NULL;
    geometry->lockedSinceLastInst = 0;
    geometry->refCount = 1;
    geometry->mesh = NULL;
    geometry->numTexCoordSets = numTexCoordSets;
    memset(geometry->texCoords, 0, sizeof(geometry->texCoords));
    geometry->preLitLum = NULL;
    geometry->triangles = NULL;
    geometry->numTriangles = numTriangles;
    geometry->flags = flags | (format & rpGEOMETRYNATIVEFLAGSMASK);
    geometry->numVertices = numVerts;

    if (!native) {
        RwUInt8* mem = (RwUInt8*)geometry + geometryTKList.sizeOfStruct;

        if ((flags & rpGEOMETRYPRELIT) && numVerts) {
            geometry->preLitLum = (RwRGBA*)mem;
            mem += numVerts * sizeof(RwRGBA);
        }

        if (numTexCoordSets && numVerts) {
            RwUInt32 i;

            for (i = 0; i < numTexCoordSets; i++) {
                geometry->texCoords[i] = (RwTexCoords*)mem;
                mem += numVerts * sizeof(RwTexCoords);
            }
        }

        if (numTriangles) {
            RwInt32 i;

            geometry->triangles = (RpTriangle*)mem;

            for (i = 0; i < numTriangles; i++) {
                geometry->triangles[i].matIndex = -1;
            }
        }
    }

    if (RpGeometryAddMorphTargets(geometry, 1) < 0) {
        _rpMaterialListDeinitialize(&geometry->matList);
        RwFree(geometry);
        return NULL;
    }

    geometry->surfaceProps.ambient = 1.0f;
    geometry->surfaceProps.diffuse = 1.0f;
    geometry->surfaceProps.specular = 1.0f;

    _rwPluginRegistryInitObject(&geometryTKList, geometry);

    return geometry;
}

RpGeometry* _rpGeometryAddRef(RpGeometry* geometry) {
    geometry->refCount++;
    return geometry;
}

RwBool RpGeometryDestroy(RpGeometry* geometry) {
    RwBool result;

    geometry->refCount--;
    result = TRUE;
    if (geometry->refCount <= 0) {
        if (geometry->repEntry) {
            RwResourcesFreeResEntry(geometry->repEntry);
        }

        geometry->refCount++;
        RpGeometryLock(geometry, rpGEOMETRYLOCKALL);

        _rwPluginRegistryDeInitObject(&geometryTKList, geometry);

        if (geometry->morphTarget) {
            RwFree(geometry->morphTarget);
            geometry->morphTarget = NULL;
        }

        _rpMaterialListDeinitialize(&geometry->matList);

        geometry->refCount--;
        RwFree(geometry);

        result = TRUE;
    }

    return result;
}

RwInt32 RpGeometryRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB,
                                 RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB) {
    return _rwPluginRegistryAddPlugin(&geometryTKList, size, pluginID, constructCB, destructCB, copyCB);
}

RwInt32 RpGeometryRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB,
                                       RwPluginDataChunkWriteCallBack writeCB,
                                       RwPluginDataChunkGetSizeCallBack getSizeCB) {
    return _rwPluginRegistryAddPluginStream(&geometryTKList, pluginID, readCB, writeCB, getSizeCB);
}

RpGeometry* RpGeometryStreamRead(RwStream* stream) {
    _rpGeometry gf;
    _rpMorphTarget mt;
    RpGeometry* geometry;
    RwUInt32 size;
    RwInt32 i;
    RwInt32 j;
    RwUInt32 version;

    if (!RwStreamFindChunk(stream, rwID_STRUCT, NULL, &version)) {
        return NULL;
    }

    if (!(version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION)) {
        RWERROR((E_RW_BADVERSION));
        return NULL;
    }

    if (RwStreamRead(stream, &gf, sizeof(gf)) != sizeof(gf)) {
        return NULL;
    }

    RwMemNative32(&gf, sizeof(gf));
    RwMemFloat32ToReal(&gf.surfaceProps, sizeof(gf.surfaceProps));

    geometry = RpGeometryCreate(gf.numVertices, gf.numTriangles, gf.format);
    if (!geometry) {
        return NULL;
    }

    geometry->surfaceProps = gf.surfaceProps;

    if (gf.numMorphTargets > 1) {
        if (RpGeometryAddMorphTargets(geometry, gf.numMorphTargets - 1) < 0) {
            RpGeometryDestroy(geometry);
            return NULL;
        }
    }

    if (!(geometry->flags & rpGEOMETRYNATIVE) && geometry->numVertices) {
        if (gf.format & rpGEOMETRYPRELIT) {
            size = geometry->numVertices * sizeof(RwRGBA);
            if (size != RwStreamRead(stream, geometry->preLitLum, size)) {
                RpGeometryDestroy(geometry);
                return NULL;
            }
        }

        if (geometry->numTexCoordSets > 0) {
            RwUInt32 texSize = geometry->numVertices * sizeof(RwTexCoords);

            for (i = 0; i < geometry->numTexCoordSets; i++) {
                if (!RwStreamReadReal(stream, (RwReal*)geometry->texCoords[i], texSize)) {
                    RpGeometryDestroy(geometry);
                    return NULL;
                }
            }
        }

        if (geometry->numTriangles) {
            RwUInt32* triangles;
            RwInt32 count;
            RwUInt32 triSize;

            triangles = (RwUInt32*)geometry->triangles;
            count = geometry->numTriangles;
            triSize = count * sizeof(RpTriangle);
            if (triSize != RwStreamRead(stream, triangles, triSize)) {
                RpGeometryDestroy(geometry);
                return NULL;
            }

            RwMemNative32(triangles, triSize);

            while (count--) {
                RwUInt32 word;

                word = triangles[0];
                ((RwUInt16*)triangles)[0] = (RwUInt16)(word >> 16);
                ((RwUInt16*)triangles)[1] = (RwUInt16)word;
                word = triangles[1];
                ((RwUInt16*)triangles)[2] = (RwUInt16)(word >> 16);
                ((RwUInt16*)triangles)[3] = (RwUInt16)word;
                triangles += 2;
            }
        }
    }

    for (j = 0; j < geometry->numMorphTargets; j++) {
        RpMorphTarget* morphTarget = &geometry->morphTarget[j];

        if (RwStreamRead(stream, &mt, sizeof(mt)) != sizeof(mt)) {
            RpGeometryDestroy(geometry);
            return NULL;
        }

        RwMemFloat32ToReal(&mt.boundingSphere.center, sizeof(RwV3d));
        RwMemFloat32ToReal(&mt.boundingSphere.radius, sizeof(RwReal));
        RwMemNative32(&mt, sizeof(mt));

        morphTarget->boundingSphere = mt.boundingSphere;

        if (mt.pointsPresent && mt.normalsPresent) {
            if (!RwStreamReadReal(stream, (RwReal*)morphTarget->verts, geometry->numVertices * sizeof(RwV3d) * 2)) {
                RpGeometryDestroy(geometry);
                return NULL;
            }
        } else {
            if (mt.pointsPresent) {
                if (!RwStreamReadReal(stream, (RwReal*)morphTarget->verts, geometry->numVertices * sizeof(RwV3d))) {
                    RpGeometryDestroy(geometry);
                    return NULL;
                }
            }

            if (mt.normalsPresent) {
                if (!RwStreamReadReal(stream, (RwReal*)morphTarget->normals,
                                      geometry->numVertices * sizeof(RwV3d))) {
                    RpGeometryDestroy(geometry);
                    return NULL;
                }
            }
        }
    }

    if (!RwStreamFindChunk(stream, rwID_MATLIST, NULL, &version)) {
        return NULL;
    }

    if (!(version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION)) {
        RpGeometryDestroy(geometry);
        RWERROR((E_RW_BADVERSION));
        return NULL;
    }

    RpMaterialSetDefaultSurfaceProperties(&gf.surfaceProps);

    if (!_rpMaterialListStreamRead(stream, &geometry->matList)) {
        RpGeometryDestroy(geometry);
        return NULL;
    }

    RpMaterialSetDefaultSurfaceProperties(NULL);

    if (!_rwPluginRegistryReadDataChunks(&geometryTKList, stream, geometry)) {
        RpGeometryDestroy(geometry);
        return NULL;
    }

    if (!RpGeometryUnlock(geometry)) {
        RpGeometryDestroy(geometry);
        return NULL;
    }

    return geometry;
}
