/*
 * Image resampling: each destination pixel is the area weighted average of
 * the 32 bit source pixels it covers, walked in 16.16 fixed point one span
 * (row) at a time. Images of other depths are first copied to 32 bits.
 */

#include "rwsdk/rwcore.h"

#define rwIMAGEGAMMACORRECTED 0x02

typedef struct RwRGBAReal {
    RwReal red;
    RwReal green;
    RwReal blue;
    RwReal alpha;
} RwRGBAReal;

RwImage* RwImageCreate(RwInt32 width, RwInt32 height, RwInt32 depth);
RwBool RwImageDestroy(RwImage* image);
RwImage* RwImageAllocatePixels(RwImage* image);
RwImage* RwImageFreePixels(RwImage* image);
RwImage* RwImageCopy(RwImage* destImage, const RwImage* sourceImage);

#define RwRGBARealFromRwRGBA(o, i)                                                                     \
    do {                                                                                               \
        (o)->red = (RwReal)(i)->red * (1.0f / 255.0f);                                                 \
        (o)->green = (RwReal)(i)->green * (1.0f / 255.0f);                                             \
        (o)->blue = (RwReal)(i)->blue * (1.0f / 255.0f);                                               \
        (o)->alpha = (RwReal)(i)->alpha * (1.0f / 255.0f);                                             \
    } while (0)

#define RwRGBARealScale(o, a, scale)                                                                   \
    do {                                                                                               \
        (o)->red = (a)->red * (scale);                                                                 \
        (o)->green = (a)->green * (scale);                                                             \
        (o)->blue = (a)->blue * (scale);                                                               \
        (o)->alpha = (a)->alpha * (scale);                                                             \
    } while (0)

#define RwRGBARealAdd(o, a, b)                                                                         \
    do {                                                                                               \
        (o)->red = (a)->red + (b)->red;                                                                \
        (o)->green = (a)->green + (b)->green;                                                          \
        (o)->blue = (a)->blue + (b)->blue;                                                             \
        (o)->alpha = (a)->alpha + (b)->alpha;                                                          \
    } while (0)

/* Average colour of row y between the 16.16 x positions x0 and x1. */
static void ImageResampleGetSpan(const RwImage* image, RwInt32 x0, RwInt32 x1, RwInt32 y, RwRGBAReal* out)
{
    RwInt32 ix0;
    RwInt32 ix1;
    RwRGBAReal color;
    RwInt32 ix;
    RwRGBA* pixel;
    RwReal frac;
    RwInt32 pos;
    RwReal span;

    span = (RwReal)(x1 - x0) * (1.0f / 65536.0f);
    ix0 = x0 >> 16;
    ix1 = x1 >> 16;
    pixel = (RwRGBA*)(image->cpPixels + (y >> 16) * image->stride + ix0 * 4);

    if (ix0 == ix1) {
        RwRGBARealFromRwRGBA(out, pixel);
        RwRGBARealScale(out, out, span);
    } else {
        pos = (RwInt32)(65536.0f * (RwReal)(ix0 + 1));
        frac = (RwReal)(pos - x0) * (1.0f / 65536.0f);
        RwRGBARealFromRwRGBA(out, pixel);
        RwRGBARealScale(out, out, frac);
        pixel++;

        for (ix = pos >> 16; ix != ix1; ix++) {
            RwRGBARealFromRwRGBA(&color, pixel);
            RwRGBARealAdd(out, out, &color);
            pos += 0x10000;
            pixel++;
        }

        frac = (RwReal)(x1 - pos) * (1.0f / 65536.0f);
        RwRGBARealFromRwRGBA(&color, pixel);
        RwRGBARealScale(&color, &color, frac);
        RwRGBARealAdd(out, out, &color);
    }

    RwRGBARealScale(out, out, 1.0f / span);
}

