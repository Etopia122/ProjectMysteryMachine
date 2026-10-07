/*
 * Textures and texture dictionaries: creation, lookup by name, the default
 * read callback (with mipmap levels) and mipmap generation.
 */

#include "rwsdk/rwcore.h"

#define rwPLUGIN_ID 1 /* core */

#define E_RW_INVRASTERFORMAT 0x80000009
#define E_RW_STRINGTOOLONG 0x8000001E
#define E_RW_READTEXMASK 0x16

#define rwTEXDICTIONARY 6

#define rwRASTERTYPETEXTURE 0x04
#define rwRASTERFORMATAUTOMIPMAP 0x1000
#define rwRASTERFORMATPAL8 0x2000
#define rwRASTERFORMATPAL4 0x4000
#define rwRASTERFORMATMIPMAP 0x8000

#define rwRASTERLOCKWRITE 0x01
#define rwRASTERLOCKREAD 0x02
#define rwRASTERLOCKNOFETCH 0x04

#define rwFILTERNEAREST 1
#define rwTEXTUREADDRESSWRAP 1

#define rwTEXTUREBASENAMELENGTH 32
#define rwTEXTUREMAXNAMELENGTH 256
#define rwMAXMIPLEVELS 16

typedef RwTexture* (*RwTextureCallBackRead)(const RwChar* name, const RwChar* maskName);
typedef RwRaster* (*RwTextureCallBackMipmapGeneration)(RwRaster* raster, RwImage* image);
typedef RwBool (*RwTextureCallBackMipmapName)(RwChar* name, RwChar* maskName, RwUInt8 mipLevel, RwInt32 format);

typedef struct rwTextureGlobals {
    RwLinkList texDictList;
    RwFreeList* textureFreeList;
    RwFreeList* texDictFreeList;
    RwTexDictionary* currentTexDict;
    RwTextureCallBackRead textureReadFunc;
    RwBool haveTexMipmaps;
    RwBool generateTexMipmaps;
    RwChar* scratchMem;
    RwUInt16 scratchMemSize;
    RwTextureCallBackMipmapGeneration mipmapGenerationFunc;
    RwTextureCallBackMipmapName mipmapNameFunc;
} rwTextureGlobals;

typedef struct RwPalQuant {
    void* root;
    void* cubeFreeList;
    void* leafFreeList;
    void* palette;
} RwPalQuant;

RwPluginRegistry textureTKList = { sizeof(RwTexture), sizeof(RwTexture), 0, 0, NULL, NULL };
RwPluginRegistry texDictTKList = { sizeof(RwTexDictionary), sizeof(RwTexDictionary), 0, 0, NULL, NULL };

static const RwChar hexDigits[] = "0123456789abcdef";

RwModuleInfo textureModule;
static RwTexDictionary* defaultTexDict;

#define RWTEXTUREGLOBALS() RWPLUGINOFFSET(rwTextureGlobals, RwEngineInstance, textureModule.globalsOffset)
#define RWTEXTUREGLOBAL(var) (RWTEXTUREGLOBALS()->var)

#define RwTextureSetFilterMode(t, f) ((t)->filterAddressing = ((t)->filterAddressing & ~0xFF) | (f))
#define RwTextureSetAddressing(t, a) \
    ((t)->filterAddressing = ((t)->filterAddressing & ~0xFF00) | (((a) << 8) & 0x0F00) | (((a) << 12) & 0xF000))

/* Copy a name into a fixed buffer, truncating it with an error if it does not fit. */
#define rwTextureNameCopy(dst, src, size)                                                     \
    do {                                                                                      \
        RWSRCGLOBAL(stringFuncs).rwstrncpy((dst), (src), (size));                            \
        if (RWSRCGLOBAL(stringFuncs).rwstrlen(src) >= (size)) {                               \
            RWERROR((E_RW_STRINGTOOLONG, (src), (size), (size) - 1, (src)[(size) - 1]));      \
            (dst)[(size) - 1] = '\0';                                                         \
        }                                                                                     \
    } while (0)

void* memcpy(void* dst, const void* src, unsigned long n);
RwBool RwTextureDestroy(RwTexture* texture);
RwTexture* RwTextureCreate(RwRaster* raster);
const RwTexDictionary* RwTexDictionaryForAllTextures(const RwTexDictionary* dict, RwTextureCallBack fpCallBack,
                                                     void* pData);
