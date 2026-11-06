/*
 * Material effects plugin: per material bump map, environment map and dual
 * texture passes, plus the atomic and world sector "effects enabled" flags
 * that switch them onto the MatFX pipelines. Rendering is platform code.
 */

#include "rwsdk/rpworld_int.h"
#include "rwsdk/batkreg.h"

#define rwID_MATERIALEFFECTSPLUGIN 0x120
#define rwID_TEXTURE 6

#define rpMATFXEFFECTNULL 0
#define rpMATFXEFFECTBUMPMAP 1
#define rpMATFXEFFECTENVMAP 2
#define rpMATFXEFFECTBUMPENVMAP 3
#define rpMATFXEFFECTDUAL 4

#define rpMAXPASS 2

#define rwRASTERTYPETEXTURE 4
#define rwRASTERFORMATMIPMAP 0x8000
#define rwRASTERFORMATAUTOMIPMAP 0x1000
#define rwTEXTUREADDRESSNATEXTUREADDRESS 0

#define rwRENDERSTATETEXTURERASTER 1
#define rwRENDERSTATETEXTUREADDRESS 2
#define rwRENDERSTATESRCBLEND 10
#define rwRENDERSTATEDESTBLEND 11

#define rwBLENDSRCALPHA 5
#define rwBLENDINVSRCALPHA 6

#define rwTEXTUREBASENAMELENGTH 32

typedef struct RpMaterial {
    /* 0x00 */ RwTexture* texture;
} RpMaterial;

typedef struct MatFXBumpMapData {
    /* 0x00 */ RwFrame* frame;
    /* 0x04 */ RwTexture* bumpedTexture;
    /* 0x08 */ RwTexture* bumpTexture;
    /* 0x0C */ RwReal coef;
    /* 0x10 */ RwReal invBumpWidth;
} MatFXBumpMapData;

typedef struct MatFXEnvMapData {
    /* 0x00 */ RwFrame* frame;
    /* 0x04 */ RwTexture* texture;
    /* 0x08 */ RwReal coef;
    /* 0x0C */ RwInt32 useFrameBufferAlpha;
} MatFXEnvMapData;

typedef struct MatFXDualData {
    /* 0x00 */ RwTexture* texture;
    /* 0x04 */ RwInt32 srcBlendMode;
    /* 0x08 */ RwInt32 dstBlendMode;
} MatFXDualData;

typedef union MatFXEffectUnion {
    MatFXBumpMapData bumpMap;
    MatFXEnvMapData envMap;
    MatFXDualData dual;
} MatFXEffectUnion;

typedef struct MatFXEffectData {
    /* 0x00 */ MatFXEffectUnion data;
    /* 0x14 */ RwInt32 flag;
} MatFXEffectData;

typedef struct rpMatFXMaterialData {
    /* 0x00 */ MatFXEffectData data[rpMAXPASS];
    /* 0x30 */ RwInt32 flags;
} rpMatFXMaterialData;

typedef struct rpMatFXGlobals {
    /* 0x00 */ RwModuleInfo module;
    /* 0x08 */ RwFreeList* materialFreeList;
    /* 0x0C */ RwUInt32 pad0C;
} rpMatFXGlobals;

#define MATFXMATERIALGETDATA(material) (*(rpMatFXMaterialData**)((RwUInt8*)(material) + MatFXMaterialDataOffset))
#define MATFXATOMICGETDATA(atomic) (*(RwBool*)((RwUInt8*)(atomic) + MatFXAtomicDataOffset))
#define MATFXWORLDSECTORGETDATA(sector) (*(RwBool*)((RwUInt8*)(sector) + MatFXWorldSectorDataOffset))

void* memset(void* dst, int c, unsigned long n);
RwUInt32 RwStreamRead(RwStream* stream, void* buffer, RwUInt32 length);
RwStream* RwStreamWrite(RwStream* stream, const void* buffer, RwUInt32 length);
void* RwMemNative32(void* mem, RwUInt32 size);
RwTexture* RwTextureStreamRead(RwStream* stream);
const RwTexture* RwTextureStreamWrite(const RwTexture* texture, RwStream* stream);
RwUInt32 RwTextureStreamGetSize(const RwTexture* texture);
RwBool RwTextureDestroy(RwTexture* texture);
RwTexture* RwTextureCreate(RwRaster* raster);
RwTexture* RwTextureSetName(RwTexture* texture, const RwChar* name);
RwTexDictionary* RwTexDictionaryGetCurrent(void);
RwTexture* RwTexDictionaryAddTexture(RwTexDictionary* dict, RwTexture* texture);
RwImage* RwImageCreate(RwInt32 width, RwInt32 height, RwInt32 depth);
RwBool RwImageDestroy(RwImage* image);
RwImage* RwImageAllocatePixels(RwImage* image);
RwImage* RwImageSetFromRaster(RwImage* image, RwRaster* raster);
RwImage* RwImageResample(RwImage* dstImage, const RwImage* srcImage);
RwImage* RwImageMakeMask(RwImage* image);
RwImage* RwImageApplyMask(RwImage* image, const RwImage* mask);
RwImage* RwImageFindRasterFormat(RwImage* ipImage, RwInt32 nRasterType, RwInt32* npWidth, RwInt32* npHeight,
                                 RwInt32* npDepth, RwInt32* npFormat);
RwRaster* RwRasterCreate(RwInt32 width, RwInt32 height, RwInt32 depth, RwInt32 flags);
RwRaster* RwRasterSetFromImage(RwRaster* raster, RwImage* image);
RwInt32 RpMaterialRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB,
                                 RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);
RwInt32 RpMaterialRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB,
                                       RwPluginDataChunkWriteCallBack writeCB,
                                       RwPluginDataChunkGetSizeCallBack getSizeCB);
