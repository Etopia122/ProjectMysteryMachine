/*
 * GameCube multi-texture rendering: sends an effect's TEV configuration to
 * GX (constant colours, texture and indirect matrices driven by registered
 * texture frames, indirect and TEV stages, texture coordinate generation),
 * replays a mesh with it, and builds the per-vertex tangent space the bump
 * texture coordinate generators need.
 */

#include "rwsdk/gcallinone_int.h"

#define rwID_GCMTPIPEPLUGIN 0x129

#define rpMTEFFECTGAMECUBE 6

#define rpPRIMTYPETRISTRIP 4

#define rpGCMTCONFIGTEVCOLORS 0x01
#define rpGCMTCONFIGKCOLORS 0x02
#define rpGCMTCONFIGBLEND 0x04
#define rpGCMTCONFIGALPHA 0x08
#define rpGCMTCONFIGCHANNELS 0x10
#define rpGCMTCONFIGENVMAP 0x20

#define rpGCMTDISABLED 0x01
#define rpGCMTTEXGENNORMALIZE 0x02
#define rpGCMTTEXGENSCALE 0x04
#define rpGCMTTEXMTX3x4 0x01
#define rpGCMTTEXMTXTRANSLATE 0x02

#define rpGCMTFRAMEVIEW 2
#define rpGCMTFRAMEWORLD 3
#define rpGCMTFRAMEUSER 16

#define rwRENDERSTATESRCBLEND 10
#define rwRENDERSTATEDESTBLEND 11

#define rwMATRIXTYPEORTHONORMAL 0x00000003
#define rwMATRIXINTERNALIDENTITY 0x00020000

#define GX_TEVREG0 1
#define GX_TEVREG1 2
#define GX_TEVREG2 3
#define GX_KCOLOR0 0
#define GX_KCOLOR1 1
#define GX_KCOLOR2 2
#define GX_KCOLOR3 3
#define GX_COLOR_NULL 0xFF
#define GX_CA_RASA 5
#define GX_TG_BINRM 2
#define GX_TG_TANGENT 3
#define GX_TG_BUMP0 2
#define GX_TG_BUMP7 9
#define GX_TG_TEX0 4
#define GX_TG_TEX7 11
#define GX_TEXMTX0 30
#define GX_MTX2x4 1

typedef struct GXColorS10 {
    RwInt16 r;
    RwInt16 g;
    RwInt16 b;
    RwInt16 a;
} GXColorS10;

typedef struct RpMTEffect {
    /* 0x00 */ RwInt32 platform;
    /* 0x04 */ RwUInt32 refCount;
    /* 0x08 */ RwChar name[32];
    /* 0x28 */ RwLLLink inDict;
} RpMTEffect;

typedef struct RpGameCubeTexGen {
    /* 0x00 */ RwUInt32 flags;
    /* 0x04 */ RwInt32 func;
    /* 0x08 */ RwInt32 src;
    /* 0x0C */ RwUInt32 mtx;
    /* 0x10 */ RwUInt32 postMtx;
    /* 0x14 */ RwUInt16 scaleS;
    /* 0x16 */ RwUInt16 scaleT;
} RpGameCubeTexGen;

typedef struct RpGameCubeTevStage {
    /* 0x00 */ RwUInt32 colorIn[4];
    /* 0x10 */ RwUInt32 alphaIn[4];
    /* 0x20 */ RwUInt32 colorOp;
    /* 0x24 */ RwUInt32 colorBias;
    /* 0x28 */ RwUInt32 colorScale;
    /* 0x2C */ RwUInt8 colorClamp;
    /* 0x30 */ RwUInt32 colorOut;
    /* 0x34 */ RwUInt32 alphaOp;
    /* 0x38 */ RwUInt32 alphaBias;
    /* 0x3C */ RwUInt32 alphaScale;
    /* 0x40 */ RwUInt8 alphaClamp;
    /* 0x44 */ RwUInt32 alphaOut;
    /* 0x48 */ RwUInt32 kColorSel;
    /* 0x4C */ RwUInt32 kAlphaSel;
    /* 0x50 */ RwUInt32 texCoord;
    /* 0x54 */ RwUInt32 texMap;
    /* 0x58 */ RwInt32 channel;
    /* 0x5C */ RwUInt32 flags;
    /* 0x60 */ RwUInt32 indirect;
} RpGameCubeTevStage;