RwTexture* RwTextureSetName(RwTexture* texture, const RwChar* name);
RwTexture* RwTextureSetMaskName(RwTexture* texture, const RwChar* maskName);
RwBool RwTextureGenerateMipmapName(RwChar* name, RwChar* maskName, RwUInt8 mipLevel, RwInt32 format);

RwImage* RwImageCreate(RwInt32 width, RwInt32 height, RwInt32 depth);
RwBool RwImageDestroy(RwImage* image);
RwImage* RwImageAllocatePixels(RwImage* image);
RwImage* RwImageCopy(RwImage* destImage, const RwImage* sourceImage);
RwImage* RwImageResample(RwImage* dstImage, const RwImage* srcImage);
RwImage* RwImageCreateResample(const RwImage* srcImage, RwInt32 width, RwInt32 height);
RwImage* RwImageGammaCorrect(RwImage* image);
const RwChar* RwImageFindFileType(const RwChar* imageName);
RwImage* RwImageReadMaskedImage(const RwChar* imageName, const RwChar* maskname);
RwImage* RwImageSetFromRaster(RwImage* image, RwRaster* raster);
RwRaster* RwRasterSetFromImage(RwRaster* raster, RwImage* image);
RwImage* RwImageFindRasterFormat(RwImage* ipImage, RwInt32 nRasterType, RwInt32* npWidth, RwInt32* npHeight,
                                 RwInt32* npDepth, RwInt32* npFormat);
RwRaster* RwRasterCreate(RwInt32 width, RwInt32 height, RwInt32 depth, RwInt32 flags);
RwBool RwRasterDestroy(RwRaster* raster);
RwInt32 RwRasterGetNumLevels(RwRaster* raster);
RwUInt8* RwRasterLock(RwRaster* raster, RwUInt8 level, RwInt32 lockMode);
RwRaster* RwRasterUnlock(RwRaster* raster);

RwBool _rwPalQuantInit(RwPalQuant* pq);
void _rwPalQuantTerm(RwPalQuant* pq);
void _rwPalQuantAddImage(RwPalQuant* pq, RwImage* img, RwReal weight);
RwInt32 _rwPalQuantResolvePalette(RwRGBA* palette, RwInt32 maxcols, RwPalQuant* pq);
void _rwPalQuantMatchImage(RwUInt8* dstpixels, RwInt32 dststride, RwInt32 dstdepth, RwBool dstPacked, RwPalQuant* pq,
                           RwImage* img);

static RwBool TextureDefaultMipmapName(RwChar* name, RwChar* maskName, RwUInt8 mipLevel, RwInt32 format) {
    RwChar mipName[3];

    mipName[0] = 'm';
    mipName[1] = (mipLevel > 0 && mipLevel < 16) ? hexDigits[mipLevel] : '\0';
    mipName[2] = '\0';

    if (mipName[1]) {
        RWSRCGLOBAL(stringFuncs).rwstrcat(name, mipName);

        if (maskName && maskName[0]) {
            RWSRCGLOBAL(stringFuncs).rwstrcat(maskName, mipName);
        }
    }

    return TRUE;
}

static RwBool PalettizeImage(RwImage** image, RwInt32 depth) {
    RwPalQuant palQuant;
    RwRGBA palette[256];
    RwInt32 palSize;
    RwImage* palImage;

    if (!_rwPalQuantInit(&palQuant)) {
        return FALSE;
    }

    _rwPalQuantAddImage(&palQuant, *image, (RwReal)1.0);

    palSize = 1 << depth;
    _rwPalQuantResolvePalette(palette, palSize, &palQuant);

    palImage = RwImageCreate((*image)->width, (*image)->height, depth);
    if (palImage) {
        RwImageAllocatePixels(palImage);
        _rwPalQuantMatchImage(palImage->cpPixels, palImage->stride, palImage->depth, FALSE, &palQuant, *image);
        memcpy(palImage->palette, palette, palSize * sizeof(RwRGBA));

        RwImageDestroy(*image);
        *image = palImage;
    } else {
        return FALSE;
    }

    _rwPalQuantTerm(&palQuant);

    return TRUE;
}

