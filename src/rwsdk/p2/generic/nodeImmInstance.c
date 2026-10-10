/*
 * ImmInstance.csl: wraps the immediate mode vertex pool in a packet, with a
 * mesh state built from the pool's matrix (or the camera view matrix) and
 * the default render state.
 */

#include "rwsdk/baim3d_int.h"
#include "rwsdk/rpworld_int.h"

extern RxClusterDefinition RxClObjSpace3DVertices;
extern RxClusterDefinition RxClMeshState;
extern RxClusterDefinition RxClRenderState;

RwBool ImmInstanceNode(RxPipelineNodeInstance* self, const RxPipelineNodeParam* params)
{
    RwRGBA white = { 255, 255, 255, 255 };
    rwIm3DPool* pool = (rwIm3DPool*)params->dataParam;
    RwUInt32 numVerts = pool->numElements;
    RxPacket* packet;
    RxCluster* objVerts;
    RxCluster* meshState;
    RxCluster* renderState;
    RxMeshStateVector* meshData;
    RxRenderStateVector* rsvp;

    packet = RxPacketCreate(self);
    objVerts = RxClusterLockWrite(packet, 0, self);
    meshState = RxClusterLockWrite(packet, 1, self);
    renderState = RxClusterLockWrite(packet, 2, self);
    meshState = RxClusterInitializeData(meshState, 1, sizeof(RxMeshStateVector));
    renderState = RxClusterInitializeData(renderState, 1, sizeof(RxRenderStateVector));
    RxClusterSetExternalData(objVerts, pool->elements, pool->stride, numVerts);

    meshData = RxClusterGetCursorData(meshState, RxMeshStateVector);
    meshData->Flags = rxGEOMETRY_COLORED;
    meshData->Flags |= (pool->stash.flags & rwIM3D_VERTEXUV) ? rxGEOMETRY_TEXTURED : 0;
    meshData->SourceObject = pool;

    if (pool->stash.ltm) {
        meshData->Obj2Cam = *pool->stash.ltm;
        RwMatrixTransform(&meshData->Obj2Cam, &((RwCamera*)RWSRCGLOBAL(curCamera))->viewMatrix, rwCOMBINEPOSTCONCAT);
        meshData->Obj2World = *pool->stash.ltm;
    } else {
        meshData->Obj2Cam = ((RwCamera*)RWSRCGLOBAL(curCamera))->viewMatrix;
        meshData->Obj2World.right.x = meshData->Obj2World.up.y = meshData->Obj2World.at.z = 1.0f;
        meshData->Obj2World.right.y = meshData->Obj2World.right.z = meshData->Obj2World.up.x = 0.0f;
        meshData->Obj2World.up.z = meshData->Obj2World.at.x = meshData->Obj2World.at.y = 0.0f;
        meshData->Obj2World.pos.x = meshData->Obj2World.pos.y = meshData->Obj2World.pos.z = 0.0f;
        meshData->Obj2World.flags |= rwMATRIXINTERNALIDENTITY | rwMATRIXTYPEORTHONORMAL;
    }

    meshData->SurfaceProperties.ambient = 1.0f;
    meshData->SurfaceProperties.diffuse = 1.0f;
    meshData->SurfaceProperties.specular = 1.0f;
    meshData->Texture = NULL;
    meshData->MatCol = white;
    meshData->Pipeline = NULL;
    meshData->PrimType = rwPRIMTYPENAPRIMTYPE;
    meshData->NumElements = 0;
    meshData->NumVertices = numVerts;
    meshData->ClipFlagsAnd = 0;
    meshData->ClipFlagsOr = 0;
    meshState->numUsed++;

    rsvp = RxClusterGetCursorData(renderState, RxRenderStateVector);
    *rsvp = RXPIPELINEGLOBAL(defaultRenderState);
    rsvp->Flags |= (pool->stash.flags & rwIM3D_ALLOPAQUE) ? 0 : rxRENDERSTATEFLAG_VERTEXALPHAENABLE;
    renderState->numUsed++;

    RxPacketDispatch(packet, 0, self);

    return TRUE;
}

RxNodeDefinition* RxNodeDefinitionGetImmInstance(void)
{
    static RxClusterRef N1clofinterest[] = {
        { &RxClObjSpace3DVertices, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClMeshState, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClRenderState, rxCLALLOWABSENT, rxCLRESERVED },
    };

#pragma explicit_zero_data on
    static RxClusterValidityReq N1inputreqs[] = { rxCLREQ_DONTWANT, rxCLREQ_DONTWANT, rxCLREQ_DONTWANT };
#pragma explicit_zero_data reset

    static RxClusterValid N1outcl1[] = { rxCLVALID_VALID, rxCLVALID_VALID, rxCLVALID_VALID };

    static RwChar _ImmInstanceOut[] = "ImmInstanceOut";

    static RxOutputSpec N1outputs[] = { { _ImmInstanceOut, N1outcl1, rxCLVALID_NOCHANGE } };

    static RwChar _ImmInstance_csl[] = "ImmInstance.csl";

    static RxNodeDefinition nodeImmInstanceCSL = {
        _ImmInstance_csl,
        { ImmInstanceNode, NULL, NULL, NULL, NULL, NULL, NULL },
        { 3, N1clofinterest, N1inputreqs, 1, N1outputs },
        0,
        0,
        0,
    };

    return &nodeImmInstanceCSL;
}
