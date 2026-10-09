/*
 * GameCube device layer textures: each texture carries a GXTexObj that is
 * rebuilt lazily when its filtering or addressing changes, a per texture map
 * cache of what is currently loaded, and the native texture stream format
 * (GX ready texel data plus the LOD settings).
 */

#include "rwsdk/rwcore.h"

#define rwPLUGIN_ID 1

#define rwID_DLTEXTUREPLUGIN 0x40C

#define rpTEXTURENATIVEPLATFORM 6

#define rwRASTERDONTALLOCATE 0x80
#define rwRASTERFORMATAUTOMIPMAP 0x1000
#define rwRASTERFORMATPAL8 0x2000
#define rwRASTERFORMATPAL4 0x4000
#define rwRASTERFORMATMIPMAP 0x8000

#define rwDLTEXTUREDIRTY 0x01000000
#define rwDLTEXTUREPRELOADED 0x02000000

typedef struct GXTexObj {
    RwUInt32 dummy[8];
} GXTexObj;

typedef struct GXTlutObj {
    RwUInt32 dummy[3];
} GXTlutObj;

typedef struct RwDlRasterExt {
    /* 0x00 */ GXTlutObj tlutObj;
    /* 0x0C */ RwUInt32 format;
    /* 0x10 */ RwUInt32 tlutFormat;
    /* 0x14 */ RwUInt32 flags;
    /* 0x18 */ void* unaligned;
    /* 0x1C */ RwUInt8* pixels;
    /* 0x20 */ RwUInt8* palette;
    /* 0x24 */ RwUInt8* lockedPixels;
    /* 0x28 */ RwUInt8* linear;
    /* 0x2C */ void* texRegion;
    /* 0x30 */ RwUInt16 token;
    /* 0x32 */ RwUInt8 maxLevel;
    /* 0x33 */ RwUInt8 lockedLevel;
} RwDlRasterExt;

typedef struct RwDlTextureExt {
    /* 0x00 */ GXTexObj texObj;
    /* 0x20 */ RwUInt32 flags;
} RwDlTextureExt;

typedef struct DlNativeTextureHeader {
    /* 0x00 */ RwInt32 platform;
    /* 0x04 */ RwUInt32 filterAddressing;
    /* 0x08 */ RwInt32 maxAniso;
    /* 0x0C */ RwInt32 biasClamp;
    /* 0x10 */ RwInt32 edgeLOD;
    /* 0x14 */ RwReal lodBias;
    /* 0x18 */ RwChar name[32];
    /* 0x38 */ RwChar mask[32];
} DlNativeTextureHeader;

typedef struct DlNativeRasterHeader {
    /* 0x00 */ RwInt32 format;
    /* 0x04 */ RwUInt16 width;
    /* 0x06 */ RwUInt16 height;
    /* 0x08 */ RwUInt8 depth;
    /* 0x09 */ RwUInt8 numLevels;
    /* 0x0A */ RwUInt8 gxFormat;
    /* 0x0B */ RwUInt8 tlutFormat;
    /* 0x0C */ RwInt32 hasAlpha;
} DlNativeRasterHeader;

#define RASTEREXTFROMRASTER(raster) ((RwDlRasterExt*)((RwUInt8*)(raster) + _rwDlRasterExtOffset))
#define TEXTUREEXTFROMTEXTURE(texture) ((RwDlTextureExt*)((RwUInt8*)(texture) + _rwDlTextureExtOffset))

#define RwTextureGetFilterMode(tex) ((RwInt32)((tex)->filterAddressing & 0xFF))
#define RwTextureGetAddressingU(tex) (((tex)->filterAddressing >> 8) & 0xF)
#define RwTextureGetAddressingV(tex) (((tex)->filterAddressing >> 12) & 0xF)

static inline RwInt32 DlTextureGetFilterMode(const RwTexture* texture)
{
    return texture->filterAddressing & 0xFF;
}

static inline RwInt32 DlTextureGetAddressingU(const RwTexture* texture)
{
    return (texture->filterAddressing & 0xF00) >> 8;
}

static inline RwInt32 DlTextureGetAddressingV(const RwTexture* texture)
{
    return (texture->filterAddressing & 0xF000) >> 12;
}

RwInt32 _rwDlTextureExtOffset;

extern RwInt32 _rwDlRasterExtOffset;
extern volatile RwUInt16 lbl_8025E800;
extern RwTexture* lbl_8025F87C;
extern RwRaster* lbl_8025F878;