RwInt32 RpAtomicRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB,
                                     RwPluginDataChunkWriteCallBack writeCB,
                                     RwPluginDataChunkGetSizeCallBack getSizeCB);

RwBool _rpMatFXPipelinesCreate(void);
RwBool _rpMatFXPipelinesDestroy(void);
RpAtomic* _rpMatFXPipelineAtomicSetup(RpAtomic* atomic);
RpWorldSector* _rpMatFXPipelineWorldSectorSetup(RpWorldSector* worldSector);
void _rpMatFXSetupBumpMapMaterial(RpMaterial* material);
void _rpMatFXSetupEnvMapMaterial(RpMaterial* material);
void _rpMatFXSetupDualMaterial(RpMaterial* material);
void _rpMatFXSetupDualRenderState(MatFXDualData* dualData, RwInt32 renderState);
RwTexture* _rpMatFXSetupBumpMapTexture(const RwTexture* baseTexture, const RwTexture* effectTexture);
RwBool fn_80142C2C(void);

RpMaterial* RpMatFXMaterialSetEffects(RpMaterial* material, RwInt32 flags);
RpMaterial* RpMatFXMaterialSetBumpMapTexture(RpMaterial* material, RwTexture* texture);
RpMaterial* RpMatFXMaterialSetBumpMapFrame(RpMaterial* material, RwFrame* frame);
RpMaterial* RpMatFXMaterialSetBumpMapCoefficient(RpMaterial* material, RwReal coef);
RwFrame* RpMatFXMaterialGetBumpMapFrame(const RpMaterial* material);
RwReal RpMatFXMaterialGetBumpMapCoefficient(const RpMaterial* material);
RpMaterial* RpMatFXMaterialSetEnvMapTexture(RpMaterial* material, RwTexture* texture);
RpMaterial* RpMatFXMaterialSetEnvMapFrame(RpMaterial* material, RwFrame* frame);
RpMaterial* RpMatFXMaterialSetEnvMapFrameBufferAlpha(RpMaterial* material, RwBool useFrameBufferAlpha);
RpMaterial* RpMatFXMaterialSetEnvMapCoefficient(RpMaterial* material, RwReal coef);
RwTexture* RpMatFXMaterialGetEnvMapTexture(const RpMaterial* material);
RwFrame* RpMatFXMaterialGetEnvMapFrame(const RpMaterial* material);
RwBool RpMatFXMaterialGetEnvMapFrameBufferAlpha(const RpMaterial* material);
RwReal RpMatFXMaterialGetEnvMapCoefficient(const RpMaterial* material);
RpMaterial* RpMatFXMaterialSetDualTexture(RpMaterial* material, RwTexture* texture);
RpMaterial* RpMatFXMaterialSetDualBlendModes(RpMaterial* material, RwInt32 srcBlendMode, RwInt32 dstBlendMode);
RwTexture* RpMatFXMaterialGetDualTexture(const RpMaterial* material);
const RpMaterial* RpMatFXMaterialGetDualBlendModes(const RpMaterial* material, RwInt32* srcBlendMode,
                                                   RwInt32* dstBlendMode);
RpAtomic* RpMatFXAtomicEnableEffects(RpAtomic* atomic);
RpWorldSector* RpMatFXWorldSectorEnableEffects(RpWorldSector* worldSector);

rpMatFXGlobals MatFXInfo = { 0 };

RwInt32 MatFXMaterialDataOffset;
RwInt32 MatFXAtomicDataOffset;
RwInt32 MatFXWorldSectorDataOffset;

static inline MatFXEffectData* MatFXMaterialFindData(const RpMaterial* material, RwInt32 flag)
{
    MatFXEffectData* effect;
    rpMatFXMaterialData* matfxData = MATFXMATERIALGETDATA(material);
    RwUInt8 pass;

    for (pass = 0; pass < rpMAXPASS; pass++) {
        effect = &matfxData->data[pass];
        if (effect->flag == flag) {
            return effect;
        }
    }
    return NULL;
}

static inline rpMatFXMaterialData* MatFXMaterialDataGet(RpMaterial* material)
{
    rpMatFXMaterialData* matfxData = MATFXMATERIALGETDATA(material);

    if (matfxData == NULL) {
        matfxData = (rpMatFXMaterialData*)RwFreeListAlloc(MatFXInfo.materialFreeList);
        if (matfxData == NULL) {
            return NULL;
        }
        memset(matfxData, 0, sizeof(rpMatFXMaterialData));
        MATFXMATERIALGETDATA(material) = matfxData;
    }
    return matfxData;
}

static inline void MatFXMaterialDataClean(rpMatFXMaterialData* matfxData)
{
    MatFXEffectData* effect;
    RwUInt8 pass;

    for (pass = 0; pass < rpMAXPASS; pass++) {
        effect = &matfxData->data[pass];

        switch (effect->flag) {
        case rpMATFXEFFECTNULL:
            break;
        case rpMATFXEFFECTBUMPMAP:
            if (effect->data.bumpMap.bumpedTexture) {
                RwTextureDestroy(effect->data.bumpMap.bumpedTexture);
            }
            if (effect->data.bumpMap.bumpTexture) {
                RwTextureDestroy(effect->data.bumpMap.bumpTexture);
            }
            break;
        case rpMATFXEFFECTENVMAP:
            if (effect->data.envMap.texture) {
                RwTextureDestroy(effect->data.envMap.texture);
            }
            break;
        case rpMATFXEFFECTDUAL:
            if (effect->data.dual.texture) {
                RwTextureDestroy(effect->data.dual.texture);
            }
            break;
        }
    }

    memset(matfxData, 0, sizeof(rpMatFXMaterialData));
}

