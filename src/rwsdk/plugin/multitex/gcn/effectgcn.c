/*
 * GameCube multi-texture effects: an effect is a complete TEV configuration
 * (texture coordinate generation, texture and indirect matrices, indirect and
 * TEV stages plus constant colours) allocated as one block, and streamed in a
 * compact byte packed form.
 */

#include "rwsdk/rpworld_int.h"

#define rwPLUGIN_ID 0x120 /* reported under material effects */

#define rpMTEFFECTGAMECUBE 6

typedef struct RpMTEffect {
    /* 0x00 */ RwInt32 platform;
    /* 0x04 */ RwUInt32 refCount;
    /* 0x08 */ RwChar name[32];
    /* 0x28 */ RwLLLink inDict;
} RpMTEffect;

typedef struct GXColor {
    RwUInt8 r;
    RwUInt8 g;
    RwUInt8 b;
    RwUInt8 a;
} GXColor;

typedef struct GXColorS10 {
    RwInt16 r;
    RwInt16 g;
    RwInt16 b;
    RwInt16 a;
} GXColorS10;

typedef struct RpGameCubeTexGen {
    /* 0x00 */ RwUInt32 dst;
    /* 0x04 */ RwUInt32 func;
    /* 0x08 */ RwUInt32 src;
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
    /* 0x58 */ RwUInt32 channel;
    /* 0x5C */ RwUInt32 flags;
    /* 0x60 */ RwUInt32 indirect;
} RpGameCubeTevStage;

typedef struct RpGameCubeTexMtx {
    /* 0x00 */ RwUInt32 id;
    /* 0x04 */ RwUInt32 type;
    /* 0x08 */ RwUInt32 flags;
    /* 0x0C */ RwReal mtx[3][4];
} RpGameCubeTexMtx;

typedef struct RpGameCubeIndMtx {
    /* 0x00 */ RwUInt32 id;
    /* 0x04 */ RwUInt32 flags;
    /* 0x08 */ RwUInt32 type;
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
    /* 0x54 */ RwUInt32 alphaCompare;
    /* 0x58 */ RwUInt32 blendMode;
} RpGameCubeMTConfig;

typedef struct RpGameCubeMTEffect {
    /* 0x00 */ RpMTEffect base;
    /* 0x30 */ RpGameCubeMTConfig config;
} RpGameCubeMTEffect;

typedef struct GameCubeMTEffectHeader {
    /* 0x00 */ RwUInt8 numTexGens;
    /* 0x01 */ RwUInt8 numTevStages;
    /* 0x02 */ RwUInt8 flags;
    /* 0x03 */ RwUInt8 modes;
    /* 0x04 */ GXColorS10 tevColors[4];
    /* 0x24 */ GXColor kColors[4];
    /* 0x34 */ RwUInt8 numTexMtx;
    /* 0x35 */ RwUInt8 numIndMtx;
    /* 0x36 */ RwUInt8 numIndStages;
    /* 0x37 */ RwUInt8 pad;
} GameCubeMTEffectHeader;

typedef struct GameCubeTexGenRecord {
    /* 0x00 */ RwUInt8 dst;
    /* 0x01 */ RwUInt8 func;
    /* 0x02 */ RwUInt8 src;
    /* 0x03 */ RwUInt8 mtx;
    /* 0x04 */ RwUInt8 postMtx;
    /* 0x05 */ RwUInt8 pad[3];
    /* 0x08 */ RwUInt16 scale[2];
} GameCubeTexGenRecord;

