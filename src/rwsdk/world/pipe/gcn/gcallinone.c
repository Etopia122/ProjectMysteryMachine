/*
 * GameCube all-in-one pipeline support: waiting on an instanced resource
 * entry's GPU token, lighting channel and TEV setup, the default render path
 * that replays each mesh's display list, and sizing and filling the vertex
 * arrays an atomic or world sector is instanced into.
 */

#include "rwsdk/rpworld_int.h"
#include "rwsdk/rwgx.h"

#define rpATOMIC 1

#define rpGEOMETRYPOSITIONS 0x02

#define rxGEOMETRY_TEXTURED 0x04
#define rxGEOMETRY_PRELIT 0x08
#define rxGEOMETRY_NORMALS 0x10
#define rxGEOMETRY_MODULATE 0x40
#define rxGEOMETRY_TEXTURED2 0x80

#define GX_COLOR1A1 5

#define GX_U8 0
#define GX_S8 1
#define GX_U16 2
#define GX_S16 3

#define GX_VA_NBT 25
#define GX_NRM_XYZ 0
#define GX_NRM_NBT 1
#define GX_CLR_RGB 0

#define GX_RGB565 0
#define GX_RGB8 1
#define GX_RGBX8 2
#define GX_RGBA4 3
#define GX_RGBA6 4

void* memcpy(void* dst, const void* src, RwUInt32 n);
#define GX_DF_CLAMP 2
#define GX_AF_SPOT 1

#define GX_TEVSTAGE1 1
#define GX_CC_CPREV 0
#define GX_CC_TEXC 8
#define GX_CC_RASC 10
#define GX_CC_ONE 12
#define GX_CC_ZERO 15
#define GX_CA_APREV 0
#define GX_CA_ZERO 7
#define GX_TEV_ADD 0
#define GX_TB_ZERO 0
#define GX_CS_SCALE_1 0
#define GX_TEVPREV 0

typedef struct GXColor {
    RwUInt8 r;
    RwUInt8 g;
    RwUInt8 b;
    RwUInt8 a;
} GXColor;

typedef struct RwDlRasterExt {
    /* 0x00 */ RwUInt32 tlutObj[3];
    /* 0x0C */ RwUInt32 format;
    /* 0x10 */ RwUInt32 tlutFormat;
    /* 0x14 */ RwUInt32 flags;
} RwDlRasterExt;

#define rwDLRASTERHASALPHA 0x01

extern RwInt32 _rwDlRasterExtOffset;

#define RASTEREXTFROMRASTER(raster) ((RwDlRasterExt*)((RwUInt8*)(raster) + _rwDlRasterExtOffset))

typedef struct RpMaterial {
    /* 0x00 */ RwTexture* texture;
    /* 0x04 */ RwRGBA color;
    /* 0x08 */ RxPipeline* pipeline;
    /* 0x0C */ RwSurfaceProperties surfaceProps;
} RpMaterial;

struct RpMeshHeader {
    /* 0x00 */ RwUInt32 flags;
    /* 0x04 */ RwUInt16 numMeshes;
    /* 0x06 */ RwUInt16 serialNum;
    /* 0x08 */ RwUInt32 totalIndicesInMesh;
    /* 0x0C */ RwUInt32 firstMeshOffset;
};

typedef struct RpMesh {
    /* 0x00 */ RwUInt16* indices;
    /* 0x04 */ RwUInt32 numIndices;
    /* 0x08 */ RpMaterial* material;
} RpMesh;

typedef struct RpGameCubeVtxFmt {
    /* 0x00 */ RwUInt8 posType;
    /* 0x01 */ RwUInt8 nrmType;
    /* 0x02 */ RwUInt8 texType[8];
    /* 0x0A */ RwUInt8 clrType;
    /* 0x0B */ RwUInt8 flags;
    /* 0x0C */ RwUInt8 posFrac;
    /* 0x0D */ RwUInt8 nbt;
    /* 0x0E */ RwUInt8 texFrac[8];
    /* 0x16 */ RwUInt16 refCount;
} RpGameCubeVtxFmt;

#define rpGCVTXFMTINTERLEAVED 0x02

/* World sector normals are packed into signed bytes. */
typedef struct RpVertexNormal {
    RwInt8 x;
    RwInt8 y;
    RwInt8 z;
    RwUInt8 pad;
} RpVertexNormal;

/* A display list replayed for one mesh. */
typedef struct RxGameCubeDisplayList {
    /* 0x00 */ RwUInt32 size;
    /* 0x04 */ void* list;
} RxGameCubeDisplayList;

/* Resource entry payload of an instanced atomic or world sector. */
typedef struct RxGameCubeInstanceData {
    /* 0x00 */ RwUInt8 unk00[2];
    /* 0x02 */ RwUInt16 descType;
    /* 0x04 */ RwUInt16 token;
    /* 0x06 */ RwUInt8 posStride;
    /* 0x07 */ RwUInt8 nrmStride;
    /* 0x08 */ RwUInt8 clrStride;
    /* 0x09 */ RwUInt8 texStride[8];
    /* 0x11 */ RwUInt8 pad11[7];
    /* 0x18 */ void* posData;
    /* 0x1C */ void* nrmData;
    /* 0x20 */ void* clrData;
    /* 0x24 */ void* texData[8];
    /* 0x44 */ RxGameCubeDisplayList displayLists[1];
} RxGameCubeInstanceData;

