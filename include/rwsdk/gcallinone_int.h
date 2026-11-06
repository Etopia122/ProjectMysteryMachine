#ifndef RWSDK_GCALLINONE_INT_H
#define RWSDK_GCALLINONE_INT_H

/*
 * Types shared by the GameCube all-in-one pipeline nodes and their support
 * code: vertex formats, instanced resource entry layout and render data.
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
    /* 0x00 */ RwUInt16 serialNum;
    /* 0x02 */ RwUInt16 descType;
    /* 0x04 */ RwUInt16 token;
    /* 0x06 */ RwUInt8 posStride;
    /* 0x07 */ RwUInt8 nrmStride;
    /* 0x08 */ RwUInt8 clrStride;
    /* 0x09 */ RwUInt8 texStride[8];
    /* 0x11 */ RwUInt8 pad11[3];
    /* 0x14 */ RwUInt32 unk14;
    /* 0x18 */ void* posData;
    /* 0x1C */ void* nrmData;
    /* 0x20 */ void* clrData;
    /* 0x24 */ void* texData[8];
    /* 0x44 */ RxGameCubeDisplayList displayLists[1];
} RxGameCubeInstanceData;

typedef struct RxGameCubeAllInOnePvtData RxGameCubeAllInOnePvtData;

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
    /* 0x2C */ RxGameCubeAllInOnePvtData* pvtData;
} RxGameCubeRenderData;

typedef void* (*RxGameCubeAllInOneCallBack)(void* object, RxGameCubeRenderData* data);

/* The all-in-one node's private data. */
struct RxGameCubeAllInOnePvtData {
    /* 0x00 */ RxGameCubeAllInOneCallBack instanceCB;
    /* 0x04 */ RxGameCubeAllInOneCallBack reinstanceCB;
    /* 0x08 */ RxGameCubeAllInOneCallBack lightingCB;
    /* 0x0C */ RxGameCubeAllInOneCallBack renderCB;
    /* 0x10 */ RwInt32 unk10;
};

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

void _rxGCResEntryWaitDone(RwResEntry* entry);
void _rxGCSetupLighting(RxGameCubeRenderData* data);
void _rxGCSetupMaterialColor(RxGameCubeRenderData* data, RpMaterial* material);
void _rxGCDefaultRenderCallback(RxGameCubeRenderData* data);
void* _rxGCDefaultRender(void* object, RxGameCubeRenderData* data);
RwUInt32 _rxGCVertexArraySize(RpGameCubeVtxFmt* fmt, RwUInt32 flags, RwUInt32 numVerts, RwUInt8 numTexCoords);
void _rxGCInstanceGeometryArrays(RpGeometry* geometry, RxGameCubeInstanceData* data, RpGameCubeVtxFmt* fmt,
                                 RwUInt32 flags, RwUInt8 numTexCoords, RwUInt8* dst);
void _rxGCInstanceWorldSectorArrays(RpWorldSector* sector, RxGameCubeInstanceData* data, RpGameCubeVtxFmt* fmt,
                                    RwUInt32 flags, RwUInt8 numTexCoords, RwUInt8* dst);

#endif