typedef struct RpGameCubeTexMtx {
    /* 0x00 */ RwUInt32 id;
    /* 0x04 */ RwInt32 frame;
    /* 0x08 */ RwUInt32 flags;
    /* 0x0C */ RwReal mtx[3][4];
} RpGameCubeTexMtx;

typedef struct RpGameCubeIndMtx {
    /* 0x00 */ RwUInt32 id;
    /* 0x04 */ RwInt32 frame;
    /* 0x08 */ RwUInt32 flags;
    /* 0x0C */ RwReal mtx[2][3];
    /* 0x24 */ RwInt32 scaleExp;
} RpGameCubeIndMtx;

typedef struct RpGameCubeIndStage {
    /* 0x00 */ RwUInt32 flags;
    /* 0x04 */ RwUInt32 texCoord;
    /* 0x08 */ RwUInt32 texMap;
    /* 0x0C */ RwUInt32 scaleS;
    /* 0x10 */ RwUInt32 scaleT;
} RpGameCubeIndStage;

typedef struct RpGameCubeTevIndirect {
    /* 0x00 */ RwUInt32 indStage;
    /* 0x04 */ RwUInt32 format;
    /* 0x08 */ RwUInt32 biasSel;
    /* 0x0C */ RwUInt32 matrixSel;
    /* 0x10 */ RwUInt32 wrapS;
    /* 0x14 */ RwUInt32 wrapT;
    /* 0x18 */ RwUInt8 addPrev;
    /* 0x19 */ RwUInt8 utcLod;
    /* 0x1C */ RwUInt32 alphaSel;
} RpGameCubeTevIndirect;

typedef struct RpGameCubeMTConfig {
    /* 0x00 */ RwUInt32 flags;
    /* 0x04 */ RwUInt8 maxTexGens;
    /* 0x05 */ RwUInt8 maxTevStages;
    /* 0x06 */ RwUInt8 maxTexMtx;
    /* 0x07 */ RwUInt8 maxIndStages;
    /* 0x08 */ RwUInt8 maxIndMtx;
    /* 0x09 */ RwUInt8 numTexGens;
    /* 0x0A */ RwUInt8 numTevStages;
    /* 0x0B */ RwUInt8 numTexMtx;
    /* 0x0C */ RwUInt8 numIndStages;
    /* 0x0D */ RwUInt8 numIndMtx;
    /* 0x0E */ GXColorS10 tevColors[4];
    /* 0x2E */ GXColor kColors[4];
    /* 0x40 */ RpGameCubeTexMtx* texMtx;
    /* 0x44 */ RpGameCubeTexGen* texGens;
    /* 0x48 */ RpGameCubeIndMtx* indMtx;
    /* 0x4C */ RpGameCubeIndStage* indStages;
    /* 0x50 */ RpGameCubeTevStage* tevStages;
    /* 0x54 */ RwUInt32 srcBlend;
    /* 0x58 */ RwUInt32 destBlend;
} RpGameCubeMTConfig;

typedef struct RpMultiTexture RpMultiTexture;

typedef void (*RpGameCubeMTRenderCallBack)(RpMaterial* material, void* object, RxGameCubeRenderData* data,
                                           void* callBackData);

typedef struct RpGameCubeMTPlatformData {
    /* 0x00 */ RpGameCubeMTRenderCallBack preRenderCB;
    /* 0x04 */ RpGameCubeMTRenderCallBack postRenderCB;
    /* 0x08 */ void* callBackData;
} RpGameCubeMTPlatformData;

struct RpMultiTexture {
    /* 0x00 */ void* platformInfo;
    /* 0x04 */ RwUInt32 numTextures;
    /* 0x08 */ RwTexture* textures[8];
    /* 0x28 */ RwUInt8 coords[8];
    /* 0x30 */ RpMTEffect* effect;
    /* 0x34 */ RpGameCubeMTPlatformData* platformData;
};

/* Normal, tangent and binormal of one vertex, as instanced for NBT lighting. */
typedef struct RpGameCubeNBT {
    /* 0x00 */ RwV3d normal;
    /* 0x0C */ RwV3d tangent;
    /* 0x18 */ RwV3d binormal;
} RpGameCubeNBT;

/* Frames registered to drive texture and indirect matrices. */
typedef struct rpGameCubeMTPipeGlobals {
    /* 0x00 */ RwUInt32 numFrames;
    /* 0x04 */ RwFrame** frames;
} rpGameCubeMTPipeGlobals;

