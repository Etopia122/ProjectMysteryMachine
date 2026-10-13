// RenderWare rasters, textures, texture dictionaries and images.
// GameCube texture-native data is decoded to RGBA8 at load time.
#include "rwcore.h"
#include "rw_internal.h"
#include "pc_platform.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

// ---------------------------------------------------------------- GX texture decoding
static inline RwUInt16 be16(const RwUInt8* p) { return (RwUInt16)(p[0] << 8 | p[1]); }
static inline RwUInt32 be32(const RwUInt8* p) { return (RwUInt32)(p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3]); }

static void rgb565(RwUInt16 c, RwUInt8* o)
{
    o[0] = (RwUInt8)(((c >> 11) & 0x1F) * 255 / 31);
    o[1] = (RwUInt8)(((c >> 5) & 0x3F) * 255 / 63);
    o[2] = (RwUInt8)((c & 0x1F) * 255 / 31);
    o[3] = 255;
}

static void rgb5a3(RwUInt16 c, RwUInt8* o)
{
    if (c & 0x8000)
    {
        o[0] = (RwUInt8)(((c >> 10) & 0x1F) * 255 / 31);
        o[1] = (RwUInt8)(((c >> 5) & 0x1F) * 255 / 31);
        o[2] = (RwUInt8)((c & 0x1F) * 255 / 31);
        o[3] = 255;
    }
    else
    {
        o[0] = (RwUInt8)(((c >> 8) & 0xF) * 17);
        o[1] = (RwUInt8)(((c >> 4) & 0xF) * 17);
        o[2] = (RwUInt8)((c & 0xF) * 17);
        o[3] = (RwUInt8)(((c >> 12) & 0x7) * 255 / 7);
    }
}

static void ia8(RwUInt16 c, RwUInt8* o)
{
    RwUInt8 i = (RwUInt8)(c & 0xFF);
    o[0] = o[1] = o[2] = i;
    o[3] = (RwUInt8)(c >> 8);
}

static void tlut_color(const RwUInt8* pal, int tlutFmt, int idx, RwUInt8* o)
{
    RwUInt16 c = be16(pal + idx * 2);
    if (tlutFmt == 0)
    {
        ia8(c, o);
    }
    else if (tlutFmt == 1)
    {
        rgb565(c, o);
    }
    else
    {
        rgb5a3(c, o);
    }
}

enum { GX_TF_I4 = 0, GX_TF_I8 = 1, GX_TF_IA4 = 2, GX_TF_IA8 = 3, GX_TF_RGB565 = 4, GX_TF_RGB5A3 = 5,
       GX_TF_RGBA8 = 6, GX_TF_C4 = 8, GX_TF_C8 = 9, GX_TF_C14X2 = 10, GX_TF_CMPR = 14 };

// Size in bytes of one image level in a GX format.
int pc_gx_level_size(int fmt, int w, int h)
{
    int bw, bh, bb; // block width/height, bytes per block
    switch (fmt)
    {
    case GX_TF_I4: case GX_TF_C4: case GX_TF_CMPR: bw = 8; bh = 8; bb = 32; break;
    case GX_TF_I8: case GX_TF_IA4: case GX_TF_C8: bw = 8; bh = 4; bb = 32; break;
    case GX_TF_RGBA8: bw = 4; bh = 4; bb = 64; break;
    default: bw = 4; bh = 4; bb = 32; break;
    }
    return ((w + bw - 1) / bw) * ((h + bh - 1) / bh) * bb;
}

static void cmpr_block(const RwUInt8* src, RwUInt8* dst, int stride, int bx, int by, int w, int h)
{
    RwUInt16 c0 = be16(src), c1 = be16(src + 2);
    RwUInt8 pal[4][4];
    rgb565(c0, pal[0]);
    rgb565(c1, pal[1]);
    if (c0 > c1)
    {
        for (int k = 0; k < 3; k++)
        {
            pal[2][k] = (RwUInt8)((2 * pal[0][k] + pal[1][k]) / 3);
            pal[3][k] = (RwUInt8)((pal[0][k] + 2 * pal[1][k]) / 3);
        }
        pal[2][3] = pal[3][3] = 255;
    }
    else
    {
        for (int k = 0; k < 3; k++)
        {
            pal[2][k] = (RwUInt8)((pal[0][k] + pal[1][k]) / 2);
            pal[3][k] = 0;
        }
        pal[2][3] = 255;
        pal[3][3] = 0;
    }
    for (int y = 0; y < 4; y++)
    {
        RwUInt8 bits = src[4 + y];
        for (int x = 0; x < 4; x++)
        {
            int idx = (bits >> (6 - x * 2)) & 3;
            int px = bx + x, py = by + y;
            if (px < w && py < h)
            {
                memcpy(dst + py * stride + px * 4, pal[idx], 4);
            }
        }
    }
}

