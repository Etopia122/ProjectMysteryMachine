/*
 * GameCube device layer rasters: texture memory is kept in the GX tiled
 * layouts, so locking a mip level untiles it into a linear buffer and
 * unlocking tiles it back and flushes the data cache. Camera rasters have no
 * storage of their own; frame buffer copies go through GXCopyTex.
 */

#include "rwsdk/rwcore.h"

#define rwPLUGIN_ID 1

#define rwID_DLRASTERPLUGIN 0x40C

#define E_RW_DLNOTSUPPORTED 2
#define E_RW_INVRASTERDEPTH 0x8000000C
#define E_RW_INVRASTERFORMAT 0x8000000D
#define E_RW_INVRASTERLOCKREQ 0x8000000E
#define E_RW_INVRASTERUNLOCKREQ 0x80000011
#define E_RW_NOMEM 0x80000013

#define rwRASTERTYPENORMAL 0x00
#define rwRASTERTYPEZBUFFER 0x01
#define rwRASTERTYPECAMERA 0x02
#define rwRASTERTYPETEXTURE 0x04
#define rwRASTERTYPECAMERATEXTURE 0x05
#define rwRASTERTYPEMASK 0x07
#define rwRASTERDONTALLOCATE 0x80

#define rwRASTERFORMAT1555 0x0100
#define rwRASTERFORMAT565 0x0200
#define rwRASTERFORMAT4444 0x0300
#define rwRASTERFORMAT8888 0x0500
#define rwRASTERFORMAT888 0x0600
#define rwRASTERFORMAT16 0x0700
#define rwRASTERFORMAT32 0x0800
#define rwRASTERFORMAT555 0x0A00
#define rwRASTERFORMATAUTOMIPMAP 0x1000
#define rwRASTERFORMATPAL8 0x2000
#define rwRASTERFORMATPAL4 0x4000
#define rwRASTERFORMATMIPMAP 0x8000
#define rwRASTERFORMATPIXELFORMATMASK 0x0F00

#define rwRASTERLOCKWRITE 0x01
#define rwRASTERLOCKREAD 0x02
#define rwRASTERLOCKRAW 0x08

#define rwRASTERPIXELLOCKEDREAD 0x02
#define rwRASTERPIXELLOCKEDWRITE 0x04
#define rwRASTERPALETTELOCKEDREAD 0x08
#define rwRASTERPALETTELOCKEDWRITE 0x10
#define rwRASTERPIXELLOCKEDRAW 0x20

#define GX_TF_RGB565 4
#define GX_TF_RGB5A3 5
#define GX_TF_RGBA8 6
#define GX_TF_C4 8
#define GX_TF_C8 9
#define GX_TF_CMPR 14

#define GX_TL_RGB565 1
#define GX_TL_RGB5A3 2

typedef struct GXTlutObj {
    RwUInt32 dummy[3];
} GXTlutObj;

typedef struct GXRenderModeObj {
    /* 0x00 */ RwUInt8 pad00[0x19];
    /* 0x19 */ RwUInt8 aa;
    /* 0x1A */ RwUInt8 sample_pattern[12][2];
    /* 0x32 */ RwUInt8 vfilter[7];
} GXRenderModeObj;

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

#define RASTEREXTFROMRASTER(raster) ((RwDlRasterExt*)((RwUInt8*)(raster) + _rwDlRasterExtOffset))

#define ALIGN4(x) (((x) + 3) & ~3)
#define ALIGN8(x) (((x) + 7) & ~7)
#define ALIGN32(x) (((x) + 31) & ~31)

#define DLMIPLEVELS(size) ((RwUInt8)((RwUInt8)_rwDlFindMSB(size) + 1))

#define GCDRAWSYNCWAIT(_token)                                      \
    do {                                                            \
        if ((_token) == lbl_8025E800) {                             \
            GXSetDrawSync(lbl_8025E800);                            \
            lbl_8025E800 = (lbl_8025E800 + 1) % 0xE000;             \
        }                                                           \
        while (!_rwDlTokenDone(_token)) {                           \
        }                                                           \
    } while (0)

RwInt32 _rwDlRasterExtOffset;

extern volatile RwUInt16 lbl_8025E800;
extern RwUInt16 lbl_8025F890;
extern RwTexture* lbl_8025F87C;
extern RwInt32 lbl_8025F888;
extern RwBool lbl_8025F7F8;
extern GXRenderModeObj* lbl_8025F860;

