/*
 * World pipelines: creation of the generic material, atomic and world
 * sector pipelines and selection of the current default pipelines.
 */

#include "rwsdk/p2core_int.h"

#define rwPLUGIN_ID 2 /* world */

#define rwID_WORLDPLUGIN 2

RxNodeDefinition* RxNodeDefinitionGetTransform(void);
RxNodeDefinition* RxNodeDefinitionGetCullTriangle(void);
RxNodeDefinition* RxNodeDefinitionGetLight(void);
RxNodeDefinition* RxNodeDefinitionGetPostLight(void);
RxNodeDefinition* RxNodeDefinitionGetClipTriangle(void);
RxNodeDefinition* RxNodeDefinitionGetSubmitTriangle(void);
RxNodeDefinition* RxNodeDefinitionGetAtomicInstance(void);
RxNodeDefinition* RxNodeDefinitionGetAtomicEnumerateLights(void);
RxNodeDefinition* RxNodeDefinitionGetMaterialScatter(void);
RxNodeDefinition* RxNodeDefinitionGetWorldSectorInstance(void);
RxNodeDefinition* RxNodeDefinitionGetWorldSectorEnumerateLights(void);

RwBool _rpCreatePlatformMaterialPipelines(void);
void _rpDestroyPlatformMaterialPipelines(void);
RwBool _rpCreatePlatformAtomicPipelines(void);
void _rpDestroyPlatformAtomicPipelines(void);
RwBool _rpCreatePlatformWorldSectorPipelines(void);
void _rpDestroyPlatformWorldSectorPipelines(void);

RxPipeline* RpWorldSetDefaultSectorPipeline(RxPipeline* pipeline) {
    if (!pipeline) {
        if (RXPIPELINEGLOBAL(platformWorldSectorPipeline)) {
            pipeline = RXPIPELINEGLOBAL(platformWorldSectorPipeline);
        } else {
            pipeline = RXPIPELINEGLOBAL(genericWorldSectorPipeline);
        }
    }

    RXPIPELINEGLOBAL(currentWorldSectorPipeline) = pipeline;
    return pipeline;
}

RxPipeline* RpAtomicSetDefaultPipeline(RxPipeline* pipeline) {
    if (!pipeline) {
        if (RXPIPELINEGLOBAL(platformAtomicPipeline)) {
            pipeline = RXPIPELINEGLOBAL(platformAtomicPipeline);
        } else {
            pipeline = RXPIPELINEGLOBAL(genericAtomicPipeline);
        }
    }

    RXPIPELINEGLOBAL(currentAtomicPipeline) = pipeline;
    return pipeline;
}

static inline void WorldSectorPipelineDestroy(void) {
    RXPIPELINEGLOBAL(currentWorldSectorPipeline) = NULL;

    if (RXPIPELINEGLOBAL(genericWorldSectorPipeline)) {
        _rxPipelineDestroy(RXPIPELINEGLOBAL(genericWorldSectorPipeline));
        RXPIPELINEGLOBAL(genericWorldSectorPipeline) = NULL;
    }
}

static inline void AtomicPipelineDestroy(void) {
    RXPIPELINEGLOBAL(currentAtomicPipeline) = NULL;

    if (RXPIPELINEGLOBAL(genericAtomicPipeline)) {
        _rxPipelineDestroy(RXPIPELINEGLOBAL(genericAtomicPipeline));
        RXPIPELINEGLOBAL(genericAtomicPipeline) = NULL;
    }
}

static inline void MaterialPipelineDestroy(void) {
    RXPIPELINEGLOBAL(currentMaterialPipeline) = NULL;

    if (RXPIPELINEGLOBAL(genericMaterialPipeline)) {
        _rxPipelineDestroy(RXPIPELINEGLOBAL(genericMaterialPipeline));
        RXPIPELINEGLOBAL(genericMaterialPipeline) = NULL;
    }
}