static RwBool PalettizeMipmaps(RwRGBA* palette, RwImage* keepImage, RwImage** images, RwInt32 numLevels,
                               RwInt32 depth) {
    RwPalQuant palQuant;
    RwInt32 i;

    if (images[0]->palette) {
        RwRGBA* basePalette = images[0]->palette;

        /* If every level already shares one palette there is nothing to do. */
        for (i = 1; i < numLevels; i++) {
            RwRGBA* levelPalette = images[i]->palette;
            RwInt32 j;

            if (!basePalette || !levelPalette) {
                i = 64;
                break;
            }

            for (j = 0; j < (1 << depth); j++) {
                if (((RwUInt32*)basePalette)[j] != ((RwUInt32*)levelPalette)[j]) {
                    i = 64;
                    break;
                }
            }
        }

        if (i == numLevels) {
            memcpy(palette, basePalette, (1 << images[0]->depth) * sizeof(RwRGBA));
            return TRUE;
        }
    }

    if (!_rwPalQuantInit(&palQuant)) {
        return FALSE;
    }

    for (i = 0; i < numLevels; i++) {
        _rwPalQuantAddImage(&palQuant, images[i], (RwReal)1.0);
    }

    _rwPalQuantResolvePalette(palette, 1 << depth, &palQuant);

    for (i = 0; i < numLevels; i++) {
        RwImage* image = images[i];
        RwImage* palImage = RwImageCreate(image->width, image->height, depth);

        if (palImage) {
            RwImageAllocatePixels(palImage);
            _rwPalQuantMatchImage(palImage->cpPixels, palImage->stride, palImage->depth, FALSE, &palQuant, image);
            palImage->palette = palette;

            images[i] = palImage;
            if (image != keepImage) {
                RwImageDestroy(image);
            }
        } else {
            return FALSE;
        }
    }

    _rwPalQuantTerm(&palQuant);

    return TRUE;
}

static RwImage* TextureImageReadAndSize(const RwChar* name, const RwChar* maskName, RwInt32 rasterType,
                                        RwInt32* width, RwInt32* height, RwInt32* depth, RwInt32* format) {
    RwChar nameBuffer[rwTEXTUREMAXNAMELENGTH];
    RwChar maskBuffer[rwTEXTUREMAXNAMELENGTH];
    RwImage* image;
    const RwChar* extension;
    RwImage* srcImage;

    rwTextureNameCopy(nameBuffer, name, rwTEXTUREMAXNAMELENGTH);

    extension = RwImageFindFileType(name);
    if (extension) {
        RWSRCGLOBAL(stringFuncs).rwstrcat(nameBuffer, extension);
    }

    maskBuffer[0] = '\0';
    if (maskName && maskName[0]) {
        rwTextureNameCopy(maskBuffer, maskName, rwTEXTUREMAXNAMELENGTH);

        extension = RwImageFindFileType(maskName);
        if (extension) {
            RWSRCGLOBAL(stringFuncs).rwstrcat(maskBuffer, extension);
        }
    }

    image = RwImageReadMaskedImage(nameBuffer, maskBuffer);
    if (!image) {
        return NULL;
    }

    if (!*width || !*height) {
        if (!RwImageFindRasterFormat(image, rasterType, width, height, depth, format)) {
            RwImageDestroy(image);
            RWERROR((E_RW_INVRASTERFORMAT));
            return NULL;
        }
    }

    if (image->width != *width || image->height != *height) {
        RwInt32 srcDepth;

        srcImage = image;
        srcDepth = srcImage->depth;
        if (srcDepth != 32) {
            image = RwImageCreate(image->width, image->height, 32);
            if (!image) {
                RwImageDestroy(srcImage);
                return NULL;
            }

            if (!RwImageAllocatePixels(image)) {
                RwImageDestroy(image);
                RwImageDestroy(srcImage);
                return NULL;
            }

            RwImageCopy(image, srcImage);
            RwImageDestroy(srcImage);
        }

        srcImage = RwImageCreate(*width, *height, 32);
        if (!srcImage) {
            RwImageDestroy(image);
            return NULL;
        }

        if (!RwImageAllocatePixels(srcImage)) {
            RwImageDestroy(srcImage);
            RwImageDestroy(image);
            return NULL;
        }

        RwImageResample(srcImage, image);
        RwImageDestroy(image);
        image = srcImage;

        if (srcDepth == 4) {
            PalettizeImage(&image, srcDepth);
        } else if (srcDepth == 8) {
            PalettizeImage(&image, srcDepth);
        }
    }

    return image;
}