void* MatFXClose(void* instance, RwInt32 offset, RwInt32 size)
{
    if (--MatFXInfo.module.numInstances == 0) {
        _rpMatFXPipelinesDestroy();
    }

    if (MatFXInfo.materialFreeList) {
        RwFreeListDestroy(MatFXInfo.materialFreeList);
        MatFXInfo.materialFreeList = NULL;
    }
    return instance;
}

void* MatFXOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    if (MatFXInfo.module.numInstances == 0) {
        MatFXInfo.materialFreeList = RwFreeListCreate(sizeof(rpMatFXMaterialData), 50, 4);
        if (MatFXInfo.materialFreeList == NULL) {
            return NULL;
        }
        if (!_rpMatFXPipelinesCreate()) {
            return NULL;
        }
    }

    MatFXInfo.module.numInstances++;
    return instance;
}

void* MatFXMaterialConstructor(void* object, RwInt32 offset, RwInt32 size)
{
    MATFXMATERIALGETDATA(object) = NULL;
    return object;
}

void* MatFXMaterialDestructor(void* object, RwInt32 offset, RwInt32 size)
{
    rpMatFXMaterialData* matfxData = MATFXMATERIALGETDATA(object);

    if (matfxData != NULL) {
        MatFXMaterialDataClean(matfxData);
        RwFreeListFree(MatFXInfo.materialFreeList, matfxData);
        MATFXMATERIALGETDATA(object) = NULL;
    }
    return object;
}

void* MatFXMaterialCopy(void* dstObject, const void* srcObject, RwInt32 offset, RwInt32 size)
{
    const RpMaterial* srcMaterial = (const RpMaterial*)srcObject;
    rpMatFXMaterialData* srcData = MATFXMATERIALGETDATA(srcMaterial);
    RpMaterial* dstMaterial = (RpMaterial*)dstObject;
    RwUInt8 pass;
    rpMatFXMaterialData* dstData;

    if (srcData == NULL) {
        return NULL;
    }

    dstData = MatFXMaterialDataGet(dstMaterial);
    if (dstData == NULL) {
        return NULL;
    }

    RpMatFXMaterialSetEffects(dstMaterial, srcData->flags);

    for (pass = 0; pass < rpMAXPASS; pass++) {
        MatFXEffectData* srcEffect = &srcData->data[pass];

        switch (srcEffect->flag) {
        case rpMATFXEFFECTNULL:
            break;
        case rpMATFXEFFECTBUMPMAP: {
            MatFXEffectData* dstEffect = &dstData->data[pass];
            RwFrame* frame = RpMatFXMaterialGetBumpMapFrame(srcMaterial);
            RwReal coef = RpMatFXMaterialGetBumpMapCoefficient(srcMaterial);

            RpMatFXMaterialSetBumpMapFrame(dstMaterial, frame);
            RpMatFXMaterialSetBumpMapCoefficient(dstMaterial, coef);

            dstEffect->data.bumpMap.bumpedTexture = srcEffect->data.bumpMap.bumpedTexture;
            dstEffect->data.bumpMap.bumpTexture = srcEffect->data.bumpMap.bumpTexture;
            if (dstEffect->data.bumpMap.bumpedTexture) {
                dstEffect->data.bumpMap.bumpedTexture->refCount++;
            }
            if (dstEffect->data.bumpMap.bumpTexture) {
                dstEffect->data.bumpMap.bumpTexture->refCount++;
            }
            break;
        }
        case rpMATFXEFFECTENVMAP: {
            RwTexture* texture = RpMatFXMaterialGetEnvMapTexture(srcMaterial);
            RwFrame* frame = RpMatFXMaterialGetEnvMapFrame(srcMaterial);
            RwReal coef = RpMatFXMaterialGetEnvMapCoefficient(srcMaterial);
            RwBool useFrameBufferAlpha = RpMatFXMaterialGetEnvMapFrameBufferAlpha(srcMaterial);

            if (texture) {
                RpMatFXMaterialSetEnvMapTexture(dstMaterial, texture);
            }
            RpMatFXMaterialSetEnvMapFrame(dstMaterial, frame);
            RpMatFXMaterialSetEnvMapFrameBufferAlpha(dstMaterial, useFrameBufferAlpha);
            RpMatFXMaterialSetEnvMapCoefficient(dstMaterial, coef);
            break;
        }
        case rpMATFXEFFECTDUAL: {
            RwTexture* texture = RpMatFXMaterialGetDualTexture(srcMaterial);
            RwInt32 srcBlendMode, dstBlendMode;

            RpMatFXMaterialGetDualBlendModes(srcMaterial, &srcBlendMode, &dstBlendMode);
            if (texture) {
                RpMatFXMaterialSetDualTexture(dstMaterial, texture);
            }
            RpMatFXMaterialSetDualBlendModes(dstMaterial, srcBlendMode, dstBlendMode);
            break;
        }
        }
    }
    return dstObject;
}

RwUInt32 _rpMatFXStreamWriteTexture(RwStream* stream, const RwTexture* texture)
{
    RwUInt32 size;
    RwBool hasTexture = (texture != NULL);

    RwStreamWrite(stream, &hasTexture, sizeof(RwBool));
    size = sizeof(RwBool);
    if (hasTexture) {
        RwUInt32 texSize = RwTextureStreamGetSize(texture) + rwCHUNKHEADERSIZE;

        RwTextureStreamWrite(texture, stream);
        size += texSize;
    }
    return size;
}

RwUInt32 _rpMatFXStreamReadTexture(RwStream* stream, RwTexture** texture)
{
    RwBool hasTexture;
    RwUInt32 length;
    RwUInt32 size;

    RwStreamRead(stream, &hasTexture, sizeof(RwBool));
    size = sizeof(RwBool);
    if (hasTexture) {
        if (RwStreamFindChunk(stream, rwID_TEXTURE, &length, NULL)) {
            *texture = RwTextureStreamRead(stream);
        }
        size = length + rwCHUNKHEADERSIZE + sizeof(RwBool);
    }
    return size;
}

