/*
 * PostLight.csl: clamps the lit camera space vertex colours to [0, 255]
 * (optionally modulating them by the material colour first) and copies
 * them into the screen space vertices, skipping vertices as directed by
 * the optional vertex steps cluster.
 */

#include "rwsdk/rpworld_int.h"
#include "rwsdk/p2core_int.h"

extern RxClusterDefinition RxClMeshState;
extern RxClusterDefinition RxClCamSpace3DVertices;
extern RxClusterDefinition RxClScrSpace2DVertices;
extern RxClusterDefinition RxClVSteps;

typedef struct RwRGBAReal {
    RwReal red;
    RwReal green;
    RwReal blue;
    RwReal alpha;
} RwRGBAReal;

typedef struct RxCamSpace3DVertex {
    /* 0x00 */ RwV3d cameraVertex;
    /* 0x0C */ RwUInt8 clipFlags;
    /* 0x0D */ RwUInt8 pad[3];
    /* 0x10 */ RwRGBAReal col;
    /* 0x20 */ RwReal u;
    /* 0x24 */ RwReal v;
} RxCamSpace3DVertex;

typedef struct RxScrSpace2DVertex {
    /* 0x00 */ RwReal x;
    /* 0x04 */ RwReal y;
    /* 0x08 */ RwReal z;
    /* 0x0C */ RwRGBA color;
} RxScrSpace2DVertex;

typedef union RwSplitFloat {
    RwReal f;
    RwUInt32 i;
} RwSplitFloat;

static const RwSplitFloat PostLightMaxColor = { 255.0f };

#define RxClusterResetCursor(_cl) ((_cl)->currentData = (_cl)->data)

#define RxClusterLockRead(_packet, _clusterIndex)                                                                      \
    (((RwInt32)((_packet)->inputToClusterSlot[(_clusterIndex)]) == -1)                                                 \
         ? (RxCluster*)NULL                                                                                            \
         : (RxClusterResetCursor(&(_packet)->clusters[(_packet)->inputToClusterSlot[(_clusterIndex)]]),               \
            &(_packet)->clusters[(_packet)->inputToClusterSlot[(_clusterIndex)]]))

#define RxClusterIncCursor(_cl) ((_cl)->currentData = (RwUInt8*)(_cl)->currentData + (_cl)->stride)

/* Clamps a non negative float to max by comparing the bit patterns; negative
 * values (sign bit set) clamp to zero. */
#define POSTLIGHTCLAMP(_f, _max)                                                                                       \
    do {                                                                                                               \
        if (*(volatile RwUInt32*)&(_f) > (_max)) {                                                                     \
            *(volatile RwUInt32*)&(_f) = (_max) & ~((RwInt32)(*(volatile RwUInt32*)&(_f)) >> 31);                      \
        }                                                                                                              \
    } while (0)

#define POSTLIGHTSKIP(_steps, _stepInc, _camVerts, _devVerts)                                                          \
    do {                                                                                                               \
        RwUInt32 _n = *(_steps);                                                                                       \
                                                                                                                       \
        (_steps) += (_stepInc);                                                                                        \
        while (_n--) {                                                                                                 \
            RxClusterIncCursor(_camVerts);                                                                             \
            RxClusterIncCursor(_devVerts);                                                                             \
        }                                                                                                              \
    } while (0)