typedef struct rpMeshGlobals {
    RwInt16 nextSerialNum;
    RwInt16 pad;
    RwUInt32 unused;
    RwUInt8 meshFlagsToPrimType[32];
    RwUInt8 primTypeToMeshFlags[8];
} rpMeshGlobals;

extern RwModuleInfo meshModule;
extern RwMatrix lbl_8025E304;

static RwInt32 _rpGameCubeMTPipeOffset;

#define RWMESHGLOBAL(var) (RWPLUGINOFFSET(rpMeshGlobals, RwEngineInstance, meshModule.globalsOffset)->var)
#define MTPIPEGLOBAL(var) (RWPLUGINOFFSET(rpGameCubeMTPipeGlobals, RwEngineInstance, _rpGameCubeMTPipeOffset)->var)

void* memset(void* dst, int c, unsigned long n);
void DCFlushRange(void* addr, RwUInt32 nBytes);
void PSMTXConcat(RwReal a[3][4], RwReal b[3][4], RwReal ab[3][4]);
void GXLoadTexMtxImm(RwReal mtx[3][4], RwUInt32 id, RwInt32 type);
void GXSetTevColorS10(RwInt32 id, GXColorS10 color);
void GXSetTevKColor(RwInt32 id, GXColor color);
void GXSetTevKColorSel(RwInt32 stage, RwInt32 sel);
void GXSetTevKAlphaSel(RwInt32 stage, RwInt32 sel);
void GXSetIndTexMtx(RwInt32 id, RwReal offset[2][3], RwInt8 scaleExp);
void GXSetNumIndStages(RwUInt8 nIndStages);
void GXSetIndTexOrder(RwInt32 indStage, RwInt32 texCoord, RwInt32 texMap);
void GXSetIndTexCoordScale(RwInt32 indStage, RwInt32 scaleS, RwInt32 scaleT);
void GXSetTevIndirect(RwInt32 tevStage, RwInt32 indStage, RwInt32 format, RwInt32 biasSel, RwInt32 matrixSel,
                      RwInt32 wrapS, RwInt32 wrapT, RwUInt8 addPrev, RwUInt8 utcLod, RwInt32 alphaSel);
void GXSetTevDirect(RwInt32 tevStage);
void GXSetTexCoordScaleManually(RwInt32 coord, RwUInt8 enable, RwUInt16 ss, RwUInt16 ts);
RwBool RwRenderStateSet(RwInt32 state, void* value);
RwBool RwRenderStateGet(RwInt32 state, void* value);
RwMatrix* RwMatrixMultiply(RwMatrix* matrixOut, const RwMatrix* matrixIn1, const RwMatrix* matrixIn2);
RwMatrix* RwMatrixInvert(RwMatrix* matrixOut, const RwMatrix* matrixIn);
RwReal _rwInvSqrt(RwReal num);
RpMultiTexture* RpMaterialGetMultiTexture(const RpMaterial* material, RwUInt32 platform);
RpGameCubeMTConfig* RpGameCubeMTEffectGetConfig(RpMTEffect* effect);
RpGameCubeTevIndirect* RpGameCubeTevIndUnpack(RpGameCubeTevIndirect* out, const RwUInt32* packed);
RwBool _rpMultiTexturePluginAttach(void);
RwBool _rpMaterialRegisterMultiTexturePlugin(RwUInt32 platform, RwUInt32 pluginID, RwUInt32 dataSize);
RwBool _rpGameCubeMTDataPluginAttach(void);

/* TEV inputs to use when the rasterized colour is replaced by the lit sum. */
static RwUInt32 ColorInputs[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 6, 7, 12, 13, 14, 15 };
static RwUInt32 AlphaInputs[8] = { 0, 1, 2, 3, 4, 3, 6, 7 };

static void* GameCubeMTPipeOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    memset(&MTPIPEGLOBAL(numFrames), 0, sizeof(rpGameCubeMTPipeGlobals));
    return instance;
}

static void* GameCubeMTPipeClose(void* instance, RwInt32 offset, RwInt32 size)
{
    return instance;
}

