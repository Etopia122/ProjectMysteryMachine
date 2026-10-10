/*
 * AtomicInstance.csl: instances an atomic's geometry (vertices, normals,
 * texture coordinates, prelight colours and per mesh triangle indices) into
 * a resource entry cached on the atomic or its geometry, then sends one
 * packet per mesh down the pipeline.
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

#define rpGEOMETRYTEXTURED 0x04
#define rpGEOMETRYPRELIT 0x08
#define rpGEOMETRYNORMALS 0x10

#define rpGEOMETRYLOCKVERTICES 0x02
#define rpGEOMETRYLOCKNORMALS 0x04

#define rpINTERPOLATORDIRTYINSTANCE 0x01

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

typedef struct RxObjSpace3DVertex {
    /* 0x00 */ RwV3d objVertex;
    /* 0x0C */ RwV3d objNormal;
    /* 0x18 */ RwRGBA color;
    /* 0x1C */ RwReal u;
    /* 0x20 */ RwReal v;
} RxObjSpace3DVertex;

/* One instanced mesh: where its indices and vertices live in the entry. */
typedef struct AtomicInstanceMesh {
    /* 0x00 */ RxVertexIndex* indices;
    /* 0x04 */ RwUInt32 numIndices;
    /* 0x08 */ RwUInt8* vertices;
    /* 0x0C */ RwUInt32 numVertices;
    /* 0x10 */ RwUInt32 stride;
    /* 0x14 */ RpMaterial* material;
    /* 0x18 */ RpMesh* mesh;
} AtomicInstanceMesh;

/* Resource entry payload: meshes, then the vertices, then the indices. */
typedef struct AtomicInstanceData {
    /* 0x00 */ RwUInt32 serialNum;
    /* 0x04 */ AtomicInstanceMesh meshes[1];
} AtomicInstanceData;

RwMatrix* RwFrameGetLTM(RwFrame* frame);
RwResEntry* RwResourcesAllocateResEntry(void* owner, RwResEntry** ownerRef, RwInt32 size,
                                        void (*destroyNotify)(RwResEntry* resEntry));
RwBool RwResourcesFreeResEntry(RwResEntry* entry);

static const RwV3d defaultNormal = { 0.0f, 0.0f, 0.0f };

/* Positions and normals, interpolated between two morph targets if needed. */
static void AtomicInstanceVertices(RwUInt8* verts, RpAtomic* atomic, RpGeometry* geom, RwUInt32 stride)
{
    RwInt32 start = atomic->interpolator.startMorphTarget;
    RwInt32 end = atomic->interpolator.endMorphTarget;
    RwInt32 numVerts = geom->numVertices;

    if (start >= geom->numMorphTargets || end >= geom->numMorphTargets) {
        end = 0;
        start = 0;
    }

    if (start == end) {
        RpMorphTarget* mt = &geom->morphTarget[start];
        RwV3d* pos = mt->verts;

        if (geom->flags & rpGEOMETRYNORMALS) {
            RwV3d* normal = mt->normals;

            while (numVerts--) {
                RxObjSpace3DVertex* v = (RxObjSpace3DVertex*)verts;

                v->objVertex.x = pos->x;
                v->objVertex.y = pos->y;
                v->objVertex.z = pos->z;
                v->objNormal.x = normal->x;
                v->objNormal.y = normal->y;
                v->objNormal.z = normal->z;
                pos++;
                normal++;
                verts += stride;
            }
        } else {
            while (numVerts--) {
                RxObjSpace3DVertex* v = (RxObjSpace3DVertex*)verts;

                v->objVertex.x = pos->x;
                v->objVertex.y = pos->y;
                v->objVertex.z = pos->z;
                v->objNormal.x = defaultNormal.x;
                v->objNormal.y = defaultNormal.y;
                v->objNormal.z = defaultNormal.z;
                pos++;
                verts += stride;
            }
        }
    } else {
        RpMorphTarget* mt1 = &geom->morphTarget[start];
        RpMorphTarget* mt2 = &geom->morphTarget[end];
        RwReal scale = atomic->interpolator.recipTime * atomic->interpolator.position;
        RwV3d* pos1 = mt1->verts;
        RwV3d* pos2 = mt2->verts;

        if (geom->flags & rpGEOMETRYNORMALS) {
            RwV3d* normal1 = mt1->normals;
            RwV3d* normal2 = mt2->normals;

            while (numVerts--) {
                RxObjSpace3DVertex* v = (RxObjSpace3DVertex*)verts;
                RwV3d d;

                RwV3dSub(&d, pos2, pos1);
                RwV3dScale(&d, &d, scale);
                RwV3dAdd(&d, &d, pos1);
                v->objVertex.x = d.x;
                v->objVertex.y = d.y;
                v->objVertex.z = d.z;
                pos1++;
                pos2++;
                RwV3dSub(&d, normal2, normal1);
                RwV3dScale(&d, &d, scale);
                RwV3dAdd(&d, &d, normal1);
                v->objNormal.x = d.x;
                v->objNormal.y = d.y;
                v->objNormal.z = d.z;
                normal1++;
                normal2++;
                verts += stride;
            }
        } else {
            while (numVerts--) {
                RxObjSpace3DVertex* v = (RxObjSpace3DVertex*)verts;
                RwV3d d;

                RwV3dSub(&d, pos2, pos1);
                RwV3dScale(&d, &d, scale);
                RwV3dAdd(&d, &d, pos1);
                v->objVertex.x = d.x;
                v->objVertex.y = d.y;
                v->objVertex.z = d.z;
                pos1++;
                pos2++;
                v->objNormal.x = defaultNormal.x;
                v->objNormal.y = defaultNormal.y;
                v->objNormal.z = defaultNormal.z;
                verts += stride;
            }
        }
    }
}

