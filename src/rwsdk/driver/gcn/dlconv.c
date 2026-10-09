/*
 * GameCube device layer pixel conversion: RGBA to and from the native texel
 * formats (RGB565, RGB5A3 as 555/3444, and the 24/32 bit forms), and copying
 * between images and rasters, quantizing true colour images when a palettized
 * raster is the destination.
 */

#include "rwsdk/rwcore.h"

#define rwPLUGIN_ID 1

#define E_RW_DLNOTSUPPORTED 2
#define E_RW_INVIMAGEDEPTH 0x80000008
#define E_RW_INVRASTERDEPTH 0x8000000C
#define E_RW_INVRASTERFORMAT 0x8000000D

#define rwRASTERFORMATDEFAULT 0x0000
#define rwRASTERFORMAT1555 0x0100
#define rwRASTERFORMAT565 0x0200
#define rwRASTERFORMAT4444 0x0300
#define rwRASTERFORMATLUM8 0x0400
#define rwRASTERFORMAT8888 0x0500
#define rwRASTERFORMAT888 0x0600
#define rwRASTERFORMAT555 0x0A00
#define rwRASTERFORMATPIXELFORMATMASK 0x0F00
#define rwRASTERFORMATPAL8 0x2000
#define rwRASTERFORMATPAL4 0x4000

#define rwRASTERLOCKWRITE 0x01
#define rwRASTERLOCKREAD 0x02
#define rwRASTERLOCKNOFETCH 0x04

#define rwRASTERPIXELLOCKEDREAD 0x02
#define rwRASTERPIXELLOCKEDWRITE 0x04
#define rwRASTERPALETTELOCKEDREAD 0x08
#define rwRASTERPALETTELOCKEDWRITE 0x10

typedef struct rwPalQuant {
    void* Mcube;
    void* cubes;
    void* root;
    RwFreeList* cubefreelist;
} rwPalQuant;

typedef struct RwDlRasterExt {
    /* 0x00 */ RwUInt8 pad00[0x33];
    /* 0x33 */ RwUInt8 customPalette;
} RwDlRasterExt;

typedef void (*RwDlConvToRGBFn)(RwRGBA* rgb, RwUInt32 pixel);
typedef RwUInt32 (*RwDlConvFromRGBFn)(const RwRGBA* rgb);

#define RASTEREXTFROMRASTER(raster) ((RwDlRasterExt*)((RwUInt8*)(raster) + _rwDlRasterExtOffset))

extern RwInt32 _rwDlRasterExtOffset;

RwUInt8* RwRasterLock(RwRaster* raster, RwUInt8 level, RwInt32 lockMode);
RwRaster* RwRasterUnlock(RwRaster* raster);
RwUInt8* RwRasterLockPalette(RwRaster* raster, RwInt32 lockMode);
RwRaster* RwRasterUnlockPalette(RwRaster* raster);
RwImage* RwImageCreate(RwInt32 width, RwInt32 height, RwInt32 depth);
RwBool RwImageDestroy(RwImage* image);
RwImage* RwImageAllocatePixels(RwImage* image);
RwBool _rwPalQuantInit(rwPalQuant* pq);
void _rwPalQuantTerm(rwPalQuant* pq);
void _rwPalQuantAddImage(rwPalQuant* pq, RwImage* img, RwReal weight);
RwInt32 _rwPalQuantResolvePalette(RwRGBA* palette, RwInt32 maxcols, rwPalQuant* pq);
void _rwPalQuantMatchImage(RwUInt8* dstpixels, RwInt32 dststride, RwInt32 dstdepth, RwBool dstPacked, rwPalQuant* pq,
                           RwImage* img);

RwInt32 _rwDlFindMSB(RwInt32 num)
{
    RwInt32 pos = -1;

    while (num) {
        num >>= 1;
        pos++;
    }
    return pos;
}

#define DLCONV8888TO555(rgb)                                                                         \
    (0x8000 | (((rgb)->red << 7) & 0x7C00) | (((rgb)->green << 2) & 0x3E0) | (((rgb)->blue >> 3) & 0x1F))

#define DLCONV8888TO565(rgb)                                                                         \
    ((((rgb)->red << 8) & 0xF800) | (((rgb)->green << 3) & 0x7E0) | (((rgb)->blue >> 3) & 0x1F))