void _rpGameCubeMTPipeDataCalcNBTs(RpMesh* mesh, RxGameCubeInstanceData* inst, RwInt32 primType)
{
    RpGameCubeNBT* nbt;
    RwV3d* positions;
    RwReal dua;
    RwReal dub;
    RwV3d* pa;
    RwReal lengthSq;
    RwV3d* pb;
    RwReal dva;
    RwTexCoords* texCoords;
    RwTexCoords* tb;
    RpGameCubeNBT* nbts;
    RwUInt16* indices;
    RwReal dot;
    RwV3d* p0;
    RwUInt16 a;
    RwUInt16 b;
    RwTexCoords* t0;
    RwReal dvb;
    RwUInt16 v;
    RwV3d tangent;
    RwInt32 i;
    RwTexCoords* ta;

    positions = (RwV3d*)inst->posData;
    nbts = (RpGameCubeNBT*)inst->nrmData;
    texCoords = (RwTexCoords*)inst->texData[0];
    indices = mesh->indices;
    for (i = 0; i < mesh->numIndices; i++) {
        v = indices[i];
        nbt = &nbts[v];
        if (nbt->tangent.x == 3.4028235e38f) {
            if (primType == rpPRIMTYPETRISTRIP) {
                if (i < 2) {
                    if (i == 0) {
                        a = indices[1];
                        b = indices[2];
                    } else {
                        a = indices[2];
                        b = indices[0];
                    }
                } else {
                    a = indices[i - 2];
                    b = indices[i - 1];
                    if (a == b) {
                        RwUInt16 next = indices[i + 1];

                        if (v == next) {
                            i++;
                            a = indices[i + 1];
                            if (v == a) {
                                i++;
                                a = indices[i + 1];
                            }
                            b = indices[i + 2];
                        } else {
                            a = b;
                            b = next;
                        }
                    }
                    if (i & 1) {
                        RwUInt16 tmp = b;

                        b = a;
                        a = tmp;
                    }
                }
            } else {
                switch (i % 3) {
                case 0:
                    a = indices[i + 1];
                    b = indices[i + 2];
                    break;
                case 1:
                    a = indices[i + 1];
                    b = indices[i - 1];
                    break;
                case 2:
                    a = indices[i - 2];
                    b = indices[i - 1];
                    break;
                }
            }

            p0 = &positions[v];
            pa = &positions[a];
            pb = &positions[b];
            t0 = &texCoords[v];
            ta = &texCoords[a];
            tb = &texCoords[b];

            dvb = tb->v - t0->v;
            dva = ta->v - t0->v;
            dub = tb->u - t0->u;
            dua = ta->u - t0->u;
            tangent.x = (pb->x - p0->x) * dva - (pa->x - p0->x) * dvb;
            tangent.y = (pb->y - p0->y) * dva - (pa->y - p0->y) * dvb;
            tangent.z = (pb->z - p0->z) * dva - (pa->z - p0->z) * dvb;

            dot = -(tangent.x * nbt->normal.x + tangent.y * nbt->normal.y + tangent.z * nbt->normal.z);
            tangent.x = tangent.x + dot * nbt->normal.x;
            tangent.y = tangent.y + dot * nbt->normal.y;
            tangent.z = tangent.z + dot * nbt->normal.z;

            lengthSq = tangent.x * tangent.x + tangent.y * tangent.y + tangent.z * tangent.z;
            if (lengthSq > 0.0f) {
                RwReal recip = _rwInvSqrt(lengthSq);

                tangent.x *= recip;
                tangent.y *= recip;
                tangent.z *= recip;
            }

            nbt->binormal.x = tangent.y * nbt->normal.z - tangent.z * nbt->normal.y;
            nbt->binormal.y = tangent.z * nbt->normal.x - tangent.x * nbt->normal.z;
            nbt->binormal.z = tangent.x * nbt->normal.y - tangent.y * nbt->normal.x;

            if (dva * dub < dvb * dua) {
                nbt->tangent.x = -tangent.x;
                nbt->tangent.y = -tangent.y;
                nbt->tangent.z = -tangent.z;
            } else {
                nbt->tangent = tangent;
            }
        }
    }
}

