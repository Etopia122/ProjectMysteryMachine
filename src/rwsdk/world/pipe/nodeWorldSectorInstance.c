/*
 * WorldSectorInstance.csl: instances a world sector's vertices, normals,
 * texture coordinates, prelight colours and per mesh triangle indices into a
 * resource entry cached on the sector, then sends one packet per mesh down
 * the pipeline. World geometry is static, so a cached entry is reused as is.
 */

#include "rwsdk/rpworld_int.h"
#include "rwsdk/p2core_int.h"

extern RxClusterDefinition RxClObjSpace3DVertices;
extern RxClusterDefinition RxClIndices;
extern RxClusterDefinition RxClMeshState;
extern RxClusterDefinition RxClRenderState;

extern RwModuleInfo resourcesModule;

#define rwPRIMTYPETRILIST 3

typedef RwUInt16 RxVertexIndex;

#define RwMin(_a, _b) (((_a) < (_b)) ? (_a) : (_b))
#define RwMax(_a, _b) (((_a) > (_b)) ? (_a) : (_b))

#define rpWORLDTEXTURED 0x04
#define rpWORLDPRELIT 0x08
#define rpWORLDNORMALS 0x10
#define rpWORLDTEXTURED2 0x80

#define rpMESHHEADERTRISTRIP 0x0001
#define rpMESHHEADERUNINDEXED 0x0100

typedef struct RpMaterial {
    /* 0x00 */ RwTexture* texture;
    /* 0x04 */ RwRGBA color;
    /* 0x08 */ RxPipeline* pipeline;
    /* 0x0C */ RwSurfaceProperties surfaceProps;
} RpMaterial;

struct RpMeshHeader {
    /* 0x00 */ RwUInt32 flags;
    /* 0x04 */ RwUInt16 numMeshes;
    /* 0x06 */ RwUInt16 serialNum;
    /* 0x08 */ RwUInt32 totalIndicesInMesh;
    /* 0x0C */ RwUInt32 firstMeshOffset;
};

typedef struct RpMesh {
    /* 0x00 */ RxVertexIndex* indices;
    /* 0x04 */ RwUInt32 numIndices;
    /* 0x08 */ RpMaterial* material;
} RpMesh;

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

#define RWRESOURCESGLOBAL(var) \
    (RWPLUGINOFFSET(rwResourcesGlobals, RwEngineInstance, resourcesModule.globalsOffset)->var)

#define RwResourcesUseResEntry(_ntry)                                      \
    do {                                                                   \
        if ((_ntry)->link.next) {                                          \
            rwLLLinkRemove(&(_ntry)->link);                                \
            rwLinkListAddLLLink(RWRESOURCESGLOBAL(usedEntries), &(_ntry)->link); \
        }                                                                  \
    } while (0)

#define RwV3dSub(o, a, b)              do {                                   (o)->x = (a)->x - (b)->x;          (o)->y = (a)->y - (b)->y;          (o)->z = (a)->z - (b)->z;      } while (0)

#define RwV3dAdd(o, a, b)              do {                                   (o)->x = (a)->x + (b)->x;          (o)->y = (a)->y + (b)->y;          (o)->z = (a)->z + (b)->z;      } while (0)

#define RwV3dScale(o, a, s)            do {                                   (o)->x = (a)->x * (s);             (o)->y = (a)->y * (s);             (o)->z = (a)->z * (s);         } while (0)

/* World sector normals are packed into signed bytes. */
typedef struct RpVertexNormal {
    RwInt8 x;
    RwInt8 y;
    RwInt8 z;
    RwUInt8 pad;
} RpVertexNormal;

typedef struct RxObjSpace3DVertex {
    /* 0x00 */ RwV3d objVertex;
    /* 0x0C */ RwV3d objNormal;
    /* 0x18 */ RwRGBA color;
    /* 0x1C */ RwReal u;
    /* 0x20 */ RwReal v;
} RxObjSpace3DVertex;

/* One instanced mesh: where its indices and vertices live in the entry. */
typedef struct WorldSectorInstanceMesh {
    /* 0x00 */ RxVertexIndex* indices;
    /* 0x04 */ RwUInt32 numIndices;
    /* 0x08 */ RwUInt8* vertices;
    /* 0x0C */ RwUInt32 numVertices;
    /* 0x10 */ RwUInt32 stride;
    /* 0x14 */ RpMaterial* material;
    /* 0x18 */ RpMesh* mesh;
} WorldSectorInstanceMesh;

/* Resource entry payload: meshes, then the vertices, then the indices. */
typedef struct WorldSectorInstanceData {
    /* 0x00 */ RwUInt32 serialNum;
    /* 0x04 */ WorldSectorInstanceMesh meshes[1];
} WorldSectorInstanceData;

