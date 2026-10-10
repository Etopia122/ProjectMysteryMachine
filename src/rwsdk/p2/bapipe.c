/*
 * Render pipeline module glue: opening and closing the pipeline globals,
 * plus the camera and plugin hooks that this platform leaves empty.
 */

#include "rwsdk/p2core_int.h"

RwBool _rxPipelineOpen(void);
RwBool _rxPipelineClose(void);

RwInt32 _rxPipelineGlobalsOffset;

void* _rwRenderPipelineOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    _rxPipelineGlobalsOffset = offset;
    if (!_rxPipelineOpen()) {
        return NULL;
    }
    return instance;
}

void* _rwRenderPipelineClose(void* instance, RwInt32 offset, RwInt32 size)
{
    _rxPipelineClose();
    return instance;
}

RwBool _rwPipeAttach(void)
{
    return TRUE;
}

void _rwPipeInitForCamera(void)
{
}