RwUInt32 _rpMatFXStreamSizeTexture(const RwTexture* texture)
{
    RwUInt32 size = sizeof(RwBool);

    if (texture) {
        size = RwTextureStreamGetSize(texture) + rwCHUNKHEADERSIZE + sizeof(RwBool);
    }
    return size;
}

RwStream* MatFXMaterialStreamWrite(RwStream* stream, RwInt32 length, const void* object, RwInt32 offset,
                                   RwInt32 size)
{
    const rpMatFXMaterialData* matfxData = MATFXMATERIALGETDATA(object);
    RwInt32 flags;
    RwUInt8 pass;

    flags = (matfxData == NULL) ? rpMATFXEFFECTNULL : matfxData->flags;
    if (!RwStreamWrite(stream, &flags, sizeof(RwUInt32))) {
        return NULL;
    }
    if (flags == rpMATFXEFFECTNULL) {
        return NULL;
    }

    for (pass = 0; pass < rpMAXPASS; pass++) {
        const MatFXEffectData* effect = &matfxData->data[pass];
        RwInt32 flag = effect->flag;

        if (!RwStreamWrite(stream, &flag, sizeof(RwUInt32))) {
            return NULL;
        }

        switch (flag) {
        case rpMATFXEFFECTNULL:
            break;
        case rpMATFXEFFECTBUMPMAP: {
            RwReal coef = -effect->data.bumpMap.coef;

            if (!RwStreamWrite(stream, &coef, sizeof(RwReal))) {
                return NULL;
            }
            _rpMatFXStreamWriteTexture(stream, effect->data.bumpMap.bumpedTexture);
            _rpMatFXStreamWriteTexture(stream, effect->data.bumpMap.bumpTexture);
            break;
        }
        case rpMATFXEFFECTENVMAP:
            if (!RwStreamWrite(stream, &effect->data.envMap.coef, sizeof(RwReal))) {
                return NULL;
            }
            if (!RwStreamWrite(stream, &effect->data.envMap.useFrameBufferAlpha, sizeof(RwBool))) {
                return NULL;
            }
            _rpMatFXStreamWriteTexture(stream, effect->data.envMap.texture);
            break;
        case rpMATFXEFFECTDUAL:
            if (!RwStreamWrite(stream, &effect->data.dual.srcBlendMode, sizeof(RwInt32))) {
                return NULL;
            }
            if (!RwStreamWrite(stream, &effect->data.dual.dstBlendMode, sizeof(RwInt32))) {
                return NULL;
            }
            _rpMatFXStreamWriteTexture(stream, effect->data.dual.texture);
            break;
        }
    }
    return stream;
}

RwStream* MatFXMaterialStreamRead(RwStream* stream, RwInt32 length, void* object, RwInt32 offset, RwInt32 size)
{
    MatFXEffectData* effect;
    RpMaterial* material = (RpMaterial*)object;
    rpMatFXMaterialData* matfxData = MatFXMaterialDataGet(material);
    RwUInt32 flags;
    RwUInt8 pass;

    if (matfxData == NULL) {
        return NULL;
    }

    if (!RwStreamRead(stream, &flags, sizeof(RwUInt32))) {
        return NULL;
    }
    RwMemNative32(&flags, sizeof(RwUInt32));

    RpMatFXMaterialSetEffects(material, flags);
    if (matfxData->flags == rpMATFXEFFECTNULL) {
        return NULL;
    }

    for (pass = 0; pass < rpMAXPASS; pass++) {
        RwUInt32 flag;

        if (!RwStreamRead(stream, &flag, sizeof(RwUInt32))) {
            return NULL;
        }
        RwMemNative32(&flag, sizeof(RwUInt32));

        switch (flag) {
        case rpMATFXEFFECTNULL:
            break;
        case rpMATFXEFFECTBUMPMAP: {
            RwTexture* bumpedTexture = NULL;
            RwTexture* bumpTexture = NULL;
            RwReal coef;

            effect = &matfxData->data[pass];
            if (!RwStreamRead(stream, &coef, sizeof(RwReal))) {
                return NULL;
            }
            RwMemNative32(&coef, sizeof(RwReal));

            _rpMatFXStreamReadTexture(stream, &bumpedTexture);
            _rpMatFXStreamReadTexture(stream, &bumpTexture);

            if (bumpedTexture) {
                effect->data.bumpMap.bumpedTexture = bumpedTexture;
                effect->data.bumpMap.bumpTexture = bumpTexture;
                effect->data.bumpMap.invBumpWidth =
                    1.0f / (RwReal)effect->data.bumpMap.bumpedTexture->raster->width;
            } else if (bumpTexture) {
                RpMatFXMaterialSetBumpMapTexture(material, bumpTexture);
                RwTextureDestroy(bumpTexture);
            } else {
                effect->data.bumpMap.bumpedTexture = NULL;
                effect->data.bumpMap.bumpTexture = NULL;
            }

            RpMatFXMaterialSetBumpMapCoefficient(material, coef);
            break;
        }
        case rpMATFXEFFECTENVMAP: {
            RwTexture* texture = NULL;
            RwReal coef;
            RwBool useFrameBufferAlpha;

            if (!RwStreamRead(stream, &coef, sizeof(RwReal))) {
                return NULL;
            }
            RwMemNative32(&coef, sizeof(RwReal));

            if (!RwStreamRead(stream, &useFrameBufferAlpha, sizeof(RwBool))) {
                return NULL;
            }
            RwMemNative32(&useFrameBufferAlpha, sizeof(RwBool));

            _rpMatFXStreamReadTexture(stream, &texture);
            if (texture) {
                RpMatFXMaterialSetEnvMapTexture(material, texture);
                RwTextureDestroy(texture);
            }

            RpMatFXMaterialSetEnvMapCoefficient(material, coef);
            RpMatFXMaterialSetEnvMapFrameBufferAlpha(material, useFrameBufferAlpha);
            break;
        }
        case rpMATFXEFFECTDUAL: {
            RwTexture* texture = NULL;
            RwInt32 srcBlendMode;
            RwInt32 dstBlendMode;

            if (!RwStreamRead(stream, &srcBlendMode, sizeof(RwInt32))) {
                return NULL;
            }
            RwMemNative32(&srcBlendMode, sizeof(RwInt32));

            if (!RwStreamRead(stream, &dstBlendMode, sizeof(RwInt32))) {
                return NULL;
            }
            RwMemNative32(&dstBlendMode, sizeof(RwInt32));

            _rpMatFXStreamReadTexture(stream, &texture);
            if (texture) {
                RpMatFXMaterialSetDualTexture(material, texture);
                RwTextureDestroy(texture);
            }

            RpMatFXMaterialSetDualBlendModes(material, srcBlendMode, dstBlendMode);
            break;
        }
        }
    }
    return stream;
}

