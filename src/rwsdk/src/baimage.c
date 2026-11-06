/*
 * Images: creation, pixel storage, masks, depth conversion, gamma tables and
 * reading through the registered file formats and the image search path.
 */

#include "rwsdk/rwcore.h"
#include "math.h"

#define rwPLUGIN_ID 1 /* core */

#define E_RW_INVIMAGEDEPTH 0x80000008
#define E_RW_INVIMAGEFORMAT 0x80000009
#define E_RW_INVIMAGESIZE 0x8000000A
#define E_RW_NULLP 0x80000016

#define rwIMAGEALLOCATED 0x1
#define rwIMAGEGAMMACORRECTED 0x2

#define rwSTANDARDPIXELTORGB 3

typedef RwImage* (*RwImageCallBackRead)(const RwChar* imageName);
typedef RwImage* (*RwImageCallBackWrite)(RwImage* image, const RwChar* imageName);

typedef struct rwImageFormat {
    RwChar lcExtension[20];
    RwChar ucExtension[20];
    RwImageCallBackRead imageRead;
    RwImageCallBackWrite imageWrite;
    struct rwImageFormat* nextFormat;
} rwImageFormat;

typedef struct rwImageGlobals {
    RwFreeList* imageFreeList;
    RwChar* imagePath;
    RwInt32 imagePathSize;
    RwUInt8 gammaTable[256];
    RwUInt8 invGammaTable[256];
    RwReal gammaVal;
    RwChar* scratchMem;
    RwInt32 scratchMemSize;
    RwFreeList* imageFormatFreeList;
    rwImageFormat* imageFormats;
} rwImageGlobals;

typedef struct rwImageReadData {
    RwImageCallBackRead imageRead;
    RwImage* image;
} rwImageReadData;

typedef const RwChar* (*rwImagePathCallBack)(const RwChar* fullName, void* data);

RwPluginRegistry imageTKList = { sizeof(RwImage), sizeof(RwImage), 0, 0, NULL, NULL };

RwModuleInfo imageModule;

#define RWIMAGEGLOBAL(var) (RWPLUGINOFFSET(rwImageGlobals, RwEngineInstance, imageModule.globalsOffset)->var)
#define RwRealloc(_p, _s) ((RWSRCGLOBAL(memoryFuncs).rwrealloc)((_p), (_s)))

void* memcpy(void* dst, const void* src, unsigned long n);
RwBool _rwpathIsAbsolute(const RwChar* path);
RwBool RwImageSetGamma(RwReal gammaValue);
RwImage* RwImageFreePixels(RwImage* image);
RwImage* RwImageCopy(RwImage* destImage, const RwImage* sourceImage);

void* _rwImageOpen(void* instance, RwInt32 offset, RwInt32 size) {
    imageModule.globalsOffset = offset;

    RWIMAGEGLOBAL(imageFreeList) = RwFreeListCreate(imageTKList.sizeOfStruct, 20, 0);
    if (!RWIMAGEGLOBAL(imageFreeList)) {
        return NULL;
    }

    RWIMAGEGLOBAL(imageFormatFreeList) = RwFreeListCreate(sizeof(rwImageFormat), 3, 0);
    if (!RWIMAGEGLOBAL(imageFormatFreeList)) {
        RwFreeListDestroy(RWIMAGEGLOBAL(imageFreeList));
        RWIMAGEGLOBAL(imageFreeList) = NULL;
        return NULL;
    }

    RWIMAGEGLOBAL(imagePathSize) = 256;
    RWIMAGEGLOBAL(imagePath) = (RwChar*)RwMalloc(RWIMAGEGLOBAL(imagePathSize));
    if (!RWIMAGEGLOBAL(imagePath)) {
        RwFreeListDestroy(RWIMAGEGLOBAL(imageFormatFreeList));
        RWIMAGEGLOBAL(imageFormatFreeList) = NULL;
        RwFreeListDestroy(RWIMAGEGLOBAL(imageFreeList));
        RWIMAGEGLOBAL(imageFreeList) = NULL;
        return NULL;
    }

    RWIMAGEGLOBAL(imagePath)[0] = '\0';
    imageModule.numInstances++;

    RwImageSetGamma((RwReal)1.0);

    RWIMAGEGLOBAL(imageFormats) = NULL;
    RWIMAGEGLOBAL(scratchMemSize) = 256;
    RWIMAGEGLOBAL(scratchMem) = (RwChar*)RwMalloc(RWIMAGEGLOBAL(scratchMemSize));
    if (!RWIMAGEGLOBAL(scratchMem)) {
        RwFree(RWIMAGEGLOBAL(imagePath));
        RWIMAGEGLOBAL(imagePath) = NULL;
        RWIMAGEGLOBAL(imagePathSize) = 0;
        RwFreeListDestroy(RWIMAGEGLOBAL(imageFormatFreeList));
        RWIMAGEGLOBAL(imageFormatFreeList) = NULL;
        RwFreeListDestroy(RWIMAGEGLOBAL(imageFreeList));
        RWIMAGEGLOBAL(imageFreeList) = NULL;
        return NULL;
    }

    return instance;
}

