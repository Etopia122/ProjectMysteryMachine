/*
 * GameCube Im3D pipelines: a transform pipeline built from the immediate mode
 * instance node, and one render pipeline (the unlit submit node) shared by
 * every primitive type.
 */

#include "rwsdk/p2core_int.h"

#define rwID_COREPLUGIN 1

#define rwPRIMTYPELINELIST 1
#define rwPRIMTYPEPOLYLINE 2
#define rwPRIMTYPETRILIST 3
#define rwPRIMTYPETRISTRIP 4
#define rwPRIMTYPETRIFAN 5

typedef struct rwIm3DRenderPipelines {
    /* 0x00 */ RxPipeline* triList;
    /* 0x04 */ RxPipeline* triFan;
    /* 0x08 */ RxPipeline* triStrip;
    /* 0x0C */ RxPipeline* lineList;
    /* 0x10 */ RxPipeline* polyLine;
} rwIm3DRenderPipelines;

RxPipeline* RwIm3DSetTransformPipeline(RxPipeline* pipeline);
RxPipeline* RwIm3DSetRenderPipeline(RxPipeline* pipeline, RwInt32 primType);
RxNodeDefinition* RxNodeDefinitionGetGameCubeImmInstance(void);
RxNodeDefinition* RxNodeDefinitionGetGameCubeSubmitNoLight(void);

RwBool _rwIm3DCreatePlatformTransformPipeline(RxPipeline** globalPipe)
{
    RxPipeline* pipe;
    RxLockedPipe* lpipe;

    pipe = RxPipelineCreate();
    if (pipe != NULL) {
        pipe->pluginId = rwID_COREPLUGIN;
        lpipe = RxPipelineLock(pipe);
        if (lpipe != NULL) {
            lpipe = RxLockedPipeAddFragment(lpipe, NULL, RxNodeDefinitionGetGameCubeImmInstance(), NULL);
            pipe = RxLockedPipeUnlock(lpipe);
            if (pipe != NULL) {
                *globalPipe = pipe;
                RwIm3DSetTransformPipeline(pipe);
                return TRUE;
            }
        }
        _rxPipelineDestroy(pipe);
    }
    return FALSE;
}

void _rwIm3DDestroyPlatformTransformPipeline(RxPipeline** globalPipe)
{
    RwIm3DSetTransformPipeline(NULL);
    if (*globalPipe != NULL) {
        _rxPipelineDestroy(*globalPipe);
        *globalPipe = NULL;
    }
}

void _rwIm3DDestroyPlatformRenderPipelines(rwIm3DRenderPipelines* globalPipes)
{
    RwIm3DSetRenderPipeline(NULL, rwPRIMTYPETRILIST);
    RwIm3DSetRenderPipeline(NULL, rwPRIMTYPETRIFAN);
    RwIm3DSetRenderPipeline(NULL, rwPRIMTYPETRISTRIP);
    RwIm3DSetRenderPipeline(NULL, rwPRIMTYPELINELIST);
    RwIm3DSetRenderPipeline(NULL, rwPRIMTYPEPOLYLINE);

    if (globalPipes->triList != NULL) {
        _rxPipelineDestroy(globalPipes->triList);
    }
    globalPipes->triList = NULL;
    globalPipes->triFan = NULL;
    globalPipes->triStrip = NULL;
    globalPipes->lineList = NULL;
    globalPipes->polyLine = NULL;
}

RwBool _rwIm3DCreatePlatformRenderPipelines(rwIm3DRenderPipelines* globalPipes)
{
    RxPipeline* pipe;
    RxLockedPipe* lpipe;

    pipe = RxPipelineCreate();
    if (pipe != NULL) {
        pipe->pluginId = rwID_COREPLUGIN;
        lpipe = RxPipelineLock(pipe);
        if (lpipe != NULL) {
            lpipe = RxLockedPipeAddFragment(lpipe, NULL, RxNodeDefinitionGetGameCubeSubmitNoLight(), NULL);
            pipe = RxLockedPipeUnlock(lpipe);
            if (pipe != NULL) {
                globalPipes->triList = pipe;
                globalPipes->triFan = pipe;
                globalPipes->triStrip = pipe;
                globalPipes->lineList = pipe;
                globalPipes->polyLine = pipe;
                RwIm3DSetRenderPipeline(pipe, rwPRIMTYPETRILIST);
                RwIm3DSetRenderPipeline(pipe, rwPRIMTYPETRIFAN);
                RwIm3DSetRenderPipeline(pipe, rwPRIMTYPETRISTRIP);
                RwIm3DSetRenderPipeline(pipe, rwPRIMTYPELINELIST);
                RwIm3DSetRenderPipeline(pipe, rwPRIMTYPEPOLYLINE);
                return TRUE;
            }
        }
        _rxPipelineDestroy(pipe);
    }
    return FALSE;
}
