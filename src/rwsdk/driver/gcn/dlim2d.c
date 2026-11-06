/*
 * GameCube device layer Im2D: screen space primitives are drawn straight
 * into the GX FIFO with an orthographic projection covering the frame
 * buffer, textured when the render state texture has a raster. The
 * camera's projection and viewport are restored afterwards.
 */

#include "rwsdk/rwcore.h"
#include "rwsdk/rpworld_int.h"
#include "rwsdk/rwgx.h"

#define rwPLUGIN_ID 1

#define E_RW_INVALIDPRIMTYPE 0x25

#define rwPRIMTYPELINELIST 1
#define rwPRIMTYPEPOLYLINE 2
#define rwPRIMTYPETRILIST 3
#define rwPRIMTYPETRISTRIP 4
#define rwPRIMTYPETRIFAN 5

#define GX_ORTHOGRAPHIC 1
#define GX_PNMTX0 0

typedef struct RwIm2DVertex {
    /* 0x00 */ RwReal x;
    /* 0x04 */ RwReal y;
    /* 0x08 */ RwReal z;
    /* 0x0C */ RwRGBA color;
    /* 0x10 */ RwReal u;
    /* 0x14 */ RwReal v;
    /* 0x18 */ RwReal recipZ;
} RwIm2DVertex;

typedef struct GXRenderModeObj {
    /* 0x00 */ RwUInt32 viTVmode;
    /* 0x04 */ RwUInt16 fbWidth;
    /* 0x06 */ RwUInt16 efbHeight;
    /* 0x08 */ RwUInt16 xfbHeight;
    /* 0x0A */ RwUInt16 viXOrigin;
    /* 0x0C */ RwUInt16 viYOrigin;
    /* 0x0E */ RwUInt16 viWidth;
    /* 0x10 */ RwUInt16 viHeight;
    /* 0x14 */ RwUInt32 xFBmode;
    /* 0x18 */ RwUInt8 field_rendering;
} GXRenderModeObj;

extern RwTexture* lbl_8025F87C;
extern GXRenderModeObj* lbl_8025F860;
extern RwInt32 lbl_8025F85C;
extern RwInt32 lbl_8025F808;
extern RwInt32 lbl_8025E7F0;

void _rwDlRenderStateSetZCompLoc(void);

RwUInt32 VIGetNextField(void);
void GXSetViewport(RwReal left, RwReal top, RwReal wd, RwReal ht, RwReal nearz, RwReal farz);
void GXSetViewportJitter(RwReal left, RwReal top, RwReal wd, RwReal ht, RwReal nearz, RwReal farz, RwUInt32 field);
void GXSetScissor(RwUInt32 left, RwUInt32 top, RwUInt32 wd, RwUInt32 ht);
void GXSetScissorBoxOffset(RwInt32 xOffset, RwInt32 yOffset);
void GXSetProjection(RwReal mtx[4][4], RwInt32 type);
void GXSetProjectionv(const RwReal* ptr);
void GXGetProjectionv(RwReal* ptr);
void GXLoadPosMtxImm(RwReal mtx[3][4], RwUInt32 id);
void GXSetCurrentMtx(RwUInt32 id);
void C_MTXOrtho(RwReal m[4][4], RwReal t, RwReal b, RwReal l, RwReal r, RwReal n, RwReal f);

static GXPrimitive DlIm2DPrimTypeMap[] = {
    0, GX_LINES, GX_LINESTRIP, GX_TRIANGLES, GX_TRIANGLESTRIP, GX_TRIANGLEFAN, GX_POINTS,
};

static RwReal DlIm2DIdentity[3][4] = {
    { 1.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 1.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, -1.0f, 0.0f },
};

RwReal lbl_8025E290[7];

#define DLIM2DVERTEX(_v)                                                                                               \
    GXPosition3f32((_v)->x, (_v)->y, (_v)->z);                                                                         \
    GXColor4u8((_v)->color.red, (_v)->color.green, (_v)->color.blue, (_v)->color.alpha)

#define DLIM2DVERTEXUV(_v)                                                                                             \
    DLIM2DVERTEX(_v);                                                                                                  \
    GXTexCoord2f32((_v)->u, (_v)->v)

