/*
 * ImmRenderSetup.csl: builds the render packet for an immediate mode
 * primitive from the stash left by the transform pipeline, sending it out
 * of the indexed or unindexed output.
 */

#include "rwsdk/baim3d_int.h"

extern RxClusterDefinition RxClObjSpace3DVertices;
extern RxClusterDefinition RxClCamSpace3DVertices;
extern RxClusterDefinition RxClScrSpace2DVertices;
extern RxClusterDefinition RxClMeshState;
extern RxClusterDefinition RxClRenderState;
extern RxClusterDefinition RxClIndices;

RxRenderStateVector* RxRenderStateVectorCreate(RwBool current);
void RxRenderStateVectorDestroy(RxRenderStateVector* rsvp);

RwBool ImmRenderSetupNode(RxPipelineNodeInstance* self, const RxPipelineNodeParam* params)
{
    rwIm3DPoolStash* stash = (rwIm3DPoolStash*)params->dataParam;
    RxRenderStateVector* rsvp;
    RxPacket* packet;
    RxCluster* cl;
    RwUInt32 numElements;

    if (!stash->objVerts) {
        return TRUE;
    }

    rsvp = RxRenderStateVectorCreate(TRUE);
    packet = RxPacketCreate(self);

    cl = RxClusterLockWrite(packet, 0, self);
    if (cl) {
        RxClusterSetExternalData(cl, stash->objVerts, sizeof(RxObjSpace3DVertex), stash->numVerts);
    }
    if (stash->camVerts) {
        cl = RxClusterLockWrite(packet, 1, self);
        RxClusterSetExternalData(cl, stash->camVerts, 0x28, stash->numVerts);
    }
    if (stash->devVerts) {
        cl = RxClusterLockWrite(packet, 2, self);
        RxClusterSetExternalData(cl, stash->devVerts, 0x1C, stash->numVerts);
    }
    cl = RxClusterLockWrite(packet, 3, self);
    RxClusterSetExternalData(cl, stash->meshState, sizeof(RxMeshStateVector), 1);

    switch (stash->primType) {
    case rwPRIMTYPETRISTRIP:
        numElements = stash->numIndices - 2;
        break;
    case rwPRIMTYPETRILIST:
        numElements = stash->numIndices / 3;
        break;
    case rwPRIMTYPEPOINTLIST:
        numElements = stash->numIndices;
        break;
    case rwPRIMTYPELINELIST:
        numElements = stash->numIndices / 2;
        break;
    case rwPRIMTYPEPOLYLINE:
        numElements = stash->numIndices - 1;
        break;
    case rwPRIMTYPETRIFAN:
        numElements = stash->numIndices - 2;
        break;
    default:
        return FALSE;
    }
    ((RxMeshStateVector*)stash->meshState)->NumElements = numElements;
    ((RxMeshStateVector*)stash->meshState)->PrimType = stash->primType;

    cl = RxClusterLockWrite(packet, 4, self);
    if (stash->renderState) {
        RwBool vertexAlpha = FALSE;

        RxClusterSetExternalData(cl, stash->renderState, sizeof(RxRenderStateVector), 1);
        if (((RxRenderStateVector*)stash->renderState)->Flags & rxRENDERSTATEFLAG_VERTEXALPHAENABLE) {
            vertexAlpha = TRUE;
        }
        *(RxRenderStateVector*)stash->renderState = *rsvp;
        if (!vertexAlpha) {
            ((RxRenderStateVector*)stash->renderState)->Flags &= ~rxRENDERSTATEFLAG_VERTEXALPHAENABLE;
        }
    } else {
        cl = RxClusterInitializeData(cl, 1, sizeof(RxRenderStateVector));
        *RxClusterGetCursorData(cl, RxRenderStateVector) = *rsvp;
        cl->numUsed++;
    }
    RxRenderStateVectorDestroy(rsvp);

    if (stash->indices) {
        cl = RxClusterLockWrite(packet, 5, self);
        RxClusterSetExternalData(cl, stash->indices, sizeof(RxVertexIndex), stash->numIndices);
        RxPacketDispatch(packet, 0, self);
    } else {
        RxPacketDispatch(packet, 1, self);
    }

    return TRUE;
}

RxNodeDefinition* RxNodeDefinitionGetImmRenderSetup(void)
{
    static RxClusterRef N1clofinterest[] = {
        { &RxClObjSpace3DVertices, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClCamSpace3DVertices, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClScrSpace2DVertices, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClMeshState, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClRenderState, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClIndices, rxCLALLOWABSENT, rxCLRESERVED },
    };

#pragma explicit_zero_data on
    static RxClusterValidityReq N1inputreqs[] = {
        rxCLREQ_DONTWANT, rxCLREQ_DONTWANT, rxCLREQ_DONTWANT, rxCLREQ_DONTWANT, rxCLREQ_DONTWANT, rxCLREQ_DONTWANT,
    };
#pragma explicit_zero_data reset

    static RxClusterValid N1outcl1[] = {
        rxCLVALID_VALID, rxCLVALID_VALID, rxCLVALID_VALID, rxCLVALID_VALID, rxCLVALID_VALID, rxCLVALID_VALID,
    };

    static RxClusterValid N1outcl2[] = {
        rxCLVALID_VALID, rxCLVALID_VALID, rxCLVALID_VALID, rxCLVALID_VALID, rxCLVALID_VALID, rxCLVALID_INVALID,
    };

    static RwChar _ImmRenderSetupOut[] = "ImmRenderSetupOut";

    static RwChar _ImmRenderSetupOutUnindexed[] = "ImmRenderSetupOutUnindexed";

    static RxOutputSpec N1outputs[] = {
        { _ImmRenderSetupOut, N1outcl1, rxCLVALID_INVALID },
        { _ImmRenderSetupOutUnindexed, N1outcl2, rxCLVALID_INVALID },
    };

    static RwChar _ImmRenderSetup_csl[] = "ImmRenderSetup.csl";

    static RxNodeDefinition nodeImmRenderSetupCSL = {
        _ImmRenderSetup_csl,
        { ImmRenderSetupNode, NULL, NULL, NULL, NULL, NULL, NULL },
        { 6, N1clofinterest, N1inputreqs, 2, N1outputs },
        0,
        0,
        0,
    };

    return &nodeImmRenderSetupCSL;
}