RwBool PostLightNode(RxPipelineNodeInstance* self, const RxPipelineNodeParam* params)
{
    RxPacket* packet = RxPacketFetch(self);
    RxCluster* meshState = RxClusterLockRead(packet, 0);
    RxMeshStateVector* meshData = RxClusterGetCursorData(meshState, RxMeshStateVector);

    if (meshData->ClipFlagsAnd == 0) {
        RwUInt8 oneStep = 1;
        RwRGBA matCol = meshData->MatCol;
        RxCluster* camVerts = RxClusterLockWrite(packet, 1, self);
        RxCluster* devVerts = RxClusterLockWrite(packet, 2, self);
        RxCluster* vSteps = RxClusterLockRead(packet, 3);
        RwUInt8* steps;
        RwInt32 stepInc;
        RwInt32 numVerts;

        if (vSteps && vSteps->numUsed) {
            RwUInt32 n;

            numVerts = vSteps->numUsed;
            steps = (RwUInt8*)vSteps->data;
            stepInc = 1;
            for (n = *steps; n; n--) {
                RxClusterIncCursor(camVerts);
                RxClusterIncCursor(devVerts);
            }
            steps++;
        } else {
            numVerts = meshData->NumVertices;
            steps = &oneStep;
            stepInc = 0;
        }

        if (!(meshData->Flags & rxGEOMETRY_MODULATE) || *(RwUInt32*)&matCol == 0xFFFFFFFF) {
            RwUInt32 max = PostLightMaxColor.i;

            while (numVerts--) {
                RxCamSpace3DVertex* cv = RxClusterGetCursorData(camVerts, RxCamSpace3DVertex);
                RxScrSpace2DVertex* dv = RxClusterGetCursorData(devVerts, RxScrSpace2DVertex);

                POSTLIGHTCLAMP(cv->col.red, max);
                POSTLIGHTCLAMP(cv->col.green, max);
                POSTLIGHTCLAMP(cv->col.blue, max);
                dv->color.red = (RwUInt8)cv->col.red;
                dv->color.green = (RwUInt8)cv->col.green;
                dv->color.blue = (RwUInt8)cv->col.blue;
                dv->color.alpha = (RwUInt8)cv->col.alpha;
                POSTLIGHTSKIP(steps, stepInc, camVerts, devVerts);
            }
        } else {
            RwUInt32 max = PostLightMaxColor.i;
            RwReal red = (1.0f / 255.0f) * matCol.red;
            RwReal green = (1.0f / 255.0f) * matCol.green;
            RwReal blue = (1.0f / 255.0f) * matCol.blue;
            RwReal alpha = (1.0f / 255.0f) * matCol.alpha;

            while (numVerts--) {
                RxCamSpace3DVertex* cv = RxClusterGetCursorData(camVerts, RxCamSpace3DVertex);
                RxScrSpace2DVertex* dv = RxClusterGetCursorData(devVerts, RxScrSpace2DVertex);

                cv->col.red *= red;
                cv->col.green *= green;
                cv->col.blue *= blue;
                cv->col.alpha *= alpha;
                POSTLIGHTCLAMP(cv->col.red, max);
                POSTLIGHTCLAMP(cv->col.green, max);
                POSTLIGHTCLAMP(cv->col.blue, max);
                POSTLIGHTCLAMP(cv->col.alpha, max);
                dv->color.red = (RwUInt8)cv->col.red;
                dv->color.green = (RwUInt8)cv->col.green;
                dv->color.blue = (RwUInt8)cv->col.blue;
                dv->color.alpha = (RwUInt8)cv->col.alpha;
                POSTLIGHTSKIP(steps, stepInc, camVerts, devVerts);
            }
        }
    }

    RxPacketDispatch(packet, 0, self);

    return TRUE;
}

RxNodeDefinition* RxNodeDefinitionGetPostLight(void)
{
    static RxClusterRef N1clofinterest[] = {
        { &RxClMeshState, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClCamSpace3DVertices, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClScrSpace2DVertices, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClVSteps, rxCLALLOWABSENT, rxCLRESERVED },
    };

    static RxClusterValidityReq N1inputreqs[] = {
        rxCLREQ_REQUIRED, rxCLREQ_REQUIRED, rxCLREQ_REQUIRED, rxCLREQ_OPTIONAL,
    };

    static RxClusterValid N1outcl1[] = {
        rxCLVALID_NOCHANGE, rxCLVALID_VALID, rxCLVALID_VALID, rxCLVALID_NOCHANGE,
    };

    static RwChar _Output[] = "Output";

    static RxOutputSpec N1outputs[] = { { _Output, N1outcl1, rxCLVALID_NOCHANGE } };

    static RwChar _PostLight_csl[] = "PostLight.csl";

    static RxNodeDefinition nodePostLightCSL = {
        _PostLight_csl,
        { PostLightNode, NULL, NULL, NULL, NULL, NULL, NULL },
        { 4, N1clofinterest, N1inputreqs, 1, N1outputs },
        0,
        0,
        0,
    };

    return &nodePostLightCSL;
}
