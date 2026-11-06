/*
 * GameCube vertex formats: a reference counted description of the GX
 * component types and fixed point fractions used to instance geometry, held
 * per geometry and per world through plugin slots.
 */

#include "rwsdk/rpworld_int.h"

#define rwID_GAMECUBEVTXFMTPLUGIN 0x511

#define GX_F32 4
#define GX_RGBA8 5

typedef struct RpGameCubeVtxFmt {
    /* 0x00 */ RwUInt8 compType[12];
    /* 0x0C */ RwUInt8 frac[10];
    /* 0x16 */ RwUInt16 refCount;
} RpGameCubeVtxFmt;

#define VTXFMTSLOT(object, offset) (*(RpGameCubeVtxFmt**)((RwUInt8*)(object) + (offset)))

RwInt32 RpGeometryRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB,
                                 RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);
RwInt32 RpWorldRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB,
                              RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);

void RpGameCubeVtxFmtInit(RpGameCubeVtxFmt* fmt);
void RpGameCubeVtxFmtDestroy(RpGameCubeVtxFmt* fmt);

RpGameCubeVtxFmt DlDefaultVtxFmt;

static RwModuleInfo DlVtxFmtModule;
static RwInt32 DlVtxFmtWorldOffset;
static RwInt32 DlVtxFmtGeometryOffset;

RpGameCubeVtxFmt* _rpGameCubeVtxFmtGetDefault(void)
{
    return &DlDefaultVtxFmt;
}

void* _rxDlVertexFmtConst(void* object, RwInt32 offset, RwInt32 size)
{
    VTXFMTSLOT(object, offset) = NULL;
    return object;
}

void* _rxDlVertexFmtDest(void* object, RwInt32 offset, RwInt32 size)
{
    if (VTXFMTSLOT(object, offset) != NULL) {
        RpGameCubeVtxFmtDestroy(VTXFMTSLOT(object, offset));
    }
    return object;
}

void* _rpDlVtxFmtOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    if (++DlVtxFmtModule.numInstances == 1) {
        RpGameCubeVtxFmtInit(&DlDefaultVtxFmt);
    }
    return instance;
}

void* _rpDlVtxFmtClose(void* instance, RwInt32 offset, RwInt32 size)
{
    DlVtxFmtModule.numInstances--;
    return instance;
}

RwBool _rpDlVtxFmtPluginAttach(void)
{
    if (RwEngineRegisterPlugin(0, rwID_GAMECUBEVTXFMTPLUGIN, _rpDlVtxFmtOpen, _rpDlVtxFmtClose) < 0) {
        return FALSE;
    }

    DlVtxFmtGeometryOffset = RpGeometryRegisterPlugin(sizeof(RpGameCubeVtxFmt*), rwID_GAMECUBEVTXFMTPLUGIN,
                                                      _rxDlVertexFmtConst, _rxDlVertexFmtDest, NULL);
    if (DlVtxFmtGeometryOffset < 0) {
        return FALSE;
    }

    DlVtxFmtWorldOffset = RpWorldRegisterPlugin(sizeof(RpGameCubeVtxFmt*), rwID_GAMECUBEVTXFMTPLUGIN,
                                                _rxDlVertexFmtConst, _rxDlVertexFmtDest, NULL);
    if (DlVtxFmtWorldOffset < 0) {
        return FALSE;
    }
    return TRUE;
}

void RpGameCubeVtxFmtSetNormal(RpGameCubeVtxFmt* fmt, RwUInt8 compType, RwUInt8 frac)
{
    fmt->compType[1] = compType;
    fmt->frac[1] = frac;
}

void RpGameCubeVtxFmtInit(RpGameCubeVtxFmt* fmt)
{
    fmt->compType[0] = GX_F32;
    fmt->compType[1] = GX_F32;
    fmt->compType[2] = GX_F32;
    fmt->compType[3] = GX_F32;
    fmt->compType[4] = GX_F32;
    fmt->compType[5] = GX_F32;
    fmt->compType[6] = GX_F32;
    fmt->compType[7] = GX_F32;
    fmt->compType[8] = GX_F32;
    fmt->compType[9] = GX_F32;
    fmt->compType[10] = GX_RGBA8;
    fmt->compType[11] = 1;
    fmt->frac[0] = 0;
    fmt->frac[2] = 0;
    fmt->frac[3] = 0;
    fmt->frac[4] = 0;
    fmt->frac[5] = 0;
    fmt->frac[6] = 0;
    fmt->frac[7] = 0;
    fmt->frac[8] = 0;
    fmt->frac[9] = 0;
    fmt->frac[1] = 0;
    fmt->refCount = 1;
}

RpGameCubeVtxFmt* RpGameCubeVtxFmtCreate(void)
{
    RpGameCubeVtxFmt* fmt = (RpGameCubeVtxFmt*)RwMalloc(sizeof(RpGameCubeVtxFmt));

    RpGameCubeVtxFmtInit(fmt);
    return fmt;
}

void RpGameCubeVtxFmtDestroy(RpGameCubeVtxFmt* fmt)
{
    if (fmt->refCount == 1) {
        RwFree(fmt);
    } else {
        fmt->refCount--;
    }
}

void RpGameCubeGeometrySetVtxFmt(RpGeometry* geometry, RpGameCubeVtxFmt* fmt)
{
    RwInt32 offset = DlVtxFmtGeometryOffset;

    if (VTXFMTSLOT(geometry, offset) != NULL) {
        RpGameCubeVtxFmtDestroy(VTXFMTSLOT(geometry, offset));
    }
    VTXFMTSLOT(geometry, offset) = fmt;
    VTXFMTSLOT(geometry, offset)->refCount++;
}

RpGameCubeVtxFmt* RpGameCubeGeometryGetVtxFmt(RpGeometry* geometry)
{
    return VTXFMTSLOT(geometry, DlVtxFmtGeometryOffset);
}

RpGameCubeVtxFmt* RpGameCubeWorldGetVtxFmt(RpWorld* world)
{
    return VTXFMTSLOT(world, DlVtxFmtWorldOffset);
}
