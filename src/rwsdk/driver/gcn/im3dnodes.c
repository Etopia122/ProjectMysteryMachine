/*
 * GameCube Im3D nodes: the instance node just records the transform stash, and
 * the unlit submit node streams its vertices straight into the GX FIFO.
 */

#include "rwsdk/p2core_int.h"
#include "rwsdk/rwgx.h"

#define rwPLUGIN_ID 1 /* core */

#define E_RW_INVALIDPRIMTYPE 0x25

#define rwPRIMTYPELINELIST 1
#define rwPRIMTYPEPOLYLINE 2
#define rwPRIMTYPETRILIST 3
#define rwPRIMTYPETRISTRIP 4
#define rwPRIMTYPETRIFAN 5

#define rwIM3D_VERTEXUV 0x01

typedef RwUInt16 RxVertexIndex;

typedef struct RxObjSpace3DVertex {
    /* 0x00 */ RwV3d objVertex;
    /* 0x0C */ RwV3d objNormal;
    /* 0x18 */ RwRGBA color;
    /* 0x1C */ RwReal u;
    /* 0x20 */ RwReal v;
} RxObjSpace3DVertex;

typedef struct rwIm3DPoolStash {
    /* 0x00 */ RwUInt16 numVerts;
    /* 0x04 */ RxObjSpace3DVertex* objVerts;
    /* 0x08 */ RwUInt32 pad08;
    /* 0x0C */ RwUInt32 flags;
    /* 0x10 */ RwMatrix* ltm;
    /* 0x14 */ RwUInt8 pad14[0x30 - 0x14];
    /* 0x30 */ RwInt32 primType;
    /* 0x34 */ RxVertexIndex* indices;
    /* 0x38 */ RwUInt32 numIndices;
} rwIm3DPoolStash;

void _rwDlTransformSetup(RwMatrix* ltm, RwInt32 flags);
void _rwDlRenderStateSetZCompLoc(void);

static GXPrimitive DlPrimTypeMap[] = {
    0, GX_LINES, GX_LINESTRIP, GX_TRIANGLES, GX_TRIANGLESTRIP, GX_TRIANGLEFAN, GX_POINTS,
};

static rwIm3DPoolStash* DlImmStash;

RwBool _rwDlImmInstanceNode(RxPipelineNodeInstance* self, const RxPipelineNodeParam* params)
{
    DlImmStash = (rwIm3DPoolStash*)params->dataParam;
    return TRUE;
}

RxNodeDefinition* RxNodeDefinitionGetGameCubeImmInstance(void)
{
    static RwChar _ImmInstance_csl[] = "ImmInstance.csl";

    static RxNodeDefinition nodeImmInstanceCSL = {
        _ImmInstance_csl,
        { _rwDlImmInstanceNode, NULL, NULL, NULL, NULL, NULL, NULL },
        { 0, NULL, NULL, 0, NULL },
        0,
        0,
        0,
    };

    return &nodeImmInstanceCSL;
}

static inline void DlSubmitSetup(rwIm3DPoolStash* stash)
{
    _rwDlTransformSetup(stash->ltm, 0);

    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXSetNumTevStages(1);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_VTX, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);

    if (stash->flags & rwIM3D_VERTEXUV) {
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
}

#define DLEMITVERTEX(_v)                                                                                               \
    GXPosition3f32((_v)->objVertex.x, (_v)->objVertex.y, (_v)->objVertex.z);                                           \
    GXColor4u8((_v)->color.red, (_v)->color.green, (_v)->color.blue, (_v)->color.alpha)

#define DLEMITVERTEXUV(_v)                                                                                             \
    DLEMITVERTEX(_v);                                                                                                  \
    GXTexCoord2f32((_v)->u, (_v)->v)