RwResEntry* RwResourcesAllocateResEntry(void* owner, RwResEntry** ownerRef, RwInt32 size,
                                        void (*destroyNotify)(RwResEntry* resEntry));
RwBool RwResourcesFreeResEntry(RwResEntry* entry);

/* Everything per vertex: texture coordinates, colours, normals, positions. */
static void WorldSectorInstanceAllVertices(RwUInt8* verts, RpWorld* world, RpWorldSector* sector, RwUInt32 stride)
{
    RwUInt32 flags;
    RwUInt32 numVerts;
    RwUInt8* dst;
    RwTexCoords* tc;
    RwRGBA* col;
    RpVertexNormal* normal;
    RwV3d* pos;

    flags = ((RpWorld*)RWSRCGLOBAL(curWorld))->flags;
    if (flags & (rpWORLDTEXTURED | rpWORLDTEXTURED2)) {
        numVerts = sector->numVertices;
        dst = verts;
        tc = sector->texCoords[0];
        while (numVerts--) {
            ((RxObjSpace3DVertex*)dst)->u = tc->u;
            ((RxObjSpace3DVertex*)dst)->v = tc->v;
            tc++;
            dst += stride;
        }
    }

    if (flags & rpWORLDPRELIT) {
        numVerts = sector->numVertices;
        dst = verts;
        col = sector->preLitLum;
        while (numVerts--) {
            ((RxObjSpace3DVertex*)dst)->color.red = col->red;
            ((RxObjSpace3DVertex*)dst)->color.green = col->green;
            ((RxObjSpace3DVertex*)dst)->color.blue = col->blue;
            ((RxObjSpace3DVertex*)dst)->color.alpha = col->alpha;
            col++;
            dst += stride;
        }
    } else {
        static const RwRGBA opaqueBlack = { 0, 0, 0, 255 };

        numVerts = sector->numVertices;
        dst = verts;
        while (numVerts--) {
            ((RxObjSpace3DVertex*)dst)->color.red = opaqueBlack.red;
            ((RxObjSpace3DVertex*)dst)->color.green = opaqueBlack.green;
            ((RxObjSpace3DVertex*)dst)->color.blue = opaqueBlack.blue;
            ((RxObjSpace3DVertex*)dst)->color.alpha = opaqueBlack.alpha;
            dst += stride;
        }
    }

    if (flags & rpWORLDNORMALS) {
        numVerts = sector->numVertices;
        dst = verts;
        normal = (RpVertexNormal*)sector->normals;
        while (numVerts--) {
            RwV3d n;

            n.x = (1.0f / 128.0f) * normal->x;
            n.y = (1.0f / 128.0f) * normal->y;
            n.z = (1.0f / 128.0f) * normal->z;
            ((RxObjSpace3DVertex*)dst)->objNormal.x = n.x;
            ((RxObjSpace3DVertex*)dst)->objNormal.y = n.y;
            ((RxObjSpace3DVertex*)dst)->objNormal.z = n.z;
            normal++;
            dst += stride;
        }
    }

    numVerts = sector->numVertices;
    pos = sector->vertices;
    while (numVerts--) {
        ((RxObjSpace3DVertex*)verts)->objVertex.x = pos->x;
        ((RxObjSpace3DVertex*)verts)->objVertex.y = pos->y;
        ((RxObjSpace3DVertex*)verts)->objVertex.z = pos->z;
        pos++;
        verts += stride;
    }
}