void* memcpy(void* dst, const void* src, unsigned long n);
void DCFlushRange(void* addr, RwUInt32 nBytes);
void DCInvalidateRange(void* addr, RwUInt32 nBytes);
void GXSetCopyFilter(RwUInt8 aa, const RwUInt8 sample_pattern[12][2], RwUInt8 vf, const RwUInt8 vfilter[7]);
void GXSetTexCopySrc(RwUInt16 left, RwUInt16 top, RwUInt16 wd, RwUInt16 ht);
void GXSetTexCopyDst(RwUInt16 wd, RwUInt16 ht, RwInt32 fmt, RwUInt8 mipmap);
void GXCopyTex(void* dest, RwUInt8 clear);
void GXPixModeSync(void);
void GXInvalidateTexRegion(void* region);
void GXInvalidateTexAll(void);
void GXInitTlutObj(GXTlutObj* tlutObj, void* lut, RwInt32 fmt, RwUInt16 nEntries);
void GXSetDrawSync(RwUInt16 token);
RwBool _rwDlTokenDone(RwUInt16 token);
RwInt32 _rwDlFindMSB(RwInt32 num);
RwInt32 RwRasterRegisterPlugin(RwInt32 size, RwUInt32 pluginID, void* constructCB, void* destructCB, void* copyCB);
RwRaster* RwTextureRasterGenerateMipmaps(RwRaster* raster, RwImage* image);
RwBool _rwDlTextureSetRaster(RwTexture* texture, RwRaster* raster, RwInt32 flags);

void _rwDlRasterCopyEFB(RwRaster* raster)
{
    RwDlRasterExt* rasterExt = RASTEREXTFROMRASTER(raster);

    GXSetCopyFilter(0, NULL, 0, NULL);
    GXSetTexCopySrc(0, 0, raster->width, raster->height);
    GXSetTexCopyDst(raster->width, raster->height, rasterExt->format, 0);
    GXCopyTex(rasterExt->pixels, lbl_8025F7F8);
    GXPixModeSync();
    GXSetCopyFilter(lbl_8025F860->aa, lbl_8025F860->sample_pattern, 1, lbl_8025F860->vfilter);

    if (rasterExt->texRegion) {
        GXInvalidateTexRegion(rasterExt->texRegion);
    } else {
        GXInvalidateTexAll();
    }
}

void _rwDlRasterPluginAttach(void)
{
    _rwDlRasterExtOffset = RwRasterRegisterPlugin(sizeof(RwDlRasterExt), rwID_DLRASTERPLUGIN, NULL, NULL, NULL);
}

void DlRasterTile(RwUInt8* dst, RwUInt8* src, RwInt32 width, RwInt32 height, RwInt32 depth, RwInt32 stride)
{
    RwInt32 tilesWide;
    RwInt32 srcOffset;
    RwInt32 x;
    RwInt32 y;

    switch (depth) {
    case 4:
        tilesWide = ALIGN8(width) >> 3;
        srcOffset = 0;
        for (y = 0; y < height; y++) {
            RwInt32 rowTile = tilesWide * (y >> 3);
            RwUInt8* srcRow = src + srcOffset;
            RwInt32 xByte;
            RwInt32 tile;
            RwInt32 yOffset = (y & 7) << 3;

            tile = xByte = 0;
            for (x = 0; x < width; x += 8) {
                memcpy(dst + ((RwUInt32)(((rowTile + tile) << 6) + yOffset) >> 1), srcRow + xByte, 4);
                xByte += 4;
                tile++;
            }
            srcOffset += stride;
        }
        break;
    case 8:
        tilesWide = ALIGN8(width) >> 3;
        srcOffset = 0;
        for (y = 0; y < height; y++) {
            RwInt32 rowTile = tilesWide * (y >> 2);
            RwUInt8* srcRow = src + srcOffset;
            RwInt32 yOffset = (y & 3) << 3;
            RwInt32 tile = 0;

            for (x = 0; x < width; x += 8) {
                memcpy(dst + ((rowTile + tile) << 5) + yOffset, srcRow + x, 8);
                tile++;
            }
            srcOffset += stride;
        }
        break;
    case 16:
        tilesWide = ALIGN4(width) >> 2;
        srcOffset = 0;
        for (y = 0; y < height; y++) {
            RwInt32 rowTile = tilesWide * (y >> 2);
            RwInt32 yOffset = (y & 3) << 2;
            RwUInt8* srcPtr = src + srcOffset;
            RwInt32 tile = 0;

            for (x = 0; x < width; x += 4) {
                memcpy(dst + ((((rowTile + tile) << 4) + yOffset) << 1), srcPtr, 8);
                srcPtr += 8;
                tile++;
            }
            srcOffset += stride;
        }
        break;
    case 32:
        tilesWide = ALIGN4(width) >> 2;
        srcOffset = 0;
        for (y = 0; y < height; y++) {
            RwInt32 rowTile = tilesWide * (y >> 2);
            RwUInt8* srcRow = src + srcOffset;
            RwInt32 yOffset = (y & 3) << 2;
            RwInt32 tileOffset = 0;
            RwInt32 xByte = 0;

            for (x = 0; x < width; x++) {
                RwInt32 sub = x & 3;
                RwUInt8* texel;
                RwInt32 index;

                if (sub == 0) {
                    tileOffset = (rowTile + (x >> 2)) << 4;
                }
                texel = srcRow + xByte;
                index = (tileOffset << 2) + ((yOffset + sub) << 1);
                *(RwUInt16*)(dst + index) = ((RwUInt16*)texel)[0];
                *(RwUInt16*)(dst + index + 32) = ((RwUInt16*)texel)[1];
                xByte += 4;
            }
            srcOffset += stride;
        }
        break;
    case 24:
    default:
        RWERROR((E_RW_INVRASTERDEPTH));
        break;
    }
}