#define DLCONV8888TO555OR3444(rgb)                                                                   \
    (((rgb)->alpha != 0xFF) ? ((((rgb)->alpha << 7) & 0x7000) | (((rgb)->red << 4) & 0xF00) |       \
                               ((rgb)->green & 0xF0) | (((rgb)->blue >> 4) & 0xF))                  \
                            : DLCONV8888TO555(rgb))

#define DLCONV8888TODL888(rgb) (0xFF000000 | ((rgb)->red << 16) | ((rgb)->green << 8) | (rgb)->blue)

#define DLCONV8888TODL8888(rgb) (((rgb)->alpha << 24) | ((rgb)->red << 16) | ((rgb)->green << 8) | (rgb)->blue)

#define DLCONV555TO8888(rgb, pixel)                                                                  \
    do {                                                                                             \
        (rgb)->red = ((pixel) >> 7) & 0xF8;                                                          \
        (rgb)->green = ((pixel) >> 2) & 0xF8;                                                        \
        (rgb)->blue = (pixel) << 3;                                                                  \
        (rgb)->alpha = 0xFF;                                                                         \
    } while (0)

#define DLCONV565TO8888(rgb, pixel)                                                                  \
    do {                                                                                             \
        (rgb)->red = ((pixel) >> 8) & 0xF8;                                                          \
        (rgb)->green = ((pixel) >> 3) & 0xFC;                                                        \
        (rgb)->blue = (pixel) << 3;                                                                  \
        (rgb)->alpha = 0xFF;                                                                         \
    } while (0)

#define DLCONV1555TO8888(rgb, pixel)                                                                 \
    do {                                                                                             \
        if ((pixel) & 0x8000) {                                                                      \
            DLCONV555TO8888(rgb, pixel);                                                             \
        } else {                                                                                     \
            (rgb)->red = ((pixel) >> 4) & 0xF0;                                                      \
            (rgb)->green = (pixel) & 0xF0;                                                           \
            (rgb)->blue = (pixel) << 4;                                                              \
            (rgb)->alpha = 0;                                                                        \
        }                                                                                            \
    } while (0)

#define DLCONV4444TO8888(rgb, pixel)                                                                 \
    do {                                                                                             \
        if ((pixel) & 0x8000) {                                                                      \
            DLCONV555TO8888(rgb, pixel);                                                             \
        } else {                                                                                     \
            (rgb)->red = ((pixel) >> 4) & 0xF0;                                                      \
            (rgb)->green = (pixel) & 0xF0;                                                           \
            (rgb)->blue = (pixel) << 4;                                                              \
            (rgb)->alpha = ((pixel) >> 7) & 0xE0;                                                    \
        }                                                                                            \
    } while (0)

#define DLCONVDL888TO8888(rgb, pixel)                                                                \
    do {                                                                                             \
        (rgb)->alpha = 0xFF;                                                                         \
        (rgb)->red = (pixel) >> 16;                                                                  \
        (rgb)->green = (pixel) >> 8;                                                                 \
        (rgb)->blue = (pixel);                                                                       \
    } while (0)

#define DLCONVDL8888TO8888(rgb, pixel)                                                               \
    do {                                                                                             \
        (rgb)->alpha = (pixel) >> 24;                                                                \
        (rgb)->red = (pixel) >> 16;                                                                  \
        (rgb)->green = (pixel) >> 8;                                                                 \
        (rgb)->blue = (pixel);                                                                       \
    } while (0)

#pragma dont_inline on
RwUInt32 _rwDlConv8888To555(const RwRGBA* rgb)
{
    return DLCONV8888TO555(rgb);
}

RwUInt32 _rwDlConv8888To565(const RwRGBA* rgb)
{
    return DLCONV8888TO565(rgb);
}

RwUInt32 _rwDlConv8888To555or3444(const RwRGBA* rgb)
{
    if (rgb->alpha != 0xFF) {
        return ((rgb->alpha << 7) & 0x7000) | ((rgb->red << 4) & 0xF00) | (rgb->green & 0xF0) | ((rgb->blue >> 4) & 0xF);
    }
    return DLCONV8888TO555(rgb);
}

RwUInt32 _rwDlConv8888ToDl888(const RwRGBA* rgb)
{
    return DLCONV8888TODL888(rgb);
}

RwUInt32 _rwDlConv8888ToDl8888(const RwRGBA* rgb)
{
    return DLCONV8888TODL8888(rgb);
}

#pragma dont_inline reset