void _rpWorldPipelineClose(void) {
    _rpDestroyPlatformWorldSectorPipelines();
    _rpDestroyPlatformAtomicPipelines();
    _rpDestroyPlatformMaterialPipelines();

    WorldSectorPipelineDestroy();
    AtomicPipelineDestroy();
    MaterialPipelineDestroy();
}

static inline RwBool MaterialPipelineCreate(void) {
    RxPipeline* pipe = RxPipelineCreate();

    if (pipe) {
        RxLockedPipe* lpipe;

        pipe->pluginId = rwID_WORLDPLUGIN;

        lpipe = RxPipelineLock(pipe);
        if (lpipe) {
            lpipe = RxLockedPipeAddFragment(lpipe, NULL, RxNodeDefinitionGetTransform(), RxNodeDefinitionGetCullTriangle(), RxNodeDefinitionGetLight(), RxNodeDefinitionGetPostLight(),
                                            RxNodeDefinitionGetClipTriangle(), RxNodeDefinitionGetSubmitTriangle(), NULL);

            pipe = RxLockedPipeUnlock(lpipe);
            if (pipe) {
                RXPIPELINEGLOBAL(genericMaterialPipeline) = pipe;
                RXPIPELINEGLOBAL(currentMaterialPipeline) = pipe;
                return TRUE;
            }
        }

        _rxPipelineDestroy(pipe);
    }

    return FALSE;
}

static inline RwBool AtomicPipelineCreate(void) {
    RxPipeline* pipe = RxPipelineCreate();

    if (pipe) {
        RxLockedPipe* lpipe;

        pipe->pluginId = rwID_WORLDPLUGIN;

        lpipe = RxPipelineLock(pipe);
        if (lpipe) {
            lpipe = RxLockedPipeAddFragment(lpipe, NULL, RxNodeDefinitionGetAtomicInstance(), RxNodeDefinitionGetAtomicEnumerateLights(),
                                            RxNodeDefinitionGetMaterialScatter(), NULL);

            pipe = RxLockedPipeUnlock(lpipe);
            if (pipe) {
                RXPIPELINEGLOBAL(genericAtomicPipeline) = pipe;
                RXPIPELINEGLOBAL(currentAtomicPipeline) = pipe;
                return TRUE;
            }
        }

        _rxPipelineDestroy(pipe);
    }

    return FALSE;
}

static inline RwBool WorldSectorPipelineCreate(void) {
    RxPipeline* pipe = RxPipelineCreate();

    if (pipe) {
        RxLockedPipe* lpipe;

        pipe->pluginId = rwID_WORLDPLUGIN;

        lpipe = RxPipelineLock(pipe);
        if (lpipe) {
            lpipe = RxLockedPipeAddFragment(lpipe, NULL, RxNodeDefinitionGetWorldSectorInstance(), RxNodeDefinitionGetWorldSectorEnumerateLights(), RxNodeDefinitionGetMaterialScatter(), NULL);

            pipe = RxLockedPipeUnlock(lpipe);
            if (pipe) {
                RXPIPELINEGLOBAL(genericWorldSectorPipeline) = pipe;
                RXPIPELINEGLOBAL(currentWorldSectorPipeline) = pipe;
                return TRUE;
            }
        }

        _rxPipelineDestroy(pipe);
    }

    return FALSE;
}

RwBool _rpWorldPipelineOpen(void) {
    RwBool result;

    result = MaterialPipelineCreate();

    if (result) {
        result = AtomicPipelineCreate();
    }

    if (result) {
        result = WorldSectorPipelineCreate();
    }

    if (result) {
        result = _rpCreatePlatformMaterialPipelines();
    }

    if (result) {
        result = _rpCreatePlatformAtomicPipelines();
    }

    if (result) {
        result = _rpCreatePlatformWorldSectorPipelines();
    }

    if (result) {
        return TRUE;
    }

    _rpWorldPipelineClose();
    return FALSE;
}

RwBool fn_8015470C(void) {
    return TRUE;
}