void fn_80141D20(RxGameCubeRenderData* data, RpGameCubeVtxFmt* fmt, RwInt32 numVerts)
{
    RxGameCubeInstanceData* inst;
    RwInt32 primType;
    RpMesh* mesh;
    RwInt32 i;
    RpGameCubeNBT* nbt;
    RpMeshHeader* meshHeader;

    meshHeader = data->meshHeader;
    inst = RESENTRYDATA(data->repEntry);
    primType = RWMESHGLOBAL(meshFlagsToPrimType)[meshHeader->flags & 0xFF];
    nbt = (RpGameCubeNBT*)inst->nrmData;

    for (i = 0; i < numVerts; i++) {
        nbt->tangent.x = 3.4028235e38f;
        nbt++;
    }

    mesh = (RpMesh*)(meshHeader + 1);
    for (i = 0; i < meshHeader->numMeshes; i++, mesh++) {
        RpMultiTexture* multiTexture = RpMaterialGetMultiTexture(mesh->material, rpMTEFFECTGAMECUBE);

        if (multiTexture && multiTexture->effect && multiTexture->effect->platform == rpMTEFFECTGAMECUBE) {
            RpGameCubeMTConfig* config = RpGameCubeMTEffectGetConfig(multiTexture->effect);
            RwInt32 j;

            for (j = 0; j < config->numTexGens; j++) {
                RpGameCubeTexGen* texGen = &config->texGens[j];

                if (texGen->src == GX_TG_BINRM || texGen->src == GX_TG_TANGENT ||
                    (texGen->func >= GX_TG_BUMP0 && texGen->func <= GX_TG_BUMP7)) {
                    _rpGameCubeMTPipeDataCalcNBTs(mesh, inst, primType);
                    break;
                }
            }
        }
    }

    DCFlushRange(inst->nrmData, numVerts * inst->nrmStride);
    GXInvalidateVtxCache();
}