void DlRasterUntile(RwUInt8* dst, RwUInt8* src, RwInt32 width, RwInt32 height, RwInt32 depth, RwInt32 stride)
{
    RwInt32 tilesWide;
    RwInt32 dstOffset;
    RwInt32 x;
    RwInt32 y;

    switch (depth) {
    case 4:
        tilesWide = ALIGN8(width) >> 3;
        dstOffset = 0;
        for (y = 0; y < height; y++) {
            RwInt32 rowTile = tilesWide * (y >> 3);
            RwUInt8* dstRow = dst + dstOffset;
            RwInt32 xByte;
            RwInt32 tile;
            RwInt32 yOffset = (y & 7) << 3;

            xByte = tile = 0;
            for (x = 0; x < width; x += 8) {
                memcpy(dstRow + xByte, src + ((RwUInt32)(((rowTile + tile) << 6) + yOffset) >> 1), 4);
                tile++;
                xByte += 4;
            }
            dstOffset += stride;
        }
        break;
    case 8:
        tilesWide = ALIGN8(width) >> 3;
        dstOffset = 0;
        for (y = 0; y < height; y++) {
            RwInt32 rowTile = tilesWide * (y >> 2);
            RwUInt8* dstRow = dst + dstOffset;
            RwInt32 yOffset = (y & 3) << 3;
            RwInt32 tile = 0;

            for (x = 0; x < width; x += 8) {
                memcpy(dstRow + x, src + ((rowTile + tile) << 5) + yOffset, 8);
                tile++;
            }
            dstOffset += stride;
        }
        break;
    case 16:
        tilesWide = ALIGN4(width) >> 2;
        dstOffset = 0;
        for (y = 0; y < height; y++) {
            RwInt32 rowTile = tilesWide * (y >> 2);
            RwInt32 yOffset = (y & 3) << 2;
            RwUInt8* dstPtr = dst + dstOffset;
            RwInt32 tile = 0;

            for (x = 0; x < width; x += 4) {
                memcpy(dstPtr, src + ((((rowTile + tile) << 4) + yOffset) << 1), 8);
                tile++;
                dstPtr += 8;
            }
            dstOffset += stride;
        }
        break;
    case 32:
        tilesWide = ALIGN4(width) >> 2;
        dstOffset = 0;
        for (y = 0; y < height; y++) {
            RwInt32 rowTile = tilesWide * (y >> 2);
            RwUInt8* dstRow = dst + dstOffset;
            RwInt32 yOffset = (y & 3) << 2;
            RwInt32 tileOffset = 0;
            RwInt32 xByte = 0;

            for (x = 0; x < width; x++) {
                RwInt32 sub = x & 3;
                RwUInt8* texel;
                RwInt32 index;

                if (sub == 0) {
                    tileOffset = (rowTile + (x >> 2)) << 4;
                }
                index = (tileOffset << 2) + ((yOffset + sub) << 1);
                texel = dstRow + xByte;
                ((RwUInt16*)texel)[0] = *(RwUInt16*)(src + index);
                ((RwUInt16*)texel)[1] = *(RwUInt16*)(src + index + 32);
                xByte += 4;
            }
            dstOffset += stride;
        }
        break;
    case 24:
    default:
        RWERROR((E_RW_INVRASTERDEPTH));
        break;
    }
}

