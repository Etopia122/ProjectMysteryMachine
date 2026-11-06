/*
 * Multi-texture plugin: up to eight extra textures per material, each with a
 * texture coordinate set index, plus the effect that combines them. Each
 * platform registers its own material plugin slot and private data size.
 */

#include "rwsdk/rpworld_int.h"
#include "rwsdk/batkreg.h"

#define rwPLUGIN_ID 0x120 /* reported under material effects */

#define rwID_MULTITEXPLUGIN 0x12C

#define rpMULTITEXTUREMAXTEXTURES 8
#define rpMULTITEXTUREMAXPLATFORMS 9

typedef struct RpMaterial RpMaterial;
typedef struct RpMTEffect RpMTEffect;

struct RpMTEffect {
    /* 0x00 */ RwUInt32 pad00[2];
    /* 0x08 */ RwChar name[32];
};

typedef struct RpMultiTexturePlatformInfo {
    /* 0x00 */ RwUInt32 platform;
    /* 0x04 */ RwUInt32 pluginID;
    /* 0x08 */ RwInt32 offset;
    /* 0x0C */ RwUInt32 dataSize;
} RpMultiTexturePlatformInfo;

typedef struct RpMultiTexture {
    /* 0x00 */ RpMultiTexturePlatformInfo* platformInfo;
    /* 0x04 */ RwUInt32 numTextures;
    /* 0x08 */ RwTexture* textures[rpMULTITEXTUREMAXTEXTURES];
    /* 0x28 */ RwUInt8 coords[rpMULTITEXTUREMAXTEXTURES];
    /* 0x30 */ RpMTEffect* effect;
    /* 0x34 */ void* platformData;
} RpMultiTexture;

#define MULTITEXTUREGETDATA(object, offset) (*(RpMultiTexture**)((RwUInt8*)(object) + (offset)))

void* memset(void* dst, int c, unsigned long n);
RwUInt32 RwStreamRead(RwStream* stream, void* buffer, RwUInt32 length);
RwStream* RwStreamWrite(RwStream* stream, const void* buffer, RwUInt32 length);
RwUInt32 RwEngineGetVersion(void);
RwBool RwTextureDestroy(RwTexture* texture);
RwInt32 _rwStringStreamGetSize(const RwChar* string);
const RwChar* _rwStringStreamWrite(const RwChar* string, RwStream* stream);
RwChar* _rwStringStreamFindAndRead(RwChar* string, RwStream* stream);
RwInt32 RpMaterialRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB,
                                 RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);
RwInt32 RpMaterialRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB,
                                       RwPluginDataChunkWriteCallBack writeCB,
                                       RwPluginDataChunkGetSizeCallBack getSizeCB);

RwUInt32 _rpMatFXStreamWriteTexture(RwStream* stream, const RwTexture* texture);
RwUInt32 _rpMatFXStreamReadTexture(RwStream* stream, RwTexture** texture);
RwUInt32 _rpMatFXStreamSizeTexture(const RwTexture* texture);

RwBool _rpMTEffectSystemInit(void);
void _rpMTEffectOpen(void);
void _rpMTEffectClose(void);
RwBool RpMTEffectDestroy(RpMTEffect* effect);
RpMTEffect* RpMTEffectFind(const RwChar* name);
RpMTEffect* RpMTEffectCreateDummy(void);
RpMTEffect* RpMTEffectSetName(RpMTEffect* effect, const RwChar* name);
void RpMTEffectAddRef(RpMTEffect* effect);

RpMultiTexture* RpMultiTextureSetEffect(RpMultiTexture* multiTexture, RpMTEffect* effect);
RpMTEffect* RpMultiTextureGetEffect(const RpMultiTexture* multiTexture);
RpMultiTexture* RpMultiTextureSetTexture(RpMultiTexture* multiTexture, RwUInt32 index, RwTexture* texture);
RwTexture* RpMultiTextureGetTexture(const RpMultiTexture* multiTexture, RwUInt32 index);
RpMultiTexture* RpMultiTextureSetCoords(RpMultiTexture* multiTexture, RwUInt32 index, RwUInt32 coords);
RwUInt32 RpMultiTextureGetCoords(const RpMultiTexture* multiTexture, RwUInt32 index);

static RpMultiTexturePlatformInfo MultiTextureInfo[rpMULTITEXTUREMAXPLATFORMS];

RwModuleInfo MultiTextureModule = { 0 };

static inline RpMultiTexture* MultiTextureCreate(RwUInt32 numTextures, RpMultiTexturePlatformInfo* platformInfo)
{
    RpMultiTexture* multiTexture;
    RwUInt32 size = platformInfo->dataSize + sizeof(RpMultiTexture);

    multiTexture = (RpMultiTexture*)RwMalloc(size);

    if (multiTexture == NULL) {
        RWERROR((E_RW_NOMEM, size));
        return NULL;
    }

    memset(multiTexture, 0, size);
    multiTexture->platformInfo = platformInfo;
    multiTexture->numTextures = numTextures;
    if (platformInfo->dataSize) {
        multiTexture->platformData = multiTexture + 1;
    }
    return multiTexture;
}