RwInt32 MatFXMaterialStreamGetSize(const void* object, RwInt32 offset, RwInt32 size)
{
    const MatFXEffectData* effect;
    const rpMatFXMaterialData* matfxData = MATFXMATERIALGETDATA(object);
    RwInt32 streamSize;
    RwUInt8 pass;

    if (matfxData == NULL || matfxData->flags == rpMATFXEFFECTNULL) {
        return 0;
    }

    for (pass = 0, streamSize = sizeof(RwUInt32); pass < rpMAXPASS; pass++) {
        effect = &matfxData->data[pass];

        streamSize += sizeof(RwUInt32);

        switch (effect->flag) {
        case rpMATFXEFFECTNULL:
            break;
        case rpMATFXEFFECTBUMPMAP:
            streamSize += sizeof(RwReal);
            streamSize += _rpMatFXStreamSizeTexture(effect->data.bumpMap.bumpedTexture);
            streamSize += _rpMatFXStreamSizeTexture(effect->data.bumpMap.bumpTexture);
            break;
        case rpMATFXEFFECTENVMAP:
            streamSize += sizeof(RwReal) + sizeof(RwBool);
            streamSize += _rpMatFXStreamSizeTexture(effect->data.envMap.texture);
            break;
        case rpMATFXEFFECTDUAL:
            streamSize += sizeof(RwInt32) + sizeof(RwInt32);
            streamSize += _rpMatFXStreamSizeTexture(effect->data.dual.texture);
            break;
        }
    }
    return streamSize;
}

void* MatFXAtomicConstructor(void* object, RwInt32 offset, RwInt32 size)
{
    MATFXATOMICGETDATA(object) = FALSE;
    return object;
}

void* MatFXAtomicDestructor(void* object, RwInt32 offset, RwInt32 size)
{
    MATFXATOMICGETDATA(object) = FALSE;
    return object;
}

void* MatFXAtomicCopy(void* dstObject, const void* srcObject, RwInt32 offset, RwInt32 size)
{
    if (MATFXATOMICGETDATA(srcObject)) {
        RpMatFXAtomicEnableEffects((RpAtomic*)dstObject);
    }
    return dstObject;
}

RwStream* MatFXAtomicStreamWrite(RwStream* stream, RwInt32 length, const void* object, RwInt32 offset,
                                 RwInt32 size)
{
    if (RwStreamWrite(stream, &MATFXATOMICGETDATA(object), sizeof(RwBool)) == NULL) {
        return NULL;
    }
    return stream;
}

RwStream* MatFXAtomicStreamRead(RwStream* stream, RwInt32 length, void* object, RwInt32 offset, RwInt32 size)
{
    RwBool enabled;

    if (!RwStreamRead(stream, &enabled, sizeof(RwBool))) {
        return NULL;
    }
    RwMemNative32(&enabled, sizeof(RwBool));

    if (enabled) {
        RpMatFXAtomicEnableEffects((RpAtomic*)object);
    }
    return stream;
}

RwInt32 MatFXAtomicStreamGetSize(const void* object, RwInt32 offset, RwInt32 size)
{
    return sizeof(RwBool);
}

void* MatFXWorldSectorConstructor(void* object, RwInt32 offset, RwInt32 size)
{
    MATFXWORLDSECTORGETDATA(object) = FALSE;
    return object;
}

void* MatFXWorldSectorDestructor(void* object, RwInt32 offset, RwInt32 size)
{
    MATFXWORLDSECTORGETDATA(object) = FALSE;
    return object;
}

void* MatFXWorldSectorCopy(void* dstObject, const void* srcObject, RwInt32 offset, RwInt32 size)
{
    if (MATFXWORLDSECTORGETDATA(srcObject)) {
        RpMatFXWorldSectorEnableEffects((RpWorldSector*)dstObject);
    }
    return dstObject;
}

RwStream* MatFXWorldSectorStreamWrite(RwStream* stream, RwInt32 length, const void* object, RwInt32 offset,
                                      RwInt32 size)
{
    if (RwStreamWrite(stream, &MATFXWORLDSECTORGETDATA(object), sizeof(RwBool)) == NULL) {
        return NULL;
    }
    return stream;
}

RwStream* MatFXWorldSectorStreamRead(RwStream* stream, RwInt32 length, void* object, RwInt32 offset,
                                     RwInt32 size)
{
    RwBool enabled;

    if (!RwStreamRead(stream, &enabled, sizeof(RwBool))) {
        return NULL;
    }
    RwMemNative32(&enabled, sizeof(RwBool));

    if (enabled) {
        RpMatFXWorldSectorEnableEffects((RpWorldSector*)object);
    }
    return stream;
}