RwBool _rpGameCubeMTPipeDataQueryNBTs(RxGameCubeRenderData* data)
{
    RpMeshHeader* meshHeader = data->meshHeader;
    RpMesh* mesh = (RpMesh*)(meshHeader + 1);
    RwInt32 i;

    for (i = 0; i < meshHeader->numMeshes; i++, mesh++) {
        RpMultiTexture* multiTexture = RpMaterialGetMultiTexture(mesh->material, rpMTEFFECTGAMECUBE);

        if (multiTexture && multiTexture->effect && multiTexture->effect->platform == rpMTEFFECTGAMECUBE) {
            RpGameCubeMTConfig* config = RpGameCubeMTEffectGetConfig(multiTexture->effect);
            RwInt32 j;

            for (j = 0; j < config->numTexGens; j++) {
                RpGameCubeTexGen* texGen = &config->texGens[j];

                if (texGen->src == GX_TG_BINRM || texGen->src == GX_TG_TANGENT ||
                    (texGen->func >= GX_TG_BUMP0 && texGen->func <= GX_TG_BUMP7)) {
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

#pragma dont_inline on
static RwMatrix* GetTexFrameMatrix(RwInt32 frameType, RwMatrix* ltm, RwMatrix* out)
{
    if (frameType >= rpGCMTFRAMEUSER) {
        rpGameCubeMTPipeGlobals* globals =
            RWPLUGINOFFSET(rpGameCubeMTPipeGlobals, RwEngineInstance, _rpGameCubeMTPipeOffset);
        RwUInt32 index = frameType - rpGCMTFRAMEUSER;
        RwUInt32 numFrames = globals->numFrames;
        RwFrame** frames = globals->frames;

        if (index < numFrames && frames && frames[index]) {
            if (ltm) {
                RwMatrix inverse;

                RwMatrixInvert(&inverse, RwFrameGetLTM(frames[index]));
                RwMatrixMultiply(out, ltm, &inverse);
            } else {
                RwMatrixInvert(out, RwFrameGetLTM(frames[index]));
            }
            return out;
        }
        if (ltm) {
            return ltm;
        }
        out->right.x = out->up.y = out->at.z = 1.0f;
        out->right.y = out->right.z = out->up.x = 0.0f;
        out->up.z = out->at.x = out->at.y = 0.0f;
        out->pos.x = out->pos.y = out->pos.z = 0.0f;
        out->flags |= rwMATRIXINTERNALIDENTITY | rwMATRIXTYPEORTHONORMAL;
        return out;
    }
    if (frameType == rpGCMTFRAMEWORLD) {
        RwMatrix* result;

        if (ltm) {
            RwMatrixMultiply(out, ltm, &lbl_8025E304);
            result = out;
        } else {
            result = &lbl_8025E304;
        }
        return result;
    }
    if (frameType == rpGCMTFRAMEVIEW) {
        if (ltm) {
            return ltm;
        }
        return NULL;
    }
    return NULL;
}
#pragma dont_inline reset

RpMTEffect* _rpGameCubeMTEffectSend(RpMTEffect* effect, RwUInt32 numChans, RwUInt32 numTextures, RwUInt8* coords,
                                    RwMatrix* ltm)
{
    RwInt32 src;
    RwInt32 stage;
    RwInt32 i;
    RwMatrix* matrix;
    RpGameCubeIndStage* indStage;
    RwReal(*offset)[3];
    RwInt32 channel;
    RpGameCubeTexMtx* tm;
    RpGameCubeIndMtx* indMtx;
    RwInt32 firstStage;
    RpGameCubeMTConfig* config;
    RpGameCubeTexGen* texGen;
    RwInt8 texMaps[8];
    RwInt32 chanMode;
    RpGameCubeTevStage* tevStage;

    config = RpGameCubeMTEffectGetConfig(effect);

    if (config->flags & rpGCMTCONFIGTEVCOLORS) {
        GXSetTevColorS10(GX_TEVREG0, config->tevColors[0]);
        GXSetTevColorS10(GX_TEVREG1, config->tevColors[1]);
        GXSetTevColorS10(GX_TEVREG2, config->tevColors[2]);
        GXSetTevColorS10(GX_TEVPREV, config->tevColors[3]);
    }
    if (config->flags & rpGCMTCONFIGKCOLORS) {
        GXSetTevKColor(GX_KCOLOR0, config->kColors[0]);
        GXSetTevKColor(GX_KCOLOR1, config->kColors[1]);
        GXSetTevKColor(GX_KCOLOR2, config->kColors[2]);
        GXSetTevKColor(GX_KCOLOR3, config->kColors[3]);
    }
    if (config->flags & rpGCMTCONFIGENVMAP) {
        RwMatrix combined;
        RwReal texMtx[3][4];
        matrix = ltm ? RwMatrixMultiply(&combined, ltm, &lbl_8025E304) : &lbl_8025E304;
        texMtx[0][0] = -0.5f * matrix->right.x;
        texMtx[0][1] = -0.5f * matrix->up.x;
        texMtx[0][2] = -0.5f * matrix->at.x;
        texMtx[0][3] = 0.5f;
        texMtx[1][0] = -0.5f * matrix->right.y;
        texMtx[1][1] = -0.5f * matrix->up.y;
        texMtx[1][2] = -0.5f * matrix->at.y;
        texMtx[1][3] = 0.5f;
        GXLoadTexMtxImm(texMtx, GX_TEXMTX0, GX_MTX2x4);
    }

    for (i = 0; i < config->numTexMtx; i++) {
        RwMatrix frameMatrix;

        tm = &config->texMtx[i];
        matrix = GetTexFrameMatrix(tm->frame, ltm, &frameMatrix);

        if (matrix) {
            RwReal frame[3][4];
            RwReal result[3][4];

            frame[0][0] = matrix->right.x;
            frame[0][1] = matrix->up.x;
            frame[0][2] = matrix->at.x;
            frame[1][0] = matrix->right.y;
            frame[1][1] = matrix->up.y;
            frame[1][2] = matrix->at.y;
            frame[2][0] = matrix->right.z;
            frame[2][1] = matrix->up.z;
            frame[2][2] = matrix->at.z;
            if (tm->flags & rpGCMTTEXMTXTRANSLATE) {
                frame[0][3] = matrix->pos.x;
                frame[1][3] = matrix->pos.y;
                frame[2][3] = matrix->pos.z;
            } else {
                frame[0][3] = 0.0f;
                frame[1][3] = 0.0f;
                frame[2][3] = 0.0f;
            }
            PSMTXConcat(tm->mtx, frame, result);
            GXLoadTexMtxImm(result, tm->id, tm->flags & rpGCMTTEXMTX3x4);
        } else {
            GXLoadTexMtxImm(tm->mtx, tm->id, tm->flags & rpGCMTTEXMTX3x4);
        }
    }

    for (i = 0; i < config->numIndMtx; i++) {
        RwMatrix frameMatrix;
        RwReal result[2][3];

        indMtx = &config->indMtx[i];
        matrix = GetTexFrameMatrix(indMtx->frame, ltm, &frameMatrix);

        if (matrix) {
            result[0][0] = indMtx->mtx[0][0] * matrix->right.x + indMtx->mtx[0][1] * matrix->right.y +
                           indMtx->mtx[0][2] * matrix->right.z;
            result[0][1] = indMtx->mtx[0][0] * matrix->up.x + indMtx->mtx[0][1] * matrix->up.y +
                           indMtx->mtx[0][2] * matrix->up.z;
            result[0][2] = indMtx->mtx[0][0] * matrix->at.x + indMtx->mtx[0][1] * matrix->at.y +
                           indMtx->mtx[0][2] * matrix->at.z;
            result[1][0] = indMtx->mtx[1][0] * matrix->right.x + indMtx->mtx[1][1] * matrix->right.y +
                           indMtx->mtx[1][2] * matrix->right.z;
            result[1][1] = indMtx->mtx[1][0] * matrix->up.x + indMtx->mtx[1][1] * matrix->up.y +
                           indMtx->mtx[1][2] * matrix->up.z;
            result[1][2] = indMtx->mtx[1][0] * matrix->at.x + indMtx->mtx[1][1] * matrix->at.y +
                           indMtx->mtx[1][2] * matrix->at.z;
            offset = result;
        } else {
            offset = indMtx->mtx;
        }
        GXSetIndTexMtx(indMtx->id, offset, (RwInt8)indMtx->scaleExp);
    }

    if (config->flags & rpGCMTCONFIGCHANNELS) {
        if (numChans > 1) {
            GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
            GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
            GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR1A1);
            GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_RASC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_CPREV);
            GXSetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVREG2);
            GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_RASA, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV);
            GXSetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVREG2);
            firstStage = 2;
            chanMode = 3;
        } else {
            firstStage = 0;
            chanMode = 2;
        }
    } else {
        firstStage = 0;
        chanMode = 1;
    }

    for (i = 0; i < config->numTexGens; i++) {
        texMaps[i] = -1;
    }

    if (config->numIndStages) {
        GXSetNumIndStages(config->numIndStages);
        for (i = 0; i < config->numIndStages; i++) {
            indStage = &config->indStages[i];
            if (!(indStage->flags & rpGCMTDISABLED)) {
                GXSetIndTexOrder(i, indStage->texCoord, indStage->texMap);
                GXSetIndTexCoordScale(i, indStage->scaleS, indStage->scaleT);
                texMaps[indStage->texCoord] = indStage->texMap;
            }
        }
    }

    if (config->numTevStages) {
        GXSetNumTevStages(config->numTevStages + firstStage);
        for (i = 0; i < config->numTevStages; i++) {
            tevStage = &config->tevStages[i];
            if (!(tevStage->flags & rpGCMTDISABLED)) {
                stage = firstStage + i;

                if (tevStage->indirect) {
                    RpGameCubeTevIndirect ind;

                    RpGameCubeTevIndUnpack(&ind, &tevStage->indirect);
                    GXSetTevIndirect(stage, ind.indStage, ind.format, ind.biasSel, ind.matrixSel, ind.wrapS,
                                     ind.wrapT, ind.addPrev, ind.utcLod, ind.alphaSel);
                } else {
                    GXSetTevDirect(stage);
                }

                if (tevStage->channel <= GX_COLOR1A1 && chanMode == 3) {
                    GXSetTevColorIn(stage, ColorInputs[tevStage->colorIn[0]], ColorInputs[tevStage->colorIn[1]],
                                    ColorInputs[tevStage->colorIn[2]], ColorInputs[tevStage->colorIn[3]]);
                    GXSetTevAlphaIn(stage, AlphaInputs[tevStage->alphaIn[0]], AlphaInputs[tevStage->alphaIn[1]],
                                    AlphaInputs[tevStage->alphaIn[2]], AlphaInputs[tevStage->alphaIn[3]]);
                    channel = GX_COLOR_NULL;
                } else {
                    GXSetTevColorIn(stage, tevStage->colorIn[0], tevStage->colorIn[1], tevStage->colorIn[2],
                                    tevStage->colorIn[3]);
                    GXSetTevAlphaIn(stage, tevStage->alphaIn[0], tevStage->alphaIn[1], tevStage->alphaIn[2],
                                    tevStage->alphaIn[3]);
                    channel = tevStage->channel;
                    if (channel <= GX_COLOR1A1 && chanMode == 2) {
                        channel = GX_COLOR0A0;
                    }
                }
                GXSetTevOrder(stage, tevStage->texCoord, tevStage->texMap, channel);
                GXSetTevKColorSel(stage, tevStage->kColorSel);
                GXSetTevKAlphaSel(stage, tevStage->kAlphaSel);
                GXSetTevColorOp(stage, tevStage->colorOp, tevStage->colorBias, tevStage->colorScale,
                                tevStage->colorClamp, tevStage->colorOut);
                GXSetTevAlphaOp(stage, tevStage->alphaOp, tevStage->alphaBias, tevStage->alphaScale,
                                tevStage->alphaClamp, tevStage->alphaOut);
                texMaps[tevStage->texCoord] = tevStage->texMap;
            }
        }
    }

    GXSetNumTexGens(config->numTexGens);
    for (i = 0; i < config->numTexGens; i++) {
        texGen = &config->texGens[i];
        if (!(texGen->flags & rpGCMTDISABLED)) {
            src = texGen->src;

            if (coords && src >= GX_TG_TEX0 && src <= GX_TG_TEX7 && texMaps[i] >= 0) {
                src = coords[texMaps[i]] + GX_TG_TEX0;
            }
            GXSetTexCoordGen2(i, texGen->func, src, texGen->mtx, (texGen->flags & rpGCMTTEXGENNORMALIZE) >> 1,
                              texGen->postMtx);
            if (texGen->flags & rpGCMTTEXGENSCALE) {
                GXSetTexCoordScaleManually(i, GX_TRUE, texGen->scaleS, texGen->scaleT);
            }
        }
    }

    if (config->flags & rpGCMTCONFIGALPHA) {
        _rwDlTextureRasterFlush(3);
    } else {
        _rwDlTextureRasterFlush(1);
    }
    if (config->flags & rpGCMTCONFIGBLEND) {
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)config->srcBlend);
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)config->destBlend);
    }
    return effect;
}