static RwTexture* TextureDefaultNormalRead(const RwChar* name, const RwChar* maskName) {
    RwChar nameBuffer[rwTEXTUREMAXNAMELENGTH];
    RwChar maskBuffer[rwTEXTUREMAXNAMELENGTH];
    RwRGBA palette[256];
    RwImage* image;
    RwInt32 width;
    RwInt32 height;
    RwInt32 depth;
    RwInt32 format;
    RwTexture* texture;
    RwRaster* raster;

    rwTextureNameCopy(nameBuffer, name, rwTEXTUREMAXNAMELENGTH);

    maskBuffer[0] = '\0';
    if (maskName && maskName[0]) {
        rwTextureNameCopy(maskBuffer, maskName, rwTEXTUREMAXNAMELENGTH);
    }

    RwTextureGenerateMipmapName(nameBuffer, maskBuffer, 0, rwRASTERTYPETEXTURE);

    width = 0;
    height = 0;
    image = TextureImageReadAndSize(nameBuffer, maskBuffer, rwRASTERTYPETEXTURE, &width, &height, &depth, &format);
    if (!image) {
        return NULL;
    }

    raster = RwRasterCreate(width, height, depth, format);
    if (!raster) {
        RwImageDestroy(image);
        return NULL;
    }

    if (RwRasterGetFormat(raster) & (rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8)) {
        if (RwRasterGetFormat(raster) & rwRASTERFORMATPAL4) {
            PalettizeMipmaps(palette, NULL, &image, 1, 4);
        } else {
            PalettizeMipmaps(palette, NULL, &image, 1, 8);
        }

        image->palette = palette;
    }

    RwImageGammaCorrect(image);

    if (!RwRasterSetFromImage(raster, image)) {
        RwRasterDestroy(raster);
        RwImageDestroy(image);
        return NULL;
    }

    RwImageDestroy(image);

    texture = RwTextureCreate(raster);
    if (!texture) {
        RwRasterDestroy(raster);
        return NULL;
    }

    RwTextureSetName(texture, name);
    if (maskName) {
        RwTextureSetMaskName(texture, maskName);
    } else {
        RwTextureSetMaskName(texture, "");
    }

    return texture;
}

static RwTexture* TextureDefaultMipmapRead(const RwChar* name, const RwChar* maskName) {
    RwChar nameBuffer[rwTEXTUREMAXNAMELENGTH];
    RwChar maskBuffer[rwTEXTUREMAXNAMELENGTH];
    RwRGBA palette[256];
    RwImage* images[rwMAXMIPLEVELS];
    RwInt32 width;
    RwInt32 height;
    RwInt32 depth;
    RwInt32 format;
    RwTexture* texture;
    RwRaster* raster;
    RwInt32 rasterType;

    rwTextureNameCopy(nameBuffer, name, rwTEXTUREMAXNAMELENGTH);

    maskBuffer[0] = '\0';
    if (maskName && maskName[0]) {
        rwTextureNameCopy(maskBuffer, maskName, rwTEXTUREMAXNAMELENGTH);
    }

    rasterType = rwRASTERTYPETEXTURE;
    if (RWTEXTUREGLOBAL(haveTexMipmaps)) {
        rasterType |= rwRASTERFORMATMIPMAP;
        if (RWTEXTUREGLOBAL(generateTexMipmaps)) {
            rasterType |= rwRASTERFORMATAUTOMIPMAP;
        }
    }

    RwTextureGenerateMipmapName(nameBuffer, maskBuffer, 0, rasterType);

    width = 0;
    height = 0;
    images[0] = TextureImageReadAndSize(nameBuffer, maskBuffer, rasterType, &width, &height, &depth, &format);
    if (!images[0]) {
        return NULL;
    }

    raster = RwRasterCreate(width, height, depth, format);
    if (!raster) {
        RwImageDestroy(images[0]);
        return NULL;
    }

    if (format & rwRASTERFORMATMIPMAP) {
        if (format & rwRASTERFORMATAUTOMIPMAP) {
            if (!RwRasterSetFromImage(raster, images[0])) {
                RwRasterDestroy(raster);
                RwImageDestroy(images[0]);
                return NULL;
            }

            RwImageDestroy(images[0]);
        } else {
            RwInt32 i;
            RwInt32 numLevels = RwRasterGetNumLevels(raster);

            for (i = 1; i < numLevels; i++) {
                rwTextureNameCopy(nameBuffer, name, rwTEXTUREMAXNAMELENGTH);

                maskBuffer[0] = '\0';
                if (maskName && maskName[0]) {
                    rwTextureNameCopy(maskBuffer, maskName, rwTEXTUREMAXNAMELENGTH);
                }

                RwTextureGenerateMipmapName(nameBuffer, maskBuffer, (RwUInt8)i, rasterType);

                RwRasterLock(raster, (RwUInt8)i, rwRASTERLOCKWRITE | rwRASTERLOCKNOFETCH);
                width = raster->width;
                height = raster->height;
                depth = raster->depth;
                format = RwRasterGetFormat(raster) | RwRasterGetType(raster);
                RwRasterUnlock(raster);

                images[i] =
                    TextureImageReadAndSize(nameBuffer, maskBuffer, rasterType, &width, &height, &depth, &format);
                if (!images[i]) {
                    while (--i >= 0) {
                        RwImageDestroy(images[i]);
                    }

                    RwRasterDestroy(raster);
                    return NULL;
                }
            }

            if (RwRasterGetFormat(raster) & (rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8)) {
                if (RwRasterGetFormat(raster) & rwRASTERFORMATPAL4) {
                    PalettizeMipmaps(palette, NULL, images, numLevels, 4);
                } else {
                    PalettizeMipmaps(palette, NULL, images, numLevels, 8);
                }

                RwImageGammaCorrect(images[0]);
            } else {
                for (i = 0; i < numLevels; i++) {
                    RwImageGammaCorrect(images[i]);
                }
            }

            for (i = 0; i < numLevels; i++) {
                if (RwRasterLock(raster, (RwUInt8)i, rwRASTERLOCKWRITE | rwRASTERLOCKNOFETCH)) {
                    if (!RwRasterSetFromImage(raster, images[i])) {
                        for (; i < numLevels; i++) {
                            RwImageDestroy(images[i]);
                        }

                        RwRasterDestroy(raster);
                        return NULL;
                    }

                    RwRasterUnlock(raster);
                }

                RwImageDestroy(images[i]);
            }
        }
    } else {
        RwImageGammaCorrect(images[0]);

        if (!RwRasterSetFromImage(raster, images[0])) {
            RwRasterDestroy(raster);
            RwImageDestroy(images[0]);
            return NULL;
        }

        RwImageDestroy(images[0]);
    }

    texture = RwTextureCreate(raster);
    if (!texture) {
        RwRasterDestroy(raster);
        return NULL;
    }

    RwTextureSetName(texture, name);
    if (maskName) {
        RwTextureSetMaskName(texture, maskName);
    } else {
        RwTextureSetMaskName(texture, "");
    }

    return texture;
}

