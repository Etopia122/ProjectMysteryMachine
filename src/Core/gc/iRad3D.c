// Bink's RAD 3D image helpers for GameCube (rad3d), compiled as their own unit.

#ifndef NULL
#define NULL 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GXTexObj
{
    unsigned int dummy[8];
} GXTexObj;

typedef struct RAD3DIMAGE
{
    unsigned int width;     // 0x0
    unsigned int height;    // 0x4
    unsigned int alpha;     // 0x8
    unsigned int bpp;       // 0xC
    unsigned int type;      // 0x10
    void* pixels;           // 0x14
    unsigned int size;      // 0x18
    GXTexObj tex;           // 0x1C
} RAD3DIMAGE;

typedef RAD3DIMAGE* HRAD3DIMAGE;

typedef union
{
    unsigned char u8;
    signed char s8;
    unsigned short u16;
    signed short s16;
    unsigned int u32;
    signed int s32;
    float f32;
} PPCWGPipe;

#define GXWGFifo (*(volatile PPCWGPipe*)0xCC008000)

int OSReport(const char* fmt, ...);
void DCStoreRange(void* addr, unsigned long nBytes);
unsigned int GXGetTexBufferSize(unsigned short width, unsigned short height, unsigned int format,
                                unsigned char mipmap, unsigned char max_lod);
void GXInitTexObj(GXTexObj* obj, void* image_ptr, unsigned short width, unsigned short height,
                  int format, int wrap_s, int wrap_t, unsigned char mipmap);
void GXInitTexObjLOD(GXTexObj* obj, int min_filt, int mag_filt, float min_lod, float max_lod,
                     float lod_bias, unsigned char bias_clamp, unsigned char do_edge_lod,
                     int max_aniso);
void GXLoadTexObj(GXTexObj* obj, int id);
void GXSetCullMode(int mode);
void GXBegin(int type, int vtxfmt, unsigned short nverts);
void GXSetTevAlphaIn(int stage, int a, int b, int c, int d);
// TODO: unidentified GX function (blend mode)
void GXSetBlendMode(int type, int src_factor, int dst_factor, int op);

extern unsigned char* sFMVWorkArea;

static unsigned int sGXFormats[5];
static unsigned int sTypeInfo[5];
static int sInited;

// Functions are in reverse DOL order (-inline deferred emits last to first).

HRAD3DIMAGE Open_RAD_3D_image(void* window, unsigned int width, unsigned int height,
                              unsigned int rad3d_surface_format)
{
    HRAD3DIMAGE image;
    unsigned char bpp;

    if (!sInited)
    {
        sGXFormats[0] = 6;
        sGXFormats[1] = 6;
        sGXFormats[2] = 4;
        sGXFormats[3] = 5;
        sGXFormats[4] = -1;
        sTypeInfo[0] = 4;
        sTypeInfo[1] = 0x80000004;
        sTypeInfo[2] = 2;
        sTypeInfo[3] = 0x80000002;
        sTypeInfo[4] = 2;
        sInited = 1;
    }

    image = (HRAD3DIMAGE)sFMVWorkArea;
    sFMVWorkArea += sizeof(RAD3DIMAGE);
    bpp = sTypeInfo[rad3d_surface_format];
    OSReport("### Using %d bytes from FMV work buffer\n", sizeof(RAD3DIMAGE));
    if (image == NULL)
    {
        return NULL;
    }

    image->width = width;
    image->height = height;
    image->alpha = sTypeInfo[rad3d_surface_format] >> 31;
    image->bpp = bpp;
    image->type = rad3d_surface_format;
    image->size = GXGetTexBufferSize(width, height, sGXFormats[rad3d_surface_format], 0, 0);
    sFMVWorkArea = (unsigned char*)(((unsigned int)sFMVWorkArea + 0x1F) & ~0x1F);
    image->pixels = sFMVWorkArea;
    sFMVWorkArea += image->size;
    OSReport("### Using %d bytes from FMV work buffer, %08X - %08X\n", image->size,
             sFMVWorkArea - image->size, sFMVWorkArea - 1);
    GXInitTexObj(&image->tex, image->pixels, width, height, sGXFormats[rad3d_surface_format], 0,
                 0, 0);
    GXInitTexObjLOD(&image->tex, 1, 0, 0.0f, 0.0f, 0.0f, 0, 0, 0);
    return image;
}