#pragma dont_inline on
RwUInt32 _rwDlRasterGetMipSize(RwRaster* raster, RwUInt8 level)
{
    RwUInt32 size = 0;
    RwInt32 width;
    RwRaster* parent = raster->parent;
    RwInt32 height;

    if (raster->privateFlags & (rwRASTERPIXELLOCKEDREAD | rwRASTERPIXELLOCKEDWRITE)) {
        width = parent->width;
        height = parent->height;
    } else {
        RwInt32 w = parent->width >> level;
        RwUInt32 h = parent->height >> level;

        width = 1;
        if (w) {
            width = w;
        }
        height = 1;
        if (h) {
            height = h;
        }
    }

    switch (raster->depth) {
    case 4:
        size = (RwUInt32)(ALIGN8(width) * ALIGN8(height)) >> 1;
        break;
    case 8:
        size = ALIGN8(width) * ALIGN4(height);
        break;
    case 16:
        size = (ALIGN4(width) * ALIGN4(height)) << 1;
        break;
    case 32:
        size = (ALIGN4(width) * ALIGN4(height)) << 2;
        break;
    case 24:
    default:
        RWERROR((E_RW_INVRASTERDEPTH));
        break;
    }

    return ALIGN32(size);
}

#pragma dont_inline reset

RwBool _rwDlRasterLock(void* pixelsOut, void* pRaster, RwInt32 flags)
{
    RwRaster* raster = (RwRaster*)pRaster;
    RwRaster* parent = raster->parent;
    RwDlRasterExt* rasterExt = RASTEREXTFROMRASTER(parent);
    RwUInt8 level = (flags >> 8) & 0xFF;
    RwUInt8* pixels;
    RwInt32 stride;
    RwUInt32 offset;

    switch (raster->cType & rwRASTERTYPEMASK) {
    case rwRASTERTYPENORMAL:
    case rwRASTERTYPETEXTURE:
    case rwRASTERTYPECAMERATEXTURE: {
        RwUInt8 mip = level;
        RwInt32 w = parent->width >> level;
        RwInt32 mipWidth = w ? w : 1;

        switch (raster->depth) {
        case 4:
            stride = ALIGN8(mipWidth) >> 1;
            break;
        case 8:
            stride = ALIGN8(mipWidth);
            break;
        case 16:
            stride = ALIGN4(mipWidth) << 1;
            break;
        case 32:
            stride = ALIGN4(mipWidth) << 2;
            break;
        case 24:
            RWERROR((E_RW_INVRASTERDEPTH));
            break;
        }

        offset = 0;
        while (mip--) {
            offset += _rwDlRasterGetMipSize(raster, mip);
        }
        pixels = rasterExt->pixels + offset;

        if (rasterExt->format != GX_TF_CMPR && !(flags & rwRASTERLOCKRAW)) {
            RwInt32 h = parent->height >> level;

            rasterExt->linear = (RwUInt8*)RwMalloc(stride * (h ? h : 1));
            if (rasterExt->linear == NULL) {
                return FALSE;
            }
        }

        if (parent == raster) {
            RwInt32 rw = raster->width;
            RwInt32 rh;
            RwInt32 newWidth = (rw >> level) ? (rw >> level) : 1;

            rh = raster->height >> level;
            raster->originalWidth = rw;
            raster->originalHeight = raster->height;
            raster->width = newWidth;
            raster->height = rh ? rh : 1;

            if (rasterExt->format != GX_TF_CMPR && !(flags & rwRASTERLOCKRAW)) {
                raster->cpPixels = rasterExt->linear;
            } else {
                raster->cpPixels = pixels;
            }
        } else if (rasterExt->format != GX_TF_CMPR && !(flags & rwRASTERLOCKRAW)) {
            switch (raster->depth) {
            case 4:
                raster->cpPixels = rasterExt->linear + (stride * raster->nOffsetY + (raster->nOffsetX >> 1));
                break;
            case 8:
                raster->cpPixels = rasterExt->linear + (stride * raster->nOffsetY + raster->nOffsetX);
                break;
            case 16:
                raster->cpPixels = rasterExt->linear + (stride * raster->nOffsetY + (raster->nOffsetX << 1));
                break;
            case 32:
                raster->cpPixels = rasterExt->linear + (stride * raster->nOffsetY + (raster->nOffsetX << 2));
                break;
            case 24:
            default:
                RWERROR((E_RW_INVRASTERDEPTH));
                break;
            }
        } else {
            RWERROR((E_RW_INVRASTERLOCKREQ));
            return FALSE;
        }

        rasterExt->lockedPixels = pixels;
        raster->stride = stride;
        rasterExt->lockedLevel = level;

        if (flags & rwRASTERLOCKREAD) {
            if ((raster->cType & rwRASTERTYPEMASK) == rwRASTERTYPECAMERATEXTURE) {
                DCFlushRange(rasterExt->lockedPixels, _rwDlRasterGetMipSize(raster, level));
            }
            raster->privateFlags |= rwRASTERPIXELLOCKEDREAD;
            parent->privateFlags |= rwRASTERPIXELLOCKEDREAD;

            if (rasterExt->format != GX_TF_CMPR && !(flags & rwRASTERLOCKRAW)) {
                DlRasterUntile(rasterExt->linear, rasterExt->lockedPixels, parent->width, parent->height, raster->depth,
                               raster->stride);
            }
        }

        if (flags & rwRASTERLOCKWRITE) {
            raster->privateFlags |= rwRASTERPIXELLOCKEDWRITE;
            parent->privateFlags |= rwRASTERPIXELLOCKEDWRITE;
            GCDRAWSYNCWAIT(rasterExt->token);
        }

        if (flags & rwRASTERLOCKRAW) {
            raster->privateFlags |= rwRASTERPIXELLOCKEDRAW;
            parent->privateFlags |= rwRASTERPIXELLOCKEDRAW;
        }
        break;
    }
    case 3:
    default:
        RWERROR((E_RW_INVRASTERLOCKREQ));
        return FALSE;
    }

    *(RwUInt8**)pixelsOut = raster->cpPixels;
    return TRUE;
}