// Decodes one level to RGBA8 (dst: w*h*4 bytes).
void pc_gx_decode(int fmt, const RwUInt8* src, int w, int h, const RwUInt8* pal, int tlutFmt, RwUInt8* dst)
{
    int stride = w * 4;
    memset(dst, 0, (size_t)w * h * 4);
    switch (fmt)
    {
    case GX_TF_I4:
    case GX_TF_C4:
        for (int ty = 0; ty < h; ty += 8)
            for (int tx = 0; tx < w; tx += 8)
                for (int y = 0; y < 8; y++)
                    for (int x = 0; x < 8; x += 2, src++)
                        for (int k = 0; k < 2; k++)
                        {
                            int v = k == 0 ? *src >> 4 : *src & 0xF;
                            int px = tx + x + k, py = ty + y;
                            if (px >= w || py >= h) continue;
                            RwUInt8* o = dst + py * stride + px * 4;
                            if (fmt == GX_TF_I4) { o[0] = o[1] = o[2] = o[3] = (RwUInt8)(v * 17); }
                            else tlut_color(pal, tlutFmt, v, o);
                        }
        break;
    case GX_TF_I8:
    case GX_TF_IA4:
    case GX_TF_C8:
        for (int ty = 0; ty < h; ty += 4)
            for (int tx = 0; tx < w; tx += 8)
                for (int y = 0; y < 4; y++)
                    for (int x = 0; x < 8; x++, src++)
                    {
                        int px = tx + x, py = ty + y;
                        if (px >= w || py >= h) continue;
                        RwUInt8* o = dst + py * stride + px * 4;
                        if (fmt == GX_TF_I8) { o[0] = o[1] = o[2] = o[3] = *src; }
                        else if (fmt == GX_TF_IA4) { o[0] = o[1] = o[2] = (RwUInt8)((*src & 0xF) * 17); o[3] = (RwUInt8)((*src >> 4) * 17); }
                        else tlut_color(pal, tlutFmt, *src, o);
                    }
        break;
    case GX_TF_IA8:
    case GX_TF_RGB565:
    case GX_TF_RGB5A3:
    case GX_TF_C14X2:
        for (int ty = 0; ty < h; ty += 4)
            for (int tx = 0; tx < w; tx += 4)
                for (int y = 0; y < 4; y++)
                    for (int x = 0; x < 4; x++, src += 2)
                    {
                        int px = tx + x, py = ty + y;
                        if (px >= w || py >= h) continue;
                        RwUInt8* o = dst + py * stride + px * 4;
                        RwUInt16 c = be16(src);
                        if (fmt == GX_TF_IA8) ia8(c, o);
                        else if (fmt == GX_TF_RGB565) rgb565(c, o);
                        else if (fmt == GX_TF_RGB5A3) rgb5a3(c, o);
                        else tlut_color(pal, tlutFmt, c & 0x3FFF, o);
                    }
        break;
    case GX_TF_RGBA8:
        for (int ty = 0; ty < h; ty += 4)
            for (int tx = 0; tx < w; tx += 4, src += 64)
                for (int i = 0; i < 16; i++)
                {
                    int px = tx + (i & 3), py = ty + (i >> 2);
                    if (px >= w || py >= h) continue;
                    RwUInt8* o = dst + py * stride + px * 4;
                    o[3] = src[i * 2];
                    o[0] = src[i * 2 + 1];
                    o[1] = src[32 + i * 2];
                    o[2] = src[32 + i * 2 + 1];
                }
        break;
    case GX_TF_CMPR:
        for (int ty = 0; ty < h; ty += 8)
            for (int tx = 0; tx < w; tx += 8)
                for (int s = 0; s < 4; s++, src += 8)
                    cmpr_block(src, dst, stride, tx + (s & 1) * 4, ty + (s >> 1) * 4, w, h);
        break;
    default:
        pc_log("[rw] unsupported GX texture format %d", fmt);
        memset(dst, 0xFF, (size_t)w * h * 4);
        break;
    }
}

// ---------------------------------------------------------------- rasters
PcRaster* pc_raster(RwRaster* r) { return (PcRaster*)r; }

// Offset of the platform raster extension, read by the game (_RwGameCubeRasterExtOffset).
extern "C" RwInt32 lbl_8025F870;
RwInt32 lbl_8025F870 = offsetof(PcRaster, ext);