static inline void MultiTextureDestroy(RpMultiTexture* multiTexture)
{
    RwUInt32 i;

    for (i = 0; i < multiTexture->numTextures; i++) {
        if (multiTexture->textures[i]) {
            RwTextureDestroy(multiTexture->textures[i]);
            multiTexture->textures[i] = NULL;
        }
    }

    if (multiTexture->effect) {
        RpMTEffectDestroy(multiTexture->effect);
        multiTexture->effect = NULL;
    }

    RwFree(multiTexture);
}

void* MultiTextureOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    MultiTextureModule.numInstances++;
    _rpMTEffectOpen();
    return instance;
}

void* MultiTextureClose(void* instance, RwInt32 offset, RwInt32 size)
{
    _rpMTEffectClose();
    MultiTextureModule.numInstances--;
    return instance;
}

void* MultiTextureConstructor(void* object, RwInt32 offset, RwInt32 size)
{
    MULTITEXTUREGETDATA(object, offset) = NULL;
    return object;
}

void* MultiTextureDestructor(void* object, RwInt32 offset, RwInt32 size)
{
    RpMultiTexture* multiTexture = MULTITEXTUREGETDATA(object, offset);

    if (multiTexture != NULL) {
        MultiTextureDestroy(multiTexture);
        MULTITEXTUREGETDATA(object, offset) = NULL;
    }
    return object;
}

void* MultiTextureCopy(void* dstObject, const void* srcObject, RwInt32 offset, RwInt32 size)
{
    RpMultiTexture* dstMultiTexture;
    RwUInt32 i;
    const RpMultiTexture* srcMultiTexture = MULTITEXTUREGETDATA(srcObject, offset);

    if (srcMultiTexture == NULL) {
        return dstObject;
    }

    dstMultiTexture = MultiTextureCreate(srcMultiTexture->numTextures, srcMultiTexture->platformInfo);
    if (dstMultiTexture == NULL) {
        return NULL;
    }

    MULTITEXTUREGETDATA(dstObject, offset) = dstMultiTexture;

    for (i = 0; i < srcMultiTexture->numTextures; i++) {
        RpMultiTextureSetTexture(dstMultiTexture, i, RpMultiTextureGetTexture(srcMultiTexture, i));
        RpMultiTextureSetCoords(dstMultiTexture, i, RpMultiTextureGetCoords(srcMultiTexture, i));
    }

    RpMultiTextureSetEffect(dstMultiTexture, RpMultiTextureGetEffect(srcMultiTexture));
    return dstObject;
}

RwInt32 MultiTextureStreamGetSize(const void* object, RwInt32 offset, RwInt32 size)
{
    const RpMultiTexture* multiTexture = MULTITEXTUREGETDATA(object, offset);
    RwInt32 streamSize = 0;

    if (multiTexture != NULL) {
        RwUInt32 i;

        streamSize = sizeof(RwUInt32) + sizeof(RwUInt32) + multiTexture->numTextures;

        for (i = 0; i < multiTexture->numTextures; i++) {
            streamSize += _rpMatFXStreamSizeTexture(multiTexture->textures[i]);
        }

        if (multiTexture->effect) {
            streamSize += rwCHUNKHEADERSIZE + _rwStringStreamGetSize(multiTexture->effect->name);
        }
    }
    return streamSize;
}

RwStream* MultiTextureStreamWrite(RwStream* stream, RwInt32 length, const void* object, RwInt32 offset,
                                  RwInt32 size)
{
    const RpMultiTexture* multiTexture = MULTITEXTUREGETDATA(object, offset);

    if (multiTexture != NULL) {
        RwUInt32 version;
        RwUInt8 header[4];

        version = RwEngineGetVersion();
        if (!RwStreamWriteInt32(stream, (RwInt32*)&version, sizeof(RwUInt32))) {
            return NULL;
        }

        header[0] = multiTexture->platformInfo->platform;
        header[1] = multiTexture->numTextures;
        header[2] = (multiTexture->effect != NULL);
        header[3] = 0;
        if (!RwStreamWrite(stream, header, sizeof(header))) {
            return NULL;
        }

        if (multiTexture->numTextures) {
            RwUInt32 i;

            if (!RwStreamWrite(stream, multiTexture->coords, multiTexture->numTextures)) {
                return NULL;
            }

            for (i = 0; i < multiTexture->numTextures; i++) {
                _rpMatFXStreamWriteTexture(stream, multiTexture->textures[i]);
            }
        }

        if (multiTexture->effect) {
            if (!_rwStringStreamWrite(multiTexture->effect->name, stream)) {
                return NULL;
            }
        }
    }
    return stream;
}