static RwTexture* TextureDefaultRead(const RwChar* name, const RwChar* maskName) {
    if (RWTEXTUREGLOBAL(haveTexMipmaps)) {
        return TextureDefaultMipmapRead(name, maskName);
    }

    return TextureDefaultNormalRead(name, maskName);
}

static RwRaster* TextureRasterDefaultBuildMipmaps(RwRaster* raster, RwImage* image) {
    RwImage* images[rwMAXMIPLEVELS];
    RwRGBA palette[256];
    RwInt32 autoMipmap;
    RwInt32 i;
    RwInt32 numLevels;
    RwInt32 width = raster->width;
    RwInt32 height = raster->height;

    if (!image) {
        images[0] = RwImageCreate(width, height, 32);
        if (images[0]) {
            if (!RwImageAllocatePixels(images[0])) {
                return NULL;
            }

            RwImageSetFromRaster(images[0], raster);
        }
    } else if (image->depth != 32) {
        images[0] = RwImageCreate(width, height, 32);
        if (images[0]) {
            if (!RwImageAllocatePixels(images[0])) {
                return NULL;
            }

            RwImageCopy(images[0], image);
        }
    } else {
        images[0] = image;
    }

    if (!images[0]) {
        return NULL;
    }

    autoMipmap = raster->cFormat & (rwRASTERFORMATAUTOMIPMAP >> 8);
    raster->cFormat &= ~autoMipmap;

    numLevels = RwRasterGetNumLevels(raster);
    for (i = 1; i < numLevels; i++) {
        images[i] = NULL;

        if (RwRasterLock(raster, (RwUInt8)i, rwRASTERLOCKREAD)) {
            images[i] = RwImageCreateResample(images[i - 1], raster->width, raster->height);
            RwRasterUnlock(raster);
        }

        if (!images[i]) {
            while (--i >= 0) {
                if (images[i] != image) {
                    RwImageDestroy(images[i]);
                }
            }

            raster->cFormat |= autoMipmap;
            return NULL;
        }
    }

    if (RwRasterGetFormat(raster) & (rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8)) {
        if (RwRasterGetFormat(raster) & rwRASTERFORMATPAL4) {
            if (!PalettizeMipmaps(palette, image, images, numLevels, 4)) {
                for (i = 0; i < numLevels; i++) {
                    if (images[i] != image) {
                        RwImageDestroy(images[i]);
                    }

                    raster->cFormat |= autoMipmap;
                    return NULL;
                }
            }
        } else {
            if (!PalettizeMipmaps(palette, image, images, numLevels, 8)) {
                for (i = 0; i < numLevels; i++) {
                    if (images[i] != image) {
                        RwImageDestroy(images[i]);
                    }

                    raster->cFormat |= autoMipmap;
                    return NULL;
                }
            }
        }

        RwImageGammaCorrect(images[0]);
    } else {
        for (i = 0; i < numLevels; i++) {
            RwImageGammaCorrect(images[i]);
        }
    }

    for (i = 0; i < numLevels; i++) {
        if (RwRasterLock(raster, (RwUInt8)i, rwRASTERLOCKWRITE | rwRASTERLOCKNOFETCH)) {
            RwRasterSetFromImage(raster, images[i]);
            RwRasterUnlock(raster);
        }

        if (images[i] != image) {
            RwImageDestroy(images[i]);
        }
    }

    raster->cFormat |= autoMipmap;

    return raster;
}