/* What the render path needs to know about the object being drawn. */
typedef struct RxGameCubeRenderData {
    /* 0x00 */ RwResEntry* repEntry;
    /* 0x04 */ RpMeshHeader* meshHeader;
    /* 0x08 */ RwUInt32 flags;
    /* 0x0C */ RwReal ambient[4];
    /* 0x1C */ RwBool ambientLight;
    /* 0x20 */ RwUInt32 lightMask0;
    /* 0x24 */ RwUInt32 lightMask1;
    /* 0x28 */ RwInt32 numLights;
} RxGameCubeRenderData;

/* The all-in-one node's private data. */
typedef struct RxGameCubeAllInOnePvtData {
    /* 0x00 */ void* instanceCB;
    /* 0x04 */ void* reinstanceCB;
    /* 0x08 */ void* unk08;
    /* 0x0C */ void* renderCB;
} RxGameCubeAllInOnePvtData;

#define RESENTRYDATA(entry) ((RxGameCubeInstanceData*)((RwUInt8*)(entry) + 0x18))

extern volatile RwUInt16 lbl_8025E800;
extern RpGameCubeVtxFmt DlDefaultVtxFmt;

RwBool _rwDlTokenDone(RwUInt16 token);
void _rwDlTransformSetup(RwMatrix* matrix, RwInt32 flags);
RwBool _rwDlTextureSet(RwTexture* texture, RwInt32 stage);
void _rwDlTextureRasterFlush(RwInt32 mode);
RwMatrix* RwFrameGetLTM(RwFrame* frame);
RpGameCubeVtxFmt* RpGameCubeGeometryGetVtxFmt(RpGeometry* geometry);
RpGameCubeVtxFmt* RpGameCubeWorldGetVtxFmt(RpWorld* world);
void _rxGCSetVtxDescs(RpGameCubeVtxFmt* fmt, RxGameCubeRenderData* data);
RwInt32 RpWorldSectorGetNumVertices(const RpWorldSector* sector);

void GXSetDrawSync(RwUInt16 token);
void GXInvalidateVtxCache(void);
void GXSetChanAmbColor(RwInt32 chan, GXColor color);
void GXSetChanMatColor(RwInt32 chan, GXColor color);
void GXSetTevColorIn(RwInt32 stage, RwInt32 a, RwInt32 b, RwInt32 c, RwInt32 d);
void GXSetTevAlphaIn(RwInt32 stage, RwInt32 a, RwInt32 b, RwInt32 c, RwInt32 d);
void GXSetTevColorOp(RwInt32 stage, RwInt32 op, RwInt32 bias, RwInt32 scale, RwUInt8 clamp, RwInt32 outReg);
void GXSetTevAlphaOp(RwInt32 stage, RwInt32 op, RwInt32 bias, RwInt32 scale, RwUInt8 clamp, RwInt32 outReg);
void GXCallDisplayList(void* list, RwUInt32 nbytes);
void GXSetArray(RwInt32 attr, void* base, RwUInt8 stride);

static const GXColor opaqueWhite = { 255, 255, 255, 255 };
static const GXColor opaqueBlack = { 0, 0, 0, 255 };
static const GXColor transparentBlack = { 0, 0, 0, 0 };

/* Component sizes by GX component type, and colour sizes by colour format. */
static RwUInt32 compSizes[] = { 1, 1, 2, 2, 4 };
static RwUInt32 colorSizes[] = { 2, 3, 4, 2, 3, 4, 0 };

void _rxGCResEntryWaitDone(RwResEntry* entry)
{
    if (RESENTRYDATA(entry)->token == lbl_8025E800) {
        GXSetDrawSync(lbl_8025E800);
        lbl_8025E800 = (lbl_8025E800 + 1) % 0xE000;
    }

    while (!_rwDlTokenDone(RESENTRYDATA(entry)->token)) {
    }
    GXInvalidateVtxCache();
}