/* Average colour of the area between the 16.16 corners (x0, y0) and (x1, y1). */
static void ImageResampleGetAvgPixel(const RwImage* image, RwInt32 x0, RwInt32 x1, RwInt32 y0, RwInt32 y1,
                                     RwRGBAReal* out)
{
    RwInt32 iy;
    RwInt32 iy1;
    RwInt32 pos;
    RwReal frac;
    RwInt32 iy0;
    RwReal span;
    RwRGBAReal color;

    span = (RwReal)(y1 - y0) * (1.0f / 65536.0f);
    iy0 = y0 >> 16;
    iy1 = y1 >> 16;

    if (iy0 == iy1) {
        ImageResampleGetSpan(image, x0, x1, y0, out);
        RwRGBARealScale(out, out, span);
    } else {
        pos = (RwInt32)(65536.0f * (RwReal)(iy0 + 1));
        ImageResampleGetSpan(image, x0, x1, y0, out);
        frac = (RwReal)(pos - y0) * (1.0f / 65536.0f);
        RwRGBARealScale(out, out, frac);

        for (iy = pos >> 16; iy != iy1; iy++) {
            ImageResampleGetSpan(image, x0, x1, pos, &color);
            pos += 0x10000;
            RwRGBARealAdd(out, &color, out);
        }

        ImageResampleGetSpan(image, x0, x1, pos, &color);
        frac = (RwReal)(y1 - pos) * (1.0f / 65536.0f);
        RwRGBARealScale(&color, &color, frac);
        RwRGBARealAdd(out, out, &color);
    }

    span = 1.0f / span;
    RwRGBARealScale(out, out, span);
}

static inline RwImage* ImageResample(RwImage* dstImage, const RwImage* srcImage)
{
    RwInt32 xStep;
    RwInt32 j;
    RwInt32 height;
    RwInt32 width;
    RwInt32 yStep;
    RwInt32 y;

    width = dstImage->width;
    height = dstImage->height;
    xStep = (RwInt32)(65536.0f * ((RwReal)srcImage->width / (RwReal)width));
    yStep = (RwInt32)(65536.0f * ((RwReal)srcImage->height / (RwReal)height));
    y = 0;

    dstImage->flags |= srcImage->flags & rwIMAGEGAMMACORRECTED;

    for (j = 0; j < height; j++) {
        RwUInt8* dstPixel = dstImage->cpPixels + dstImage->stride * j;
        RwInt32 yEnd = y + (yStep - 1);
        RwInt32 x = 0;
        RwInt32 i;

        for (i = 0; i < width; i++) {
            RwRGBAReal color;

            ImageResampleGetAvgPixel(srcImage, x, x + (xStep - 1), y, yEnd, &color);
            x += xStep;
            dstPixel[0] = (RwUInt8)(RwInt32)(0.5f + 255.0f * color.red);
            dstPixel[1] = (RwUInt8)(RwInt32)(0.5f + 255.0f * color.green);
            dstPixel[2] = (RwUInt8)(RwInt32)(0.5f + 255.0f * color.blue);
            dstPixel[3] = (RwUInt8)(RwInt32)(0.5f + 255.0f * color.alpha);
            dstPixel += 4;
        }
        y += yStep;
    }

    return dstImage;
}

RwImage* RwImageResample(RwImage* dstImage, const RwImage* srcImage)
{
    return ImageResample(dstImage, srcImage);
}

RwImage* RwImageCreateResample(const RwImage* srcImage, RwInt32 width, RwInt32 height)
{
    RwImage* dstImage = RwImageCreate(width, height, 32);

    if (!dstImage) {
        return NULL;
    }
    if (!RwImageAllocatePixels(dstImage)) {
        RwImageDestroy(dstImage);
        return NULL;
    }

    if (srcImage->depth != 32) {
        RwImage* tmpImage = RwImageCreate(srcImage->width, srcImage->height, 32);

        if (!tmpImage) {
            RwImageFreePixels(dstImage);
            RwImageDestroy(dstImage);
            return NULL;
        }
        if (!RwImageAllocatePixels(tmpImage)) {
            RwImageDestroy(tmpImage);
            RwImageFreePixels(dstImage);
            RwImageDestroy(dstImage);
            return NULL;
        }
        RwImageCopy(tmpImage, srcImage);

        if (!ImageResample(dstImage, tmpImage)) {
            RwImageFreePixels(tmpImage);
            RwImageDestroy(tmpImage);
            RwImageFreePixels(dstImage);
            RwImageDestroy(dstImage);
            return NULL;
        }
        RwImageFreePixels(tmpImage);
        RwImageDestroy(tmpImage);
    } else if (!ImageResample(dstImage, srcImage)) {
        RwImageFreePixels(dstImage);
        RwImageDestroy(dstImage);
        return NULL;
    }

    return dstImage;
}