/* Everything per vertex: texture coordinates, colours, then positions. */
static void AtomicInstanceAllVertices(RwUInt8* verts, RpAtomic* atomic, RpGeometry* geom, RwUInt32 stride)
{
    RwUInt32 numVerts;
    RwUInt8* dst;
    RwUInt32 flags;
    RwTexCoords* tc;
    RwRGBA* col;

    flags = geom->flags;
    if (flags & rpGEOMETRYTEXTURED) {
        numVerts = geom->numVertices;
        dst = verts;
        tc = geom->texCoords[0];
        while (numVerts--) {
            ((RxObjSpace3DVertex*)dst)->u = tc->u;
            ((RxObjSpace3DVertex*)dst)->v = tc->v;
            tc++;
            dst += stride;
        }
    }

    if (flags & rpGEOMETRYPRELIT) {
        numVerts = geom->numVertices;
        dst = verts;
        col = geom->preLitLum;
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

        numVerts = geom->numVertices;
        dst = verts;
        while (numVerts--) {
            ((RxObjSpace3DVertex*)dst)->color.red = opaqueBlack.red;
            ((RxObjSpace3DVertex*)dst)->color.green = opaqueBlack.green;
            ((RxObjSpace3DVertex*)dst)->color.blue = opaqueBlack.blue;
            ((RxObjSpace3DVertex*)dst)->color.alpha = opaqueBlack.alpha;
            dst += stride;
        }
    }

    AtomicInstanceVertices(verts, atomic, geom, stride);
}

/* Per mesh triangle list indices, relative to the mesh's first vertex. */
static void AtomicInstanceMeshes(AtomicInstanceMesh* inst, RpMeshHeader* meshHeader, RxVertexIndex* indices,
                                 RwUInt8* verts, RwUInt32 stride)
{
    RwInt32 numIndices;
    RwUInt32 numTris;
    RwUInt32 maxVert;
    RwUInt32 numMeshes;
    RpMesh* mesh;
    RwUInt32 minVert;
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

            inst->vertices = verts + minVert * stride;
            inst->numVertices = maxVert - minVert + 1;

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
        }

        inst++;
        mesh++;
    }
}

