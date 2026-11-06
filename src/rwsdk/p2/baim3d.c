/*
 * Immediate mode 3D: vertices are stashed by RwIm3DTransform and pushed
 * through the current transform pipeline; RwIm3DRender[Indexed]Primitive
 * then runs the render pipeline for the primitive type over the stash.
 * Generic triangle and line render pipelines are built here; platform
 * pipelines override them when the driver provides some.
 */

#include "rwsdk/p2core_int.h"

#define rwPLUGIN_ID 1
#define rwID_COREPLUGIN 1

#define E_RW_IM3DNOTACTIVE 0x23
#define E_RW_INVALIDPRIMTYPE 0x25
#define E_RW_TOOMANYVERTICES 0x32

#define rwPRIMTYPELINELIST 1
#define rwPRIMTYPEPOLYLINE 2
#define rwPRIMTYPETRILIST 3
#define rwPRIMTYPETRISTRIP 4
#define rwPRIMTYPETRIFAN 5
#define rwPRIMTYPEPOINTLIST 6

#define rwIM3D_VERTEXXYZ 0x08
#define rwIM3D_VERTEXRGBA 0x10

typedef RwUInt16 RxVertexIndex;
typedef struct RxHeap RxHeap;
typedef RwUInt32* RxNodeOutput;
typedef RxPipelineNode* RxNodeInput;

typedef struct RxObjSpace3DVertex {
    /* 0x00 */ RwV3d objVertex;
    /* 0x0C */ RwV3d objNormal;
    /* 0x18 */ RwRGBA color;
    /* 0x1C */ RwReal u;
    /* 0x20 */ RwReal v;
} RxObjSpace3DVertex;

typedef struct rwIm3DRenderPipelines {
    /* 0x00 */ RxPipeline* triList;
    /* 0x04 */ RxPipeline* triFan;
    /* 0x08 */ RxPipeline* triStrip;
    /* 0x0C */ RxPipeline* lineList;
    /* 0x10 */ RxPipeline* polyLine;
    /* 0x14 */ RxPipeline* pointList;
} rwIm3DRenderPipelines;

typedef struct rwIm3DPoolStash {
    /* 0x00 */ RwUInt32 flags;
    /* 0x04 */ RwMatrix* ltm;
    /* 0x08 */ RwUInt32 numVerts;
    /* 0x0C */ RxObjSpace3DVertex* objVerts;
    /* 0x10 */ void* camVerts;
    /* 0x14 */ void* devVerts;
    /* 0x18 */ void* meshState;
    /* 0x1C */ RwUInt32 pad1C;
    /* 0x20 */ RxPipeline* pipeline;
    /* 0x24 */ RwInt32 primType;
    /* 0x28 */ RxVertexIndex* indices;
    /* 0x2C */ RwUInt32 numIndices;
} rwIm3DPoolStash;

typedef struct rwIm3DPool {
    /* 0x00 */ RwUInt16 numElements;
    /* 0x02 */ RwUInt16 pad;
    /* 0x04 */ void* elements;
    /* 0x08 */ RwInt32 stride;
    /* 0x0C */ rwIm3DPoolStash stash;
} rwIm3DPool;

typedef struct rwImmGlobals {
    /* 0x00 */ RxPipeline* genericIm3DTransformPipeline;
    /* 0x04 */ rwIm3DRenderPipelines genericIm3DRenderPipelines;
    /* 0x1C */ RxPipeline* im3DTransformPipeline;
    /* 0x20 */ rwIm3DRenderPipelines im3DRenderPipelines;
    /* 0x38 */ RxPipeline* platformIm3DTransformPipeline;
    /* 0x3C */ rwIm3DRenderPipelines platformIm3DRenderPipelines;
    /* 0x54 */ rwIm3DPool curPool;
} rwImmGlobals;

#define RWIMMGLOBAL(var) (RWPLUGINOFFSET(rwImmGlobals, RwEngineInstance, _rwIm3DModule.globalsOffset)->var)

void* memset(void* dst, int c, unsigned long n);

RxHeap* RxHeapGetGlobalHeap(void);
RxPipelineNode* RxPipelineFindNodeByName(RxPipeline* pipeline, const RwChar* name, RxPipelineNode* start,
                                         RwInt32* nodeIndex);
RxNodeInput RxPipelineNodeFindInput(RxPipelineNode* node);
RxNodeOutput RxPipelineNodeFindOutputByName(RxPipelineNode* node, const RwChar* outputname);
RxLockedPipe* RxLockedPipeAddPath(RxLockedPipe* pipeline, RxNodeOutput out, RxNodeInput in);