RwBool _rwDlRGBToPixel(void* pixel, const RwRGBA* rgb, RwInt32 format)
{
    volatile RwUInt32 value;

    switch (format & rwRASTERFORMATPIXELFORMATMASK) {
    case rwRASTERFORMATDEFAULT:
        value = DLCONV8888TO565(rgb);
        break;
    case rwRASTERFORMATLUM8:
        RWERROR((E_RW_DLNOTSUPPORTED, "rwRASTERFORMATLUM8 not yet supported"));
        break;
    case rwRASTERFORMAT1555:
    case rwRASTERFORMAT4444:
        value = DLCONV8888TO555OR3444(rgb);
        break;
    case rwRASTERFORMAT555:
        value = DLCONV8888TO555(rgb);
        break;
    case rwRASTERFORMAT565:
        value = DLCONV8888TO565(rgb);
        break;
    case rwRASTERFORMAT8888:
        value = DLCONV8888TODL8888(rgb);
        break;
    case rwRASTERFORMAT888:
        value = DLCONV8888TODL888(rgb);
        break;
    default:
        RWERROR((E_RW_INVRASTERFORMAT));
        break;
    }

    *(RwUInt32*)pixel = value;
    return TRUE;
}

#pragma dont_inline on
void _rwDlConv555To8888(RwRGBA* rgb, RwUInt32 pixel)
{
    DLCONV555TO8888(rgb, pixel);
}

void _rwDlConv565To8888(RwRGBA* rgb, RwUInt32 pixel)
{
    DLCONV565TO8888(rgb, pixel);
}

void _rwDlConv1555To8888(RwRGBA* rgb, RwUInt32 pixel)
{
    DLCONV1555TO8888(rgb, pixel);
}

void _rwDlConv4444To8888(RwRGBA* rgb, RwUInt32 pixel)
{
    DLCONV4444TO8888(rgb, pixel);
}

void _rwDlConvDl888To8888(RwRGBA* rgb, RwUInt32 pixel)
{
    DLCONVDL888TO8888(rgb, pixel);
}

void _rwDlConvDl8888To8888(RwRGBA* rgb, RwUInt32 pixel)
{
    DLCONVDL8888TO8888(rgb, pixel);
}

#pragma dont_inline reset

RwBool _rwDlPixelToRGB(RwRGBA* rgb, const void* pixel, RwInt32 format)
{
    RwUInt32 value = *(const RwUInt32*)pixel;

    switch (format & rwRASTERFORMATPIXELFORMATMASK) {
    case rwRASTERFORMATDEFAULT:
        _rwDlConv565To8888(rgb, value);
        break;
    case rwRASTERFORMATLUM8:
        RWERROR((E_RW_DLNOTSUPPORTED, "rwRASTERFORMATLUM8 not yet supported"));
        break;
    case rwRASTERFORMAT555:
        DLCONV555TO8888(rgb, value);
        break;
    case rwRASTERFORMAT1555:
        DLCONV1555TO8888(rgb, value);
        break;
    case rwRASTERFORMAT565:
        DLCONV565TO8888(rgb, value);
        break;
    case rwRASTERFORMAT4444:
        DLCONV4444TO8888(rgb, value);
        break;
    case rwRASTERFORMAT888:
        DLCONVDL888TO8888(rgb, value);
        break;
    case rwRASTERFORMAT8888:
        DLCONVDL8888TO8888(rgb, value);
        break;
    default:
        RWERROR((E_RW_INVRASTERFORMAT));
        break;
    }
    return TRUE;
}

static inline RwDlConvToRGBFn DlRasterGetConvToRGB(RwRaster* raster)
{
    RwDlConvToRGBFn convFn = NULL;

    switch (RwRasterGetFormat(raster) & rwRASTERFORMATPIXELFORMATMASK) {
    case rwRASTERFORMATLUM8:
        RWERROR((E_RW_DLNOTSUPPORTED, "rwRASTERFORMATLUM8 not yet supported"));
        break;
    case rwRASTERFORMAT555:
        convFn = _rwDlConv555To8888;
        break;
    case rwRASTERFORMAT565:
        convFn = _rwDlConv565To8888;
        break;
    case rwRASTERFORMAT1555:
        convFn = _rwDlConv1555To8888;
        break;
    case rwRASTERFORMAT4444:
        convFn = _rwDlConv4444To8888;
        break;
    case rwRASTERFORMAT8888:
        convFn = _rwDlConvDl8888To8888;
        break;
    case rwRASTERFORMAT888:
        convFn = _rwDlConvDl888To8888;
        break;
    default:
        RWERROR((E_RW_INVRASTERFORMAT));
        break;
    }
    return convFn;
}