RwBool _rwDlRasterUnlock(void* pOut, void* pRaster, RwInt32 flags)
{
    RwRaster* raster = (RwRaster*)pRaster;
    RwRaster* parent = raster->parent;
    RwDlRasterExt* rasterExt = RASTEREXTFROMRASTER(parent);

    switch (raster->cType & rwRASTERTYPEMASK) {
    case rwRASTERTYPENORMAL:
    case rwRASTERTYPETEXTURE:
    case rwRASTERTYPECAMERATEXTURE:
        if (raster->privateFlags & rwRASTERPIXELLOCKEDWRITE) {
            if (rasterExt->format != GX_TF_CMPR && !(raster->privateFlags & rwRASTERPIXELLOCKEDRAW)) {
                DlRasterTile(rasterExt->lockedPixels, rasterExt->linear, parent->width, parent->height, raster->depth,
                             raster->stride);
            }
            DCFlushRange(rasterExt->lockedPixels, _rwDlRasterGetMipSize(raster, rasterExt->lockedLevel));
            GXInvalidateTexAll();
        }

        if (parent == raster) {
            raster->width = raster->originalWidth;
            raster->height = raster->originalHeight;
        }

        if (rasterExt->format != GX_TF_CMPR && !(raster->privateFlags & rwRASTERPIXELLOCKEDRAW)) {
            RwFree(rasterExt->linear);
            rasterExt->linear = NULL;
        }

        raster->stride = 0;
        raster->cpPixels = NULL;

        if ((raster->privateFlags & rwRASTERPIXELLOCKEDWRITE) &&
            (raster->cFormat & (rwRASTERFORMATAUTOMIPMAP >> 8)) && rasterExt->lockedLevel == 0) {
            rasterExt->lockedLevel = 0xFF;
            raster->privateFlags &= ~(rwRASTERPIXELLOCKEDREAD | rwRASTERPIXELLOCKEDWRITE | rwRASTERPIXELLOCKEDRAW);
            parent->privateFlags &= ~(rwRASTERPIXELLOCKEDREAD | rwRASTERPIXELLOCKEDWRITE | rwRASTERPIXELLOCKEDRAW);
            RwTextureRasterGenerateMipmaps(raster, NULL);
        } else {
            rasterExt->lockedLevel = 0xFF;
            raster->privateFlags &= ~(rwRASTERPIXELLOCKEDREAD | rwRASTERPIXELLOCKEDWRITE | rwRASTERPIXELLOCKEDRAW);
            parent->privateFlags &= ~(rwRASTERPIXELLOCKEDREAD | rwRASTERPIXELLOCKEDWRITE | rwRASTERPIXELLOCKEDRAW);
        }
        break;
    case 3:
    default:
        RWERROR((E_RW_INVRASTERUNLOCKREQ));
        return FALSE;
    }
    return TRUE;
}

RwBool _rwDlRasterLockPalette(void* paletteOut, void* pRaster, RwInt32 flags)
{
    RwRaster* raster = (RwRaster*)pRaster;

    switch (raster->cType & rwRASTERTYPEMASK) {
    case rwRASTERTYPECAMERA:
        return FALSE;
    case rwRASTERTYPENORMAL:
    case rwRASTERTYPETEXTURE:
    case rwRASTERTYPECAMERATEXTURE:
        if (raster == raster->parent && raster->palette == NULL) {
            RwDlRasterExt* rasterExt = RASTEREXTFROMRASTER(raster);

            if (RwRasterGetFormat(raster) & (rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8)) {
                if (flags & rwRASTERLOCKREAD) {
                    raster->privateFlags |= rwRASTERPALETTELOCKEDREAD;
                }
                if (flags & rwRASTERLOCKWRITE) {
                    raster->privateFlags |= rwRASTERPALETTELOCKEDWRITE;
                }
                raster->palette = rasterExt->palette;
                *(RwUInt8**)paletteOut = raster->palette;
            } else {
                return FALSE;
            }
        }
        return TRUE;
    default:
        return FALSE;
    }
}