void _rxGCSetupLighting(RxGameCubeRenderData* data)
{
    RwUInt32 flags = data->flags;
    RwInt32 ambSrc;

    if (data->numLights > 0) {
        if (flags & rxGEOMETRY_PRELIT) {
            ambSrc = GX_SRC_VTX;
        } else {
            ambSrc = GX_SRC_REG;
            if (!data->ambientLight) {
                GXSetChanAmbColor(GX_COLOR0A0, opaqueBlack);
            }
        }

        if (data->lightMask0 && data->lightMask1) {
            GXSetNumChans(2);
            GXSetChanCtrl(GX_COLOR0A0, GX_TRUE, ambSrc, GX_SRC_REG, data->lightMask0, GX_DF_CLAMP, GX_AF_NONE);
            GXSetChanCtrl(GX_COLOR1A1, GX_TRUE, GX_SRC_REG, GX_SRC_REG, data->lightMask1, GX_DF_CLAMP, GX_AF_SPOT);
            if (!(flags & rxGEOMETRY_MODULATE)) {
                GXSetChanMatColor(GX_COLOR0A0, opaqueWhite);
                GXSetChanMatColor(GX_COLOR1A1, opaqueWhite);
            }
            GXSetChanAmbColor(GX_COLOR1A1, transparentBlack);
        } else {
            RwInt32 attnFn;

            GXSetNumChans(1);
            attnFn = GX_AF_NONE;
            if (data->lightMask1) {
                attnFn = GX_AF_SPOT;
            }
            GXSetChanCtrl(GX_COLOR0A0, GX_TRUE, ambSrc, GX_SRC_REG, data->lightMask0 | data->lightMask1, GX_DF_CLAMP,
                          attnFn);
            if (!(flags & rxGEOMETRY_MODULATE)) {
                GXSetChanMatColor(GX_COLOR0A0, opaqueWhite);
            }
        }
    } else if (flags & rxGEOMETRY_PRELIT) {
        if (flags & rxGEOMETRY_MODULATE) {
            GXSetNumChans(1);
            GXSetChanCtrl(GX_COLOR0A0, GX_TRUE, GX_SRC_VTX, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
        } else {
            GXSetNumChans(1);
            GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
        }
    } else {
        if (!data->ambientLight && !(flags & rxGEOMETRY_MODULATE)) {
            GXSetChanMatColor(GX_COLOR0A0, opaqueBlack);
        }
        GXSetNumChans(1);
        GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
    }
}

void _rxGCSetupMaterialColor(RxGameCubeRenderData* data, RpMaterial* material)
{
    GXColor color;

    if (data->numLights > 0) {
        if (data->ambientLight && !(data->flags & rxGEOMETRY_PRELIT)) {
            RwReal scale = 255.0f * material->surfaceProps.ambient;

            color.r = scale * data->ambient[0];
            color.g = scale * data->ambient[1];
            color.b = scale * data->ambient[2];
            color.a = 0xFF;
            GXSetChanAmbColor(GX_COLOR0A0, color);
        }

        if (data->flags & rxGEOMETRY_MODULATE) {
            GXSetChanMatColor(GX_COLOR0A0, *(GXColor*)&material->color);
            if (data->lightMask0 && data->lightMask1) {
                GXSetChanMatColor(GX_COLOR1A1, *(GXColor*)&material->color);
            }
        }
    } else {
        RwUInt32 flags = data->flags;

        if (flags & rxGEOMETRY_PRELIT) {
            if (flags & rxGEOMETRY_MODULATE) {
                GXSetChanMatColor(GX_COLOR0A0, *(GXColor*)&material->color);
            }
        } else if (data->ambientLight) {
            RwReal ambient = material->surfaceProps.ambient;

            if (flags & rxGEOMETRY_MODULATE) {
                RwReal red = ambient * data->ambient[0];
                RwReal green = ambient * data->ambient[1];
                RwReal blue = ambient * data->ambient[2];

                color.r = material->color.red * red;
                color.g = material->color.green * green;
                color.b = material->color.blue * blue;
                color.a = material->color.alpha;
            } else {
                ambient *= 255.0f;
                color.r = ambient * data->ambient[0];
                color.g = ambient * data->ambient[1];
                color.b = ambient * data->ambient[2];
                color.a = 0xFF;
            }
            GXSetChanMatColor(GX_COLOR0A0, color);
        } else if (flags & rxGEOMETRY_MODULATE) {
            color.r = color.g = color.b = 0;
            color.a = material->color.alpha;
            GXSetChanMatColor(GX_COLOR0A0, color);
        }
    }
}

void _rxGCDefaultRenderCallback(RxGameCubeRenderData* data)
{
    RwBool twoChans = FALSE;
    RwUInt32 numStages;

    if (data->lightMask0 && data->lightMask1) {
        twoChans = TRUE;
    }
    numStages = (twoChans != FALSE) + 1;
    GXSetNumTevStages(numStages);

    if (data->flags & (rxGEOMETRY_TEXTURED | rxGEOMETRY_TEXTURED2)) {
        GXSetNumTexGens(1);
        GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
        GXSetTevOp(GX_TEVSTAGE0, GX_MODULATE);
        if (numStages > 1) {
            GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR1A1);
            GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC, GX_CC_CPREV);
            GXSetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
            GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV);
            GXSetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        }
    } else {
        GXSetNumTexGens(0);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
        GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
        if (numStages > 1) {
            GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR1A1);
            GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_RASC, GX_CC_ONE, GX_CC_CPREV);
            GXSetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
            GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV);
            GXSetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        }
    }
}