RwBool RwTextureSetReadCallBack(RwTextureCallBackRead fpCallBack) {
    RWTEXTUREGLOBAL(textureReadFunc) = fpCallBack;

    return TRUE;
}

RwBool RwTextureSetMipmapping(RwBool enable) {
    RWTEXTUREGLOBAL(haveTexMipmaps) = enable;

    return TRUE;
}

RwBool RwTextureGetMipmapping(void) {
    return RWTEXTUREGLOBAL(haveTexMipmaps);
}

RwBool RwTextureSetAutoMipmapping(RwBool enable) {
    RWTEXTUREGLOBAL(generateTexMipmaps) = enable;

    return TRUE;
}

RwBool RwTextureGetAutoMipmapping(void) {
    return RWTEXTUREGLOBAL(generateTexMipmaps);
}

RwTexDictionary* RwTexDictionaryCreate(void) {
    RwTexDictionary* dict = (RwTexDictionary*)RwFreeListAlloc(RWTEXTUREGLOBAL(texDictFreeList));

    if (!dict) {
        return NULL;
    }

    rwObjectInitialize(dict, rwTEXDICTIONARY, 0);
    rwLinkListAddLLLink(&RWTEXTUREGLOBAL(texDictList), &dict->lInInstance);
    rwLinkListInitialize(&dict->texturesInDict);

    _rwPluginRegistryInitObject(&texDictTKList, dict);

    return dict;
}

RwBool RwTexDictionaryDestroy(RwTexDictionary* dict) {
    if (RWTEXTUREGLOBAL(currentTexDict) == dict) {
        RWTEXTUREGLOBAL(currentTexDict) = NULL;
    }

    RwTexDictionaryForAllTextures(dict, (RwTextureCallBack)RwTextureDestroy, NULL);

    _rwPluginRegistryDeInitObject(&texDictTKList, dict);

    rwLLLinkRemove(&dict->lInInstance);
    RwFreeListFree(RWTEXTUREGLOBAL(texDictFreeList), dict);

    return TRUE;
}

const RwTexDictionary* RwTexDictionaryForAllTextures(const RwTexDictionary* dict, RwTextureCallBack fpCallBack,
                                                     void* pData) {
    const RwLLLink* next;
    const RwLLLink* cur = rwLinkListGetFirstLLLink(&dict->texturesInDict);
    const RwLLLink* end = rwLinkListGetTerminator(&dict->texturesInDict);

    while (cur != end) {
        RwTexture* texture = rwLLLinkGetData(cur, RwTexture, lInDictionary);
        next = rwLLLinkGetNext(cur);

        if (!fpCallBack(texture, pData)) {
            break;
        }

        cur = next;
    }

    return dict;
}

RwTexture* RwTextureCreate(RwRaster* raster) {
    RwTexture* texture = (RwTexture*)RwFreeListAlloc(RWTEXTUREGLOBAL(textureFreeList));

    if (texture) {
        texture->dict = NULL;
        texture->name[0] = '\0';
        texture->mask[0] = '\0';
        texture->raster = raster;
        texture->refCount = 1;
        texture->filterAddressing = 0;
        RwTextureSetAddressing(texture, rwTEXTUREADDRESSWRAP);
        RwTextureSetFilterMode(texture, rwFILTERNEAREST);

        _rwPluginRegistryInitObject(&textureTKList, texture);
    }

    return texture;
}