RwBool _rwDlRasterUnlockPalette(void* pOut, void* pRaster, RwInt32 flags)
{
    RwRaster* raster = (RwRaster*)pRaster;

    switch (raster->cType & rwRASTERTYPEMASK) {
    case rwRASTERTYPECAMERA:
        return FALSE;
    case rwRASTERTYPENORMAL:
    case rwRASTERTYPETEXTURE:
    case rwRASTERTYPECAMERATEXTURE:
        if (raster == raster->parent) {
            RwDlRasterExt* rasterExt = RASTEREXTFROMRASTER(raster);

            if (raster->privateFlags & rwRASTERPALETTELOCKEDWRITE) {
                DCFlushRange(rasterExt->palette, (1 << raster->depth) << 1);
            }
            raster->privateFlags &= ~(rwRASTERPALETTELOCKEDREAD | rwRASTERPALETTELOCKEDWRITE);
            raster->palette = NULL;
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

RwBool _rwDlRasterSetFormat(RwRaster* raster, RwInt32 flags)
{
    RwUInt32 format = flags & 0xFF00;

    raster->cType = flags & rwRASTERTYPEMASK;
    raster->cFlags = flags & 0xF8;

    switch (raster->cType) {
    case rwRASTERTYPENORMAL:
    case rwRASTERTYPETEXTURE:
        if (!(format & rwRASTERFORMATPIXELFORMATMASK)) {
            switch (raster->depth) {
            case 0:
                format |= rwRASTERFORMAT1555;
                if (format & (rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8)) {
                    raster->depth = (format & rwRASTERFORMATPAL4) ? 4 : 8;
                } else {
                    raster->depth = 16;
                }
                break;
            case 4:
                raster->depth = 4;
                format |= rwRASTERFORMATPAL4 | rwRASTERFORMAT1555;
                break;
            case 8:
                raster->depth = 8;
                format |= rwRASTERFORMATPAL8 | rwRASTERFORMAT1555;
                break;
            case 16:
                raster->depth = 16;
                format |= rwRASTERFORMAT1555;
                break;
            case 32:
                raster->depth = 32;
                format |= rwRASTERFORMAT8888;
                break;
            default:
                RWERROR((E_RW_INVRASTERDEPTH));
                break;
            }
        } else if (format & (rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8)) {
            raster->depth = (format & rwRASTERFORMATPAL4) ? 4 : 8;
        } else if ((format & rwRASTERFORMATPIXELFORMATMASK) == rwRASTERFORMAT888 ||
                   (format & rwRASTERFORMATPIXELFORMATMASK) == rwRASTERFORMAT8888) {
            raster->depth = 32;
        } else {
            raster->depth = 16;
        }
        break;
    case rwRASTERTYPECAMERATEXTURE:
        if (!(format & rwRASTERFORMATPIXELFORMATMASK)) {
            raster->depth = 16;
            format |= rwRASTERFORMAT565;
        } else if ((format & rwRASTERFORMATPIXELFORMATMASK) == rwRASTERFORMAT888 ||
                   (format & rwRASTERFORMATPIXELFORMATMASK) == rwRASTERFORMAT8888) {
            raster->depth = 32;
        } else {
            raster->depth = 16;
        }
        break;
    case rwRASTERTYPEZBUFFER:
    case rwRASTERTYPECAMERA:
        if (!(format & rwRASTERFORMATPIXELFORMATMASK)) {
            if (lbl_8025F860->aa) {
                raster->depth = 16;
                format = rwRASTERFORMAT565;
            } else {
                raster->depth = 32;
                format = rwRASTERFORMAT888;
            }
        } else if (!(format == rwRASTERFORMAT16 && lbl_8025F860->aa) ||
                   (format == rwRASTERFORMAT32 && !lbl_8025F860->aa)) {
            RWERROR((E_RW_INVRASTERFORMAT));
            return FALSE;
        }
        break;
    case 3:
    default:
        RWERROR((E_RW_INVRASTERFORMAT));
        return FALSE;
    }

    raster->cFormat = format >> 8;
    return TRUE;
}

RwBool _rwDlRasterGetMipLevels(RwUInt32* numLevels, RwRaster* raster)
{
    RwDlRasterExt* rasterExt = RASTEREXTFROMRASTER(raster);

    if (rasterExt->maxLevel != 0xFF) {
        *numLevels = rasterExt->maxLevel + 1;
    } else {
        RwUInt8 levels;

        if (RwRasterGetFormat(raster) & rwRASTERFORMATMIPMAP) {
            levels = (raster->width > raster->height) ? DLMIPLEVELS(raster->width) : DLMIPLEVELS(raster->height);
        } else {
            levels = 1;
        }
        *numLevels = levels;
    }
    return TRUE;
}

RwUInt32 _rwDlRasterGetSize(RwRaster* raster)
{
    RwUInt32 size = 0;
    RwInt32 numLevels;

    _rwDlRasterGetMipLevels((RwUInt32*)&numLevels, raster);
    while (numLevels--) {
        size += _rwDlRasterGetMipSize(raster, numLevels);
    }
    return size;
}

static inline RwBool DlRasterAllocate(RwRaster* raster, RwInt32 gxFormat, RwInt32 tlutFormat, RwInt32 hasAlpha,
                                      RwUInt8 depth, RwInt32 numLevels)
{
    RwDlRasterExt* rasterExt;
    RwUInt32 size;

    raster->depth = depth;
    rasterExt = RASTEREXTFROMRASTER(raster);
    if (hasAlpha == 1) {
        rasterExt->flags |= 1;
    }
    rasterExt->maxLevel = numLevels - 1;
    rasterExt->format = gxFormat;

    size = _rwDlRasterGetSize(raster);

    if (RwRasterGetFormat(raster) & (rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8)) {
        RwUInt32 allocSize;

        rasterExt->tlutFormat = tlutFormat;
        allocSize = ((1 << raster->depth) << 1) + 31;
        allocSize = size + allocSize;
        rasterExt->unaligned = RwMalloc(allocSize);
        if (rasterExt->unaligned == NULL) {
            RWERROR((E_RW_NOMEM, allocSize));
            return FALSE;
        }
        rasterExt->pixels = (RwUInt8*)ALIGN32((RwUInt32)rasterExt->unaligned);
        rasterExt->palette = rasterExt->pixels + size;
        GXInitTlutObj(&rasterExt->tlutObj, rasterExt->palette, rasterExt->tlutFormat, 1 << raster->depth);
    } else {
        rasterExt->unaligned = RwMalloc(size + 31);
        if (rasterExt->unaligned == NULL) {
            RWERROR((E_RW_NOMEM, size + 31));
            return FALSE;
        }
        rasterExt->pixels = (RwUInt8*)ALIGN32((RwUInt32)rasterExt->unaligned);
        if (raster->cType == rwRASTERTYPECAMERATEXTURE) {
            DCInvalidateRange(rasterExt->pixels, size);
        }
    }
    return TRUE;
}

RwBool _rwDlTextureRasterCreate(RwRaster* raster, RwInt32 gxFormat, RwInt32 tlutFormat, RwInt32 hasAlpha,
                                RwUInt8 depth, RwInt32 numLevels)
{
    return DlRasterAllocate(raster, gxFormat, tlutFormat, hasAlpha, depth, numLevels);
}

RwBool _rwDlRasterCreateTexture(RwRaster* raster)
{
    RwInt32 gxFormat = 0;
    RwInt32 tlutFormat = 0;
    RwInt32 hasAlpha = 0;
    RwInt32 format = RwRasterGetFormat(raster);
    RwInt32 pixelFormat = (raster->cFormat & (rwRASTERFORMATPIXELFORMATMASK >> 8)) << 8;
    RwUInt8 numLevels;

    if (format & (rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8)) {
        switch (pixelFormat) {
        case rwRASTERFORMAT555:
            tlutFormat = GX_TL_RGB5A3;
            hasAlpha = 0;
            break;
        case rwRASTERFORMAT1555:
        case rwRASTERFORMAT4444:
            tlutFormat = GX_TL_RGB5A3;
            hasAlpha = 1;
            break;
        case rwRASTERFORMAT565:
            tlutFormat = GX_TL_RGB565;
            hasAlpha = 0;
            break;
        default:
            RWERROR((E_RW_INVRASTERFORMAT));
            break;
        }
        gxFormat = (format & rwRASTERFORMATPAL4) ? GX_TF_C4 : GX_TF_C8;
    } else {
        switch (pixelFormat) {
        case rwRASTERFORMAT555:
            gxFormat = GX_TF_RGB5A3;
            hasAlpha = 0;
            break;
        case rwRASTERFORMAT1555:
        case rwRASTERFORMAT4444:
            gxFormat = GX_TF_RGB5A3;
            hasAlpha = 1;
            break;
        case rwRASTERFORMAT565:
            gxFormat = GX_TF_RGB565;
            hasAlpha = 0;
            break;
        case rwRASTERFORMAT888:
            gxFormat = GX_TF_RGBA8;
            hasAlpha = 0;
            break;
        case rwRASTERFORMAT8888:
            gxFormat = GX_TF_RGBA8;
            hasAlpha = 1;
            break;
        default:
            RWERROR((E_RW_INVRASTERFORMAT));
            break;
        }
    }

    if (RwRasterGetFormat(raster) & rwRASTERFORMATMIPMAP) {
        if (raster->width > raster->height) {
            numLevels = DLMIPLEVELS(raster->width);
        } else {
            numLevels = DLMIPLEVELS(raster->height);
        }
    } else {
        numLevels = 1;
    }

    DlRasterAllocate(raster, gxFormat, tlutFormat, hasAlpha, raster->depth, numLevels);
    return TRUE;
}

RwBool _rwDlRasterCreate(void* unused, RwRaster* raster, RwInt32 flags)
{
    RwDlRasterExt* rasterExt = RASTEREXTFROMRASTER(raster);

    raster->stride = 0;
    rasterExt->format = 0xFF;
    rasterExt->tlutFormat = 0xFF;
    rasterExt->flags = 0;
    rasterExt->unaligned = NULL;
    rasterExt->pixels = NULL;
    rasterExt->palette = NULL;
    rasterExt->lockedPixels = NULL;
    rasterExt->linear = NULL;
    rasterExt->texRegion = NULL;
    rasterExt->token = lbl_8025F890;
    rasterExt->maxLevel = 0xFF;
    rasterExt->lockedLevel = 0xFF;

    if (!_rwDlRasterSetFormat(raster, flags)) {
        return FALSE;
    }

    if (raster->width != 0 && raster->height != 0) {
        switch (raster->cType) {
        case rwRASTERTYPENORMAL:
        case rwRASTERTYPETEXTURE:
        case rwRASTERTYPECAMERATEXTURE:
            if (!(raster->cFlags & rwRASTERDONTALLOCATE)) {
                if (!_rwDlRasterCreateTexture(raster)) {
                    RWERROR((E_RW_DLNOTSUPPORTED, "Failed to create surface for texture"));
                    return FALSE;
                }
            }
            break;
        case rwRASTERTYPEZBUFFER:
        case rwRASTERTYPECAMERA:
            raster->cFlags = rwRASTERDONTALLOCATE;
            break;
        case 3:
        default:
            RWERROR((E_RW_INVRASTERFORMAT));
            return FALSE;
        }
    } else {
        raster->cFlags = rwRASTERDONTALLOCATE;
        return TRUE;
    }
    return TRUE;
}

RwBool _rwDlRasterDestroy(void* pOut, void* pRaster, RwInt32 flags)
{
    RwRaster* raster = (RwRaster*)pRaster;

    if (raster->parent == raster && !(raster->cFlags & rwRASTERDONTALLOCATE)) {
        switch (raster->cType) {
        case rwRASTERTYPENORMAL:
        case rwRASTERTYPETEXTURE:
        case rwRASTERTYPECAMERATEXTURE: {
            RwDlRasterExt* rasterExt = RASTEREXTFROMRASTER(raster);

            GCDRAWSYNCWAIT(rasterExt->token);
            if (raster == lbl_8025F87C->raster) {
                _rwDlTextureSetRaster(lbl_8025F87C, NULL, 0);
            }
            RwFree(rasterExt->unaligned);
            break;
        }
        case rwRASTERTYPEZBUFFER:
        case rwRASTERTYPECAMERA:
            break;
        case 3:
        default:
            RWERROR((E_RW_INVRASTERFORMAT));
            break;
        }
    }
    return TRUE;
}

RwBool _rwDlTextureSetRaster(RwTexture* texture, RwRaster* raster, RwInt32 flags)
{
    texture->raster = raster;
    *(RwUInt32*)((RwUInt8*)texture + lbl_8025F888 + 0x20) = 0x01000000;
    return TRUE;
}

RwBool _rwDlRasterSubRaster(RwRaster* subRaster, RwRaster* raster, RwInt32 flags)
{
    subRaster->stride = raster->stride;
    subRaster->depth = raster->depth;
    subRaster->cType = raster->cType;
    subRaster->cFormat = raster->cFormat;
    subRaster->cpPixels = NULL;
    return TRUE;
}