static RwUInt32 DlTextureFilterModes[7][2] = {
    { 0, 0 }, { 0, 0 }, { 1, 1 }, { 2, 0 }, { 3, 1 }, { 4, 0 }, { 5, 1 },
};

static RwUInt32 DlTextureWrapModes[5] = { 0, 1, 2, 0, 0 };

static RwTexture* DlTextureCache[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

void* memcpy(void* dst, const void* src, unsigned long n);
RwUInt32 RwStreamRead(RwStream* stream, void* buffer, RwUInt32 length);
RwStream* RwStreamWrite(RwStream* stream, const void* buffer, RwUInt32 length);
void DCFlushRange(void* addr, RwUInt32 nBytes);
RwRaster* RwRasterCreate(RwInt32 width, RwInt32 height, RwInt32 depth, RwInt32 flags);
RwBool RwRasterDestroy(RwRaster* raster);
RwInt32 RwRasterGetNumLevels(RwRaster* raster);
RwTexture* RwTextureCreate(RwRaster* raster);
RwTexture* RwTextureSetName(RwTexture* texture, const RwChar* name);
RwTexture* RwTextureSetMaskName(RwTexture* texture, const RwChar* maskName);
RwInt32 RwTextureRegisterPlugin(RwInt32 size, RwUInt32 pluginID, void* constructCB, void* destructCB, void* copyCB);
RwUInt32 _rwDlRasterGetSize(RwRaster* raster);
RwBool _rwDlTextureRasterCreate(RwRaster* raster, RwInt32 gxFormat, RwInt32 tlutFormat, RwInt32 hasAlpha,
                                RwUInt8 depth, RwInt32 numLevels);
RwBool _rwDlTextureSetRaster(RwTexture* texture, RwRaster* raster, RwInt32 flags);
void GXInvalidateTexAll(void);
RwInt32 GXGetTexObjMaxAniso(GXTexObj* obj);
RwUInt8 GXGetTexObjBiasClamp(GXTexObj* obj);
RwUInt8 GXGetTexObjEdgeLOD(GXTexObj* obj);
RwReal GXGetTexObjLODBias(GXTexObj* obj);
RwUInt32 GXGetTexObjTlut(GXTexObj* obj);
void GXInitTexObj(GXTexObj* obj, void* image, RwUInt16 width, RwUInt16 height, RwInt32 format, RwInt32 wrapS,
                  RwInt32 wrapT, RwUInt8 mipmap);
void GXInitTexObjCI(GXTexObj* obj, void* image, RwUInt16 width, RwUInt16 height, RwInt32 format, RwInt32 wrapS,
                    RwInt32 wrapT, RwUInt8 mipmap, RwUInt32 tlutName);
void GXInitTexObjLOD(GXTexObj* obj, RwInt32 minFilter, RwInt32 magFilter, RwReal minLOD, RwReal maxLOD,
                     RwReal lodBias, RwUInt8 biasClamp, RwUInt8 edgeLOD, RwInt32 maxAniso);
void GXLoadTlut(GXTlutObj* tlutObj, RwUInt32 tlutName);
void GXLoadTexObj(GXTexObj* obj, RwInt32 id);
void GXLoadTexObjPreLoaded(GXTexObj* obj, void* region, RwInt32 id);
void RwGameCubeTextureSetLOD(RwTexture* texture, RwReal lodBias, RwInt32 biasClamp, RwInt32 edgeLOD,
                             RwInt32 maxAniso);
RwReal DlTextureGetLODBias(GXTexObj* texObj);

RwBool _rwDlNativeTextureGetSize(void* pSize, void* pTexture, RwInt32 flags)
{
    RwTexture* texture = (RwTexture*)pTexture;
    RwRaster* raster = texture->raster;
    RwInt32 size;

    if (raster == NULL) {
        *(RwInt32*)pSize = 12 + sizeof(DlNativeTextureHeader);
        return TRUE;
    }

    size = 12 + sizeof(DlNativeTextureHeader) + sizeof(DlNativeRasterHeader);
    if (RwRasterGetFormat(raster) & (rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8)) {
        size = ((1 << raster->depth) << 1) + 12 + sizeof(DlNativeTextureHeader) + sizeof(DlNativeRasterHeader);
    }
    size += sizeof(RwUInt32);
    size += _rwDlRasterGetSize(raster);

    *(RwInt32*)pSize = size;
    return TRUE;
}

RwBool _rwDlNativeTextureWrite(void* pStream, void* pTexture, RwInt32 flags)
{
    RwStream* stream = (RwStream*)pStream;
    RwTexture* texture = (RwTexture*)pTexture;
    RwUInt32 dataSize;
    RwInt32 size;
    DlNativeRasterHeader rasterHeader;
    DlNativeTextureHeader header;
    RwDlTextureExt* textureExt;
    RwRaster* raster;
    RwDlRasterExt* rasterExt;

    _rwDlNativeTextureGetSize(&size, texture, 0);
    size -= 12;
    if (!RwStreamWriteChunkHeader(stream, rwID_STRUCT, size)) {
        return FALSE;
    }

    header.platform = rpTEXTURENATIVEPLATFORM;
    textureExt = TEXTUREEXTFROMTEXTURE(texture);
    header.filterAddressing = (RwTextureGetAddressingU(texture) << 8) | RwTextureGetFilterMode(texture) |
                              (RwTextureGetAddressingV(texture) << 12);

    if (textureExt->flags & rwDLTEXTUREDIRTY) {
        header.maxAniso = 0;
        header.biasClamp = 1;
        header.edgeLOD = 1;
        header.lodBias = 0.0f;
    } else {
        header.maxAniso = GXGetTexObjMaxAniso(&textureExt->texObj);
        header.biasClamp = GXGetTexObjBiasClamp(&textureExt->texObj);
        header.edgeLOD = GXGetTexObjEdgeLOD(&textureExt->texObj);
        header.lodBias = DlTextureGetLODBias(&textureExt->texObj);
    }

    memcpy(header.name, texture->name, sizeof(header.name));
    memcpy(header.mask, texture->mask, sizeof(header.mask));

    if (!RwStreamWrite(stream, &header, sizeof(header))) {
        return FALSE;
    }

    raster = texture->raster;
    size -= sizeof(header);
    rasterExt = RASTEREXTFROMRASTER(raster);
    rasterHeader.format = raster->cType | (raster->cFormat << 8);
    rasterHeader.width = raster->width;
    rasterHeader.height = raster->height;
    rasterHeader.depth = raster->depth;
    rasterHeader.numLevels = RwRasterGetNumLevels(raster);
    rasterHeader.gxFormat = rasterExt->format;
    rasterHeader.tlutFormat = rasterExt->tlutFormat;
    rasterHeader.hasAlpha = rasterExt->flags & 1;

    if (!RwStreamWrite(stream, &rasterHeader, sizeof(rasterHeader))) {
        return FALSE;
    }
    size -= sizeof(rasterHeader);

    if (RwRasterGetFormat(raster) & (rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8)) {
        RwUInt32 paletteSize = (1 << raster->depth) << 1;

        if (!RwStreamWrite(stream, rasterExt->palette, paletteSize)) {
            return FALSE;
        }
        size -= paletteSize;
    }

    dataSize = _rwDlRasterGetSize(raster);
    if (!RwStreamWrite(stream, &dataSize, sizeof(dataSize))) {
        return FALSE;
    }
    size -= sizeof(dataSize);

    if (!RwStreamWrite(stream, rasterExt->pixels, dataSize)) {
        return FALSE;
    }
    size -= dataSize;

    return TRUE;
}

RwBool _rwDlNativeTextureRead(void* pStream, void* pTexture, RwInt32 flags)
{
    RwStream* stream = (RwStream*)pStream;
    RwUInt32 length;
    RwUInt32 version;
    DlNativeTextureHeader header;
    DlNativeRasterHeader rasterHeader;
    RwUInt32 dataSize;
    RwRaster* raster;
    RwTexture* texture;
    RwDlRasterExt* rasterExt;
    RwUInt32 autoMipmap;

    if (!RwStreamFindChunk(stream, rwID_STRUCT, &length, &version)) {
        return FALSE;
    }
    if (version < rwLIBRARYBASEVERSION || version > rwLIBRARYCURRENTVERSION) {
        return FALSE;
    }

    if (RwStreamRead(stream, &header, sizeof(header)) != sizeof(header)) {
        return FALSE;
    }
    if (header.platform != rpTEXTURENATIVEPLATFORM) {
        return FALSE;
    }

    if (RwStreamRead(stream, &rasterHeader, sizeof(rasterHeader)) != sizeof(rasterHeader)) {
        return FALSE;
    }

    raster = RwRasterCreate(rasterHeader.width, rasterHeader.height, rasterHeader.depth,
                            rasterHeader.format | rwRASTERDONTALLOCATE);
    if (raster == NULL) {
        return FALSE;
    }

    if (!_rwDlTextureRasterCreate(raster, rasterHeader.gxFormat, rasterHeader.tlutFormat, rasterHeader.hasAlpha,
                                  rasterHeader.depth, rasterHeader.numLevels)) {
        RwRasterDestroy(raster);
        return FALSE;
    }

    raster->cFlags &= ~rwRASTERDONTALLOCATE;
    rasterExt = RASTEREXTFROMRASTER(raster);

    if (RwRasterGetFormat(raster) & (rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8)) {
        RwUInt32 paletteSize = (1 << raster->depth) << 1;

        if (paletteSize != RwStreamRead(stream, rasterExt->palette, paletteSize)) {
            return FALSE;
        }
        DCFlushRange(rasterExt->palette, (1 << raster->depth) << 1);
    }

    autoMipmap = raster->cFormat & (rwRASTERFORMATAUTOMIPMAP >> 8);
    raster->cFormat &= ~autoMipmap;

    if (RwStreamRead(stream, &dataSize, sizeof(dataSize)) != sizeof(dataSize)) {
        return FALSE;
    }
    if (dataSize != RwStreamRead(stream, rasterExt->pixels, dataSize)) {
        return FALSE;
    }
    DCFlushRange(rasterExt->pixels, dataSize);
    GXInvalidateTexAll();

    raster->cFormat |= autoMipmap;

    texture = RwTextureCreate(raster);
    if (texture == NULL) {
        RwRasterDestroy(raster);
        return FALSE;
    }

    texture->filterAddressing = (texture->filterAddressing & ~0xFF) | (header.filterAddressing & 0xFF);
    texture->filterAddressing = (texture->filterAddressing & ~0xF00) | (header.filterAddressing & 0xF00);
    texture->filterAddressing = (texture->filterAddressing & ~0xF000) | (header.filterAddressing & 0xF000);
    RwTextureSetName(texture, header.name);
    RwTextureSetMaskName(texture, header.mask);
    RwGameCubeTextureSetLOD(texture, header.lodBias, header.biasClamp, header.edgeLOD, header.maxAniso);

    *(RwTexture**)pTexture = texture;
    return TRUE;
}

RwReal DlTextureGetLODBias(GXTexObj* texObj)
{
    RwReal bias = GXGetTexObjLODBias(texObj);

    if (bias < 0.0f) {
        return (4096.0f + bias) / -1024.0f;
    }
    return bias / 1024.0f;
}

void* _rwDlTextureConst(void* texture, RwInt32 offset, RwInt32 size)
{
    TEXTUREEXTFROMTEXTURE(texture)->flags = rwDLTEXTUREDIRTY;
    return texture;
}

void* _rwDlTextureDest(void* texture, RwInt32 offset, RwInt32 size)
{
    RwTexture** cache = DlTextureCache;
    RwInt32 i;

    for (i = 0; i < 8; i++) {
        if (texture == *cache) {
            *cache = NULL;
        }
        cache++;
    }
    return texture;
}

void _rwDlTexturePluginAttach(void)
{
    _rwDlTextureExtOffset =
        RwTextureRegisterPlugin(sizeof(RwDlTextureExt), rwID_DLTEXTUREPLUGIN, _rwDlTextureConst, _rwDlTextureDest, NULL);
}

static inline void DlTextureInitTexObj(RwTexture* texture, RwUInt32 tlutName, RwReal lodBias, RwUInt8 biasClamp,
                                       RwUInt8 edgeLOD, RwInt32 maxAniso)
{
    RwRaster* raster = texture->raster;
    RwDlTextureExt* textureExt = TEXTUREEXTFROMTEXTURE(texture);
    RwDlRasterExt* rasterExt = RASTEREXTFROMRASTER(raster->parent);
    RwUInt32 format = RwRasterGetFormat(raster);
    RwUInt32 minFilter;
    RwUInt32 magFilter;

    if (format & (rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8)) {
        GXInitTexObjCI(&textureExt->texObj, rasterExt->pixels, raster->width, raster->height, rasterExt->format,
                       DlTextureWrapModes[RwTextureGetAddressingU(texture)],
                       DlTextureWrapModes[RwTextureGetAddressingV(texture)], (format & rwRASTERFORMATMIPMAP) >> 15,
                       (textureExt->flags & rwDLTEXTUREPRELOADED) ? GXGetTexObjTlut(&textureExt->texObj) : tlutName);
        if (RwTextureGetFilterMode(texture) == 6 || RwTextureGetFilterMode(texture) == 5) {
            minFilter = DlTextureFilterModes[4][0];
            magFilter = DlTextureFilterModes[4][1];
        } else {
            minFilter = DlTextureFilterModes[RwTextureGetFilterMode(texture)][0];
            magFilter = DlTextureFilterModes[RwTextureGetFilterMode(texture)][1];
        }
    } else {
        GXInitTexObj(&textureExt->texObj, rasterExt->pixels, raster->width, raster->height, rasterExt->format,
                     DlTextureWrapModes[RwTextureGetAddressingU(texture)],
                     DlTextureWrapModes[RwTextureGetAddressingV(texture)], (format & rwRASTERFORMATMIPMAP) >> 15);
        minFilter = DlTextureFilterModes[RwTextureGetFilterMode(texture)][0];
        magFilter = DlTextureFilterModes[RwTextureGetFilterMode(texture)][1];
    }

    GXInitTexObjLOD(&textureExt->texObj, minFilter, magFilter, 0.0f, rasterExt->maxLevel, lodBias, biasClamp, edgeLOD,
                    maxAniso);
    textureExt->flags = ((textureExt->flags & 0xFFFF0000) | (texture->filterAddressing & 0xFFFF)) & ~rwDLTEXTUREDIRTY;
}

static inline void DlTextureRebuildTexObj(RwTexture* texture, RwDlTextureExt* textureExt, RwUInt32 tlutName)
{
    RwInt32 maxAniso = GXGetTexObjMaxAniso(&textureExt->texObj);
    RwUInt8 edgeLOD = GXGetTexObjEdgeLOD(&textureExt->texObj);
    RwUInt8 biasClamp = GXGetTexObjBiasClamp(&textureExt->texObj);
    RwReal lodBias = DlTextureGetLODBias(&textureExt->texObj);

    DlTextureInitTexObj(texture, tlutName, lodBias, biasClamp, edgeLOD, maxAniso);
}

void _rwDlTextureSet(RwTexture* texture, RwInt32 texMap)
{
    RwDlTextureExt* textureExt;
    RwDlRasterExt* rasterExt;

    if (texture == NULL) {
        texture = lbl_8025F87C;
        _rwDlTextureSetRaster(texture, lbl_8025F878, 0);
    }

    textureExt = TEXTUREEXTFROMTEXTURE(texture);
    rasterExt = RASTEREXTFROMRASTER(texture->raster->parent);
    rasterExt->token = lbl_8025E800;

    if (RwRasterGetFormat(texture->raster) & (rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8)) {
        if (textureExt->flags & rwDLTEXTUREDIRTY) {
            DlTextureInitTexObj(texture, texMap, 0.0f, 1, 1, 0);
        } else if ((texture->filterAddressing & 0xFFFF) != (textureExt->flags & 0xFFFF)) {
            DlTextureRebuildTexObj(texture, textureExt, texMap);
        } else if (texture == DlTextureCache[texMap]) {
            return;
        } else if (!(textureExt->flags & rwDLTEXTUREPRELOADED) && texMap != GXGetTexObjTlut(&textureExt->texObj)) {
            DlTextureRebuildTexObj(texture, textureExt, texMap);
        }

        if (!(textureExt->flags & rwDLTEXTUREPRELOADED)) {
            GXLoadTlut(&rasterExt->tlutObj, texMap);
        }
    } else {
        if (textureExt->flags & rwDLTEXTUREDIRTY) {
            DlTextureInitTexObj(texture, 0, 0.0f, 1, 1, 0);
        } else if ((texture->filterAddressing & 0xFFFF) != (textureExt->flags & 0xFFFF)) {
            DlTextureRebuildTexObj(texture, textureExt, 0);
        } else if (texture == DlTextureCache[texMap]) {
            return;
        }
    }

    if (!(textureExt->flags & rwDLTEXTUREPRELOADED)) {
        GXLoadTexObj(&textureExt->texObj, texMap);
    } else {
        GXLoadTexObjPreLoaded(&textureExt->texObj, rasterExt->texRegion, texMap);
    }
    DlTextureCache[texMap] = texture;
}

void RwGameCubeTextureSetLOD(RwTexture* texture, RwReal lodBias, RwInt32 biasClamp, RwInt32 edgeLOD,
                             RwInt32 maxAniso)
{
    DlTextureInitTexObj(texture, 0, lodBias, biasClamp, edgeLOD, maxAniso);
}