RxNodeDefinition* RxNodeDefinitionGetImmRenderSetup(void);
RxNodeDefinition* RxNodeDefinitionGetImmMangleTriangleIndices(void);
RxNodeDefinition* RxNodeDefinitionGetCullTriangle(void);
RxNodeDefinition* RxNodeDefinitionGetClipTriangle(void);
RxNodeDefinition* RxNodeDefinitionGetSubmitTriangle(void);
RxNodeDefinition* RxNodeDefinitionGetImmMangleLineIndices(void);
RxNodeDefinition* RxNodeDefinitionGetClipLine(void);
RxNodeDefinition* RxNodeDefinitionGetSubmitLine(void);
RxNodeDefinition* RxNodeDefinitionGetImmInstance(void);
RxNodeDefinition* RxNodeDefinitionGetTransform(void);
RxNodeDefinition* RxNodeDefinitionGetImmStash(void);

RwBool _rwIm3DCreatePlatformTransformPipeline(RxPipeline** globalPipe);
void _rwIm3DDestroyPlatformTransformPipeline(RxPipeline** globalPipe);
RwBool _rwIm3DCreatePlatformRenderPipelines(rwIm3DRenderPipelines* globalPipes);
void _rwIm3DDestroyPlatformRenderPipelines(rwIm3DRenderPipelines* globalPipes);

RwModuleInfo _rwIm3DModule;
rwImmGlobals* lbl_8025F898;

void* RwIm3DTransform(void* pVerts, RwUInt32 numVerts, RwMatrix* ltm, RwUInt32 flags)
{
    if (numVerts > 65536) {
        RWERROR((E_RW_TOOMANYVERTICES));
    } else {
        RWIMMGLOBAL(curPool).numElements = (RwUInt16)numVerts;
        RWIMMGLOBAL(curPool).elements = pVerts;
        RWIMMGLOBAL(curPool).stride = sizeof(RxObjSpace3DVertex);
        RWIMMGLOBAL(curPool).stash.ltm = ltm;
        RWIMMGLOBAL(curPool).stash.flags = flags | rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA;

        if (RxPipelineExecute(RWIMMGLOBAL(im3DTransformPipeline), &RWIMMGLOBAL(curPool), TRUE)) {
            return pVerts;
        }
    }
    return NULL;
}

RwBool RwIm3DEnd(void)
{
    if (!RWIMMGLOBAL(curPool).elements) {
        return FALSE;
    }
    memset(&RWIMMGLOBAL(curPool), 0, sizeof(rwIm3DPool));
    return TRUE;
}

RwBool RwIm3DRenderIndexedPrimitive(RwInt32 primType, RxVertexIndex* indices, RwInt32 numIndices)
{
    if (RWIMMGLOBAL(curPool).elements) {
        rwIm3DPoolStash* stash = &RWIMMGLOBAL(curPool).stash;

        stash->pipeline = NULL;
        stash->primType = primType;
        stash->indices = indices;
        stash->numIndices = numIndices;

        switch (primType) {
        case rwPRIMTYPETRILIST:
            stash->pipeline = RWIMMGLOBAL(im3DRenderPipelines).triList;
            stash->numIndices = numIndices - numIndices % 3;
            break;
        case rwPRIMTYPETRIFAN:
            stash->pipeline = RWIMMGLOBAL(im3DRenderPipelines).triFan;
            break;
        case rwPRIMTYPETRISTRIP:
            stash->pipeline = RWIMMGLOBAL(im3DRenderPipelines).triStrip;
            break;
        case rwPRIMTYPELINELIST:
            stash->pipeline = RWIMMGLOBAL(im3DRenderPipelines).lineList;
            stash->numIndices = numIndices - numIndices % 2;
            break;
        case rwPRIMTYPEPOLYLINE:
            stash->pipeline = RWIMMGLOBAL(im3DRenderPipelines).polyLine;
            break;
        default:
            RWERROR((E_RW_INVALIDPRIMTYPE, primType));
            break;
        }

        if (RxPipelineExecute(stash->pipeline, stash, FALSE)) {
            return TRUE;
        }
    } else {
        RWERROR((E_RW_IM3DNOTACTIVE));
    }
    return FALSE;
}