void _rw2DRenderPrimitiveInit(void)
{
    RwReal ortho[4][4];
    GXRenderModeObj* rm;

    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXSetNumTevStages(1);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_VTX, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);

    if (lbl_8025F87C->raster) {
        GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
        GXSetTevOp(GX_TEVSTAGE0, GX_MODULATE);
        GXSetNumTexGens(1);
        GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
        _rwDlRenderStateSetZCompLoc();
    } else {
        GXSetNumTexGens(0);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
        GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    }

    rm = lbl_8025F860;
    if (rm->field_rendering) {
        GXSetViewportJitter(0.0f, 0.0f, rm->fbWidth, rm->xfbHeight, 0.0f, 1.0f, VIGetNextField() ^ 1);
    } else {
        GXSetViewport(0.0f, 0.0f, rm->fbWidth, rm->xfbHeight, 0.0f, 1.0f);
    }

    if (!lbl_8025F808) {
        GXSetScissor(0, 0, lbl_8025F860->fbWidth, lbl_8025F860->xfbHeight);
    } else if (lbl_8025E7F0) {
        GXSetScissor(0, 0, lbl_8025F860->fbWidth, lbl_8025F85C + 2);
        GXSetScissorBoxOffset(0, 0);
    } else {
        GXSetScissor(0, lbl_8025F85C - 2, lbl_8025F860->fbWidth, lbl_8025F85C + 2);
        GXSetScissorBoxOffset(0, lbl_8025F85C - 2);
    }

    C_MTXOrtho(ortho, 0.0f, lbl_8025F860->xfbHeight - 1, 0.0f, lbl_8025F860->fbWidth - 1, 0.0f, 1.0f);
    GXGetProjectionv(lbl_8025E290);
    GXSetProjection(ortho, GX_ORTHOGRAPHIC);
    GXLoadPosMtxImm(DlIm2DIdentity, GX_PNMTX0);
    GXSetCurrentMtx(GX_PNMTX0);
}

void _rw2DRenderPrimativeTerm(void)
{
    RwRaster* raster = ((RwCamera*)RWSRCGLOBAL(curCamera))->frameBuffer;

    if (raster != raster->parent) {
        if (!lbl_8025F808) {
            if (lbl_8025F860->field_rendering) {
                GXSetViewportJitter(raster->nOffsetX, raster->nOffsetY, raster->width, raster->height, 0.0f, 1.0f,
                                    VIGetNextField() ^ 1);
            } else {
                GXSetViewport(raster->nOffsetX, raster->nOffsetY, raster->width, raster->height, 0.0f, 1.0f);
            }
            GXSetScissor(raster->nOffsetX, raster->nOffsetY, raster->width, raster->height);
        } else {
            if (lbl_8025F860->field_rendering) {
                GXSetViewportJitter(raster->nOffsetX, raster->nOffsetY, raster->width, raster->height, 0.0f, 1.0f,
                                    VIGetNextField() ^ 1);
            } else {
                GXSetViewport(raster->nOffsetX, raster->nOffsetY << 1, raster->width, raster->height << 1, 0.0f,
                              1.0f);
            }

            if (lbl_8025E7F0) {
                if ((raster->nOffsetY + raster->height) << 1 <= lbl_8025F85C + 2) {
                    GXSetScissor(raster->nOffsetX, raster->nOffsetY << 1, lbl_8025F860->fbWidth, raster->height << 1);
                } else if (raster->nOffsetY << 1 > lbl_8025F85C + 2) {
                    GXSetScissor(0, 0, lbl_8025F860->fbWidth, lbl_8025F85C + 2);
                } else {
                    GXSetScissor(raster->nOffsetX, raster->nOffsetY << 1, lbl_8025F860->fbWidth, lbl_8025F85C + 2);
                }
                GXSetScissorBoxOffset(0, 0);
            } else {
                if (raster->nOffsetY << 1 >= lbl_8025F85C - 2) {
                    GXSetScissor(raster->nOffsetX, raster->nOffsetY << 1, lbl_8025F860->fbWidth, raster->height << 1);
                } else if ((raster->nOffsetY + raster->height) << 1 < lbl_8025F85C + 2) {
                    GXSetScissor(0, lbl_8025F85C - 2, lbl_8025F860->fbWidth, lbl_8025F85C + 2);
                } else {
                    GXSetScissor(raster->nOffsetX, lbl_8025F85C - 2, lbl_8025F860->fbWidth, raster->height << 1);
                }
                GXSetScissorBoxOffset(0, lbl_8025F85C - 2);
            }
        }
    }

    GXSetProjectionv(lbl_8025E290);
}

RwBool _rwDlIm2DRenderTriangle(void* vertices, RwInt32 numVertices, RwInt32 vert1, RwInt32 vert2, RwInt32 vert3)
{
    RwIm2DVertex* v1 = (RwIm2DVertex*)vertices + vert1;
    RwIm2DVertex* v2 = (RwIm2DVertex*)vertices + vert2;
    RwIm2DVertex* v3 = (RwIm2DVertex*)vertices + vert3;

    _rw2DRenderPrimitiveInit();

    GXBegin(GX_TRIANGLES, GX_VTXFMT0, 3);
    if (lbl_8025F87C->raster) {
        DLIM2DVERTEXUV(v1);
        DLIM2DVERTEXUV(v2);
        DLIM2DVERTEXUV(v3);
    } else {
        DLIM2DVERTEX(v1);
        DLIM2DVERTEX(v2);
        DLIM2DVERTEX(v3);
    }
    GXEnd();

    _rw2DRenderPrimativeTerm();
    return TRUE;
}