RwBool RwTextureDestroy(RwTexture* texture) {
    RwBool result;

    texture->refCount--;
    result = TRUE;
    if (texture->refCount <= 0) {
        texture->refCount++;

        _rwPluginRegistryDeInitObject(&textureTKList, texture);

        if (texture->dict) {
            rwLLLinkRemove(&texture->lInDictionary);
        }

        if (texture->raster) {
            RwRasterDestroy(texture->raster);
            texture->raster = NULL;
        }

        texture->refCount--;
        RwFreeListFree(RWTEXTUREGLOBAL(textureFreeList), texture);

        result = TRUE;
    }

    return result;
}

RwTexture* RwTextureSetName(RwTexture* texture, const RwChar* name) {
    rwTextureNameCopy(texture->name, name, rwTEXTUREBASENAMELENGTH);

    return texture;
}

RwTexture* RwTextureSetMaskName(RwTexture* texture, const RwChar* maskName) {
    rwTextureNameCopy(texture->mask, maskName, rwTEXTUREBASENAMELENGTH);

    return texture;
}

RwTexture* RwTexDictionaryAddTexture(RwTexDictionary* dict, RwTexture* texture) {
    if (texture->dict) {
        rwLLLinkRemove(&texture->lInDictionary);
    }

    texture->dict = dict;
    rwLinkListAddLLLink(&dict->texturesInDict, &texture->lInDictionary);

    return texture;
}

RwTexture* RwTexDictionaryRemoveTexture(RwTexture* texture) {
    if (texture->dict) {
        texture->dict = NULL;
        rwLLLinkRemove(&texture->lInDictionary);
    }

    return texture;
}

RwTexDictionary* RwTexDictionaryGetCurrent(void) {
    return RWTEXTUREGLOBAL(currentTexDict);
}

RwBool RwTextureGenerateMipmapName(RwChar* name, RwChar* maskName, RwUInt8 mipLevel, RwInt32 format) {
    if (RWTEXTUREGLOBAL(mipmapNameFunc)) {
        return RWTEXTUREGLOBAL(mipmapNameFunc)(name, maskName, mipLevel, format);
    }

    return FALSE;
}

static inline RwBool TextureNameMatch(const RwChar* name1, const RwChar* name2) {
    while (*name1 && *name2) {
        RwChar c1 = *name1;
        RwChar c2 = *name2;

        if (c1 >= 'a' && c1 <= 'z') {
            c1 -= 'a' - 'A';
        }
        if (c2 >= 'a' && c2 <= 'z') {
            c2 -= 'a' - 'A';
        }

        if (c1 != c2) {
            return FALSE;
        }

        name1++;
        name2++;
    }

    if (*name1 == *name2) {
        return TRUE;
    }

    return FALSE;
}

static inline RwTexture* TexDictionaryFindNamedTexture(RwTexDictionary* dict, const RwChar* name) {
    RwTexture* texture;
    const RwLLLink* cur = rwLinkListGetFirstLLLink(&dict->texturesInDict);
    const RwLLLink* end = rwLinkListGetTerminator(&dict->texturesInDict);

    while (cur != end) {
        texture = rwLLLinkGetData(cur, RwTexture, lInDictionary);

        if (texture->name && TextureNameMatch(texture->name, name)) {
            return texture;
        }

        cur = rwLLLinkGetNext(cur);
    }

    return NULL;
}

static inline RwTexture* TextureFindNamedInAll(rwTextureGlobals* globals, const RwChar* name) {
    const RwLLLink* cur = rwLinkListGetFirstLLLink(&globals->texDictList);
    const RwLLLink* end = rwLinkListGetTerminator(&globals->texDictList);

    while (cur != end) {
        RwTexture* texture = TexDictionaryFindNamedTexture(rwLLLinkGetData(cur, RwTexDictionary, lInInstance), name);

        if (texture) {
            return texture;
        }

        cur = rwLLLinkGetNext(cur);
    }

    return NULL;
}

RwTexture* RwTextureRead(const RwChar* name, const RwChar* maskName) {
    rwTextureGlobals* globals = RWTEXTUREGLOBALS();
    RwTexture* texture;

    if (globals->currentTexDict) {
        texture = TexDictionaryFindNamedTexture(globals->currentTexDict, name);
    } else {
        texture = TextureFindNamedInAll(globals, name);
    }

    if (texture) {
        texture->refCount++;
        return texture;
    }

    texture = globals->textureReadFunc(name, maskName);
    if (!texture) {
        if (maskName) {
            RWERROR((E_RW_READTEXMASK, name, maskName));
        } else {
            RWERROR((E_RW_READTEXMASK, name, "(null)"));
        }

        return NULL;
    }

    if (RWTEXTUREGLOBAL(currentTexDict)) {
        RwTexDictionaryAddTexture(RWTEXTUREGLOBAL(currentTexDict), texture);
    }

    return texture;
}