void* _rxGCDefaultRender(void* object, RxGameCubeRenderData* data)
{
    RxGameCubeDisplayList* dl = RESENTRYDATA(data->repEntry)->displayLists;
    RwBool setMaterial;
    RwBool ambient;
    RwUInt32 flags;
    RwUInt32 numMeshes;
    RpMesh* mesh;
    RwMatrix* ltm;

    RESENTRYDATA(data->repEntry)->token = lbl_8025E800;
    if (((RwObject*)object)->type == rpATOMIC) {
        ltm = RwFrameGetLTM((RwFrame*)((RpAtomic*)object)->object.object.parent);
        _rxGCSetVtxDescs(RpGameCubeGeometryGetVtxFmt(((RpAtomic*)object)->geometry), data);
    } else {
        ltm = NULL;
        _rxGCSetVtxDescs(RpGameCubeWorldGetVtxFmt((RpWorld*)RWSRCGLOBAL(curWorld)), data);
    }
    _rwDlTransformSetup(ltm, (data->flags & rxGEOMETRY_NORMALS) >> 4);
    _rxGCSetupLighting(data);
    _rxGCDefaultRenderCallback(data);

    flags = data->flags;
    setMaterial = TRUE;
    if (!(flags & rxGEOMETRY_MODULATE)) {
        ambient = FALSE;
        if (data->ambientLight && !(flags & rxGEOMETRY_PRELIT)) {
            ambient = TRUE;
        }
        if (!ambient) {
            setMaterial = FALSE;
        }
    }

    numMeshes = data->meshHeader->numMeshes;
    mesh = (RpMesh*)(data->meshHeader + 1);
    if (flags & (rxGEOMETRY_TEXTURED | rxGEOMETRY_TEXTURED2)) {
        while (numMeshes--) {
            RwTexture* texture = mesh->material->texture;
            RwInt32 mode;

            _rwDlTextureSet(texture, 0);
            mode = 1;
            if (texture && texture->raster &&
                (RASTEREXTFROMRASTER(texture->raster->parent)->flags & rwDLRASTERHASALPHA)) {
                mode = 3;
            }
            _rwDlTextureRasterFlush(mode);
            if (setMaterial) {
                _rxGCSetupMaterialColor(data, mesh->material);
            }
            GXCallDisplayList(dl->list, dl->size);
            mesh++;
            dl++;
        }
    } else if (setMaterial) {
        while (numMeshes--) {
            _rxGCSetupMaterialColor(data, mesh->material);
            GXCallDisplayList(dl->list, dl->size);
            mesh++;
            dl++;
        }
    } else {
        while (numMeshes--) {
            GXCallDisplayList(dl->list, dl->size);
            dl++;
        }
    }

    return object;
}

void _rxGameCubeAllInOneSetInstanceCallBack(RxPipelineNode* node, void* callback)
{
    ((RxGameCubeAllInOnePvtData*)node->privateData)->instanceCB = callback;
}

void* _rxGameCubeAllInOneGetInstanceCallBack(RxPipelineNode* node)
{
    return ((RxGameCubeAllInOnePvtData*)node->privateData)->instanceCB;
}

void _rxGameCubeAllInOneSetReinstanceCallBack(RxPipelineNode* node, void* callback)
{
    ((RxGameCubeAllInOnePvtData*)node->privateData)->reinstanceCB = callback;
}

void* _rxGameCubeAllInOneGetReinstanceCallBack(RxPipelineNode* node)
{
    return ((RxGameCubeAllInOnePvtData*)node->privateData)->reinstanceCB;
}

void RxGameCubeAllInOneSetRenderCallBack(RxPipelineNode* node, void* callback)
{
    ((RxGameCubeAllInOnePvtData*)node->privateData)->renderCB = callback;
}

RwUInt32 _rxGCVertexArraySize(RpGameCubeVtxFmt* fmt, RwUInt32 flags, RwUInt32 numVerts, RwUInt8 numTexCoords)
{
    RwUInt32 size = 0;
    RwInt32 i;

    if (!fmt) {
        fmt = &DlDefaultVtxFmt;
    }

    if (fmt->flags & rpGCVTXFMTINTERLEAVED) {
        size = 0;
        if (flags & rpGEOMETRYPOSITIONS) {
            size = compSizes[fmt->posType] * 3;
        }
        if (flags & rxGEOMETRY_NORMALS) {
            size += compSizes[fmt->nrmType] * (fmt->nbt ? 9 : 3);
        }
        if (flags & rxGEOMETRY_PRELIT) {
            size += colorSizes[fmt->clrType];
        }
        if (flags & (rxGEOMETRY_TEXTURED | rxGEOMETRY_TEXTURED2)) {
            for (i = 0; i < numTexCoords; i++) {
                size += compSizes[fmt->texType[i]] * 2;
            }
        }
        size = (size * numVerts + 31) & ~31;
    } else {
        if (flags & rpGEOMETRYPOSITIONS) {
            size = (numVerts * (compSizes[fmt->posType] * 3) + 31) & ~31;
        }
        if (flags & rxGEOMETRY_NORMALS) {
            size += (numVerts * (compSizes[fmt->nrmType] * (fmt->nbt ? 9 : 3)) + 31) & ~31;
        }
        if (flags & rxGEOMETRY_PRELIT) {
            size += (numVerts * colorSizes[fmt->clrType] + 31) & ~31;
        }
        if (flags & (rxGEOMETRY_TEXTURED | rxGEOMETRY_TEXTURED2)) {
            for (i = 0; i < numTexCoords; i++) {
                size += (numVerts * (compSizes[fmt->texType[i]] * 2) + 31) & ~31;
            }
        }
    }

    return size;
}