RwBool RwIm3DRenderPrimitive(RwInt32 primType)
{
    RwBool active = (RWIMMGLOBAL(curPool).elements != NULL);

    RxHeapGetGlobalHeap();

    if (active) {
        rwIm3DPoolStash* stash = &RWIMMGLOBAL(curPool).stash;

        stash->pipeline = NULL;
        stash->primType = primType;
        stash->indices = NULL;
        stash->numIndices = RWIMMGLOBAL(curPool).numElements;

        switch (primType) {
        case rwPRIMTYPETRILIST:
            stash->pipeline = RWIMMGLOBAL(im3DRenderPipelines).triList;
            break;
        case rwPRIMTYPETRIFAN:
            stash->pipeline = RWIMMGLOBAL(im3DRenderPipelines).triFan;
            break;
        case rwPRIMTYPETRISTRIP:
            stash->pipeline = RWIMMGLOBAL(im3DRenderPipelines).triStrip;
            break;
        case rwPRIMTYPELINELIST:
            stash->pipeline = RWIMMGLOBAL(im3DRenderPipelines).lineList;
            break;
        case rwPRIMTYPEPOLYLINE:
            stash->pipeline = RWIMMGLOBAL(im3DRenderPipelines).polyLine;
            break;
        default:
            RWERROR((E_RW_INVALIDPRIMTYPE, primType));
            break;
        }

        if (RxPipelineExecute(stash->pipeline, stash, FALSE)) {
            return TRUE;
        }
    } else {
        RWERROR((E_RW_IM3DNOTACTIVE));
    }
    return FALSE;
}

RxPipeline* RwIm3DSetTransformPipeline(RxPipeline* pipeline)
{
    if (pipeline) {
        RWIMMGLOBAL(im3DTransformPipeline) = pipeline;
    } else {
        rwImmGlobals* globals = RWPLUGINOFFSET(rwImmGlobals, RwEngineInstance, _rwIm3DModule.globalsOffset);

        if (globals->platformIm3DTransformPipeline) {
            globals->im3DTransformPipeline = globals->platformIm3DTransformPipeline;
        } else {
            globals->im3DTransformPipeline = globals->genericIm3DTransformPipeline;
        }
    }
    return RWIMMGLOBAL(im3DTransformPipeline);
}

#define IM3DDEFAULTPIPE(_type)                                                                                         \
    do {                                                                                                               \
        rwImmGlobals* globals = RWPLUGINOFFSET(rwImmGlobals, RwEngineInstance, _rwIm3DModule.globalsOffset);           \
                                                                                                                       \
        if (globals->platformIm3DRenderPipelines._type) {                                                              \
            globals->im3DRenderPipelines._type = globals->platformIm3DRenderPipelines._type;                           \
        } else {                                                                                                       \
            globals->im3DRenderPipelines._type = globals->genericIm3DRenderPipelines._type;                            \
        }                                                                                                              \
    } while (0)

RxPipeline* RwIm3DSetRenderPipeline(RxPipeline* pipeline, RwInt32 primType)
{
    if (pipeline) {
        switch (primType) {
        case rwPRIMTYPETRILIST:
            RWIMMGLOBAL(im3DRenderPipelines).triList = pipeline;
            return pipeline;
        case rwPRIMTYPETRIFAN:
            RWIMMGLOBAL(im3DRenderPipelines).triFan = pipeline;
            return pipeline;
        case rwPRIMTYPETRISTRIP:
            RWIMMGLOBAL(im3DRenderPipelines).triStrip = pipeline;
            return pipeline;
        case rwPRIMTYPELINELIST:
            RWIMMGLOBAL(im3DRenderPipelines).lineList = pipeline;
            return pipeline;
        case rwPRIMTYPEPOLYLINE:
            RWIMMGLOBAL(im3DRenderPipelines).polyLine = pipeline;
            return pipeline;
        case rwPRIMTYPEPOINTLIST:
            RWIMMGLOBAL(im3DRenderPipelines).pointList = pipeline;
            return pipeline;
        default:
            RWERROR((E_RW_INVALIDPRIMTYPE));
            break;
        }
    } else {
        switch (primType) {
        case rwPRIMTYPETRILIST:
            IM3DDEFAULTPIPE(triList);
            return RWIMMGLOBAL(im3DRenderPipelines).triList;
        case rwPRIMTYPETRIFAN:
            IM3DDEFAULTPIPE(triFan);
            return RWIMMGLOBAL(im3DRenderPipelines).triFan;
        case rwPRIMTYPETRISTRIP:
            IM3DDEFAULTPIPE(triStrip);
            return RWIMMGLOBAL(platformIm3DRenderPipelines).triStrip;
        case rwPRIMTYPELINELIST:
            IM3DDEFAULTPIPE(lineList);
            return RWIMMGLOBAL(platformIm3DRenderPipelines).lineList;
        case rwPRIMTYPEPOLYLINE:
            IM3DDEFAULTPIPE(polyLine);
            return RWIMMGLOBAL(platformIm3DRenderPipelines).polyLine;
        case rwPRIMTYPEPOINTLIST:
            IM3DDEFAULTPIPE(pointList);
            return RWIMMGLOBAL(platformIm3DRenderPipelines).pointList;
        default:
            RWERROR((E_RW_INVALIDPRIMTYPE));
            break;
        }
    }
    return NULL;
}