RwBool _rwDlIm2DRenderLine(void* vertices, RwInt32 numVertices, RwInt32 vert1, RwInt32 vert2)
{
    RwIm2DVertex* v1 = (RwIm2DVertex*)vertices + vert1;
    RwIm2DVertex* v2 = (RwIm2DVertex*)vertices + vert2;

    _rw2DRenderPrimitiveInit();

    GXBegin(GX_LINES, GX_VTXFMT0, 2);
    if (lbl_8025F87C->raster) {
        DLIM2DVERTEXUV(v1);
        DLIM2DVERTEXUV(v2);
    } else {
        DLIM2DVERTEX(v1);
        DLIM2DVERTEX(v2);
    }
    GXEnd();

    _rw2DRenderPrimativeTerm();
    return TRUE;
}

RwBool _rwDlIm2DRenderPrimitive(RwInt32 primType, void* vertices, RwInt32 numVertices)
{
    RwIm2DVertex* vert = (RwIm2DVertex*)vertices;
    RwInt32 count;

    _rw2DRenderPrimitiveInit();

    GXBegin(DlIm2DPrimTypeMap[primType], GX_VTXFMT0, numVertices);
    switch (primType) {
    case rwPRIMTYPEPOLYLINE:
    case rwPRIMTYPETRISTRIP:
    case rwPRIMTYPETRIFAN:
        if (lbl_8025F87C->raster) {
            while (numVertices--) {
                DLIM2DVERTEXUV(vert);
                vert++;
            }
        } else {
            while (numVertices--) {
                DLIM2DVERTEX(vert);
                vert++;
            }
        }
        break;
    case rwPRIMTYPELINELIST:
        count = numVertices >> 1;
        if (lbl_8025F87C->raster) {
            while (count--) {
                DLIM2DVERTEXUV(&vert[0]);
                DLIM2DVERTEXUV(&vert[1]);
                vert += 2;
            }
        } else {
            while (count--) {
                DLIM2DVERTEX(&vert[0]);
                DLIM2DVERTEX(&vert[1]);
                vert += 2;
            }
        }
        break;
    case rwPRIMTYPETRILIST:
        count = numVertices / 3;
        if (lbl_8025F87C->raster) {
            while (count--) {
                DLIM2DVERTEXUV(&vert[0]);
                DLIM2DVERTEXUV(&vert[1]);
                DLIM2DVERTEXUV(&vert[2]);
                vert += 3;
            }
        } else {
            while (count--) {
                DLIM2DVERTEX(&vert[0]);
                DLIM2DVERTEX(&vert[1]);
                DLIM2DVERTEX(&vert[2]);
                vert += 3;
            }
        }
        break;
    default:
        RWERROR((E_RW_INVALIDPRIMTYPE));
        break;
    }
    GXEnd();

    _rw2DRenderPrimativeTerm();
    return TRUE;
}

RwBool _rwDlIm2DRenderIndexedPrimitive(RwInt32 primType, void* vertices, RwInt32 numVertices, RwUInt16* indices,
                                       RwInt32 numIndices)
{
    RwIm2DVertex* verts = (RwIm2DVertex*)vertices;
    RwInt32 count;
    RwIm2DVertex* v;

    _rw2DRenderPrimitiveInit();

    GXBegin(DlIm2DPrimTypeMap[primType], GX_VTXFMT0, numIndices);
    switch (primType) {
    case rwPRIMTYPEPOLYLINE:
    case rwPRIMTYPETRISTRIP:
    case rwPRIMTYPETRIFAN:
        if (lbl_8025F87C->raster) {
            while (numIndices--) {
                v = &verts[*indices++];
                DLIM2DVERTEXUV(v);
            }
        } else {
            while (numIndices--) {
                v = &verts[*indices++];
                DLIM2DVERTEX(v);
            }
        }
        break;
    case rwPRIMTYPELINELIST:
        count = numIndices >> 1;
        if (lbl_8025F87C->raster) {
            while (count--) {
                v = &verts[indices[0]];
                DLIM2DVERTEXUV(v);
                v = &verts[indices[1]];
                DLIM2DVERTEXUV(v);
                indices += 2;
            }
        } else {
            while (count--) {
                v = &verts[indices[0]];
                DLIM2DVERTEX(v);
                v = &verts[indices[1]];
                DLIM2DVERTEX(v);
                indices += 2;
            }
        }
        break;
    case rwPRIMTYPETRILIST:
        count = numIndices / 3;
        if (lbl_8025F87C->raster) {
            while (count--) {
                v = &verts[indices[0]];
                DLIM2DVERTEXUV(v);
                v = &verts[indices[1]];
                DLIM2DVERTEXUV(v);
                v = &verts[indices[2]];
                DLIM2DVERTEXUV(v);
                indices += 3;
            }
        } else {
            while (count--) {
                v = &verts[indices[0]];
                DLIM2DVERTEX(v);
                v = &verts[indices[1]];
                DLIM2DVERTEX(v);
                v = &verts[indices[2]];
                DLIM2DVERTEX(v);
                indices += 3;
            }
        }
        break;
    default:
        RWERROR((E_RW_INVALIDPRIMTYPE));
        break;
    }
    GXEnd();

    _rw2DRenderPrimativeTerm();
    return TRUE;
}