void _rpGameCubeMTMeshRenderCallBack(RxGameCubeDisplayList* dl, RpMaterial* material, void* object,
                                     RxGameCubeRenderData* data, RwMatrix* ltm)
{
    RpGameCubeMTConfig* config;
    RwInt32 srcBlend;
    RwInt32 j;
    RwUInt32 i;
    RwInt32 destBlend;
    RpMultiTexture* multiTexture;
    RpGameCubeMTPlatformData* platformData;
    RwBool twoChans;

    multiTexture = RpMaterialGetMultiTexture(material, rpMTEFFECTGAMECUBE);
    platformData = multiTexture->platformData;
    RwRenderStateGet(rwRENDERSTATESRCBLEND, &srcBlend);
    RwRenderStateGet(rwRENDERSTATEDESTBLEND, &destBlend);

    for (i = 0; i < multiTexture->numTextures; i++) {
        _rwDlTextureSet(multiTexture->textures[i], i);
    }

    if (platformData->preRenderCB) {
        platformData->preRenderCB(material, object, data, platformData->callBackData);
    }

    if (multiTexture->effect) {
        twoChans = FALSE;
        if (data->lightMask0 && data->lightMask1) {
            twoChans = TRUE;
        }
        _rpGameCubeMTEffectSend(multiTexture->effect, (twoChans != FALSE) + 1, multiTexture->numTextures,
                                multiTexture->coords, ltm);
        GXCallDisplayList(dl->list, dl->size);

        config = RpGameCubeMTEffectGetConfig(multiTexture->effect);
        for (j = 0; j < config->numTexGens; j++) {
            if (config->texGens[j].flags & rpGCMTTEXGENSCALE) {
                GXSetTexCoordScaleManually(j, GX_FALSE, 0, 0);
            }
        }
        if (config->numIndStages) {
            GXSetNumIndStages(0);
            for (j = 0; j < config->numIndStages; j++) {
                GXSetIndTexCoordScale(j, 0, 0);
            }
            for (j = 0; j < config->numTevStages; j++) {
                if (config->tevStages[j].indirect) {
                    GXSetTevDirect(j);
                }
            }
        }
    } else {
        GXCallDisplayList(dl->list, dl->size);
    }

    if (platformData->postRenderCB) {
        platformData->postRenderCB(material, object, data, platformData->callBackData);
    }

    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)srcBlend);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)destBlend);
}

#pragma dont_inline on
RwBool _rpGameCubeMTPipePluginAttach(void)
{
    _rpGameCubeMTPipeOffset =
        RwEngineRegisterPlugin(sizeof(rpGameCubeMTPipeGlobals), rwID_GCMTPIPEPLUGIN, GameCubeMTPipeOpen,
                               GameCubeMTPipeClose);
    return _rpGameCubeMTPipeOffset >= 0;
}
#pragma dont_inline reset

RwBool fn_80142C2C(void)
{
    if (!_rpMultiTexturePluginAttach()) {
        return FALSE;
    }
    if (!_rpMaterialRegisterMultiTexturePlugin(rpMTEFFECTGAMECUBE, rwID_GCMTPIPEPLUGIN,
                                               sizeof(RpGameCubeMTPlatformData))) {
        return FALSE;
    }
    if (_rpGameCubeMTDataPluginAttach() == FALSE) {
        return FALSE;
    }
    return _rpGameCubeMTPipePluginAttach();
}