static inline RwDlConvFromRGBFn DlRasterGetConvFromRGB(RwRaster* raster)
{
    RwDlConvFromRGBFn convFn = NULL;

    switch (RwRasterGetFormat(raster) & rwRASTERFORMATPIXELFORMATMASK) {
    case rwRASTERFORMATLUM8:
        RWERROR((E_RW_DLNOTSUPPORTED, "rwRASTERFORMATLUM8 not yet supported"));
        break;
    case rwRASTERFORMAT555:
        convFn = _rwDlConv8888To555;
        break;
    case rwRASTERFORMAT565:
        convFn = _rwDlConv8888To565;
        break;
    case rwRASTERFORMAT1555:
    case rwRASTERFORMAT4444:
        convFn = _rwDlConv8888To555or3444;
        break;
    case rwRASTERFORMAT888:
        convFn = _rwDlConv8888ToDl888;
        break;
    case rwRASTERFORMAT8888:
        convFn = _rwDlConv8888ToDl8888;
        break;
    default:
        RWERROR((E_RW_INVRASTERFORMAT));
        break;
    }
    return convFn;
}

void _rwDlImage4GetFromRaster(RwImage* image, RwRaster* raster)
{
    RwDlConvToRGBFn convFn;
    RwInt32 i;
    RwInt32 y;
    RwInt32 x;

    convFn = DlRasterGetConvToRGB(raster);

    switch (raster->depth) {
    case 4:
        for (i = 0; i < 16; i++) {
            convFn(&image->palette[i], ((RwUInt16*)raster->palette)[i]);
        }
        for (y = 0; y < raster->height; y++) {
            RwUInt8* dst = image->cpPixels + image->stride * y;
            RwUInt8* src = raster->cpPixels + raster->stride * y;

            for (x = 0; x < raster->width; x += 2) {
                dst[0] = (*src >> 4) & 0xF;
                dst[1] = *src & 0xF;
                src++;
                dst += 2;
            }
        }
        break;
    case 8:
    case 16:
    case 32:
        RWERROR((E_RW_DLNOTSUPPORTED, "Conversion from 8/16/32bit rasters to 4bit images is not supported"));
        break;
    case 24:
    default:
        RWERROR((E_RW_INVRASTERDEPTH));
        break;
    }
}

void _rwDlImage8GetFromRaster(RwImage* image, RwRaster* raster)
{
    RwDlConvToRGBFn convFn;
    RwInt32 i;
    RwInt32 y;
    RwInt32 x;

    convFn = DlRasterGetConvToRGB(raster);

    switch (raster->depth) {
    case 4:
        for (i = 0; i < 16; i++) {
            convFn(&image->palette[i], ((RwUInt16*)raster->palette)[i]);
        }
        for (y = 0; y < raster->height; y++) {
            RwUInt8* dst = image->cpPixels + image->stride * y;
            RwUInt8* src = raster->cpPixels + raster->stride * y;

            for (x = 0; x < raster->width; x += 2) {
                dst[0] = (*src >> 4) & 0xF;
                dst[1] = *src & 0xF;
                src++;
                dst += 2;
            }
        }
        break;
    case 8:
        for (i = 0; i < 256; i++) {
            convFn(&image->palette[i], ((RwUInt16*)raster->palette)[i]);
        }
        for (y = 0; y < raster->height; y++) {
            RwUInt8* src = raster->cpPixels + raster->stride * y;
            RwUInt8* dst = image->cpPixels + image->stride * y;

            for (x = 0; x < raster->width; x++) {
                *dst = *src;
                src++;
                dst++;
            }
        }
        break;
    case 16:
    case 32:
        RWERROR((E_RW_DLNOTSUPPORTED, "Conversion from 16/32bit rasters to 8bit images is not supported"));
        break;
    case 24:
    default:
        RWERROR((E_RW_INVRASTERDEPTH));
        break;
    }
}