void* _rwImageClose(void* instance, RwInt32 offset, RwInt32 size) {
    if (RWIMAGEGLOBAL(scratchMem)) {
        RwFree(RWIMAGEGLOBAL(scratchMem));
        RWIMAGEGLOBAL(scratchMem) = NULL;
        RWIMAGEGLOBAL(scratchMemSize) = 0;
    }

    if (RWIMAGEGLOBAL(imagePath)) {
        RwFree(RWIMAGEGLOBAL(imagePath));
        RWIMAGEGLOBAL(imagePath) = NULL;
        RWIMAGEGLOBAL(imagePathSize) = 0;
    }

    while (RWIMAGEGLOBAL(imageFormats)) {
        rwImageFormat* format = RWIMAGEGLOBAL(imageFormats);

        RWIMAGEGLOBAL(imageFormats) = format->nextFormat;
        RwFreeListFree(RWIMAGEGLOBAL(imageFormatFreeList), format);
    }

    if (RWIMAGEGLOBAL(imageFormatFreeList)) {
        RwFreeListDestroy(RWIMAGEGLOBAL(imageFormatFreeList));
        RWIMAGEGLOBAL(imageFormatFreeList) = NULL;
    }

    if (RWIMAGEGLOBAL(imageFreeList)) {
        RwFreeListDestroy(RWIMAGEGLOBAL(imageFreeList));
        RWIMAGEGLOBAL(imageFreeList) = NULL;
    }

    imageModule.numInstances--;

    return instance;
}

RwImage* RwImageCreate(RwInt32 width, RwInt32 height, RwInt32 depth) {
    RwImage* image = (RwImage*)RwFreeListAlloc(RWIMAGEGLOBAL(imageFreeList));

    if (!image) {
        return NULL;
    }

    image->width = width;
    image->height = height;
    image->depth = depth;
    image->cpPixels = NULL;
    image->palette = NULL;
    image->flags = 0;

    _rwPluginRegistryInitObject(&imageTKList, image);

    return image;
}

RwBool RwImageDestroy(RwImage* image) {
    if (image->flags & rwIMAGEALLOCATED) {
        RwImageFreePixels(image);
    }

    _rwPluginRegistryDeInitObject(&imageTKList, image);
    RwFreeListFree(RWIMAGEGLOBAL(imageFreeList), image);

    return TRUE;
}

RwImage* RwImageAllocatePixels(RwImage* image) {
    RwInt32 palettized = 0;
    RwInt32 paletteSize;
    RwInt32 pixelSize;
    RwInt32 totalSize;

    if ((RwUInt32)image->depth == 4 || (RwUInt32)image->depth == 8) {
        palettized = 1;
    }

    paletteSize = palettized ? (1 << image->depth) * (RwInt32)sizeof(RwRGBA) : 0;

    image->stride = (image->depth + 7) >> 3;
    image->stride = image->stride * image->width;
    image->stride = (image->stride + 3) & ~3;

    pixelSize = image->stride * image->height;
    totalSize = pixelSize + paletteSize;

    image->cpPixels = (RwUInt8*)RwMalloc(totalSize);
    if (!image->cpPixels) {
        RWERROR((E_RW_NOMEM, totalSize));
        return NULL;
    }

    image->palette = palettized ? (RwRGBA*)(image->cpPixels + pixelSize) : NULL;
    image->flags |= rwIMAGEALLOCATED;

    return image;
}