static inline RxPipeline* ImmCreateTriPipeline(void)
{
    RxPipeline* pipe = RxPipelineCreate();

    if (pipe) {
        RxLockedPipe* lpipe;

        pipe->pluginId = rwID_COREPLUGIN;
        lpipe = RxPipelineLock(pipe);
        if (lpipe) {
            RxPipelineNode* setup;
            RxPipelineNode* mangle;

            lpipe = RxLockedPipeAddFragment(lpipe, NULL, RxNodeDefinitionGetImmRenderSetup(),
                                            RxNodeDefinitionGetImmMangleTriangleIndices(),
                                            RxNodeDefinitionGetCullTriangle(), RxNodeDefinitionGetClipTriangle(),
                                            RxNodeDefinitionGetSubmitTriangle(), NULL);
            setup = RxPipelineFindNodeByName(lpipe, "ImmRenderSetup.csl", NULL, NULL);
            mangle = RxPipelineFindNodeByName(lpipe, "ImmMangleTriangleIndices.csl", NULL, NULL);
            pipe = RxLockedPipeUnlock(RxLockedPipeAddPath(
                lpipe, RxPipelineNodeFindOutputByName(setup, "ImmRenderSetupOutUnindexed"), RxPipelineNodeFindInput(mangle)));
            if (pipe) {
                return pipe;
            }
        }
        _rxPipelineDestroy(pipe);
    }
    return NULL;
}

static inline RxPipeline* ImmCreateLinePipeline(void)
{
    RxPipeline* pipe = RxPipelineCreate();

    if (pipe) {
        RxLockedPipe* lpipe;

        pipe->pluginId = rwID_COREPLUGIN;
        lpipe = RxPipelineLock(pipe);
        if (lpipe) {
            RxPipelineNode* setup;
            RxPipelineNode* mangle;

            lpipe = RxLockedPipeAddFragment(lpipe, NULL, RxNodeDefinitionGetImmRenderSetup(),
                                            RxNodeDefinitionGetImmMangleLineIndices(), RxNodeDefinitionGetClipLine(),
                                            RxNodeDefinitionGetSubmitLine(), NULL);
            setup = RxPipelineFindNodeByName(lpipe, "ImmRenderSetup.csl", NULL, NULL);
            mangle = RxPipelineFindNodeByName(lpipe, "ImmMangleLineIndices.csl", NULL, NULL);
            pipe = RxLockedPipeUnlock(RxLockedPipeAddPath(
                lpipe, RxPipelineNodeFindOutputByName(setup, "ImmRenderSetupOutUnindexed"), RxPipelineNodeFindInput(mangle)));
            if (pipe) {
                return pipe;
            }
        }
        _rxPipelineDestroy(pipe);
    }
    return NULL;
}

static inline void ImmDestroyRenderPipelines(void)
{
    RWIMMGLOBAL(im3DRenderPipelines).triList = NULL;
    RWIMMGLOBAL(im3DRenderPipelines).triFan = NULL;
    RWIMMGLOBAL(im3DRenderPipelines).triStrip = NULL;
    if (RWIMMGLOBAL(genericIm3DRenderPipelines).triList) {
        _rxPipelineDestroy(RWIMMGLOBAL(genericIm3DRenderPipelines).triList);
    }
    RWIMMGLOBAL(genericIm3DRenderPipelines).triList = NULL;
    RWIMMGLOBAL(genericIm3DRenderPipelines).triFan = NULL;
    RWIMMGLOBAL(genericIm3DRenderPipelines).triStrip = NULL;

    RWIMMGLOBAL(im3DRenderPipelines).lineList = NULL;
    RWIMMGLOBAL(im3DRenderPipelines).polyLine = NULL;
    if (RWIMMGLOBAL(genericIm3DRenderPipelines).lineList) {
        _rxPipelineDestroy(RWIMMGLOBAL(genericIm3DRenderPipelines).lineList);
    }
    RWIMMGLOBAL(genericIm3DRenderPipelines).lineList = NULL;
    RWIMMGLOBAL(genericIm3DRenderPipelines).polyLine = NULL;
}