typedef struct GameCubeTevStageRecord {
    /* 0x00 */ RwUInt8 colorIn[4];
    /* 0x04 */ RwUInt8 alphaIn[4];
    /* 0x08 */ RwUInt8 colorOp;
    /* 0x09 */ RwUInt8 colorBias;
    /* 0x0A */ RwUInt8 colorScale;
    /* 0x0B */ RwUInt8 colorClamp;
    /* 0x0C */ RwUInt8 colorOut;
    /* 0x0D */ RwUInt8 alphaOp;
    /* 0x0E */ RwUInt8 alphaBias;
    /* 0x0F */ RwUInt8 alphaScale;
    /* 0x10 */ RwUInt8 alphaClamp;
    /* 0x11 */ RwUInt8 alphaOut;
    /* 0x12 */ RwUInt8 kColorSel;
    /* 0x13 */ RwUInt8 kAlphaSel;
    /* 0x14 */ RwUInt8 texCoord;
    /* 0x15 */ RwUInt8 texMap;
    /* 0x16 */ RwUInt8 channel;
    /* 0x17 */ RwUInt8 flags;
    /* 0x18 */ RwUInt32 indirect;
} GameCubeTevStageRecord;

typedef struct GameCubeTexMtxRecord {
    RwUInt8 id;
    RwUInt8 type;
    RwUInt8 flags;
    RwUInt8 pad;
} GameCubeTexMtxRecord;

typedef struct GameCubeIndMtxRecord {
    RwUInt8 id;
    RwUInt8 flags;
    RwUInt8 type;
    RwInt8 scaleExp;
} GameCubeIndMtxRecord;

typedef struct GameCubeIndStageRecord {
    RwUInt8 flags;
    RwUInt8 texCoord;
    RwUInt8 texMap;
    RwUInt8 scaleS;
    RwUInt8 scaleT;
    RwUInt8 pad[3];
} GameCubeIndStageRecord;

#define rpGCTEXMAPDISABLE 0x100
#define rpGCTEVSTAGERECORDTEXDISABLE 0x80

#define ALIGN4(x) (((x) + 3) & ~3)

void* memset(void* dst, int c, unsigned long n);
void* memcpy(void* dst, const void* src, unsigned long n);
RwUInt32 RwStreamRead(RwStream* stream, void* buffer, RwUInt32 length);
RwStream* RwStreamWrite(RwStream* stream, const void* buffer, RwUInt32 length);
RwStream* RwStreamReadReal(RwStream* stream, RwReal* reals, RwUInt32 numBytes);
RwStream* RwStreamWriteReal(RwStream* stream, const RwReal* reals, RwUInt32 numBytes);
void* RwMemLittleEndian16(void* mem, RwUInt32 size);
void* RwMemLittleEndian32(void* mem, RwUInt32 size);
void* RwMemNative32(void* mem, RwUInt32 size);
RpMTEffect* _rpMTEffectInit(RpMTEffect* effect, RwInt32 platform);
RwBool _rpMTEffectRegisterPlatform(RwInt32 platform, void* readCB, void* writeCB, void* getSizeCB, void* destroyCB);

RpGameCubeMTEffect* RpGameCubeMTEffectCreate(RwUInt32 numTevStages, RwUInt32 numTexGens, RwUInt32 numTexMtx,
                                             RwUInt32 numIndStages, RwUInt32 numIndMtx);
RpGameCubeMTConfig* RpGameCubeMTEffectGetConfig(RpGameCubeMTEffect* effect);

RwUInt32 GameCubeMTEffectStreamGetSize(const RpGameCubeMTEffect* effect)
{
    RwUInt32 size;

    size = sizeof(GameCubeMTEffectHeader) + effect->config.numTexMtx * (sizeof(GameCubeTexMtxRecord) + sizeof(RwReal) * 12);
    size += effect->config.numTexGens * sizeof(GameCubeTexGenRecord);
    size += effect->config.numIndMtx * (sizeof(GameCubeIndMtxRecord) + sizeof(RwReal) * 6);
    size += effect->config.numIndStages * sizeof(GameCubeIndStageRecord);
    size += effect->config.numTevStages * sizeof(GameCubeTevStageRecord);
    return size;
}