RwImage* RwImageFreePixels(RwImage* image) {
    RwFree(image->cpPixels);
    image->cpPixels = NULL;
    image->palette = NULL;
    image->flags &= ~rwIMAGEALLOCATED;

    return image;
}

RwImage* RwImageMakeMask(RwImage* image) {
    switch (image->depth) {
    case 4:
    case 8: {
        RwInt32 numColors = 1 << image->depth;
        RwRGBA* palette = image->palette;
        RwInt32 i;

        for (i = 0; i < numColors; i++) {
            RwInt32 max = palette->red;

            if (palette->green > max) {
                max = palette->green;
            }
            if (palette->blue > max) {
                max = palette->blue;
            }

            palette->alpha = (RwUInt8)max;
            palette++;
        }
        break;
    }
    case 32: {
        RwUInt8* row = image->cpPixels;
        RwInt32 y;

        for (y = 0; y < image->height; y++) {
            RwRGBA* pixel = (RwRGBA*)row;
            RwInt32 x;

            for (x = 0; x < image->width; x++) {
                RwInt32 max = pixel->red;

                if (pixel->green > max) {
                    max = pixel->green;
                }
                if (pixel->blue > max) {
                    max = pixel->blue;
                }

                pixel->alpha = (RwUInt8)max;
                pixel++;
            }

            row += image->stride;
        }
        break;
    }
    }

    return image;
}

RwImage* RwImageApplyMask(RwImage* image, const RwImage* mask) {
    RwInt32 y;
    RwInt32 x;
    RwImage* tempImage;

    if (image->width != mask->width || image->height != mask->height) {
        RWERROR((E_RW_INVIMAGESIZE));
        return NULL;
    }

    switch (image->depth) {
    case 4:
    case 8: {
        tempImage = RwImageCreate(image->width, image->height, image->depth);
        if (!tempImage) {
            return NULL;
        }

        if (!RwImageAllocatePixels(tempImage)) {
            RwImageDestroy(tempImage);
            return NULL;
        }

        RwImageCopy(tempImage, image);

        if (image->flags & rwIMAGEALLOCATED) {
            RwImageFreePixels(image);
        }

        image->depth = 32;
        RwImageAllocatePixels(image);
        RwImageCopy(image, tempImage);

        RwImageFreePixels(tempImage);
        RwImageDestroy(tempImage);
    }
        /* fall through */
    case 32: {
        RwRGBA* maskPalette = mask->palette;
        RwUInt8* maskRow = mask->cpPixels;
        RwUInt8* imageRow = image->cpPixels;

        for (y = 0; y < image->height; y++) {
            RwRGBA* dst = (RwRGBA*)imageRow;

            switch (mask->depth) {
            case 4:
            case 8: {
                RwUInt8* src = maskRow;

                for (x = 0; x < image->width; x++) {
                    dst->alpha = maskPalette[*src].alpha;
                    src++;
                    dst++;
                }
                break;
            }
            case 32: {
                RwRGBA* src = (RwRGBA*)maskRow;

                for (x = 0; x < image->width; x++) {
                    dst->alpha = src->alpha;
                    src++;
                    dst++;
                }
                break;
            }
            }

            imageRow += image->stride;
            maskRow += mask->stride;
        }
        break;
    }
    default:
        RWERROR((E_RW_INVIMAGEFORMAT));
        return NULL;
    }

    return image;
}