RwInt32 RwTextureRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB,
                                RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB) {
    return _rwPluginRegistryAddPlugin(&textureTKList, size, pluginID, constructCB, destructCB, copyCB);
}

RwBool RwTextureRasterGenerateMipmaps(RwRaster* raster, RwImage* image) {
    return RWTEXTUREGLOBAL(mipmapGenerationFunc)(raster, image) != NULL;
}

void* _rwTextureClose(void* instance, RwInt32 offset, RwInt32 size) {
    rwTextureGlobals* globals;

    if (RWTEXTUREGLOBAL(scratchMem)) {
        RwFree(RWTEXTUREGLOBAL(scratchMem));
        RWTEXTUREGLOBAL(scratchMem) = NULL;
        RWTEXTUREGLOBAL(scratchMemSize) = 0;
    }

    globals = RWTEXTUREGLOBALS();
    if (globals->textureFreeList && globals->texDictFreeList) {
        RwLLLink* cur = rwLinkListGetFirstLLLink(&globals->texDictList);

        while (cur != rwLinkListGetTerminator(&globals->texDictList)) {
            RwTexDictionary* curDict = rwLLLinkGetData(cur, RwTexDictionary, lInInstance);

            cur = rwLLLinkGetNext(cur);
            if (curDict == defaultTexDict) {
                RwTexDictionaryDestroy(defaultTexDict);
                defaultTexDict = NULL;
                break;
            }
        }
    }

    if (RWTEXTUREGLOBAL(textureFreeList)) {
        RwFreeListDestroy(RWTEXTUREGLOBAL(textureFreeList));
        RWTEXTUREGLOBAL(textureFreeList) = NULL;
    }

    if (RWTEXTUREGLOBAL(texDictFreeList)) {
        RwFreeListDestroy(RWTEXTUREGLOBAL(texDictFreeList));
        RWTEXTUREGLOBAL(texDictFreeList) = NULL;
    }

    textureModule.numInstances--;

    return instance;
}

void* _rwTextureOpen(void* instance, RwInt32 offset, RwInt32 size) {
    textureModule.globalsOffset = offset;

    RWTEXTUREGLOBAL(textureFreeList) = RwFreeListCreate(textureTKList.sizeOfStruct, 20, 0);
    if (!RWTEXTUREGLOBAL(textureFreeList)) {
        return NULL;
    }

    RWTEXTUREGLOBAL(texDictFreeList) = RwFreeListCreate(texDictTKList.sizeOfStruct, 5, 0);
    if (!RWTEXTUREGLOBAL(texDictFreeList)) {
        RwFreeListDestroy(RWTEXTUREGLOBAL(textureFreeList));
        RWTEXTUREGLOBAL(textureFreeList) = NULL;
        return NULL;
    }

    rwLinkListInitialize(&RWTEXTUREGLOBAL(texDictList));
    textureModule.numInstances++;

    RWTEXTUREGLOBAL(currentTexDict) = defaultTexDict = RwTexDictionaryCreate();
    if (!RWTEXTUREGLOBAL(currentTexDict)) {
        RwFreeListDestroy(RWTEXTUREGLOBAL(texDictFreeList));
        RWTEXTUREGLOBAL(texDictFreeList) = NULL;
        RwFreeListDestroy(RWTEXTUREGLOBAL(textureFreeList));
        RWTEXTUREGLOBAL(textureFreeList) = NULL;
        return NULL;
    }

    RWTEXTUREGLOBAL(haveTexMipmaps) = FALSE;
    RWTEXTUREGLOBAL(generateTexMipmaps) = FALSE;
    RWTEXTUREGLOBAL(textureReadFunc) = TextureDefaultRead;
    RWTEXTUREGLOBAL(mipmapGenerationFunc) = TextureRasterDefaultBuildMipmaps;
    RWTEXTUREGLOBAL(mipmapNameFunc) = TextureDefaultMipmapName;
    RWTEXTUREGLOBAL(scratchMem) = NULL;
    RWTEXTUREGLOBAL(scratchMemSize) = 0;

    return instance;
}