RwUInt32 _rwGCNVtxFmtInstPos(RpGameCubeVtxFmt* fmt, void* dst, const RwV3d* src, RwInt32 numVerts, RwUInt8 stride)
{
    RwInt32 i;

    if (!fmt) {
        fmt = &DlDefaultVtxFmt;
    }

    switch (fmt->posType) {
    case GX_U8: {
        RwReal scale = (RwReal)(1 << fmt->posFrac);
        RwUInt8* d = (RwUInt8*)dst;
        RwInt32 j;

        for (j = 0; j < numVerts; j++) {
            d[0] = src->x * scale;
            d[1] = src->y * scale;
            d[2] = src->z * scale;
            src++;
            d += stride;
        }
        break;
    }
    case GX_S8: {
        RwReal scale = (RwReal)(1 << fmt->posFrac);
        RwInt8* d = (RwInt8*)dst;
        RwInt32 j;

        for (j = 0; j < numVerts; j++) {
            d[0] = src->x * scale;
            d[1] = src->y * scale;
            d[2] = src->z * scale;
            src++;
            d += stride;
        }
        break;
    }
    case GX_U16: {
        RwReal scale = (RwReal)(1 << fmt->posFrac);
        RwUInt16* d = (RwUInt16*)dst;
        RwInt32 j;

        for (j = 0; j < numVerts; j++) {
            d[0] = src->x * scale;
            d[1] = src->y * scale;
            d[2] = src->z * scale;
            src++;
            d = (RwUInt16*)((RwUInt8*)d + stride);
        }
        break;
    }
    case GX_S16: {
        RwReal scale = (RwReal)(1 << fmt->posFrac);
        RwInt16* d = (RwInt16*)dst;
        RwInt32 j;

        for (j = 0; j < numVerts; j++) {
            d[0] = src->x * scale;
            d[1] = src->y * scale;
            d[2] = src->z * scale;
            src++;
            d = (RwInt16*)((RwUInt8*)d + stride);
        }
        break;
    }
    case GX_F32:
        if (fmt->flags & rpGCVTXFMTINTERLEAVED) {
            RwV3d* d = (RwV3d*)dst;

            for (i = 0; i < numVerts; i++) {
                *d = *src;
                src++;
                d = (RwV3d*)((RwUInt8*)d + stride);
            }
        } else {
            memcpy(dst, src, numVerts * compSizes[fmt->posType] * 3);
        }
        break;
    }

    if (fmt->flags & rpGCVTXFMTINTERLEAVED) {
        return compSizes[fmt->posType] * 3;
    }
    return (numVerts * compSizes[fmt->posType] * 3 + 31) & ~31;
}

RwUInt32 _rwGCNVtxFmtInstNrm(RpGameCubeVtxFmt* fmt, void* dst, const RwV3d* src, RwInt32 numVerts,
                             RwUInt8 stride)
{
    if (!fmt) {
        fmt = &DlDefaultVtxFmt;
    }

    switch (fmt->nrmType) {
    case GX_U8:
        break;
    case GX_S8: {
        RwReal scale = 64.0f;
        RwInt8* d = (RwInt8*)dst;
        RwInt32 i;

        for (i = 0; i < numVerts; i++) {
            d[0] = src->x * scale;
            d[1] = src->y * scale;
            d[2] = src->z * scale;
            src++;
            d += stride;
        }
        break;
    }
    case GX_U16:
        break;
    case GX_S16: {
        RwReal scale = 16384.0f;
        RwInt16* d = (RwInt16*)dst;
        RwInt32 i;

        for (i = 0; i < numVerts; i++) {
            d[0] = src->x * scale;
            d[1] = src->y * scale;
            d[2] = src->z * scale;
            src++;
            d = (RwInt16*)((RwUInt8*)d + stride);
        }
        break;
    }
    case GX_F32:
        if ((fmt->flags & rpGCVTXFMTINTERLEAVED) || fmt->nbt) {
            RwV3d* d = (RwV3d*)dst;
            RwInt32 i;

            for (i = 0; i < numVerts; i++) {
                *d = *src;
                src++;
                d = (RwV3d*)((RwUInt8*)d + stride);
            }
        } else {
            memcpy(dst, src, numVerts * compSizes[fmt->nrmType] * 3);
        }
        break;
    }

    if (fmt->flags & rpGCVTXFMTINTERLEAVED) {
        return compSizes[fmt->nrmType] * (fmt->nbt ? 9 : 3);
    }
    return ((fmt->nbt ? 9 : 3) * (numVerts * compSizes[fmt->nrmType]) + 31) & ~31;
}