static const RwChar* ImagePathForAllFullNames(const RwChar* imageName, RwInt32 extraSize,
                                              rwImagePathCallBack callBack, void* data) {
    const RwChar* path = RWIMAGEGLOBAL(imagePath);

    if (_rwpathIsAbsolute(imageName) || !path || !path[0]) {
        RwInt32 size = RWSRCGLOBAL(stringFuncs).rwstrlen(imageName) + extraSize;
        RwChar* buffer;

        if (size > RWIMAGEGLOBAL(scratchMemSize)) {
            RwChar* mem;

            if (RWIMAGEGLOBAL(scratchMem)) {
                mem = (RwChar*)RwRealloc(RWIMAGEGLOBAL(scratchMem), size);
            } else {
                mem = (RwChar*)RwMalloc(size);
            }

            if (!mem) {
                RWERROR((E_RW_NOMEM, size));
                buffer = NULL;
                goto haveBuffer1;
            }

            RWIMAGEGLOBAL(scratchMem) = mem;
            RWIMAGEGLOBAL(scratchMemSize) = size;
        }

        buffer = RWIMAGEGLOBAL(scratchMem);
    haveBuffer1:
        if (!buffer) {
            return NULL;
        }

        RWSRCGLOBAL(stringFuncs).rwstrcpy(buffer, imageName);
        callBack(buffer, data);
    } else {
        while (path && path[0]) {
            const RwChar* nextPath;
            RwInt32 pathLength;
            RwInt32 size;
            RwChar* buffer;

            RWSRCGLOBAL(stringFuncs).rwstrchr(path, ';');
            nextPath = RWSRCGLOBAL(stringFuncs).rwstrchr(path, ';');
            if (nextPath) {
                pathLength = nextPath - path;
                nextPath++;
            } else {
                pathLength = RWSRCGLOBAL(stringFuncs).rwstrlen(path);
            }

            size = pathLength + (RWSRCGLOBAL(stringFuncs).rwstrlen(imageName) + extraSize);
            if (size > RWIMAGEGLOBAL(scratchMemSize)) {
                RwChar* mem;

                if (RWIMAGEGLOBAL(scratchMem)) {
                    mem = (RwChar*)RwRealloc(RWIMAGEGLOBAL(scratchMem), size);
                } else {
                    mem = (RwChar*)RwMalloc(size);
                }

                if (!mem) {
                    RWERROR((E_RW_NOMEM, size));
                    buffer = NULL;
                    goto haveBuffer2;
                }

                RWIMAGEGLOBAL(scratchMem) = mem;
                RWIMAGEGLOBAL(scratchMemSize) = size;
            }

            buffer = RWIMAGEGLOBAL(scratchMem);
        haveBuffer2:
            if (!buffer) {
                return NULL;
            }

            memcpy(buffer, path, pathLength);
            RWSRCGLOBAL(stringFuncs).rwstrcpy(buffer + pathLength, imageName);

            if (!callBack(buffer, data)) {
                return imageName;
            }

            path = nextPath;
        }
    }

    return imageName;
}

static const RwChar* ImageAttempRead(const RwChar* fullName, void* data) {
    rwImageReadData* readData = (rwImageReadData*)data;

    if (RWSRCGLOBAL(fileFuncs).rwfexist(fullName)) {
        readData->image = readData->imageRead(fullName);
        if (readData->image) {
            return NULL;
        }
    }

    return fullName;
}

static const RwChar* ImageDetermineExtender(const RwChar* fullName, void* data) {
    rwImageFormat* format;
    RwChar* end = (RwChar*)fullName + RWSRCGLOBAL(stringFuncs).rwstrlen(fullName);

    format = RWIMAGEGLOBAL(imageFormats);

    while (format) {
        RWSRCGLOBAL(stringFuncs).rwstrcpy(end, format->lcExtension);
        if (RWSRCGLOBAL(fileFuncs).rwfexist(fullName)) {
            *(const RwChar**)data = format->lcExtension;
            return NULL;
        }

        RWSRCGLOBAL(stringFuncs).rwstrcpy(end, format->ucExtension);
        if (RWSRCGLOBAL(fileFuncs).rwfexist(fullName)) {
            *(const RwChar**)data = format->ucExtension;
            return NULL;
        }

        format = format->nextFormat;
    }

    return fullName;
}

