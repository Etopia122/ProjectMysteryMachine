/*
 * WorldSectorEnumerateLights.csl: gathers the lights that affect a world
 * sector (the world's directional lights plus the sector's local lights)
 * into the packet's lights cluster.
 */

#include "rwsdk/rpworld_int.h"

extern RxClusterDefinition RxClLights;

RwBool _rpWorldForAllGlobalLights(RpLightCallBack callBack, void* data);
RpWorldSector* _rpWorldSectorForAllLocalLights(RpWorldSector* worldSector, RpLightCallBack callBack, void* data);

static inline RxCluster* LightsClusterAppend(RxCluster* lightsCl, RpLight* light) {
    if (lightsCl->numUsed == lightsCl->numAlloced) {
        lightsCl = RxClusterResize(lightsCl, lightsCl->numUsed + 8);
    }

    *(RpLight**)((RwUInt8*)lightsCl->data + lightsCl->stride * lightsCl->numUsed) = light;
    lightsCl->numUsed++;

    return lightsCl;
}

RpLight* WorldSectorEnumerateLight(RpLight* light, void* data) {
    if (light->object.object.flags & rpLIGHTLIGHTWORLD) {
        LightsClusterAppend((RxCluster*)data, light);
    }

    return light;
}

RwBool WorldSectorEnumerateLightsNode(RxPipelineNodeInstance* self, const RxPipelineNodeParam* params) {
    RxCluster* lightsCl;
    RpWorldSector* worldSector;

    lightsCl = RxClusterLockWrite(RxPacketFetch(self), 0, self);
    lightsCl = RxClusterInitializeData(lightsCl, 8, sizeof(RpLight*));
    worldSector = (RpWorldSector*)params->dataParam;

    if (((RpWorld*)RWSRCGLOBAL(curWorld))->flags & rpWORLDLIGHT) {
        _rpWorldForAllGlobalLights(WorldSectorEnumerateLight, lightsCl);
        _rpWorldSectorForAllLocalLights(worldSector, WorldSectorEnumerateLight, lightsCl);
    }

    {
        RxPipeline* curPipeline = _rxExecCtxGlobal.pipeline;

        if (_rxExecCtxGlobal.exitCode != FALSE) {
            RxPipelineNode* nextNode = _rxEmbeddedPacketBetweenNodes(curPipeline, self, 0);

            if (nextNode) {
                RwUInt32 exitCode = nextNode->nodeDef->nodeMethods.nodeBody(nextNode, &_rxExecCtxGlobal.params);

                if (!exitCode) {
                    _rxExecCtxGlobal.exitCode = exitCode;
                }
            }
        }

        if (curPipeline->embeddedPacketState > rxPKST_UNUSED) {
            curPipeline->embeddedPacketState = rxPKST_INUSE;
            _rxPacketDestroy(curPipeline->embeddedPacket);
        }
    }

    return TRUE;
}

RxNodeDefinition* RxNodeDefinitionGetWorldSectorEnumerateLights(void) {
    static RxClusterRef N1clofinterest[] = { { &RxClLights, rxCLALLOWABSENT, rxCLRESERVED } };

#pragma explicit_zero_data on
    static RxClusterValidityReq N1inputreqs[] = { rxCLREQ_DONTWANT };
#pragma explicit_zero_data reset

    static RxClusterValid N1outcl1[] = { rxCLVALID_VALID };

    static RwChar _DefaultOutput[] = "DefaultOutput";

    static RxOutputSpec N1outputs[] = { { _DefaultOutput, N1outcl1, rxCLVALID_NOCHANGE } };

    static RwChar _WorldSectorEnumerateLights_csl[] = "WorldSectorEnumerateLights.csl";

    static RxNodeDefinition nodeWorldSectorEnumerateLightsCSL = {
        _WorldSectorEnumerateLights_csl,
        { WorldSectorEnumerateLightsNode, NULL, NULL, NULL, NULL, NULL, NULL },
        { 1, N1clofinterest, N1inputreqs, 1, N1outputs },
        0,
        0,
        0,
    };

    return &nodeWorldSectorEnumerateLightsCSL;
}