void _rwDlImage32GetFromRaster(RwImage* image, RwRaster* raster)
{
    RwDlConvToRGBFn convFn;
    RwInt32 i;
    RwInt32 y;
    RwInt32 x;

    convFn = DlRasterGetConvToRGB(raster);

    switch (raster->depth) {
    case 4: {
        RwRGBA palette[16];

        for (i = 0; i < 16; i++) {
            convFn(&palette[i], ((RwUInt16*)raster->palette)[i]);
        }
        for (y = 0; y < raster->height; y++) {
            RwRGBA* dst = (RwRGBA*)(image->cpPixels + image->stride * y);
            RwUInt8* src = raster->cpPixels + raster->stride * y;

            for (x = 0; x < raster->width; x += 2) {
                dst[0] = palette[(*src >> 4) & 0xF];
                dst[1] = palette[*src & 0xF];
                src++;
                dst += 2;
            }
        }
        break;
    }
    case 8: {
        RwRGBA palette[256];

        for (i = 0; i < 256; i++) {
            convFn(&palette[i], ((RwUInt16*)raster->palette)[i]);
        }
        for (y = 0; y < raster->height; y++) {
            RwUInt8* src = raster->cpPixels + raster->stride * y;
            RwRGBA* dst = (RwRGBA*)(image->cpPixels + image->stride * y);

            for (x = 0; x < raster->width; x++) {
                *dst = palette[*src];
                src++;
                dst++;
            }
        }
        break;
    }
    case 16:
        for (y = 0; y < raster->height; y++) {
            RwUInt16* src = (RwUInt16*)(raster->cpPixels + raster->stride * y);
            RwRGBA* dst = (RwRGBA*)(image->cpPixels + image->stride * y);

            for (x = 0; x < raster->width; x++) {
                convFn(dst, *src);
                dst++;
                src++;
            }
        }
        break;
    case 24:
        RWERROR((E_RW_INVRASTERDEPTH));
        break;
    case 32:
        for (y = 0; y < raster->height; y++) {
            RwUInt32* src = (RwUInt32*)(raster->cpPixels + raster->stride * y);
            RwRGBA* dst = (RwRGBA*)(image->cpPixels + image->stride * y);

            for (x = 0; x < raster->width; x++) {
                convFn(dst, *src);
                dst++;
                src++;
            }
        }
        break;
    default:
        RWERROR((E_RW_INVRASTERDEPTH));
        break;
    }
}

RwBool _rwDlImageGetFromRaster(RwImage* image, RwRaster* raster)
{
    RwBool locked;
    RwBool paletteLocked;

    locked = FALSE;
    paletteLocked = FALSE;
    if (!(raster->privateFlags & rwRASTERPIXELLOCKEDREAD)) {
        RwRasterLock(raster, 0, rwRASTERLOCKREAD);
        locked = TRUE;
    }

    if ((RwRasterGetFormat(raster) & (rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8)) &&
        !(raster->privateFlags & rwRASTERPALETTELOCKEDREAD)) {
        RwRasterLockPalette(raster, rwRASTERLOCKREAD);
        paletteLocked = TRUE;
    }

    switch (image->depth) {
    case 4:
        _rwDlImage4GetFromRaster(image, raster);
        break;
    case 8:
        _rwDlImage8GetFromRaster(image, raster);
        break;
    case 32:
        _rwDlImage32GetFromRaster(image, raster);
        break;
    default:
        RWERROR((E_RW_INVIMAGEDEPTH));
        break;
    }

    if (paletteLocked == TRUE) {
        RwRasterUnlockPalette(raster);
    }
    if (locked == TRUE) {
        RwRasterUnlock(raster);
    }
    return TRUE;
}

static inline RwImage* DlImagePalettize(RwImage* image, RwInt32 depth)
{
    RwImage* palImage;
    rwPalQuant palQuant;

    palImage = RwImageCreate(image->width, image->height, depth);
    if (palImage == NULL) {
        return NULL;
    }
    RwImageAllocatePixels(palImage);

    if (!_rwPalQuantInit(&palQuant)) {
        return NULL;
    }
    _rwPalQuantAddImage(&palQuant, image, 1.0f);
    _rwPalQuantResolvePalette(palImage->palette, 1 << depth, &palQuant);
    _rwPalQuantMatchImage(palImage->cpPixels, palImage->stride, palImage->depth, FALSE, &palQuant, image);
    _rwPalQuantTerm(&palQuant);
    return palImage;
}