const RpGameCubeMTEffect* GameCubeMTEffectStreamWrite(const RpGameCubeMTEffect* effect, RwStream* stream)
{
    GameCubeMTEffectHeader header;
    GameCubeTevStageRecord stageRec;
    GameCubeTexGenRecord texGenRec;
    GameCubeIndStageRecord indStageRec;
    GameCubeTexMtxRecord texMtxRec;
    GameCubeIndMtxRecord indMtxRec;
    RwUInt32 i;

    if (!RwStreamWriteChunkHeader(stream, rwID_EXTENSION, GameCubeMTEffectStreamGetSize(effect))) {
        return NULL;
    }

    header.flags = effect->config.flags;
    header.numTevStages = effect->config.numTevStages;
    header.numTexGens = effect->config.numTexGens;
    header.numTexMtx = effect->config.numTexMtx;
    header.numIndMtx = effect->config.numIndMtx;
    header.numIndStages = effect->config.numIndStages;
    header.modes = ((effect->config.alphaCompare & 0xF) << 4) | (effect->config.blendMode & 0xF);
    memcpy(header.kColors, effect->config.kColors, sizeof(header.kColors));
    memcpy(header.tevColors, effect->config.tevColors, sizeof(header.tevColors));
    RwMemLittleEndian16(header.tevColors, sizeof(header.tevColors));

    if (!RwStreamWrite(stream, &header, sizeof(header))) {
        return NULL;
    }

    for (i = 0; i < effect->config.numTexGens; i++) {
        const RpGameCubeTexGen* texGen = &effect->config.texGens[i];

        texGenRec.dst = texGen->dst;
        texGenRec.func = texGen->func;
        texGenRec.src = texGen->src;
        texGenRec.mtx = texGen->mtx;
        texGenRec.postMtx = texGen->postMtx;
        texGenRec.pad[0] = 0;
        texGenRec.pad[1] = 0;
        texGenRec.pad[2] = 0;
        texGenRec.scale[0] = texGen->scaleS;
        texGenRec.scale[1] = texGen->scaleT;
        RwMemLittleEndian16(texGenRec.scale, sizeof(texGenRec.scale));

        if (!RwStreamWrite(stream, &texGenRec, sizeof(texGenRec))) {
            return NULL;
        }
    }

    for (i = 0; i < effect->config.numTevStages; i++) {
        const RpGameCubeTevStage* stage = &effect->config.tevStages[i];

        stageRec.colorIn[0] = stage->colorIn[0];
        stageRec.colorIn[1] = stage->colorIn[1];
        stageRec.colorIn[2] = stage->colorIn[2];
        stageRec.colorIn[3] = stage->colorIn[3];
        stageRec.alphaIn[0] = stage->alphaIn[0];
        stageRec.alphaIn[1] = stage->alphaIn[1];
        stageRec.alphaIn[2] = stage->alphaIn[2];
        stageRec.alphaIn[3] = stage->alphaIn[3];
        stageRec.colorOp = stage->colorOp;
        stageRec.colorBias = stage->colorBias;
        stageRec.colorScale = stage->colorScale;
        stageRec.colorClamp = stage->colorClamp;
        stageRec.colorOut = stage->colorOut;
        stageRec.alphaOp = stage->alphaOp;
        stageRec.alphaBias = stage->alphaBias;
        stageRec.alphaScale = stage->alphaScale;
        stageRec.alphaClamp = stage->alphaClamp;
        stageRec.alphaOut = stage->alphaOut;
        stageRec.kColorSel = stage->kColorSel;
        stageRec.kAlphaSel = stage->kAlphaSel;
        stageRec.texCoord = stage->texCoord;
        stageRec.texMap = stage->texMap;
        stageRec.channel = stage->channel;
        stageRec.flags = stage->flags;
        stageRec.indirect = stage->indirect;
        RwMemLittleEndian32(&stageRec.indirect, sizeof(stageRec.indirect));

        if (stage->texMap & rpGCTEXMAPDISABLE) {
            stageRec.flags |= rpGCTEVSTAGERECORDTEXDISABLE;
        }

        if (!RwStreamWrite(stream, &stageRec, sizeof(stageRec))) {
            return NULL;
        }
    }

    for (i = 0; i < effect->config.numTexMtx; i++) {
        const RpGameCubeTexMtx* texMtx = &effect->config.texMtx[i];

        texMtxRec.id = texMtx->id;
        texMtxRec.type = texMtx->type;
        texMtxRec.flags = texMtx->flags;
        texMtxRec.pad = 0;

        if (!RwStreamWrite(stream, &texMtxRec, sizeof(texMtxRec)) ||
            !RwStreamWriteReal(stream, &texMtx->mtx[0][0], sizeof(texMtx->mtx))) {
            return NULL;
        }
    }

    for (i = 0; i < effect->config.numIndMtx; i++) {
        const RpGameCubeIndMtx* indMtx = &effect->config.indMtx[i];

        indMtxRec.id = indMtx->id;
        indMtxRec.flags = indMtx->flags;
        indMtxRec.type = indMtx->type;
        indMtxRec.scaleExp = indMtx->scaleExp;

        if (!RwStreamWrite(stream, &indMtxRec, sizeof(indMtxRec)) ||
            !RwStreamWriteReal(stream, &indMtx->mtx[0][0], sizeof(indMtx->mtx))) {
            return NULL;
        }
    }

    for (i = 0; i < effect->config.numIndStages; i++) {
        const RpGameCubeIndStage* indStage = &effect->config.indStages[i];

        indStageRec.flags = indStage->flags;
        indStageRec.texCoord = indStage->texCoord;
        indStageRec.texMap = indStage->texMap;
        indStageRec.scaleS = indStage->scaleS;
        indStageRec.scaleT = indStage->scaleT;
        indStageRec.pad[0] = 0;
        indStageRec.pad[1] = 0;
        indStageRec.pad[2] = 0;

        if (!RwStreamWrite(stream, &indStageRec, sizeof(indStageRec))) {
            return NULL;
        }
    }

    return effect;
}