const RwChar* RwImageFindFileType(const RwChar* imageName) {
    const RwChar* extension = NULL;

    ImagePathForAllFullNames(imageName, 20, ImageDetermineExtender, &extension);

    return extension;
}

static inline RwImage* ImageRead(const RwChar* imageName) {
    const RwChar* fileName;
    const RwChar* found;
    const RwChar* extension;

    found = RWSRCGLOBAL(stringFuncs).rwstrrchr(imageName, ':');
    if (found) {
        fileName = found;
    } else {
        fileName = imageName;
    }

    found = RWSRCGLOBAL(stringFuncs).rwstrrchr(fileName, '/');
    if (found) {
        fileName = found;
    }

    found = RWSRCGLOBAL(stringFuncs).rwstrrchr(fileName, '\\');
    extension = RWSRCGLOBAL(stringFuncs).rwstrrchr(found ? found : fileName, '.');

    if (extension) {
        rwImageFormat* format = RWIMAGEGLOBAL(imageFormats);

        while (format) {
            if (!RWSRCGLOBAL(stringFuncs).rwstrcmp(format->lcExtension, extension) ||
                !RWSRCGLOBAL(stringFuncs).rwstrcmp(format->ucExtension, extension)) {
                if (format->imageRead) {
                    rwImageReadData readData;

                    readData.imageRead = format->imageRead;
                    readData.image = NULL;
                    ImagePathForAllFullNames(imageName, 5, ImageAttempRead, &readData);

                    return readData.image;
                }

                return NULL;
            }

            format = format->nextFormat;
        }

        return NULL;
    }

    return NULL;
}

RwImage* RwImageReadMaskedImage(const RwChar* imageName, const RwChar* maskname) {
    RwImage* image = ImageRead(imageName);

    if (image) {
        if (maskname && maskname[0]) {
            RwImage* mask = ImageRead(maskname);

            if (!mask) {
                RwImageDestroy(image);
                return NULL;
            }

            if (!RwImageMakeMask(mask)) {
                RwImageDestroy(image);
                RwImageDestroy(mask);
                return NULL;
            }

            if (!RwImageApplyMask(image, mask)) {
                RwImageDestroy(image);
                RwImageDestroy(mask);
                return NULL;
            }

            RwImageDestroy(mask);
        }

        return image;
    }

    return NULL;
}

RwRGBA* RwRGBASetFromPixel(RwRGBA* rgbOut, RwUInt32 pixelValue, RwInt32 rasterFormat) {
    RWSRCGLOBAL(stdFunc)[rwSTANDARDPIXELTORGB](rgbOut, &pixelValue, rasterFormat);

    return rgbOut;
}

static RwBool ImageConvertDepth(RwImage* destImage, const RwImage* sourceImage) {
    RwBool result = FALSE;
    RwInt32 width = destImage->width;
    RwInt32 height = destImage->height;
    RwUInt8* dstRow = destImage->cpPixels;
    RwUInt8* srcRow = sourceImage->cpPixels;
    RwRGBA* srcPalette = sourceImage->palette;

    switch ((sourceImage->depth << 8) | destImage->depth) {
    case 0x404:
    case 0x808:
    case 0x2020:
        result = TRUE;
        break;
    case 0x408: {
        RwInt32 y;

        for (y = 0; y < height; y++) {
            memcpy(dstRow, srcRow, width);
            srcRow += sourceImage->stride;
            dstRow += destImage->stride;
        }

        result = TRUE;
        break;
    }
    case 0x420:
    case 0x820: {
        RwInt32 y;

        for (y = 0; y < height; y++) {
            RwRGBA* dst = (RwRGBA*)dstRow;
            RwInt32 x;

            for (x = 0; x < width; x++) {
                dst[x] = srcPalette[srcRow[x]];
            }

            srcRow += sourceImage->stride;
            dstRow += destImage->stride;
        }

        result = TRUE;
        break;
    }
    case 0x804:
    case 0x2004:
    case 0x2008:
    default:
        RWERROR((E_RW_INVIMAGEFORMAT));
        break;
    }

    return result;
}