RwBool AtomicInstanceNode(RxPipelineNodeInstance* self, const RxPipelineNodeParam* params)
{
    RwUInt32 vertSize;
    RwUInt32 textured;
    RxVertexIndex* indices;
    RpGeometry* geom;
    RwResEntry** ownerRef;
    RpAtomic* atomic;
    RwResEntry* repEntry;
    void* owner;
    AtomicInstanceData* data;
    RwUInt8* verts;
    RpMeshHeader* meshHeader;
    RwUInt32 numMeshes;
    RwUInt32 numVerts;
    AtomicInstanceMesh* inst;
    RwUInt32 numTris;
    RwUInt32 stride;
    RwUInt32 flags;

    atomic = (RpAtomic*)params->dataParam;
    geom = atomic->geometry;
    numVerts = geom->numVertices;
    if (!numVerts) {
        return TRUE;
    }

    meshHeader = geom->mesh;
    numMeshes = meshHeader->numMeshes;
    if (!numMeshes) {
        return TRUE;
    }

    flags = geom->flags;
    if (geom->numMorphTargets != 1) {
        repEntry = atomic->repEntry;
        owner = atomic;
        ownerRef = &atomic->repEntry;
    } else {
        repEntry = geom->repEntry;
        owner = geom;
        ownerRef = &geom->repEntry;
    }

    stride = sizeof(RxObjSpace3DVertex);
    if (flags & rpGEOMETRYNORMALS) {
        stride = sizeof(RxObjSpace3DVertex);
    }
    textured = flags & rpGEOMETRYTEXTURED;
    if (textured) {
        stride = sizeof(RxObjSpace3DVertex);
    }

    if (meshHeader->flags & rpMESHHEADERTRISTRIP) {
        numTris = meshHeader->totalIndicesInMesh - numMeshes * 2;
    } else {
        numTris = meshHeader->totalIndicesInMesh / 3;
    }

    if (repEntry) {
        data = (AtomicInstanceData*)(repEntry + 1);
        verts = (RwUInt8*)&data->meshes[numMeshes];
        indices = (RxVertexIndex*)(verts + numVerts * stride);
        if (data->serialNum != meshHeader->serialNum) {
            RwResourcesFreeResEntry(repEntry);
            repEntry = NULL;
        }
    }

    if (!repEntry) {
        vertSize = numVerts * stride;
        repEntry = RwResourcesAllocateResEntry(owner, ownerRef,
                                               sizeof(AtomicInstanceData) + (numMeshes - 1) * sizeof(AtomicInstanceMesh) +
                                                   vertSize + numTris * 3 * sizeof(RxVertexIndex),
                                               NULL);
        data = (AtomicInstanceData*)(repEntry + 1);
        data->serialNum = meshHeader->serialNum;
        verts = (RwUInt8*)&data->meshes[numMeshes];
        AtomicInstanceAllVertices(verts, atomic, geom, stride);
        AtomicInstanceMeshes(data->meshes, meshHeader, (RxVertexIndex*)(verts + vertSize), verts, stride);
        geom->lockedSinceLastInst = 0;
    } else {
        RwUInt32 locked = geom->lockedSinceLastInst;

        RwResourcesUseResEntry(repEntry);
        if (atomic->interpolator.flags & rpINTERPOLATORDIRTYINSTANCE) {
            atomic->interpolator.flags &= ~rpINTERPOLATORDIRTYINSTANCE;
            locked |= rpGEOMETRYLOCKVERTICES | rpGEOMETRYLOCKNORMALS;
        }

        switch (locked) {
        case 0:
            break;
        case 2:
        case 4:
        case 6:
            AtomicInstanceVertices(verts, atomic, geom, stride);
            break;
        default:
            AtomicInstanceAllVertices(verts, atomic, geom, stride);
            AtomicInstanceMeshes(data->meshes, meshHeader, indices, verts, stride);
            break;
        }
        geom->lockedSinceLastInst = 0;
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
            meshData->Obj2Cam = *RwFrameGetLTM((RwFrame*)atomic->object.object.parent);
            RwMatrixTransform(&meshData->Obj2Cam, &((RwCamera*)RWSRCGLOBAL(curCamera))->viewMatrix,
                              rwCOMBINEPOSTCONCAT);
            meshData->Obj2World = *RwFrameGetLTM((RwFrame*)atomic->object.object.parent);
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

RxNodeDefinition* RxNodeDefinitionGetAtomicInstance(void)
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

    static RwChar _AtomicInstanceOut[] = "AtomicInstanceOut";

    static RxOutputSpec N1outputs[] = { { _AtomicInstanceOut, N1outcl1, rxCLVALID_NOCHANGE } };

    static RwChar _AtomicInstance_csl[] = "AtomicInstance.csl";

    static RxNodeDefinition nodeAtomicInstanceCSL = {
        _AtomicInstance_csl,
        { AtomicInstanceNode, NULL, NULL, NULL, NULL, NULL, NULL },
        { 4, N1clofinterest, N1inputreqs, 1, N1outputs },
        0,
        0,
        0,
    };

    return &nodeAtomicInstanceCSL;
}