RwInt32 MatFXWorldSectorStreamGetSize(const void* object, RwInt32 offset, RwInt32 size)
{
    return sizeof(RwBool);
}

static inline RwUInt32 TextureGetAddressing(const RwTexture* texture)
{
    RwUInt32 addressing = rwTEXTUREADDRESSNATEXTUREADDRESS;

    if (((texture->filterAddressing >> 8) & 0xF) == ((texture->filterAddressing >> 12) & 0xF)) {
        addressing = (texture->filterAddressing >> 12) & 0xF;
    }
    return addressing;
}

RwTexture* _rpMatFXTextureMaskCreate(const RwTexture* baseTexture, const RwTexture* effectTexture)
{
    RwRaster* baseRaster = NULL;
    RwRaster* effectRaster = effectTexture->raster;
    RwInt32 effectWidth = effectRaster->width;
    RwInt32 effectHeight = effectRaster->height;
    RwInt32 width;
    RwInt32 height;
    RwImage* baseImage;
    RwTexture* texture;
    RwRaster* raster;
    RwImage* effectImage;
    RwInt32 rasterWidth, rasterHeight, rasterDepth, rasterFormat;
    RwUInt32 addressing;
    RwUInt32 filter;

    effectImage = RwImageCreate(effectWidth, effectHeight, 32);
    RwImageAllocatePixels(effectImage);
    RwImageSetFromRaster(effectImage, effectRaster);

    if (baseTexture) {
        baseRaster = baseTexture->raster;
        width = baseRaster->width;
        height = baseRaster->height;
        baseImage = RwImageCreate(width, height, 32);
        RwImageAllocatePixels(baseImage);
        RwImageSetFromRaster(baseImage, baseRaster);
    } else {
        RwInt32 x, y;

        width = effectRaster->width;
        height = effectRaster->height;
        baseImage = RwImageCreate(width, height, 32);
        RwImageAllocatePixels(baseImage);

        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++) {
                ((RwUInt32*)(baseImage->cpPixels + baseImage->stride * y))[x] = 0xFFFFFFFF;
            }
        }
    }

    if (width != effectWidth || height != effectHeight) {
        RwImage* resampled = RwImageCreate(width, height, 32);

        RwImageAllocatePixels(resampled);
        RwImageResample(resampled, effectImage);
        RwImageDestroy(effectImage);
        effectImage = resampled;
    }

    RwImageMakeMask(effectImage);
    RwImageApplyMask(baseImage, effectImage);

    RwImageFindRasterFormat(baseImage, rwRASTERTYPETEXTURE, &rasterWidth, &rasterHeight, &rasterDepth, &rasterFormat);
    rasterFormat |= (baseTexture ? RwRasterGetFormat(baseRaster) : RwRasterGetFormat(effectRaster)) &
                    (rwRASTERFORMATMIPMAP | rwRASTERFORMATAUTOMIPMAP);

    raster = RwRasterCreate(rasterWidth, rasterHeight, rasterDepth, rasterFormat);
    RwRasterSetFromImage(raster, baseImage);
    texture = RwTextureCreate(raster);

    if (baseTexture) {
        filter = baseTexture->filterAddressing & 0xFF;
        addressing = TextureGetAddressing(baseTexture);
    } else {
        filter = effectTexture->filterAddressing & 0xFF;
        addressing = TextureGetAddressing(effectTexture);
    }

    texture->filterAddressing =
        (texture->filterAddressing & ~0xFF00) | (((addressing << 8) & 0xF00) | ((addressing << 12) & 0xF000));
    texture->filterAddressing = (texture->filterAddressing & ~0xFF) | (filter & 0xFF);

    RwImageDestroy(baseImage);
    RwImageDestroy(effectImage);

    {
        RwChar name[rwTEXTUREBASENAMELENGTH] = { 0 };
        const RwChar* names[2];
        RwInt32 i = 0;
        RwChar* dst = name;

        if (baseTexture) {
            names[0] = baseTexture->name;
            names[1] = effectTexture->name;
        } else {
            names[0] = names[1] = effectTexture->name;
        }

        while (i < rwTEXTUREBASENAMELENGTH - 1 && (*names[0] || *names[1])) {
            RwInt32 j;

            for (j = 0; j < 2; j++) {
                if (*names[j]) {
                    *dst++ = *names[j]++;
                    i++;
                }
            }
        }

        RwTextureSetName(texture, name);
    }
    return texture;
}

RwBool RpMatFXPluginAttach(void)
{
    if (RwEngineRegisterPlugin(0, rwID_MATERIALEFFECTSPLUGIN, MatFXOpen, MatFXClose) < 0) {
        return FALSE;
    }

    MatFXMaterialDataOffset =
        RpMaterialRegisterPlugin(sizeof(rpMatFXMaterialData*), rwID_MATERIALEFFECTSPLUGIN, MatFXMaterialConstructor,
                                 MatFXMaterialDestructor, MatFXMaterialCopy);
    if (MatFXMaterialDataOffset < 0) {
        return FALSE;
    }
    if (RpMaterialRegisterPluginStream(rwID_MATERIALEFFECTSPLUGIN, MatFXMaterialStreamRead, MatFXMaterialStreamWrite,
                                       MatFXMaterialStreamGetSize) < 0) {
        return FALSE;
    }

    MatFXAtomicDataOffset = RpAtomicRegisterPlugin(sizeof(RwBool), rwID_MATERIALEFFECTSPLUGIN,
                                                   MatFXAtomicConstructor, MatFXAtomicDestructor, MatFXAtomicCopy);
    if (MatFXAtomicDataOffset < 0) {
        return FALSE;
    }
    if (RpAtomicRegisterPluginStream(rwID_MATERIALEFFECTSPLUGIN, MatFXAtomicStreamRead, MatFXAtomicStreamWrite,
                                     MatFXAtomicStreamGetSize) < 0) {
        return FALSE;
    }

    MatFXWorldSectorDataOffset =
        RpWorldSectorRegisterPlugin(sizeof(RwBool), rwID_MATERIALEFFECTSPLUGIN, MatFXWorldSectorConstructor,
                                    MatFXWorldSectorDestructor, MatFXWorldSectorCopy);
    if (MatFXWorldSectorDataOffset < 0) {
        return FALSE;
    }
    if (RpWorldSectorRegisterPluginStream(rwID_MATERIALEFFECTSPLUGIN, MatFXWorldSectorStreamRead,
                                          MatFXWorldSectorStreamWrite, MatFXWorldSectorStreamGetSize) < 0) {
        return FALSE;
    }

    return fn_80142C2C() ? TRUE : FALSE;
}