void Close_RAD_3D_image(HRAD3DIMAGE image)
{
}

int Lock_RAD_3D_image(HRAD3DIMAGE image, void** pixels, unsigned int* pixel_pitch,
                      unsigned int* type)
{
    if (image == NULL)
    {
        return 0;
    }
    if (pixels != NULL)
    {
        *pixels = image->pixels;
    }
    if (pixel_pitch != NULL)
    {
        *pixel_pitch = image->width * image->bpp;
    }
    if (type != NULL)
    {
        *type = image->type;
    }
    return 1;
}

void Unlock_RAD_3D_image(HRAD3DIMAGE image)
{
    if (image != NULL)
    {
        DCStoreRange(image->pixels, image->size);
    }
}

void Blit_RAD_3D_image(HRAD3DIMAGE image, float x_offset, float y_offset, float x_scale,
                       float y_scale, float alpha_level)
{
    signed short x0;
    signed short y0;
    signed short x1;
    signed short y1;
    unsigned char a;

    if (image == NULL)
    {
        return;
    }

    if (alpha_level >= 0.998f)
    {
        if (image->alpha == 0)
        {
            GXSetBlendMode(0, 4, 5, 0);
        }
        else
        {
            GXSetBlendMode(1, 4, 5, 0);
            GXSetTevAlphaIn(0, 7, 4, 6, 7);
        }
    }
    else
    {
        GXSetBlendMode(1, 4, 5, 0);
        if (image->alpha == 0)
        {
            GXSetTevAlphaIn(0, 7, 5, 6, 7);
        }
        else
        {
            GXSetTevAlphaIn(0, 7, 4, 5, 7);
        }
    }

    GXLoadTexObj(&image->tex, 0);
    x1 = image->width * x_scale + x_offset;
    y1 = image->height * y_scale + y_offset;
    GXSetCullMode(0);
    GXBegin(0x80, 0, 4);
    x0 = x_offset;
    y0 = y_offset;
    a = 255.0f * alpha_level;

    GXWGFifo.s16 = x0;
    GXWGFifo.s16 = y1;
    GXWGFifo.s16 = 0;
    GXWGFifo.u8 = 255;
    GXWGFifo.u8 = 255;
    GXWGFifo.u8 = 255;
    GXWGFifo.u8 = a;
    GXWGFifo.f32 = 0.0f;
    GXWGFifo.f32 = 1.0f;

    GXWGFifo.s16 = x0;
    GXWGFifo.s16 = y0;
    GXWGFifo.s16 = 0;
    GXWGFifo.u8 = 255;
    GXWGFifo.u8 = 255;
    GXWGFifo.u8 = 255;
    GXWGFifo.u8 = a;
    GXWGFifo.f32 = 0.0f;
    GXWGFifo.f32 = 0.0f;

    GXWGFifo.s16 = x1;
    GXWGFifo.s16 = y0;
    GXWGFifo.s16 = 0;
    GXWGFifo.u8 = 255;
    GXWGFifo.u8 = 255;
    GXWGFifo.u8 = 255;
    GXWGFifo.u8 = a;
    GXWGFifo.f32 = 1.0f;
    GXWGFifo.f32 = 0.0f;

    GXWGFifo.s16 = x1;
    GXWGFifo.s16 = y1;
    GXWGFifo.s16 = 0;
    GXWGFifo.u8 = 255;
    GXWGFifo.u8 = 255;
    GXWGFifo.u8 = 255;
    GXWGFifo.u8 = a;
    GXWGFifo.f32 = 1.0f;
    GXWGFifo.f32 = 1.0f;

    GXSetBlendMode(0, 4, 5, 0);
}

#ifdef __cplusplus
}
#endif