extern "C" {

RwRaster* RwRasterCreate(RwInt32 width, RwInt32 height, RwInt32 depth, RwInt32 flags)
{
    PcRaster* p = (PcRaster*)rwCalloc(1, sizeof(PcRaster));
    RwRaster* r = &p->r;
    r->width = width;
    r->height = height;
    r->depth = depth ? depth : 32;
    r->cType = (RwUInt8)(flags & rwRASTERTYPEMASK);
    r->cFlags = (RwUInt8)(flags & 0xF8);
    r->cFormat = (RwUInt8)((flags >> 8) & 0xFF);
    r->stride = width * 4;
    r->originalWidth = width;
    r->originalHeight = height;
    r->originalStride = r->stride;
    int type = flags & rwRASTERTYPEMASK;
    if (type == rwRASTERTYPETEXTURE || type == rwRASTERTYPECAMERATEXTURE || type == rwRASTERTYPENORMAL)
    {
        if (!(flags & rwRASTERDONTALLOCATE) && width > 0 && height > 0)
        {
            p->rgba = (RwUInt8*)rwCalloc((size_t)width * height, 4);
            r->cpPixels = p->rgba;
            r->originalPixels = p->rgba;
        }
    }
    return r;
}

RwInt32 RwRasterDestroy(RwRaster* r)
{
    if (!r)
    {
        return 1;
    }
    PcRaster* p = pc_raster(r);
    pc_render_raster_free(p);
    rwFree(p->rgba);
    rwFree(p);
    return 1;
}

RwUInt8* RwRasterLock(RwRaster* r, RwUInt8 level, RwInt32 lockMode) { return pc_raster(r)->rgba; }
RwRaster* RwRasterUnlock(RwRaster* r)
{
    pc_raster(r)->dirty = 1;
    return r;
}

// ---------------------------------------------------------------- textures
static RwTexture* (*s_readCallBack)(const char*, const char*);

RwTexture* RwTextureCreate(RwRaster* raster)
{
    RwTexture* t = (RwTexture*)rwCalloc(1, sizeof(RwTexture) + 0x20);
    t->raster = raster;
    t->refCount = 1;
    t->filterAddressing = 0x1102; // linear, wrap/wrap
    return t;
}

RwTexture* RwTexDictionaryRemoveTexture(RwTexture* t);

RwInt32 RwTextureDestroy(RwTexture* t)
{
    if (!t)
    {
        return 1;
    }
    if (--t->refCount > 0)
    {
        return 1;
    }
    if (t->dict)
    {
        RwTexDictionaryRemoveTexture(t);
    }
    RwRasterDestroy(t->raster);
    rwFree(t);
    return 1;
}

RwInt32 RwTextureSetReadCallBack(RwTexture* (*cb)(const char*, const char*))
{
    s_readCallBack = cb;
    return 1;
}

RwTexture* RwTextureRead(const char* name, const char* mask)
{
    if (s_readCallBack)
    {
        RwTexture* t = s_readCallBack(name, mask);
        static const char* logTex = getenv("PC_LOG_TEXREAD");
        if (logTex)
        {
            pc_log("[texread] %s -> %p raster %p", name, t, t ? t->raster : NULL);
        }
        if (t)
        {
            // The caller owns a reference (materials release theirs when destroyed);
            // the game's callback returns its asset's texture without adding one.
            t->refCount++;
            return t;
        }
    }
    return NULL;
}

// ---------------------------------------------------------------- dictionaries
RwTexDictionary* RwTexDictionaryCreate()
{
    RwTexDictionary* d = (RwTexDictionary*)rwCalloc(1, sizeof(RwTexDictionary) + 0x10);
    d->object.type = rwTEXDICTIONARY;
    rwLinkListInitialize(&d->texturesInDict);
    return d;
}

RwTexture* RwTexDictionaryAddTexture(RwTexDictionary* d, RwTexture* t)
{
    if (t->dict)
    {
        RwTexDictionaryRemoveTexture(t);
    }
    t->dict = d;
    rwLinkListAddLLLink(&d->texturesInDict, &t->lInDictionary);
    return t;
}

RwTexture* RwTexDictionaryRemoveTexture(RwTexture* t)
{
    if (t->dict)
    {
        rwLinkListRemoveLLLink(&t->lInDictionary);
        t->dict = NULL;
    }
    return t;
}

RwTexDictionary* RwTexDictionaryForAllTextures(RwTexDictionary* d, RwTexture* (*cb)(RwTexture*, void*), void* data)
{
    RwLLLink* end = &d->texturesInDict.link;
    for (RwLLLink* l = end->next; l != end;)
    {
        RwLLLink* n = l->next;
        if (!cb(rwLLLinkGetData(l, RwTexture, lInDictionary), data))
        {
            break;
        }
        l = n;
    }
    return d;
}

RwTexture* RwTexDictionaryFindNamedTexture(RwTexDictionary* d, const char* name)
{
    RwLLLink* end = &d->texturesInDict.link;
    for (RwLLLink* l = end->next; l != end; l = l->next)
    {
        RwTexture* t = rwLLLinkGetData(l, RwTexture, lInDictionary);
        if (strcasecmp(t->name, name) == 0)
        {
            return t;
        }
    }
    return NULL;
}

RwInt32 RwTexDictionaryDestroy(RwTexDictionary* d)
{
    RwLLLink* end = &d->texturesInDict.link;
    while (end->next != end)
    {
        RwTexture* t = rwLLLinkGetData(end->next, RwTexture, lInDictionary);
        RwTexDictionaryRemoveTexture(t);
        RwTextureDestroy(t);
    }
    rwFree(d);
    return 1;
}

// Reads one GameCube texture-native chunk body (after its Struct header).
static RwTexture* read_texture_native(RwStream* s, RwUInt32 structLen)
{
    RwUInt8* buf = (RwUInt8*)rwMalloc(structLen);
    if (RwStreamRead(s, buf, structLen) != structLen)
    {
        rwFree(buf);
        return NULL;
    }
    RwUInt32 platform = be32(buf);
    if (platform != 6)
    {
        pc_log("[rw] texture native for platform %u not supported", platform);
        rwFree(buf);
        return NULL;
    }
    RwUInt32 filterAddr = be32(buf + 4);
    const RwUInt8* h = buf + 0x18;
    char name[33], mask[33];
    memcpy(name, h, 32);
    name[32] = 0;
    memcpy(mask, h + 32, 32);
    mask[32] = 0;
    const RwUInt8* q = buf + 0x58;
    RwUInt32 rasterFormat = be32(q);
    int width = be16(q + 4), height = be16(q + 6);
    int depth = q[8];
    int numLevels = q[9];
    int gxFmt = q[10];
    int tlutFmt = q[11];
    // q + 12: has-alpha flag
    const RwUInt8* p = q + 16;
    const RwUInt8* pal = NULL;
    if (gxFmt == GX_TF_C8 || gxFmt == GX_TF_C4 || gxFmt == GX_TF_C14X2)
    {
        pal = p;
        p += (gxFmt == GX_TF_C4 ? 16 : 256) * 2;
    }
    RwUInt32 dataSize = be32(p);
    p += 4;
    if (p + pc_gx_level_size(gxFmt, width, height) > buf + structLen)
    {
        pc_log("[rw] texture '%s' data truncated (%dx%d fmt %d)", name, width, height, gxFmt);
        rwFree(buf);
        return NULL;
    }

    RwRaster* r = RwRasterCreate(width, height, depth, rwRASTERTYPETEXTURE | (int)(rasterFormat & 0xFF00));
    // GameCube raster extension: the game checks the GX format at offset 0xC (iSystem TextureRead).
    RwUInt32 fmt32 = (RwUInt32)gxFmt;
    memcpy(pc_raster(r)->ext + 0xC, &fmt32, 4);
    pc_gx_decode(gxFmt, p, width, height, pal, tlutFmt, pc_raster(r)->rgba);
    pc_texture_patch(name, pc_raster(r)->rgba, width, height);
    {
        // The platform's own picture in its place, at its own size.
        int rw = 0, rh = 0, fullscreen = 0;
        if (unsigned char* rep = pc_texture_replace(name, &rw, &rh, &fullscreen))
        {
            PcRaster* pr = pc_raster(r);
            RwUInt8* px = (RwUInt8*)rwCalloc((size_t)rw * rh, 4);
            if (px)
            {
                memcpy(px, rep, (size_t)rw * rh * 4);
                rwFree(pr->rgba);
                pr->rgba = px;
                r->width = rw;
                r->height = rh;
                pr->fullscreen = fullscreen;
            }
            free(rep);
        }
    }
    pc_raster(r)->dirty = 1;
    pc_raster(r)->numLevels = numLevels;
    if (getenv("PC_DUMP_TEX"))
    {
        // PC_DUMP_TEX=<dir>: <dir>/<n>_<name>.rgba (pc/tools/rgba2png.py converts them).
        static int n;
        char fn[96];
        char safe[32];
        int k = 0;
        for (; name[k] && k < 31; k++)
        {
            safe[k] = isalnum((unsigned char)name[k]) || name[k] == '_' ? name[k] : '-';
        }
        safe[k] = 0;
        sprintf(fn, "%s/%03d_%s.rgba", getenv("PC_DUMP_TEX"), n++, safe);
        FILE* f = fopen(fn, "wb");
        if (f)
        {
            fwrite(&width, 4, 1, f);
            fwrite(&height, 4, 1, f);
            fwrite(pc_raster(r)->rgba, 4, (size_t)width * height, f);
            fclose(f);
        }
    }
    (void)depth;
    (void)dataSize;

    RwTexture* t = RwTextureCreate(r);
    strncpy(t->name, name, 31);
    strncpy(t->mask, mask, 31);
    t->filterAddressing = filterAddr & 0xFFFF;
    rwFree(buf);
    return t;
}

RwTexDictionary* RwTexDictionaryStreamRead(RwStream* s)
{
    RwUInt32 len, ver;
    if (!RwStreamFindChunk(s, rwID_STRUCT, &len, &ver))
    {
        return NULL;
    }
    RwUInt8 hdr[4];
    RwStreamRead(s, hdr, len < 4 ? len : 4);
    if (len > 4)
    {
        RwStreamSkip(s, len - 4);
    }
    int count = hdr[0] | hdr[1] << 8; // little-endian like all RW chunk structs
    RwTexDictionary* d = RwTexDictionaryCreate();
    for (int i = 0; i < count; i++)
    {
        RwUInt32 tlen;
        if (!RwStreamFindChunk(s, rwID_TEXTURENATIVE, &tlen, &ver))
        {
            break;
        }
        RwUInt32 slen;
        if (!RwStreamFindChunk(s, rwID_STRUCT, &slen, &ver))
        {
            break;
        }
        RwTexture* t = read_texture_native(s, slen);
        // Skip the texture's extension chunk.
        RwUInt32 elen;
        if (RwStreamFindChunk(s, rwID_EXTENSION, &elen, &ver))
        {
            RwStreamSkip(s, elen);
        }
        if (t)
        {
            RwTexDictionaryAddTexture(d, t);
        }
        if (getenv("PC_LOG_TEX"))
        {
            pc_log("[rw] texture %dx%d '%s' %s", t ? t->raster->width : 0, t ? t->raster->height : 0,
                   t ? t->name : "", t ? "ok" : "FAILED");
        }
    }
    RwUInt32 elen;
    if (RwStreamFindChunk(s, rwID_EXTENSION, &elen, &ver))
    {
        RwStreamSkip(s, elen);
    }
    return d;
}

// ---------------------------------------------------------------- images
RwImage* RwImageCreate(RwInt32 width, RwInt32 height, RwInt32 depth)
{
    RwImage* im = (RwImage*)rwCalloc(1, sizeof(RwImage) + 0x10);
    im->width = width;
    im->height = height;
    im->depth = depth;
    return im;
}

RwImage* RwImageAllocatePixels(RwImage* im)
{
    int bpp = im->depth <= 8 ? 1 : 4;
    im->stride = im->width * bpp;
    im->cpPixels = (RwUInt8*)rwCalloc((size_t)im->stride * im->height, 1);
    if (im->depth <= 8)
    {
        im->palette = (RwRGBA*)rwCalloc(256, sizeof(RwRGBA));
    }
    im->flags |= 1;
    return im;
}

RwInt32 RwImageDestroy(RwImage* im)
{
    if (im->flags & 1)
    {
        rwFree(im->cpPixels);
        rwFree(im->palette);
    }
    rwFree(im);
    return 1;
}

// The platform's changes to a raster the game found as an asset (its
// texture dictionary may not name it): see pc_texture_patch.
void pc_raster_patch(RwRaster* r, const char* name)
{
    PcRaster* p = r ? pc_raster(r) : NULL;
    if (p && p->rgba)
    {
        pc_texture_patch(name, p->rgba, r->width, r->height);
        p->dirty = 1;
    }
}

RwImage* RwImageSetFromRaster(RwImage* im, RwRaster* r)
{
    PcRaster* p = pc_raster(r);
    if (!im->cpPixels || !p->rgba)
    {
        return im;
    }
    for (int y = 0; y < im->height && y < r->height; y++)
    {
        memcpy(im->cpPixels + y * im->stride, p->rgba + y * r->width * 4,
               (size_t)(im->width < r->width ? im->width : r->width) * 4);
    }
    return im;
}

// GameCube texture-cache and alpha-state flush: drops any alpha compare the
// game set through GX directly.
void _rwDlTextureRasterFlush(RwInt32 mode, volatile void* data)
{
    pc_render_reset_gx_alpha();
}

}