RpGameCubeMTEffect* GameCubeMTEffectStreamRead(RwStream* stream, RwInt32 platform, RwUInt32 version, RwUInt32 length)
{
    GameCubeMTEffectHeader header;
    GameCubeTevStageRecord stageRec;
    GameCubeTexGenRecord texGenRec;
    GameCubeIndStageRecord indStageRec;
    GameCubeTexMtxRecord texMtxRec;
    GameCubeIndMtxRecord indMtxRec;
    RpGameCubeMTEffect* effect;
    RpGameCubeMTConfig* config;
    RwBool oldFormat;
    RwUInt32 size;
    RwUInt32 i;

    oldFormat = version < rwLIBRARYCURRENTVERSION;

    if (oldFormat) {
        size = sizeof(header) - 4;
        header.numTexMtx = 0;
        header.numIndMtx = 0;
        header.numIndStages = 0;
    } else {
        size = sizeof(header);
    }

    if (RwStreamRead(stream, &header, size) != size) {
        return NULL;
    }

    effect = RpGameCubeMTEffectCreate(header.numTevStages, header.numTexGens, header.numTexMtx, header.numIndStages,
                                      header.numIndMtx);
    if (effect == NULL) {
        return NULL;
    }

    config = RpGameCubeMTEffectGetConfig(effect);
    config->flags = header.flags;
    config->alphaCompare = (header.modes >> 4) & 0xF;
    config->blendMode = header.modes & 0xF;
    memcpy(config->kColors, header.kColors, sizeof(header.kColors));
    RwMemNative32(header.tevColors, sizeof(header.tevColors));
    memcpy(config->tevColors, header.tevColors, sizeof(header.tevColors));

    for (i = 0; i < config->numTexGens; i++) {
        RpGameCubeTexGen* texGen = &config->texGens[i];

        if (oldFormat) {
            size = 5;
            texGenRec.scale[0] = 0;
            texGenRec.scale[1] = 0;
        } else {
            size = sizeof(texGenRec);
        }

        if (RwStreamRead(stream, &texGenRec, size) != size) {
            RwFree(effect);
            return NULL;
        }

        texGen->dst = texGenRec.dst;
        texGen->func = texGenRec.func;
        texGen->src = texGenRec.src;
        texGen->mtx = texGenRec.mtx;
        texGen->postMtx = texGenRec.postMtx;
        RwMemNative32(texGenRec.scale, sizeof(texGenRec.scale));
        texGen->scaleS = texGenRec.scale[0];
        texGen->scaleT = texGenRec.scale[1];
    }

    for (i = 0; i < config->numTevStages; i++) {
        RpGameCubeTevStage* stage = &config->tevStages[i];

        if (oldFormat) {
            size = sizeof(stageRec) - 4;
            stageRec.indirect = 0;
        } else {
            size = sizeof(stageRec);
        }

        if (RwStreamRead(stream, &stageRec, size) != size) {
            RwFree(effect);
            return NULL;
        }

        stage->colorIn[0] = stageRec.colorIn[0];
        stage->colorIn[1] = stageRec.colorIn[1];
        stage->colorIn[2] = stageRec.colorIn[2];
        stage->colorIn[3] = stageRec.colorIn[3];
        stage->alphaIn[0] = stageRec.alphaIn[0];
        stage->alphaIn[1] = stageRec.alphaIn[1];
        stage->alphaIn[2] = stageRec.alphaIn[2];
        stage->alphaIn[3] = stageRec.alphaIn[3];
        stage->colorOp = stageRec.colorOp;
        stage->colorBias = stageRec.colorBias;
        stage->colorScale = stageRec.colorScale;
        stage->colorClamp = stageRec.colorClamp;
        stage->colorOut = stageRec.colorOut;
        stage->alphaOp = stageRec.alphaOp;
        stage->alphaBias = stageRec.alphaBias;
        stage->alphaScale = stageRec.alphaScale;
        stage->alphaClamp = stageRec.alphaClamp;
        stage->alphaOut = stageRec.alphaOut;
        stage->kColorSel = stageRec.kColorSel;
        stage->kAlphaSel = stageRec.kAlphaSel;
        stage->texCoord = stageRec.texCoord;
        stage->texMap = stageRec.texMap;
        stage->channel = stageRec.channel;
        stage->flags = stageRec.flags;
        RwMemNative32(&stageRec.indirect, sizeof(stageRec.indirect));
        stage->indirect = stageRec.indirect;

        if (stage->flags & rpGCTEVSTAGERECORDTEXDISABLE) {
            stage->texMap |= rpGCTEXMAPDISABLE;
            stage->flags &= ~rpGCTEVSTAGERECORDTEXDISABLE;
        }
    }

    for (i = 0; i < config->numTexMtx; i++) {
        RpGameCubeTexMtx* texMtx = &config->texMtx[i];

        if (RwStreamRead(stream, &texMtxRec, sizeof(texMtxRec)) != sizeof(texMtxRec) ||
            !RwStreamReadReal(stream, &texMtx->mtx[0][0], sizeof(texMtx->mtx))) {
            RwFree(effect);
            return NULL;
        }

        texMtx->id = texMtxRec.id;
        texMtx->type = texMtxRec.type;
        texMtx->flags = texMtxRec.flags;
    }

    for (i = 0; i < config->numIndMtx; i++) {
        RpGameCubeIndMtx* indMtx = &config->indMtx[i];

        if (RwStreamRead(stream, &indMtxRec, sizeof(indMtxRec)) != sizeof(indMtxRec) ||
            !RwStreamReadReal(stream, &indMtx->mtx[0][0], sizeof(indMtx->mtx))) {
            RwFree(effect);
            return NULL;
        }

        indMtx->id = indMtxRec.id;
        indMtx->flags = indMtxRec.flags;
        indMtx->type = indMtxRec.type;
        indMtx->scaleExp = indMtxRec.scaleExp;
    }

    for (i = 0; i < config->numIndStages; i++) {
        RpGameCubeIndStage* indStage = &config->indStages[i];

        if (RwStreamRead(stream, &indStageRec, sizeof(indStageRec)) != sizeof(indStageRec)) {
            RwFree(effect);
            return NULL;
        }

        indStage->flags = indStageRec.flags;
        indStage->texCoord = indStageRec.texCoord;
        indStage->texMap = indStageRec.texMap;
        indStage->scaleS = indStageRec.scaleS;
        indStage->scaleT = indStageRec.scaleT;
    }

    return effect;
}