RwUInt32 _rwGCNVtxFmtInstNrmCmp(RpGameCubeVtxFmt* fmt, void* dst, const RpVertexNormal* src, RwInt32 numVerts,
                                RwUInt8 stride)
{
    if (!fmt) {
        fmt = &DlDefaultVtxFmt;
    }

    switch (fmt->nrmType) {
    case GX_U8:
        break;
    case GX_S8: {
        RwInt32 i;
        RwInt8* d = (RwInt8*)dst;
        RwReal scale = 64.0f;

        for (i = numVerts; i > 0; i--) {
            RwInt8 x = src->x * (1.0f / 128.0f);
            RwInt8 y = src->y * (1.0f / 128.0f);
            RwInt8 z = src->z * (1.0f / 128.0f);

            d[0] = x * scale;
            d[1] = y * scale;
            d[2] = z * scale;
            src++;
            d += stride;
        }
        break;
    }
    case GX_U16:
        break;
    case GX_S16: {
        RwInt32 i;
        RwInt16* d = (RwInt16*)dst;
        RwReal scale = 16384.0f;

        for (i = numVerts; i > 0; i--) {
            RwInt8 x = src->x * (1.0f / 128.0f);
            RwInt8 y = src->y * (1.0f / 128.0f);
            RwInt8 z = src->z * (1.0f / 128.0f);

            d[0] = x * scale;
            d[1] = y * scale;
            d[2] = z * scale;
            src++;
            d = (RwInt16*)((RwUInt8*)d + stride);
        }
        break;
    }
    case GX_F32: {
        RwInt32 i;
        RwV3d* d = (RwV3d*)dst;

        for (i = 0; i < numVerts; i++) {
            d->x = src->x * (1.0f / 128.0f);
            d->y = src->y * (1.0f / 128.0f);
            d->z = src->z * (1.0f / 128.0f);
            src++;
            d = (RwV3d*)((RwUInt8*)d + stride);
        }
        break;
    }
    }

    if (fmt->flags & rpGCVTXFMTINTERLEAVED) {
        return compSizes[fmt->nrmType] * (fmt->nbt ? 9 : 3);
    }
    return ((fmt->nbt ? 9 : 3) * (numVerts * compSizes[fmt->nrmType]) + 31) & ~31;
}

RwUInt32 _rwGCNVtxFmtInstNBT(RpGameCubeVtxFmt* fmt, void* dst, const RwRGBA* src, RwInt32 numVerts,
                             RwUInt8 stride)
{
    if (!fmt) {
        fmt = &DlDefaultVtxFmt;
    }

    switch (fmt->clrType) {
    case GX_RGB565: {
        RwUInt16* d = (RwUInt16*)dst;
        RwInt32 i;

        for (i = 0; i < numVerts; i++) {
            *d = ((src->red << 8) & 0xF800) | ((src->green << 3) & 0x07E0) | ((src->blue >> 3) & 0x001F);
            src++;
            d = (RwUInt16*)((RwUInt8*)d + stride);
        }
        break;
    }
    case GX_RGB8: {
        RwUInt8* d = (RwUInt8*)dst;
        RwInt32 i;

        for (i = 0; i < numVerts; i++) {
            d[0] = src->red;
            d[1] = src->green;
            d[2] = src->blue;
            src++;
            d += stride;
        }
        break;
    }
    case GX_RGBA4: {
        RwUInt16* d = (RwUInt16*)dst;
        RwInt32 i;

        for (i = 0; i < numVerts; i++) {
            *d = ((src->red << 8) & 0xF000) | ((src->green << 4) & 0x0F00) | (src->blue & 0x00F0) |
                 ((src->alpha >> 4) & 0x000F);
            src++;
            d = (RwUInt16*)((RwUInt8*)d + stride);
        }
        break;
    }
    case GX_RGBA6: {
        RwUInt8* d = (RwUInt8*)dst;
        RwInt32 i;

        for (i = 0; i < numVerts; i++) {
            d[0] = (src->red & 0xFC) | ((src->green >> 6) & 0x03);
            d[1] = ((src->green << 2) & 0xF0) | ((src->blue >> 4) & 0x0F);
            d[2] = ((src->blue << 4) & 0xC0) | ((src->alpha >> 2) & 0x3F);
            src++;
            d += stride;
        }
        break;
    }
    case GX_RGBX8:
    case GX_RGBA8:
        if (fmt->flags & rpGCVTXFMTINTERLEAVED) {
            RwUInt32* d = (RwUInt32*)dst;
            RwInt32 i;

            for (i = 0; i < numVerts; i++) {
                *d = *(const RwUInt32*)src;
                src++;
                d = (RwUInt32*)((RwUInt8*)d + stride);
            }
        } else {
            memcpy(dst, src, numVerts * colorSizes[fmt->clrType]);
        }
        break;
    }

    if (fmt->flags & rpGCVTXFMTINTERLEAVED) {
        return colorSizes[fmt->clrType];
    }
    return (numVerts * colorSizes[fmt->clrType] + 31) & ~31;
}