RpAtomic* RpMatFXAtomicEnableEffects(RpAtomic* atomic)
{
    RwInt32 offset = MatFXAtomicDataOffset;

    if (!*(RwBool*)((RwUInt8*)atomic + offset)) {
        if (!_rpMatFXPipelineAtomicSetup(atomic)) {
            return NULL;
        }
        *(RwBool*)((RwUInt8*)atomic + offset) = TRUE;
    }
    return atomic;
}

RpWorldSector* RpMatFXWorldSectorEnableEffects(RpWorldSector* worldSector)
{
    RwInt32 offset = MatFXWorldSectorDataOffset;

    if (!*(RwBool*)((RwUInt8*)worldSector + offset)) {
        if (!_rpMatFXPipelineWorldSectorSetup(worldSector)) {
            return NULL;
        }
        *(RwBool*)((RwUInt8*)worldSector + offset) = TRUE;
    }
    return worldSector;
}

RpMaterial* RpMatFXMaterialSetEffects(RpMaterial* material, RwInt32 flags)
{
    rpMatFXMaterialData* matfxData = MatFXMaterialDataGet(material);

    if (matfxData == NULL) {
        return NULL;
    }

    if (flags == rpMATFXEFFECTNULL ||
        (matfxData->flags != rpMATFXEFFECTNULL && matfxData->flags != flags)) {
        MatFXMaterialDataClean(matfxData);
    }

    matfxData->flags = flags;

    switch (matfxData->flags) {
    case rpMATFXEFFECTNULL:
        break;
    case rpMATFXEFFECTBUMPMAP:
        matfxData->data[0].flag = rpMATFXEFFECTBUMPMAP;
        _rpMatFXSetupBumpMapMaterial(material);
        break;
    case rpMATFXEFFECTENVMAP:
        matfxData->data[0].flag = rpMATFXEFFECTENVMAP;
        _rpMatFXSetupEnvMapMaterial(material);
        break;
    case rpMATFXEFFECTBUMPENVMAP:
        matfxData->data[0].flag = rpMATFXEFFECTBUMPMAP;
        matfxData->data[1].flag = rpMATFXEFFECTENVMAP;
        _rpMatFXSetupBumpMapMaterial(material);
        _rpMatFXSetupEnvMapMaterial(material);
        break;
    case rpMATFXEFFECTDUAL:
        matfxData->data[0].flag = rpMATFXEFFECTDUAL;
        _rpMatFXSetupDualMaterial(material);
        RpMatFXMaterialSetDualBlendModes(material, rwBLENDSRCALPHA, rwBLENDINVSRCALPHA);
        break;
    }
    return material;
}

RpMaterial* RpMatFXMaterialSetupBumpMap(RpMaterial* material, RwTexture* texture, RwFrame* frame, RwReal coef)
{
    if (RpMatFXMaterialSetBumpMapTexture(material, texture) == NULL) {
        return NULL;
    }
    if (RpMatFXMaterialSetBumpMapFrame(material, frame) == NULL) {
        return NULL;
    }
    if (RpMatFXMaterialSetBumpMapCoefficient(material, coef) == NULL) {
        return NULL;
    }
    return material;
}

RpMaterial* RpMatFXMaterialSetupEnvMap(RpMaterial* material, RwTexture* texture, RwFrame* frame,
                                       RwBool useFrameBufferAlpha, RwReal coef)
{
    if (RpMatFXMaterialSetEnvMapTexture(material, texture) == NULL) {
        return NULL;
    }
    if (RpMatFXMaterialSetEnvMapFrame(material, frame) == NULL) {
        return NULL;
    }
    if (RpMatFXMaterialSetEnvMapFrameBufferAlpha(material, useFrameBufferAlpha) == NULL) {
        return NULL;
    }
    if (RpMatFXMaterialSetEnvMapCoefficient(material, coef) == NULL) {
        return NULL;
    }
    return material;
}

RwUInt32 RpMatFXMaterialGetEffects(const RpMaterial* material)
{
    const rpMatFXMaterialData* matfxData = MATFXMATERIALGETDATA(material);

    if (matfxData == NULL) {
        return rpMATFXEFFECTNULL;
    }
    return matfxData->flags;
}

RpMaterial* RpMatFXMaterialSetBumpMapTexture(RpMaterial* material, RwTexture* texture)
{
    MatFXBumpMapData* bumpMapData = &MatFXMaterialFindData(material, rpMATFXEFFECTBUMPMAP)->data.bumpMap;
    RwRaster* raster;

    if (bumpMapData->bumpTexture) {
        RwTextureDestroy(bumpMapData->bumpTexture);
        bumpMapData->bumpTexture = NULL;
    }

    if (bumpMapData->bumpedTexture) {
        RwTextureDestroy(bumpMapData->bumpedTexture);
        bumpMapData->bumpedTexture = NULL;
        bumpMapData->invBumpWidth = 0.0f;
    }

    if (texture) {
        RwTexDictionary* dict;

        bumpMapData->bumpedTexture = _rpMatFXSetupBumpMapTexture(material->texture, texture);
        if (bumpMapData->bumpedTexture == NULL) {
            return NULL;
        }

        bumpMapData->bumpTexture = texture;
        bumpMapData->bumpTexture->refCount++;

        dict = RwTexDictionaryGetCurrent();
        if (dict) {
            RwTexDictionaryAddTexture(dict, bumpMapData->bumpedTexture);
        }

        raster = bumpMapData->bumpedTexture->raster;
    } else {
        raster = material->texture->raster;
    }

    bumpMapData->invBumpWidth = 1.0f / (RwReal)(RwUInt32)raster->width;
    return material;
}