RwBool DlSubmitNode(RxPipelineNodeInstance* self, const RxPipelineNodeParam* params)
{
    rwIm3DPoolStash* stash = DlImmStash;
    RxObjSpace3DVertex* verts = stash->objVerts;
    RxVertexIndex* indices;
    RxObjSpace3DVertex* v;
    RwUInt16 n;

    if (stash->indices != NULL) {
        indices = stash->indices;
        DlSubmitSetup(stash);
        GXBegin(DlPrimTypeMap[stash->primType], GX_VTXFMT0, stash->numIndices);

        switch (stash->primType) {
        case rwPRIMTYPEPOLYLINE:
        case rwPRIMTYPETRISTRIP:
        case rwPRIMTYPETRIFAN:
            n = stash->numIndices;
            if (stash->flags & rwIM3D_VERTEXUV) {
                while (n--) {
                    RxObjSpace3DVertex* vert = &verts[*indices++];
                    DLEMITVERTEXUV(vert);
                }
            } else {
                while (n--) {
                    RxObjSpace3DVertex* vert = &verts[*indices++];
                    DLEMITVERTEX(vert);
                }
            }
            break;
        case rwPRIMTYPETRILIST:
            n = stash->numIndices / 3;
            if (stash->flags & rwIM3D_VERTEXUV) {
                while (n--) {
                    v = &verts[indices[0]];
                    DLEMITVERTEXUV(v);
                    v = &verts[indices[1]];
                    DLEMITVERTEXUV(v);
                    v = &verts[indices[2]];
                    indices += 3;
                    DLEMITVERTEXUV(v);
                }
            } else {
                while (n--) {
                    v = &verts[indices[0]];
                    DLEMITVERTEX(v);
                    v = &verts[indices[1]];
                    DLEMITVERTEX(v);
                    v = &verts[indices[2]];
                    indices += 3;
                    DLEMITVERTEX(v);
                }
            }
            break;
        case rwPRIMTYPELINELIST:
            n = stash->numIndices / 2;
            if (stash->flags & rwIM3D_VERTEXUV) {
                while (n--) {
                    v = &verts[indices[0]];
                    DLEMITVERTEXUV(v);
                    v = &verts[indices[1]];
                    indices += 2;
                    DLEMITVERTEXUV(v);
                }
            } else {
                while (n--) {
                    v = &verts[indices[0]];
                    DLEMITVERTEX(v);
                    v = &verts[indices[1]];
                    indices += 2;
                    DLEMITVERTEX(v);
                }
            }
            break;
        default:
            RWERROR((E_RW_INVALIDPRIMTYPE));
            break;
        }
    } else {
        DlSubmitSetup(stash);
        GXBegin(DlPrimTypeMap[stash->primType], GX_VTXFMT0, DlImmStash->numVerts);

        switch (stash->primType) {
        case rwPRIMTYPEPOLYLINE:
        case rwPRIMTYPETRISTRIP:
        case rwPRIMTYPETRIFAN:
            n = DlImmStash->numVerts;
            if (stash->flags & rwIM3D_VERTEXUV) {
                while (n--) {
                    DLEMITVERTEXUV(verts);
                    verts++;
                }
            } else {
                while (n--) {
                    DLEMITVERTEX(verts);
                    verts++;
                }
            }
            break;
        case rwPRIMTYPETRILIST:
            n = DlImmStash->numVerts / 3;
            if (stash->flags & rwIM3D_VERTEXUV) {
                while (n--) {
                    DLEMITVERTEXUV(&verts[0]);
                    DLEMITVERTEXUV(&verts[1]);
                    DLEMITVERTEXUV(&verts[2]);
                    verts += 3;
                }
            } else {
                while (n--) {
                    DLEMITVERTEX(&verts[0]);
                    DLEMITVERTEX(&verts[1]);
                    DLEMITVERTEX(&verts[2]);
                    verts += 3;
                }
            }
            break;
        case rwPRIMTYPELINELIST:
            n = DlImmStash->numVerts >> 1;
            if (stash->flags & rwIM3D_VERTEXUV) {
                while (n--) {
                    DLEMITVERTEXUV(&verts[0]);
                    DLEMITVERTEXUV(&verts[1]);
                    verts += 2;
                }
            } else {
                while (n--) {
                    DLEMITVERTEX(&verts[0]);
                    DLEMITVERTEX(&verts[1]);
                    verts += 2;
                }
            }
            break;
        default:
            RWERROR((E_RW_INVALIDPRIMTYPE));
            break;
        }
    }

    GXEnd();
    return TRUE;
}

RxNodeDefinition* RxNodeDefinitionGetGameCubeSubmitNoLight(void)
{
    static RwChar _SubmitNoLight_csl[] = "SubmitNoLight.csl";

    static RxNodeDefinition nodeSubmitNoLightCSL = {
        _SubmitNoLight_csl,
        { DlSubmitNode, NULL, NULL, NULL, NULL, NULL, NULL },
        { 0, NULL, NULL, 0, NULL },
        0,
        0,
        0,
    };

    return &nodeSubmitNoLightCSL;
}