static inline void ImmDestroyTransformPipeline(void)
{
    RWIMMGLOBAL(im3DTransformPipeline) = NULL;
    if (RWIMMGLOBAL(genericIm3DTransformPipeline)) {
        _rxPipelineDestroy(RWIMMGLOBAL(genericIm3DTransformPipeline));
        RWIMMGLOBAL(genericIm3DTransformPipeline) = NULL;
    }
}

static RwBool ImmCreateRenderPipelines(void)
{
    rwIm3DRenderPipelines* generic = &RWIMMGLOBAL(genericIm3DRenderPipelines);
    RxPipeline* triPipe;
    RxPipeline* linePipe;

    generic->triList = NULL;
    generic->triFan = NULL;
    generic->triStrip = NULL;
    generic->lineList = NULL;
    generic->polyLine = NULL;
    generic->pointList = NULL;

    triPipe = ImmCreateTriPipeline();
    linePipe = ImmCreateLinePipeline();

    if (triPipe && linePipe) {
        rwIm3DRenderPipelines* current;

        generic->triList = triPipe;
        generic->triFan = triPipe;
        generic->triStrip = triPipe;
        generic->lineList = linePipe;
        generic->polyLine = linePipe;

        current = &RWIMMGLOBAL(im3DRenderPipelines);
        current->triList = triPipe;
        current->triFan = triPipe;
        current->triStrip = triPipe;
        current->lineList = linePipe;
        current->polyLine = linePipe;
        current->pointList = NULL;
        return TRUE;
    }

    ImmDestroyRenderPipelines();
    return FALSE;
}

void* _rwIm3DClose(void* instance)
{
    _rwIm3DDestroyPlatformRenderPipelines(&RWIMMGLOBAL(platformIm3DRenderPipelines));
    _rwIm3DDestroyPlatformTransformPipeline(&RWIMMGLOBAL(platformIm3DTransformPipeline));
    ImmDestroyRenderPipelines();
    ImmDestroyTransformPipeline();
    _rwIm3DModule.numInstances--;
    return instance;
}

static inline RwBool ImmCreateTransformPipeline(void)
{
    RxPipeline* pipe = RxPipelineCreate();

    if (pipe) {
        RxLockedPipe* lpipe;

        pipe->pluginId = rwID_COREPLUGIN;
        lpipe = RxPipelineLock(pipe);
        if (lpipe) {
            lpipe = RxLockedPipeAddFragment(lpipe, NULL, RxNodeDefinitionGetImmInstance(),
                                            RxNodeDefinitionGetTransform(), RxNodeDefinitionGetImmStash(), NULL);
            pipe = RxLockedPipeUnlock(lpipe);
            if (pipe) {
                RWIMMGLOBAL(genericIm3DTransformPipeline) = pipe;
                RWIMMGLOBAL(im3DTransformPipeline) = pipe;
                return TRUE;
            }
        }
        _rxPipelineDestroy(pipe);
    }
    return FALSE;
}

void* _rwIm3DOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    RwBool result;

    _rwIm3DModule.globalsOffset = offset;
    lbl_8025F898 = RWPLUGINOFFSET(rwImmGlobals, RwEngineInstance, offset);
    _rwIm3DModule.numInstances++;
    memset(RWPLUGINOFFSET(rwImmGlobals, RwEngineInstance, offset), 0, sizeof(rwImmGlobals));

    result = ImmCreateTransformPipeline();
    if (result) {
        result = ImmCreateRenderPipelines();
    }
    if (result) {
        result = _rwIm3DCreatePlatformTransformPipeline(&RWIMMGLOBAL(platformIm3DTransformPipeline));
    }
    if (result) {
        result = _rwIm3DCreatePlatformRenderPipelines(&RWIMMGLOBAL(platformIm3DRenderPipelines));
    }
    if (result) {
        return instance;
    }

    _rwIm3DDestroyPlatformRenderPipelines(&RWIMMGLOBAL(platformIm3DRenderPipelines));
    _rwIm3DDestroyPlatformTransformPipeline(&RWIMMGLOBAL(platformIm3DTransformPipeline));
    ImmDestroyRenderPipelines();
    ImmDestroyTransformPipeline();
    _rwIm3DModule.numInstances--;
    return NULL;
}
