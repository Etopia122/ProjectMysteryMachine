/*
 * AtomicEnumerateLights.csl: gathers the lights that affect an atomic (the
 * world's directional lights plus the local lights of every sector the
 * atomic touches) into the packet's lights cluster.
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

RpLight* AtomicEnumerateGlobalLight(RpLight* light, void* data) {
    if (light->object.object.flags & rpLIGHTLIGHTATOMICS) {
        LightsClusterAppend((RxCluster*)data, light);
    }

    return light;
}

RpLight* AtomicEnumerateLocalLight(RpLight* light, void* data) {
    if (light->lightFrame != RWSRCGLOBAL(lightFrame)) {
        if (light->object.object.flags & rpLIGHTLIGHTATOMICS) {
            LightsClusterAppend((RxCluster*)data, light);
        }

        light->lightFrame = RWSRCGLOBAL(lightFrame);
    }

    return light;
}

RwBool AtomicEnumerateLightsNode(RxPipelineNodeInstance* self, const RxPipelineNodeParam* params) {
    RpAtomic* atomic = (RpAtomic*)params->dataParam;
    RxCluster* lightsCl;

    lightsCl = RxClusterLockWrite(RxPacketFetch(self), 0, self);
    lightsCl = RxClusterInitializeData(lightsCl, 8, sizeof(RpLight*));

    if ((atomic->geometry->flags & rpGEOMETRYLIGHT) && RWSRCGLOBAL(curWorld)) {
        RwLLLink* cur;
        RwLLLink* end;

        RWSRCGLOBAL(lightFrame)++;

        _rpWorldForAllGlobalLights(AtomicEnumerateGlobalLight, lightsCl);

        cur = rwLinkListGetFirstLLLink(&atomic->llWorldSectorsInAtomic);
        end = rwLinkListGetTerminator(&atomic->llWorldSectorsInAtomic);
        while (cur != end) {
            RpTie* tie = rwLLLinkGetData(cur, RpTie, lWorldSectorInAtomic);

            _rpWorldSectorForAllLocalLights(tie->worldSector, AtomicEnumerateLocalLight, lightsCl);
            cur = rwLLLinkGetNext(cur);
        }
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

RxNodeDefinition* RxNodeDefinitionGetAtomicEnumerateLights(void) {
    static RxClusterRef N1clofinterest[] = { { &RxClLights, rxCLALLOWABSENT, rxCLRESERVED } };

#pragma explicit_zero_data on
    static RxClusterValidityReq N1inputreqs[] = { rxCLREQ_DONTWANT };
#pragma explicit_zero_data reset

    static RxClusterValid N1outcl1[] = { rxCLVALID_VALID };

    static RwChar _DefaultOutput[] = "DefaultOutput";

    static RxOutputSpec N1outputs[] = { { _DefaultOutput, N1outcl1, rxCLVALID_NOCHANGE } };

    static RwChar _AtomicEnumerateLights_csl[] = "AtomicEnumerateLights.csl";

    static RxNodeDefinition nodeAtomicEnumerateLightsCSL = {
        _AtomicEnumerateLights_csl,
        { AtomicEnumerateLightsNode, NULL, NULL, NULL, NULL, NULL, NULL },
        { 1, N1clofinterest, N1inputreqs, 1, N1outputs },
        0,
        0,
        0,
    };

    return &nodeAtomicEnumerateLightsCSL;
}