void _rwDlRasterPalletized4SetFromImage(RwRaster* raster, RwImage* image)
{
    RwDlRasterExt* rasterExt = RASTEREXTFROMRASTER(raster->parent);
    RwDlConvFromRGBFn convFn;
    RwInt32 i;
    RwInt32 y;
    RwInt32 x;

    convFn = DlRasterGetConvFromRGB(raster);

    switch (image->depth) {
    case 4:
        for (y = 0; y < raster->height; y++) {
            RwUInt8* src = image->cpPixels + image->stride * y;
            RwUInt8* dst = raster->cpPixels + raster->stride * y;

            for (x = 0; x < raster->width; x += 2) {
                *dst++ = ((src[0] << 4) & 0xF0) | (src[1] & 0xF);
                src += 2;
            }
        }
        if (!rasterExt->customPalette) {
            RwUInt16* palette = (RwUInt16*)raster->palette;

            for (i = 0; i < 16; i++) {
                palette[i] = convFn(&image->palette[i]);
            }
        }
        break;
    case 8:
    case 32: {
        RwImage* palImage = DlImagePalettize(image, raster->depth);

        if (palImage) {
            _rwDlRasterPalletized4SetFromImage(raster, palImage);
            RwImageDestroy(palImage);
        }
        break;
    }
    default:
        RWERROR((E_RW_INVIMAGEDEPTH));
        break;
    }
}

void _rwDlRasterPalletized8SetFromImage(RwRaster* raster, RwImage* image)
{
    RwDlConvFromRGBFn convFn;
    RwDlRasterExt* rasterExt;
    RwInt32 i;
    RwInt32 y;
    RwInt32 x;

    convFn = DlRasterGetConvFromRGB(raster);

    switch (image->depth) {
    case 4:
    case 8: {
        rasterExt = RASTEREXTFROMRASTER(raster->parent);

        for (y = 0; y < raster->height; y++) {
            RwUInt8* src = image->cpPixels + image->stride * y;
            RwUInt8* dst = raster->cpPixels + raster->stride * y;

            for (x = 0; x < raster->width; x++) {
                *dst = *src;
                src++;
                dst++;
            }
        }
        if (!rasterExt->customPalette) {
            RwUInt16* palette = (RwUInt16*)raster->palette;

            for (i = 0; i < (1 << image->depth); i++) {
                palette[i] = convFn(&image->palette[i]);
            }
        }
        break;
    }
    case 32: {
        RwImage* palImage = DlImagePalettize(image, raster->depth);

        if (palImage) {
            _rwDlRasterPalletized8SetFromImage(raster, palImage);
            RwImageDestroy(palImage);
        }
        break;
    }
    default:
        RWERROR((E_RW_INVIMAGEDEPTH));
        break;
    }
}

void _rwDlRaster16SetFromImage(RwRaster* raster, RwImage* image)
{
    RwDlConvFromRGBFn convFn;
    RwInt32 i;
    RwInt32 y;
    RwInt32 x;

    convFn = DlRasterGetConvFromRGB(raster);

    switch (image->depth) {
    case 4:
    case 8: {
        RwUInt16 palette[256];

        for (i = 0; i < (1 << image->depth); i++) {
            palette[i] = convFn(&image->palette[i]);
        }
        for (y = 0; y < raster->height; y++) {
            RwUInt8* src = image->cpPixels + image->stride * y;
            RwUInt16* dst = (RwUInt16*)(raster->cpPixels + raster->stride * y);

            for (x = 0; x < raster->width; x++) {
                *dst = palette[*src];
                src++;
                dst++;
            }
        }
        break;
    }
    case 32:
        for (y = 0; y < raster->height; y++) {
            RwRGBA* src = (RwRGBA*)(image->cpPixels + image->stride * y);
            RwUInt16* dst = (RwUInt16*)(raster->cpPixels + raster->stride * y);

            for (x = 0; x < raster->width; x++) {
                *dst = convFn(src);
                dst++;
                src++;
            }
        }
        break;
    default:
        RWERROR((E_RW_INVIMAGEDEPTH));
        break;
    }
}