RwStream* MultiTextureStreamRead(RwStream* stream, RwInt32 length, void* object, RwInt32 offset, RwInt32 size)
{
    RwUInt32 version;
    RwUInt8 header[4];
    RwChar name[32];
    RwUInt32 numTextures;
    RpMultiTexturePlatformInfo* platformInfo;
    RpMultiTexture* multiTexture;

    if (!RwStreamReadInt32(stream, (RwInt32*)&version, sizeof(RwUInt32))) {
        return NULL;
    }
    if (!RwStreamRead(stream, header, sizeof(header))) {
        return NULL;
    }

    platformInfo = &MultiTextureInfo[header[0]];
    numTextures = header[1];
    multiTexture = MultiTextureCreate(numTextures, platformInfo);
    if (multiTexture == NULL) {
        return NULL;
    }

    if (header[1]) {
        RwUInt32 i;

        if (!RwStreamRead(stream, multiTexture->coords, multiTexture->numTextures)) {
            MultiTextureDestroy(multiTexture);
            return NULL;
        }

        for (i = 0; i < multiTexture->numTextures; i++) {
            _rpMatFXStreamReadTexture(stream, &multiTexture->textures[i]);
        }
    }

    if (header[2] & 1) {
        RpMTEffect* effect;

        if (!_rwStringStreamFindAndRead(name, stream)) {
            MultiTextureDestroy(multiTexture);
            return NULL;
        }

        effect = RpMTEffectFind(name);
        if (effect == NULL) {
            effect = RpMTEffectCreateDummy();
            if (effect == NULL) {
                MultiTextureDestroy(multiTexture);
                return NULL;
            }
            RpMTEffectSetName(effect, name);
        }

        RpMultiTextureSetEffect(multiTexture, effect);
        RpMTEffectDestroy(effect);
    }

    MULTITEXTUREGETDATA(object, offset) = multiTexture;
    return stream;
}

RwBool _rpMultiTexturePluginAttach(void)
{
    if (!_rpMTEffectSystemInit()) {
        return FALSE;
    }

    memset(MultiTextureInfo, 0, sizeof(MultiTextureInfo));

    MultiTextureModule.globalsOffset = RwEngineRegisterPlugin(0x18, rwID_MULTITEXPLUGIN, MultiTextureOpen,
                                                              MultiTextureClose);
    if (MultiTextureModule.globalsOffset < 0) {
        return FALSE;
    }
    return TRUE;
}

RwBool _rpMaterialRegisterMultiTexturePlugin(RwUInt32 platform, RwUInt32 pluginID, RwUInt32 dataSize)
{
    RpMultiTexturePlatformInfo* platformInfo;
    RwInt32 offset;

    offset = RpMaterialRegisterPlugin(sizeof(RpMultiTexture*), pluginID, MultiTextureConstructor,
                                      MultiTextureDestructor, MultiTextureCopy);
    if (offset < 0) {
        return FALSE;
    }

    platformInfo = &MultiTextureInfo[platform];
    platformInfo->offset = offset;

    if (RpMaterialRegisterPluginStream(pluginID, MultiTextureStreamRead, MultiTextureStreamWrite,
                                       MultiTextureStreamGetSize) < 0) {
        return FALSE;
    }

    platformInfo->platform = platform;
    platformInfo->pluginID = pluginID;
    platformInfo->dataSize = dataSize;
    return TRUE;
}

RpMultiTexture* RpMultiTextureSetEffect(RpMultiTexture* multiTexture, RpMTEffect* effect)
{
    if (multiTexture->effect) {
        RpMTEffectDestroy(multiTexture->effect);
    }

    multiTexture->effect = effect;
    if (effect) {
        RpMTEffectAddRef(multiTexture->effect);
    }
    return multiTexture;
}

RpMTEffect* RpMultiTextureGetEffect(const RpMultiTexture* multiTexture)
{
    return multiTexture->effect;
}

RpMultiTexture* RpMultiTextureSetTexture(RpMultiTexture* multiTexture, RwUInt32 index, RwTexture* texture)
{
    if (multiTexture->textures[index]) {
        RwTextureDestroy(multiTexture->textures[index]);
    }

    multiTexture->textures[index] = texture;
    if (texture) {
        texture->refCount++;
    }
    return multiTexture;
}

RwTexture* RpMultiTextureGetTexture(const RpMultiTexture* multiTexture, RwUInt32 index)
{
    return multiTexture->textures[index];
}

RpMultiTexture* RpMultiTextureSetCoords(RpMultiTexture* multiTexture, RwUInt32 index, RwUInt32 coords)
{
    multiTexture->coords[index] = coords;
    return multiTexture;
}

RwUInt32 RpMultiTextureGetCoords(const RpMultiTexture* multiTexture, RwUInt32 index)
{
    return multiTexture->coords[index];
}

RpMultiTexture* RpMaterialGetMultiTexture(const RpMaterial* material, RwUInt32 platform)
{
    RpMultiTexturePlatformInfo* platformInfo = &MultiTextureInfo[platform];

    if (platformInfo->pluginID) {
        return MULTITEXTUREGETDATA(material, platformInfo->offset);
    }
    return NULL;
}