RpMaterial* RpMatFXMaterialSetBumpMapFrame(RpMaterial* material, RwFrame* frame)
{
    MatFXMaterialFindData(material, rpMATFXEFFECTBUMPMAP)->data.bumpMap.frame = frame;
    return material;
}

RpMaterial* RpMatFXMaterialSetBumpMapCoefficient(RpMaterial* material, RwReal coef)
{
    MatFXMaterialFindData(material, rpMATFXEFFECTBUMPMAP)->data.bumpMap.coef = -coef;
    return material;
}

RwFrame* RpMatFXMaterialGetBumpMapFrame(const RpMaterial* material)
{
    return MatFXMaterialFindData(material, rpMATFXEFFECTBUMPMAP)->data.bumpMap.frame;
}

RwReal RpMatFXMaterialGetBumpMapCoefficient(const RpMaterial* material)
{
    return -MatFXMaterialFindData(material, rpMATFXEFFECTBUMPMAP)->data.bumpMap.coef;
}

RpMaterial* RpMatFXMaterialSetEnvMapTexture(RpMaterial* material, RwTexture* texture)
{
    MatFXEnvMapData* envMapData = &MatFXMaterialFindData(material, rpMATFXEFFECTENVMAP)->data.envMap;

    texture->refCount++;

    if (envMapData->texture) {
        RwTextureDestroy(envMapData->texture);
        envMapData->texture = NULL;
    }

    envMapData->texture = texture;
    return material;
}

RpMaterial* RpMatFXMaterialSetEnvMapFrame(RpMaterial* material, RwFrame* frame)
{
    MatFXMaterialFindData(material, rpMATFXEFFECTENVMAP)->data.envMap.frame = frame;
    return material;
}

RpMaterial* RpMatFXMaterialSetEnvMapFrameBufferAlpha(RpMaterial* material, RwBool useFrameBufferAlpha)
{
    MatFXMaterialFindData(material, rpMATFXEFFECTENVMAP)->data.envMap.useFrameBufferAlpha = useFrameBufferAlpha;
    return material;
}

RpMaterial* RpMatFXMaterialSetEnvMapCoefficient(RpMaterial* material, RwReal coef)
{
    MatFXMaterialFindData(material, rpMATFXEFFECTENVMAP)->data.envMap.coef = coef;
    return material;
}

RwTexture* RpMatFXMaterialGetEnvMapTexture(const RpMaterial* material)
{
    return MatFXMaterialFindData(material, rpMATFXEFFECTENVMAP)->data.envMap.texture;
}

RwFrame* RpMatFXMaterialGetEnvMapFrame(const RpMaterial* material)
{
    return MatFXMaterialFindData(material, rpMATFXEFFECTENVMAP)->data.envMap.frame;
}

RwBool RpMatFXMaterialGetEnvMapFrameBufferAlpha(const RpMaterial* material)
{
    return MatFXMaterialFindData(material, rpMATFXEFFECTENVMAP)->data.envMap.useFrameBufferAlpha;
}

RwReal RpMatFXMaterialGetEnvMapCoefficient(const RpMaterial* material)
{
    return MatFXMaterialFindData(material, rpMATFXEFFECTENVMAP)->data.envMap.coef;
}

RpMaterial* RpMatFXMaterialSetDualTexture(RpMaterial* material, RwTexture* texture)
{
    MatFXDualData* dualData = &MatFXMaterialFindData(material, rpMATFXEFFECTDUAL)->data.dual;

    texture->refCount++;

    if (dualData->texture) {
        RwTextureDestroy(dualData->texture);
        dualData->texture = NULL;
    }

    dualData->texture = texture;

    _rpMatFXSetupDualRenderState(dualData, rwRENDERSTATETEXTUREADDRESS);
    _rpMatFXSetupDualRenderState(dualData, rwRENDERSTATETEXTURERASTER);
    return material;
}

RpMaterial* RpMatFXMaterialSetDualBlendModes(RpMaterial* material, RwInt32 srcBlendMode, RwInt32 dstBlendMode)
{
    MatFXDualData* dualData = &MatFXMaterialFindData(material, rpMATFXEFFECTDUAL)->data.dual;

    dualData->srcBlendMode = srcBlendMode;
    dualData->dstBlendMode = dstBlendMode;

    _rpMatFXSetupDualRenderState(dualData, rwRENDERSTATESRCBLEND);
    _rpMatFXSetupDualRenderState(dualData, rwRENDERSTATEDESTBLEND);
    return material;
}

RwTexture* RpMatFXMaterialGetDualTexture(const RpMaterial* material)
{
    return MatFXMaterialFindData(material, rpMATFXEFFECTDUAL)->data.dual.texture;
}

const RpMaterial* RpMatFXMaterialGetDualBlendModes(const RpMaterial* material, RwInt32* srcBlendMode,
                                                   RwInt32* dstBlendMode)
{
    const MatFXDualData* dualData = &MatFXMaterialFindData(material, rpMATFXEFFECTDUAL)->data.dual;

    *srcBlendMode = dualData->srcBlendMode;
    *dstBlendMode = dualData->dstBlendMode;
    return material;
}