void _rwDlRaster32SetFromImage(RwRaster* raster, RwImage* image)
{
    RwDlConvFromRGBFn convFn;
    RwInt32 i;
    RwInt32 y;
    RwInt32 x;

    convFn = DlRasterGetConvFromRGB(raster);

    switch (image->depth) {
    case 4:
    case 8: {
        RwUInt32 palette[256];

        for (i = 0; i < (1 << image->depth); i++) {
            palette[i] = convFn(&image->palette[i]);
        }
        for (y = 0; y < raster->height; y++) {
            RwUInt8* src = image->cpPixels + image->stride * y;
            RwUInt32* dst = (RwUInt32*)(raster->cpPixels + raster->stride * y);

            for (x = 0; x < raster->width; x++) {
                *dst = palette[*src];
                src++;
                dst++;
            }
        }
        break;
    }
    case 32:
        for (y = 0; y < raster->height; y++) {
            RwRGBA* src = (RwRGBA*)(image->cpPixels + image->stride * y);
            RwUInt32* dst = (RwUInt32*)(raster->cpPixels + raster->stride * y);

            for (x = 0; x < raster->width; x++) {
                *dst = convFn(src);
                dst++;
                src++;
            }
        }
        break;
    default:
        RWERROR((E_RW_INVIMAGEDEPTH));
        break;
    }
}

RwBool _rwDlRasterSetFromImage(RwRaster* raster, RwImage* image)
{
    RwBool locked;
    RwBool paletteLocked;
    RwInt32 format;

    locked = FALSE;
    paletteLocked = FALSE;
    format = RwRasterGetFormat(raster);

    if (raster->privateFlags & rwRASTERPIXELLOCKEDWRITE) {
        locked = TRUE;
    }
    if (!locked) {
        if (!RwRasterLock(raster, 0, rwRASTERLOCKWRITE | rwRASTERLOCKNOFETCH)) {
            return FALSE;
        }
    }

    format &= rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8;
    if (format) {
        if (raster->privateFlags & rwRASTERPALETTELOCKEDWRITE) {
            paletteLocked = TRUE;
        }
        if (!paletteLocked) {
            if (!RwRasterLockPalette(raster, rwRASTERLOCKWRITE | rwRASTERLOCKNOFETCH)) {
                return FALSE;
            }
        }
    }

    switch (raster->depth) {
    case 4:
        _rwDlRasterPalletized4SetFromImage(raster, image);
        break;
    case 8:
        _rwDlRasterPalletized8SetFromImage(raster, image);
        break;
    case 16:
        _rwDlRaster16SetFromImage(raster, image);
        break;
    case 24:
        RWERROR((E_RW_INVRASTERDEPTH));
        break;
    case 32:
        _rwDlRaster32SetFromImage(raster, image);
        break;
    default:
        RWERROR((E_RW_INVRASTERDEPTH));
        break;
    }

    if (format && !paletteLocked) {
        RwRasterUnlockPalette(raster);
    }
    if (!locked) {
        RwRasterUnlock(raster);
    }
    return TRUE;
}

RwInt32 _rwDlImageFindFormat(RwImage* image)
{
    RwInt32 depth = image->depth;
    RwBool transparent = FALSE;
    RwInt32 format;
    RwInt32 x;
    RwInt32 y;

    if (depth == 4 || depth == 8) {
        RwInt32 width = image->width;
        RwInt32 height = image->height;
        RwUInt8* row = image->cpPixels;
        RwRGBA* palette = image->palette;

        for (y = 0; y < height; y++) {
            RwUInt8* pixel = row;

            for (x = 0; x < width; x++) {
                RwUInt8 alpha = palette[*pixel].alpha;

                if (alpha != 0xFF) {
                    transparent = TRUE;
                    if (alpha > 0xF) {
                        if (depth == 4) {
                            return rwRASTERFORMAT4444 | rwRASTERFORMATPAL4;
                        }
                        return rwRASTERFORMAT4444 | rwRASTERFORMATPAL8;
                    }
                }
                pixel++;
            }
            row += image->stride;
        }
    } else {
        RwInt32 width = image->width;
        RwInt32 height = image->height;
        RwUInt8* row = image->cpPixels;

        for (y = 0; y < height; y++) {
            RwRGBA* pixel = (RwRGBA*)row;

            for (x = 0; x < width; x++) {
                RwUInt8 alpha = pixel->alpha;

                if (alpha != 0xFF) {
                    transparent = TRUE;
                    if (alpha > 0xF) {
                        return rwRASTERFORMAT4444;
                    }
                }
                pixel++;
            }
            row += image->stride;
        }
    }

    format = transparent ? rwRASTERFORMAT1555 : rwRASTERFORMAT565;
    if (depth == 4) {
        return format | rwRASTERFORMATPAL4;
    }
    if (depth == 8) {
        format |= rwRASTERFORMATPAL8;
    }
    return format;
}