/* Per mesh triangle list indices, relative to the mesh's first vertex. */
static void WorldSectorInstanceMeshes(WorldSectorInstanceMesh* inst, RpMeshHeader* meshHeader, RxVertexIndex* indices,
                                 RwUInt8* verts, RwUInt32 stride)
{
    RwInt32 numIndices;
    RwUInt32 numMeshes;
    RpMesh* mesh;
    RwUInt32 minVert;
    RwUInt32 numTris;
    RwUInt32 maxVert;
    RxVertexIndex* src;
    RwBool triStrip;
    RwUInt32 count;
    RwUInt32 odd;

    triStrip = meshHeader->flags & rpMESHHEADERTRISTRIP;
    numMeshes = meshHeader->numMeshes;
    mesh = (RpMesh*)(meshHeader + 1);
    minVert = 0;
    while (numMeshes--) {
        src = mesh->indices;
        inst->indices = indices;
        inst->stride = stride;
        inst->material = mesh->material;
        inst->mesh = mesh;
        numIndices = mesh->numIndices;
        count = numIndices;

        if (meshHeader->flags & rpMESHHEADERUNINDEXED) {
            inst->vertices = verts + minVert * stride;
            inst->numVertices = mesh->numIndices;

            if (triStrip) {
                RwUInt16 i;

                numTris = numIndices - 2;
                inst->numIndices = numTris * 3;
                for (i = 0; i < numTris; i++) {
                    odd = i & 1;
                    indices[0] = i + odd;
                    indices[1] = i + (odd ^ 1);
                    indices[2] = i + 2;
                    indices += 3;
                }
            } else {
                inst->numIndices = numIndices;
                if (meshHeader->flags & rpMESHHEADERUNINDEXED) {
                    RwUInt16 i;

                    for (i = 0; i < count; i++) {
                        *indices++ = i;
                    }
                }
            }
            minVert += count;
        } else {
            maxVert = 0;
            minVert = 0xFFFF;
            while (count--) {
                minVert = RwMin(minVert, src[count]);
                maxVert = RwMax(maxVert, src[count]);
            }

            if (triStrip) {
                odd = 0;
                inst->numIndices = (numIndices - 2) * 3;
                for (; (RwUInt32)numIndices > 2; numIndices--) {
                    indices[0] = src[odd] - minVert;
                    indices[1] = src[odd ^ 1] - minVert;
                    indices[2] = src[2] - minVert;
                    if (indices[0] == indices[1] || indices[0] == indices[2] || indices[1] == indices[2]) {
                        inst->numIndices -= 3;
                    } else {
                        indices += 3;
                    }
                    odd ^= 1;
                    src++;
                }
            } else {
                RwUInt32 i;

                inst->numIndices = numIndices;
                for (i = 2; i < numIndices; i += 3) {
                    indices[0] = src[0] - minVert;
                    indices[1] = src[1] - minVert;
                    indices[2] = src[2] - minVert;
                    src += 3;
                    indices += 3;
                }
            }

            inst->vertices = verts + minVert * stride;
            inst->numVertices = maxVert - minVert + 1;
        }

        inst++;
        mesh++;
    }
}