RwUInt32 _rwGCNVtxFmtInstTex(RpGameCubeVtxFmt* fmt, RwInt32 index, void* dst, const RwTexCoords* src, RwInt32 numVerts,
                     RwUInt8 stride)
{
    RwInt32 i;
    const RwUInt8* type;

    if (!fmt) {
        fmt = &DlDefaultVtxFmt;
    }

    type = fmt->texType + index;
    switch (*type) {
    case GX_U8: {
        RwReal scale = (RwReal)(1 << fmt->texFrac[index]);
        RwUInt8* d = (RwUInt8*)dst;
        RwInt32 j;

        for (j = 0; j < numVerts; j++) {
            d[0] = src->u * scale;
            d[1] = src->v * scale;
            src++;
            d += stride;
        }
        break;
    }
    case GX_S8: {
        RwReal scale = (RwReal)(1 << fmt->texFrac[index]);
        RwInt8* d = (RwInt8*)dst;
        RwInt32 j;

        for (j = 0; j < numVerts; j++) {
            d[0] = src->u * scale;
            d[1] = src->v * scale;
            src++;
            d += stride;
        }
        break;
    }
    case GX_U16: {
        RwReal scale = (RwReal)(1 << fmt->texFrac[index]);
        RwUInt16* d = (RwUInt16*)dst;
        RwInt32 j;

        for (j = 0; j < numVerts; j++) {
            d[0] = src->u * scale;
            d[1] = src->v * scale;
            src++;
            d = (RwUInt16*)((RwUInt8*)d + stride);
        }
        break;
    }
    case GX_S16: {
        RwReal scale = (RwReal)(1 << fmt->texFrac[index]);
        RwInt16* d = (RwInt16*)dst;
        RwInt32 j;

        for (j = 0; j < numVerts; j++) {
            d[0] = src->u * scale;
            d[1] = src->v * scale;
            src++;
            d = (RwInt16*)((RwUInt8*)d + stride);
        }
        break;
    }
    case GX_F32:
        if (fmt->flags & rpGCVTXFMTINTERLEAVED) {
            RwTexCoords* d = (RwTexCoords*)dst;

            for (i = 0; i < numVerts; i++) {
                *d = *src;
                src++;
                d = (RwTexCoords*)((RwUInt8*)d + stride);
            }
        } else {
            memcpy(dst, src, numVerts * compSizes[*type] * 2);
        }
        break;
    }

    if (fmt->flags & rpGCVTXFMTINTERLEAVED) {
        return compSizes[*type] * 2;
    }
    return (numVerts * compSizes[*type] * 2 + 31) & ~31;
}

static inline RwTexCoords* RpGeometryGetVertexTexCoords(const RpGeometry* geometry, RwInt32 uvIndex)
{
    return geometry->texCoords[uvIndex - 1];
}

/* Each vertex array's stride; interleaved formats share the whole vertex. */
static inline void GCSetupStrides(RpGameCubeVtxFmt* fmt, RxGameCubeInstanceData* data, RwUInt32 flags,
                                  RwUInt8 numTexCoords)
{
    RwUInt32 stride = 0;
    RwUInt32 i;

    if (!fmt) {
        fmt = &DlDefaultVtxFmt;
    }

    if (flags & rpGEOMETRYPOSITIONS) {
        data->posStride = compSizes[fmt->posType] * 3;
        stride = data->posStride;
    }
    if (flags & rxGEOMETRY_NORMALS) {
        data->nrmStride = compSizes[fmt->nrmType] * (fmt->nbt ? 9 : 3);
        stride += data->nrmStride;
    }
    if (flags & rxGEOMETRY_PRELIT) {
        data->clrStride = colorSizes[fmt->clrType];
        stride += data->clrStride;
    }
    if (flags & (rxGEOMETRY_TEXTURED | rxGEOMETRY_TEXTURED2)) {
        for (i = 0; i < numTexCoords; i++) {
            data->texStride[i] = compSizes[fmt->texType[i]] * 2;
            stride += data->texStride[i];
        }
    }

    if (fmt->flags & rpGCVTXFMTINTERLEAVED) {
        data->posStride = stride;
        data->nrmStride = stride;
        data->clrStride = stride;
        for (i = 0; i < numTexCoords; i++) {
            data->texStride[i] = stride;
        }
    }
}

