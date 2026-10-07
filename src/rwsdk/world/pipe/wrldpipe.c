/*
 * World pipelines: creation of the generic material, atomic and world
 * sector pipelines and selection of the current default pipelines.
 */

#include "rwsdk/p2core_int.h"

#define rwPLUGIN_ID 2 /* world */

#define rwID_WORLDPLUGIN 2

RxNodeDefinition* fn_8018490C(void);
RxNodeDefinition* fn_80181EB8(void);
RxNodeDefinition* fn_80155E28(void);
RxNodeDefinition* fn_80157874(void);
RxNodeDefinition* fn_80181640(void);
RxNodeDefinition* fn_80183B04(void);
RxNodeDefinition* fn_80155B5C(void);
RxNodeDefinition* RxNodeDefinitionGetAtomicEnumerateLights(void);
RxNodeDefinition* fn_801571C0(void);
RxNodeDefinition* fn_80158850(void);
RxNodeDefinition* fn_80157A30(void);

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
            lpipe = RxLockedPipeAddFragment(lpipe, NULL, fn_8018490C(), fn_80181EB8(), fn_80155E28(), fn_80157874(),
                                            fn_80181640(), fn_80183B04(), NULL);

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
            lpipe = RxLockedPipeAddFragment(lpipe, NULL, fn_80155B5C(), RxNodeDefinitionGetAtomicEnumerateLights(),
                                            fn_801571C0(), NULL);

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
            lpipe = RxLockedPipeAddFragment(lpipe, NULL, fn_80158850(), fn_80157A30(), fn_801571C0(), NULL);

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