RwBool WorldSectorInstanceNode(RxPipelineNodeInstance* self, const RxPipelineNodeParam* params)
{
    RpWorldSector* sector;
    RpWorld* world;
    RwUInt32 textured;
    WorldSectorInstanceData* data;
    RwUInt8* verts;
    RpMeshHeader* meshHeader;
    RwUInt32 numMeshes;
    RwUInt32 numVerts;
    RwUInt32 numTris;
    RwResEntry* repEntry;
    WorldSectorInstanceMesh* inst;
    RwUInt32 flags;
    RwUInt32 vertSize;
    RwUInt32 stride;

    sector = (RpWorldSector*)params->dataParam;
    world = (RpWorld*)RWSRCGLOBAL(curWorld);
    numVerts = sector->numVertices;
    if (!numVerts) {
        return TRUE;
    }

    meshHeader = sector->mesh;
    numMeshes = meshHeader->numMeshes;
    if (!numMeshes) {
        return TRUE;
    }

    flags = world->flags;
    stride = sizeof(RxObjSpace3DVertex);
    repEntry = sector->repEntry;
    if (flags & rpWORLDNORMALS) {
        stride = sizeof(RxObjSpace3DVertex);
    }
    textured = flags & (rpWORLDTEXTURED | rpWORLDTEXTURED2);
    if (textured) {
        stride = sizeof(RxObjSpace3DVertex);
    }

    if (meshHeader->flags & rpMESHHEADERTRISTRIP) {
        numTris = meshHeader->totalIndicesInMesh - numMeshes * 2;
    } else {
        numTris = meshHeader->totalIndicesInMesh / 3;
    }

    if (repEntry) {
        data = (WorldSectorInstanceData*)(repEntry + 1);
        if (data->serialNum != meshHeader->serialNum) {
            RwResourcesFreeResEntry(repEntry);
            repEntry = NULL;
        }
    }

    if (!repEntry) {
        vertSize = numVerts * stride;
        repEntry = RwResourcesAllocateResEntry(sector, &sector->repEntry,
                                               sizeof(WorldSectorInstanceData) +
                                                   (numMeshes - 1) * sizeof(WorldSectorInstanceMesh) +
                                                   numTris * 3 * sizeof(RxVertexIndex) + vertSize,
                                               NULL);
        data = (WorldSectorInstanceData*)(repEntry + 1);
        data->serialNum = meshHeader->serialNum;
        verts = (RwUInt8*)&data->meshes[numMeshes];
        WorldSectorInstanceAllVertices(verts, world, sector, stride);
        WorldSectorInstanceMeshes(data->meshes, meshHeader, (RxVertexIndex*)(verts + vertSize), verts, stride);
    } else {
        RwResourcesUseResEntry(repEntry);
    }

    inst = data->meshes;
    while (numMeshes--) {
        if (inst->numVertices && inst->numIndices) {
            RxPacket* packet;
            RxCluster* objVerts;
            RxCluster* indexCl;
            RxCluster* meshState;
            RxCluster* renderState;
            RxMeshStateVector* meshData;
            RxRenderStateVector* rsvp;
            RpMaterial* material;

            packet = RxPacketCreate(self);
            objVerts = RxClusterLockWrite(packet, 0, self);
            indexCl = RxClusterLockWrite(packet, 1, self);
            meshState = RxClusterLockWrite(packet, 2, self);
            renderState = RxClusterLockWrite(packet, 3, self);
            meshState = RxClusterInitializeData(meshState, 1, sizeof(RxMeshStateVector));
            renderState = RxClusterInitializeData(renderState, 1, sizeof(RxRenderStateVector));
            RxClusterSetExternalData(objVerts, inst->vertices, inst->stride, inst->numVertices);
            RxClusterSetExternalData(indexCl, inst->indices, sizeof(RxVertexIndex), inst->numIndices);

            material = inst->material;
            meshData = RxClusterGetCursorData(meshState, RxMeshStateVector);
            meshData->SourceObject = material;
            meshData->DataObject = params->dataParam;
            meshData->PrimType = rwPRIMTYPETRILIST;
            meshData->NumVertices = inst->numVertices;
            meshData->NumElements = inst->numIndices / 3;
            meshData->Obj2Cam = ((RwCamera*)RWSRCGLOBAL(curCamera))->viewMatrix;
            meshData->Obj2World.right.x = meshData->Obj2World.up.y = meshData->Obj2World.at.z = 1.0f;
            meshData->Obj2World.right.y = meshData->Obj2World.right.z = meshData->Obj2World.up.x = 0.0f;
            meshData->Obj2World.up.z = meshData->Obj2World.at.x = meshData->Obj2World.at.y = 0.0f;
            meshData->Obj2World.pos.x = meshData->Obj2World.pos.y = meshData->Obj2World.pos.z = 0.0f;
            meshData->Obj2World.flags |= rwMATRIXINTERNALIDENTITY | rwMATRIXTYPEORTHONORMAL;
            *(RwSurfaceProperties*)&meshData->SurfaceProperties = material->surfaceProps;
            meshData->Flags = flags;
            meshData->Texture = material->texture;
            meshData->MatCol = material->color;
            meshData->Pipeline = material->pipeline;
            meshData->ClipFlagsAnd = 0;
            meshData->ClipFlagsOr = 0;
            meshData->SourceMesh = inst->mesh;
            meshState->numUsed++;

            rsvp = RxClusterGetCursorData(renderState, RxRenderStateVector);
            *rsvp = RXPIPELINEGLOBAL(defaultRenderState);
            if (textured && meshData->Texture) {
                RwTexture* tex = meshData->Texture;

                rsvp->TextureRaster = tex->raster;
                rsvp->AddressModeU = (tex->filterAddressing >> 8) & 0xF;
                rsvp->AddressModeV = (tex->filterAddressing >> 12) & 0xF;
                rsvp->FilterMode = tex->filterAddressing & 0xFF;
            }
            renderState->numUsed++;

            if (meshData->MatCol.alpha != 0xFF) {
                rsvp->Flags |= rxRENDERSTATEFLAG_VERTEXALPHAENABLE;
            }

            RxPacketDispatch(packet, 0, self);
        }
        inst++;
    }

    return TRUE;
}

RxNodeDefinition* RxNodeDefinitionGetWorldSectorInstance(void)
{
    static RxClusterRef N1clofinterest[] = {
        { &RxClObjSpace3DVertices, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClIndices, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClMeshState, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClRenderState, rxCLALLOWABSENT, rxCLRESERVED },
    };

#pragma explicit_zero_data on
    static RxClusterValidityReq N1inputreqs[] = {
        rxCLREQ_DONTWANT, rxCLREQ_DONTWANT, rxCLREQ_DONTWANT, rxCLREQ_DONTWANT,
    };
#pragma explicit_zero_data reset

    static RxClusterValid N1outcl1[] = { rxCLVALID_VALID, rxCLVALID_VALID, rxCLVALID_VALID, rxCLVALID_VALID };

    static RwChar _Output[] = "Output";

    static RxOutputSpec N1outputs[] = { { _Output, N1outcl1, rxCLVALID_INVALID } };

    static RwChar _WorldSectorInstance_csl[] = "WorldSectorInstance.csl";

    static RxNodeDefinition nodeWorldSectorInstanceCSL = {
        _WorldSectorInstance_csl,
        { WorldSectorInstanceNode, NULL, NULL, NULL, NULL, NULL, NULL },
        { 4, N1clofinterest, N1inputreqs, 1, N1outputs },
        0,
        0,
        0,
    };

    return &nodeWorldSectorInstanceCSL;
}