void _rxGCInstanceGeometryArrays(RpGeometry* geometry, RxGameCubeInstanceData* data, RpGameCubeVtxFmt* fmt, RwUInt32 flags,
                 RwUInt8 numTexCoords, RwUInt8* dst)
{
    RwInt32 numVerts = geometry->numVertices;

    if (!fmt) {
        fmt = &DlDefaultVtxFmt;
    }
    GCSetupStrides(fmt, data, flags, numTexCoords);

    if (flags & rpGEOMETRYPOSITIONS) {
        RwUInt32 size = _rwGCNVtxFmtInstPos(fmt, dst, geometry->morphTarget->verts, numVerts, data->posStride);

        data->posData = dst;
        dst += size;
    }
    if (flags & rxGEOMETRY_NORMALS) {
        RwUInt32 size = _rwGCNVtxFmtInstNrm(fmt, dst, geometry->morphTarget->normals, numVerts, data->nrmStride);

        data->nrmData = dst;
        dst += size;
    }
    if (flags & rxGEOMETRY_PRELIT) {
        RwUInt32 size = _rwGCNVtxFmtInstNBT(fmt, dst, geometry->preLitLum, numVerts, data->clrStride);

        data->clrData = dst;
        dst += size;
    }
    if (flags & (rxGEOMETRY_TEXTURED | rxGEOMETRY_TEXTURED2)) {
        RwInt32 i;

        for (i = 0; i < numTexCoords; i++) {
            RwUInt32 size = _rwGCNVtxFmtInstTex(fmt, i, dst, RpGeometryGetVertexTexCoords(geometry, i + 1), numVerts, data->texStride[i]);

            data->texData[i] = dst;
            dst += size;
        }
    }
}

void _rxGCInstanceWorldSectorArrays(RpWorldSector* sector, RxGameCubeInstanceData* data, RpGameCubeVtxFmt* fmt, RwUInt32 flags,
                 RwUInt8 numTexCoords, RwUInt8* dst)
{
    RwInt32 numVerts = RpWorldSectorGetNumVertices(sector);

    if (!fmt) {
        fmt = &DlDefaultVtxFmt;
    }
    GCSetupStrides(fmt, data, flags, numTexCoords);

    if (flags & rpGEOMETRYPOSITIONS) {
        RwUInt32 size = _rwGCNVtxFmtInstPos(fmt, dst, sector->vertices, numVerts, data->posStride);

        data->posData = dst;
        dst += size;
    }
    if (flags & rxGEOMETRY_NORMALS) {
        RwUInt32 size =
            _rwGCNVtxFmtInstNrmCmp(fmt, dst, (RpVertexNormal*)sector->normals, numVerts, data->nrmStride);

        data->nrmData = dst;
        dst += size;
    }
    if (flags & rxGEOMETRY_PRELIT) {
        RwUInt32 size = _rwGCNVtxFmtInstNBT(fmt, dst, sector->preLitLum, numVerts, data->clrStride);

        data->clrData = dst;
        dst += size;
    }
    if (flags & (rxGEOMETRY_TEXTURED | rxGEOMETRY_TEXTURED2)) {
        RwInt32 i;

        for (i = 0; i < numTexCoords; i++) {
            RwUInt32 size = _rwGCNVtxFmtInstTex(fmt, i, dst, sector->texCoords[i], numVerts, data->texStride[i]);

            data->texData[i] = dst;
            dst += size;
        }
    }
}

void _rxGCSetVtxDescs(RpGameCubeVtxFmt* fmt, RxGameCubeRenderData* renderData)
{
    RwInt32 attr;
    RxGameCubeInstanceData* data;
    RwInt32 i;

    if (!fmt) {
        fmt = &DlDefaultVtxFmt;
    }
    data = RESENTRYDATA(renderData->repEntry);

    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, data->descType);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, fmt->posType, fmt->posFrac);
    GXSetArray(GX_VA_POS, data->posData, data->posStride);

    if (renderData->flags & rxGEOMETRY_NORMALS) {
        if (fmt->nbt) {
            GXSetVtxDesc(GX_VA_NBT, data->descType);
            GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_NBT, GX_NRM_NBT, fmt->nrmType, 0);
            GXSetArray(GX_VA_NBT, data->nrmData, data->nrmStride);
        } else {
            GXSetVtxDesc(GX_VA_NRM, data->descType);
            GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, fmt->nrmType, 0);
            GXSetArray(GX_VA_NRM, data->nrmData, data->nrmStride);
        }
    }

    if (renderData->flags & rxGEOMETRY_PRELIT) {
        GXSetVtxDesc(GX_VA_CLR0, data->descType);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, fmt->clrType > GX_RGBX8 ? GX_CLR_RGBA : GX_CLR_RGB, fmt->clrType, 0);
        GXSetArray(GX_VA_CLR0, data->clrData, data->clrStride);
    }

    if (renderData->flags & (rxGEOMETRY_TEXTURED | rxGEOMETRY_TEXTURED2)) {
        for (i = 0; i < 8 && data->texData[i]; i++) {
            attr = GX_VA_TEX0 + i;
            GXSetVtxDesc(attr, data->descType);
            GXSetVtxAttrFmt(GX_VTXFMT0, attr, GX_TEX_ST, fmt->texType[i], fmt->texFrac[i]);
            GXSetArray(attr, data->texData[i], data->texStride[i]);
        }
    }
}
