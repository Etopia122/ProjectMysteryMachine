/*
 * GameCube platform pipelines for world sectors and atomics: each is a single
 * all-in-one node. Materials need no platform pipeline on this target.
 */

#include "rwsdk/rpworld_int.h"

#define rwID_WORLDPLUGIN 2

RwBool _rpDlVtxFmtPluginAttach(void);
RxNodeDefinition* RxNodeDefinitionGetGameCubeWorldSectorAllInOne(void);
RxNodeDefinition* RxNodeDefinitionGetGameCubeAtomicAllInOne(void);
RxPipeline* RpWorldSetDefaultSectorPipeline(RxPipeline* pipeline);
RxPipeline* RpAtomicSetDefaultPipeline(RxPipeline* pipeline);

RwBool _rpWorldPipeAttach(void)
{
    return _rpDlVtxFmtPluginAttach() != 0;
}

RwBool _rpCreatePlatformMaterialPipelines(void)
{
    return TRUE;
}

void _rpDestroyPlatformMaterialPipelines(void)
{
}

RwBool _rpCreatePlatformWorldSectorPipelines(void)
{
    RxPipeline* pipe;
    RxLockedPipe* lpipe;

    pipe = RxPipelineCreate();
    if (pipe != NULL) {
        pipe->pluginId = rwID_WORLDPLUGIN;
        lpipe = RxPipelineLock(pipe);
        if (lpipe != NULL) {
            lpipe = RxLockedPipeAddFragment(lpipe, NULL, RxNodeDefinitionGetGameCubeWorldSectorAllInOne(), NULL);
            if (RxLockedPipeUnlock(lpipe) != NULL) {
                RXPIPELINEGLOBAL(platformWorldSectorPipeline) = pipe;
                RpWorldSetDefaultSectorPipeline(pipe);
                return TRUE;
            }
        }
        _rxPipelineDestroy(pipe);
    }
    return FALSE;
}

void _rpDestroyPlatformWorldSectorPipelines(void)
{
    RpWorldSetDefaultSectorPipeline(NULL);

    if (RXPIPELINEGLOBAL(platformWorldSectorPipeline) != NULL) {
        _rxPipelineDestroy(RXPIPELINEGLOBAL(platformWorldSectorPipeline));
        RXPIPELINEGLOBAL(platformWorldSectorPipeline) = NULL;
    }
}

RwBool _rpCreatePlatformAtomicPipelines(void)
{
    RxPipeline* pipe;
    RxLockedPipe* lpipe;

    pipe = RxPipelineCreate();
    if (pipe != NULL) {
        pipe->pluginId = rwID_WORLDPLUGIN;
        lpipe = RxPipelineLock(pipe);
        if (lpipe != NULL) {
            lpipe = RxLockedPipeAddFragment(lpipe, NULL, RxNodeDefinitionGetGameCubeAtomicAllInOne(), NULL);
            if (RxLockedPipeUnlock(lpipe) != NULL) {
                RXPIPELINEGLOBAL(platformAtomicPipeline) = pipe;
                RpAtomicSetDefaultPipeline(pipe);
                return TRUE;
            }
        }
        _rxPipelineDestroy(pipe);
    }
    return FALSE;
}

void _rpDestroyPlatformAtomicPipelines(void)
{
    RpAtomicSetDefaultPipeline(NULL);

    if (RXPIPELINEGLOBAL(platformAtomicPipeline) != NULL) {
        _rxPipelineDestroy(RXPIPELINEGLOBAL(platformAtomicPipeline));
        RXPIPELINEGLOBAL(platformAtomicPipeline) = NULL;
    }
}