RwImage* RwImageCopy(RwImage* destImage, const RwImage* sourceImage) {
    if (destImage->depth == sourceImage->depth) {
        RwUInt8* srcRow;
        RwInt32 y;
        RwInt32 rowBytes;
        RwUInt8* dstRow;

        if (destImage->palette && sourceImage->palette && sourceImage->depth <= 8) {
            memcpy(destImage->palette, sourceImage->palette, (1 << sourceImage->depth) * sizeof(RwRGBA));
        }

        rowBytes = ((destImage->depth + 7) >> 3) * destImage->width;
        srcRow = sourceImage->cpPixels;
        dstRow = destImage->cpPixels;

        for (y = 0; y < destImage->height; y++) {
            memcpy(dstRow, srcRow, rowBytes);
            dstRow += destImage->stride;
            srcRow += sourceImage->stride;
        }
    } else if (!ImageConvertDepth(destImage, sourceImage)) {
        destImage = NULL;
    }

    destImage->flags &= ~rwIMAGEGAMMACORRECTED;
    destImage->flags |= sourceImage->flags & rwIMAGEGAMMACORRECTED;

    return destImage;
}

static inline void ImageGammaCorrectArrayOfRGBA(RwRGBA* rgbaOut, const RwRGBA* rgbaIn, RwInt32 numEls) {
    const RwUInt8* gammaTable = RWIMAGEGLOBAL(gammaTable);

    while (numEls--) {
        rgbaOut->red = gammaTable[rgbaIn->red];
        rgbaOut->green = gammaTable[rgbaIn->green];
        rgbaOut->blue = gammaTable[rgbaIn->blue];
        rgbaOut->alpha = rgbaIn->alpha;
        rgbaIn++;
        rgbaOut++;
    }
}

RwImage* RwImageGammaCorrect(RwImage* image) {
    switch (image->depth) {
    case 4:
    case 8: {
        RwRGBA* palette = image->palette;
        RwInt32 numColors = 1 << image->depth;

        if (!palette) {
            RWERROR((E_RW_NULLP));
            return NULL;
        }

        ImageGammaCorrectArrayOfRGBA(palette, palette, numColors);
        break;
    }
    case 32: {
        RwUInt8* row = image->cpPixels;
        RwInt32 width = image->width;
        RwInt32 height = image->height;
        RwInt32 y;

        if (!row) {
            RWERROR((E_RW_NULLP));
            return NULL;
        }

        for (y = 0; y < height; y++) {
            ImageGammaCorrectArrayOfRGBA((RwRGBA*)row, (RwRGBA*)row, width);
            row += image->stride;
        }
        break;
    }
    default:
        RWERROR((E_RW_INVIMAGEDEPTH));
        return NULL;
    }

    image->flags |= rwIMAGEGAMMACORRECTED;

    return image;
}

RwBool RwImageSetGamma(RwReal gammaValue) {
    RwReal invGamma = (RwReal)1.0 / gammaValue;
    RwInt32 i;

    RWIMAGEGLOBAL(gammaVal) = gammaValue;
    RWIMAGEGLOBAL(gammaTable)[0] = 0;
    RWIMAGEGLOBAL(invGammaTable)[0] = 0;

    for (i = 1; i < 256; i++) {
        RwReal value = (RwReal)i / (RwReal)255.0;

        RWIMAGEGLOBAL(gammaTable)[i] = (RwUInt8)(RwInt32)((RwReal)0.5 + (RwReal)255.0 * (RwReal)pow(value, invGamma));
        RWIMAGEGLOBAL(invGammaTable)[i] =
            (RwUInt8)(RwInt32)((RwReal)0.5 + (RwReal)255.0 * (RwReal)pow(value, gammaValue));
    }

    return TRUE;
}