RwBool _rpGameCubeMTDataPluginAttach(void)
{
    return _rpMTEffectRegisterPlatform(rpMTEFFECTGAMECUBE, GameCubeMTEffectStreamRead, GameCubeMTEffectStreamWrite,
                                       GameCubeMTEffectStreamGetSize, NULL);
}

RpGameCubeMTEffect* RpGameCubeMTEffectCreate(RwUInt32 numTevStages, RwUInt32 numTexGens, RwUInt32 numTexMtx,
                                             RwUInt32 numIndStages, RwUInt32 numIndMtx)
{
    RwUInt32 texMtxSize = numTexMtx * sizeof(RpGameCubeTexMtx);
    RwUInt32 texGenSize = numTexGens * sizeof(RpGameCubeTexGen);
    RwUInt32 indMtxSize = numIndMtx * sizeof(RpGameCubeIndMtx);
    RwUInt32 indStageSize = numIndStages * sizeof(RpGameCubeIndStage);
    RpGameCubeMTEffect* effect;
    RwUInt32 size;
    RwUInt8* data;

    size = ALIGN4(sizeof(RpGameCubeMTEffect) + texGenSize) + numTevStages * sizeof(RpGameCubeTevStage);
    size = ALIGN4(size) + indStageSize;
    size = ALIGN4(size) + texMtxSize;
    size = ALIGN4(size) + indMtxSize;
    size = ALIGN4(size);

    effect = (RpGameCubeMTEffect*)RwMalloc(size);
    if (effect == NULL) {
        RWERROR((E_RW_NOMEM, size));
        return NULL;
    }

    memset(effect, 0, size);
    _rpMTEffectInit(&effect->base, rpMTEFFECTGAMECUBE);

    effect->config.maxTevStages = numTevStages;
    effect->config.maxTexGens = numTexGens;
    effect->config.maxIndStages = numIndStages;
    effect->config.maxTexMtx = numTexMtx;
    effect->config.maxIndMtx = numIndMtx;
    effect->config.numTevStages = effect->config.maxTevStages;
    effect->config.numTexGens = effect->config.maxTexGens;
    effect->config.numIndStages = effect->config.maxIndStages;
    effect->config.numTexMtx = effect->config.maxTexMtx;
    effect->config.numIndMtx = effect->config.maxIndMtx;

    data = (RwUInt8*)(effect + 1);
    if (numTexMtx) {
        effect->config.texMtx = (RpGameCubeTexMtx*)data;
        data += texMtxSize;
    }
    if (numTexGens) {
        effect->config.texGens = (RpGameCubeTexGen*)data;
        data += texGenSize;
    }
    if (numIndMtx) {
        effect->config.indMtx = (RpGameCubeIndMtx*)data;
        data += indMtxSize;
    }
    if (numIndStages) {
        effect->config.indStages = (RpGameCubeIndStage*)data;
        data += indStageSize;
    }
    if (numTevStages) {
        effect->config.tevStages = (RpGameCubeTevStage*)data;
    }

    return effect;
}

RpGameCubeMTConfig* RpGameCubeMTEffectGetConfig(RpGameCubeMTEffect* effect)
{
    return &effect->config;
}

RpGameCubeTevIndirect* RpGameCubeTevIndUnpack(RpGameCubeTevIndirect* out, const RwUInt32* packed)
{
    RwUInt32 bits = *packed;

    out->indStage = bits & 3;
    out->format = (bits >> 2) & 3;
    out->biasSel = (bits >> 4) & 7;
    out->matrixSel = (bits >> 7) & 0xF;
    out->wrapS = (bits >> 11) & 7;
    out->wrapT = (bits >> 14) & 7;
    out->addPrev = (bits >> 17) & 1;
    out->utcLod = (bits >> 18) & 1;
    out->alphaSel = (bits >> 19) & 3;
    return out;
}
