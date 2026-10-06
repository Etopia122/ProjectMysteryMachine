#include "zGlobals.h"
#include "xSnd.h"
#include "rwtypes.h"

#ifndef NULL
#define NULL 0
#endif

class RwCamera;
class RwRaster;
class RwResEntry
{
public:
    RwLLLink link; // 0x0
};

class BINK
{
public:
    unsigned int Width;        // 0x0
    unsigned int Height;       // 0x4
    unsigned int Frames;       // 0x8
    unsigned int FrameNum;     // 0xC
    unsigned int LastFrameNum; // 0x10
    unsigned int FrameRate;    // 0x14
    unsigned int FrameRateDiv; // 0x18
    unsigned char pad1C[0xF0 - 0x1C];
    signed int NumTracks;      // 0xF0
};
typedef BINK* HBINK;

class RAD3DIMAGE;
typedef RAD3DIMAGE* HRAD3DIMAGE;

class DVDCommandBlock
{
public:
    unsigned char pad0[0x30];
};

class DVDFileInfo
{
public:
    DVDCommandBlock cb;     // 0x0
    unsigned int startAddr; // 0x30
    unsigned int length;    // 0x34
    void* callback;         // 0x38
};

class tag_xFile
{
public:
    char relname[16];     // 0x0
    unsigned int flags;   // 0x10
    char path[128];       // 0x14
    signed int entrynum;  // 0x94
    DVDFileInfo fileInfo; // 0x98
    unsigned char padD4[0x2C];
};

class iTRCDisk
{
public:
    static signed int CheckDVDAndResetState();
};

class RwLinkList
{
public:
    RwLLLink link; // 0x0
};

class rwResourcesGlobals
{
public:
    unsigned char pad0[0x24];
    RwLinkList* usedEntries; // 0x24
};

#define RWRESOURCESGLOBAL(var) \
    (((rwResourcesGlobals*)(RwEngineInstance + lbl_8025F798.globalsOffset))->var)
#define rwLinkListRemoveLLLink(linkvar) \
    { \
        (linkvar)->prev->next = (linkvar)->next; \
        (linkvar)->next->prev = (linkvar)->prev; \
    }
#define rwLinkListAddLLLink(list, linkvar) \
    { \
        (linkvar)->next = (list)->link.next; \
        (linkvar)->prev = &(list)->link; \
        ((list)->link.next)->prev = (linkvar); \
        (list)->link.next = (linkvar); \
    }

class RwModuleInfo
{
public:
    signed int globalsOffset; // 0x0
    signed int numInstances;  // 0x4
};

class _tagxPad;

extern "C" {
void Blit_RAD_3D_image(HRAD3DIMAGE image, float x_offset, float y_offset, float x_scale,
                       float y_scale, float alpha_level);
void Unlock_RAD_3D_image(HRAD3DIMAGE image);
signed int Lock_RAD_3D_image(HRAD3DIMAGE image, void** pixels, unsigned int* pixel_pitch,
                             unsigned int* type);
void Close_RAD_3D_image(HRAD3DIMAGE image);
HRAD3DIMAGE Open_RAD_3D_image(void* window, unsigned int width, unsigned int height,
                              unsigned int rad3d_surface_format);
signed int OSReport(const char* fmt, ...);
signed int strncmp(const char* a, const char* b, unsigned long n);
unsigned long ARAlloc(unsigned long length);
unsigned long ARFree(unsigned long* length);
HBINK BinkOpen(const char* name, unsigned int flags);
void BinkClose(HBINK bnk);
signed int BinkGetError();
signed int BinkWait(HBINK bnk);
signed int BinkDoFrame(HBINK bnk);
void BinkNextFrame(HBINK bnk);
void BinkGoto(HBINK bnk, unsigned int frame, signed int flags);
void BinkSetVolume(HBINK bnk, unsigned int trackid, signed int volume);
signed int BinkCopyToBuffer(HBINK bnk, void* dest, signed int destpitch, unsigned int destheight,
                            unsigned int destx, unsigned int desty, unsigned int flags);
void RADSetAudioMemory(void* (*malloc)(unsigned long), void (*free)(void*));
void RADSetMemory(void* (*malloc)(unsigned long), void (*free)(void*));
signed int DVDSeekAsyncPrio(DVDFileInfo* fileInfo, signed int offset, void* callback,
                            signed int prio);
signed int DVDCancelAsync(DVDCommandBlock* block, void* callback);
RwCamera* RwCameraBeginUpdate(RwCamera* camera);
RwCamera* RwCameraEndUpdate(RwCamera* camera);
RwCamera* RwCameraShowRaster(RwCamera* camera, void* dev, unsigned int flags);
RwCamera* RwCameraClear(RwCamera* camera, RwRGBA* colour, signed int clearMode);
RwResEntry* RwResourcesAllocateResEntry(void* owner, RwResEntry** ownerRef, signed int size,
                                        signed int (*destroyNotify)(RwResEntry*));
void RwResourcesFreeResEntry(RwResEntry* entry);
signed int RwIm2DRenderIndexedPrimitive(signed int primType, void* vertices, signed int numVertices,
                                        unsigned short* indices, signed int numIndices);
void GXSetViewport(float left, float top, float wd, float ht, float nearz, float farz);
void GXSetViewportJitter(float left, float top, float wd, float ht, float nearz, float farz,
                         unsigned int field);
void GXSetScissor(unsigned int left, unsigned int top, unsigned int wd, unsigned int ht);
void GXSetDispCopySrc(unsigned short left, unsigned short top, unsigned short wd, unsigned short ht);
void GXSetDispCopyDst(unsigned short wd, unsigned short ht);
unsigned int GXSetDispCopyYScale(float vscale);
void GXSetCopyFilter(unsigned char aa, unsigned char sample_pattern[12][2], unsigned char vf,
                     unsigned char vfilter[7]);
void GXSetDispCopyGamma(signed int gamma);
void GXLoadPosMtxImm(float mtx[3][4], unsigned int id);
void GXClearVtxDesc();
void GXSetVtxDesc(signed int attr, signed int type);
void GXSetVtxAttrFmt(signed int vtxfmt, signed int attr, signed int cnt, signed int type,
                     unsigned char frac);
void GXSetNumTexGens(unsigned char nTexGens);
void GXSetNumChans(unsigned char nChans);
void GXSetNumTevStages(unsigned char nStages);
void GXSetTexCoordGen2(signed int dst_coord, signed int func, signed int src_param,
                       unsigned int mtx, unsigned char normalize, unsigned int postmtx);
void GXSetTevOp(signed int id, signed int mode);
void GXSetTevOrder(signed int stage, signed int coord, signed int map, signed int color);
void GXInvalidateVtxCache();
void GXInvalidateTexAll();
unsigned int VIGetNextField();
void PSMTXIdentity(float m[3][4]);
// GX pixel format, projection and ortho matrix
void GXSetPixelFmt(signed int pix_fmt, signed int z_fmt);
void GXSetProjection(float mtx[4][4], signed int type);
void C_MTXOrtho(float m[4][4], float t, float b, float l, float r, float n, float f);
// TODO: unidentified iSystem/xTRC functions
void fn_80014A34(signed int);
void fn_800167F0();
void fn_800443AC();
unsigned int fn_800443F8();
signed int RwResourcesGetArenaSize();
extern RwModuleInfo lbl_8025F798; // resourcesModule
extern unsigned char* RwEngineInstance;
extern _tagxPad mPad;
}
void xTRCRender();

unsigned int iFileOpen(char* name, signed int flags, tag_xFile* file);
void iPadStopRumble(_tagxPad* pad);
void xPadUpdate(signed int idx, float time_passed);
void xDrawLine2D(float x1, float y1, float x2, float y2);

class GXRenderModeObj
{
public:
    signed int viTVmode;               // 0x0
    unsigned short fbWidth;            // 0x4
    unsigned short efbHeight;          // 0x6
    unsigned short xfbHeight;          // 0x8
    unsigned short viXOrigin;          // 0xA
    unsigned short viYOrigin;          // 0xC
    unsigned short viWidth;            // 0xE
    unsigned short viHeight;           // 0x10
    signed int xFBmode;                // 0x14
    unsigned char field_rendering;     // 0x18
    unsigned char aa;                  // 0x19
    unsigned char sample_pattern[12][2]; // 0x1A
    unsigned char vfilter[7];          // 0x32
};

extern "C" GXRenderModeObj* lbl_8025F860; // _RwDlRenderMode

class RwIm2DVertex
{
public:
    float x;           // 0x0
    float y;           // 0x4
    float z;           // 0x8
    RwRGBA emissiveColor; // 0xC
    float u;           // 0x10
    float v;           // 0x14
    float pad;         // 0x18
};

static unsigned int Bink_surface_type[6];

static RwResEntry* sResEntry;
static unsigned int sLetterbox;
static signed int frame_num;
static HBINK Bink;
static HRAD3DIMAGE Image;
static float sUnused1BC;
static float sUnused1C0;
static void* pixels;
static unsigned char* sWorkBase;
static unsigned char* sWorkAligned;
static unsigned int sWorkUsed;
extern "C" unsigned char* sFMVWorkArea;
unsigned char* sFMVWorkArea;
static volatile float vol;
static signed int ip;
static unsigned int sDiskError;
static signed int sBinkError;

static float Width_scale = 1.0f;
static float Height_scale = 1.0f;

static void Show_frame();
static void PlayFMV(char* fname, unsigned int buttons, float time);
static void* iFMVmalloc(unsigned long size);

// TODO: NonMatching (register allocation in iFMVmalloc/PlayFMV/Show_frame).

// Functions are in reverse DOL order (-inline deferred emits last to first).

signed int iFMVPlay(char* filename, unsigned int buttons, float time)
{
    if (filename == NULL)
    {
        return 1;
    }

    unsigned char letterbox = strncmp(filename, "FMV/cin", 7) == 0;
    frame_num = 0;
    sLetterbox = letterbox;
    while (frame_num >= 0)
    {
        PlayFMV(filename, buttons, time);
    }
    return 0;
}

static void DrawFrame(float x, float y, float sx, float sy)
{
    GXRenderModeObj* rm = lbl_8025F860;
    float mtx[4][4];
    float idt[3][4];

    GXSetViewport(0.0f, 0.0f, rm->fbWidth, rm->efbHeight, 0.0f, 1.0f);
    GXSetScissor(0, 0, rm->fbWidth, rm->efbHeight);
    GXSetDispCopySrc(0, 0, rm->fbWidth, rm->efbHeight);
    GXSetDispCopyDst(rm->fbWidth, rm->xfbHeight);
    GXSetDispCopyYScale((float)rm->xfbHeight / rm->efbHeight);
    GXSetCopyFilter(rm->aa, rm->sample_pattern, 1, rm->vfilter);
    if (rm->aa)
    {
        GXSetPixelFmt(2, 0);
    }
    else
    {
        GXSetPixelFmt(0, 0);
    }
    GXSetDispCopyGamma(0);
    C_MTXOrtho(mtx, 0.0f, 480.0f, 0.0f, 640.0f, 0.0f, 10000.0f);
    GXSetProjection(mtx, 1);
    PSMTXIdentity(idt);
    GXLoadPosMtxImm(idt, 0);
    GXClearVtxDesc();
    GXSetVtxDesc(9, 1);
    GXSetVtxDesc(11, 1);
    GXSetVtxDesc(13, 1);
    GXSetVtxAttrFmt(0, 9, 1, 3, 0);
    GXSetVtxAttrFmt(0, 11, 1, 5, 0);
    GXSetVtxAttrFmt(0, 13, 1, 4, 0);
    GXSetNumTexGens(1);
    GXSetNumChans(1);
    GXSetNumTevStages(1);
    GXSetTexCoordGen2(0, 1, 4, 60, 0, 125);
    GXSetTevOp(0, 0);
    GXSetTevOrder(0, 0, 0, 0);
    if (rm->field_rendering)
    {
        GXSetViewportJitter(0.0f, 0.0f, rm->fbWidth, rm->efbHeight, 0.0f, 1.0f, VIGetNextField());
    }
    else
    {
        GXSetViewport(0.0f, 0.0f, rm->fbWidth, rm->efbHeight, 0.0f, 1.0f);
    }
    GXInvalidateVtxCache();
    GXInvalidateTexAll();
    Blit_RAD_3D_image(Image, x, y, sx, sy, 1.0f);
}

static void Show_frame()
{
    static const RwRGBA color = { 0, 0, 0, 0 };
    RwRGBA clear = color;
    RwCamera* cam = globals.camera.lo_cam;

    RwCameraClear(cam, &clear, 1);
    RwCameraBeginUpdate(cam);
    float x = Bink->Width * (sUnused1BC * Width_scale) + 0.5f * (640.0f - Width_scale * Bink->Width);
    float y = Bink->Height * (sUnused1C0 * Height_scale) + 0.5f * (480.0f - Height_scale * Bink->Height);
    xDrawLine2D(0.0f, 0.0f, 0.0f, 0.0f);

    if (sResEntry->link.next != NULL)
    {
        rwLinkListRemoveLLLink(&sResEntry->link);
        rwLinkListAddLLLink(RWRESOURCESGLOBAL(usedEntries), &sResEntry->link);
    }

    DrawFrame(x, y, Width_scale, Height_scale);
    if (sLetterbox)
    {
        unsigned short indices[4] = { 0, 1, 2, 3 };
        RwIm2DVertex verts[4];
        verts[0].x = 0.0f;
        verts[0].y = 0.0f;
        verts[1].x = 640.0f;
        verts[1].y = 0.0f;
        verts[2].x = 0.0f;
        verts[2].y = 48.0f;
        verts[3].x = 640.0f;
        verts[3].y = 48.0f;
        verts[0].emissiveColor.red = 0;
        verts[0].emissiveColor.green = 0;
        verts[0].emissiveColor.blue = 0;
        verts[0].emissiveColor.alpha = 255;
        verts[1].emissiveColor.red = 0;
        verts[1].emissiveColor.green = 0;
        verts[1].emissiveColor.blue = 0;
        verts[1].emissiveColor.alpha = 255;
        verts[2].emissiveColor.red = 0;
        verts[2].emissiveColor.green = 0;
        verts[2].emissiveColor.blue = 0;
        verts[2].emissiveColor.alpha = 255;
        verts[3].emissiveColor.red = 0;
        verts[3].emissiveColor.green = 0;
        verts[3].emissiveColor.blue = 0;
        verts[3].emissiveColor.alpha = 255;
        RwIm2DRenderIndexedPrimitive(4, verts, 4, indices, 4);
        verts[0].x = 0.0f;
        verts[0].y = 432.0f;
        verts[1].x = 640.0f;
        verts[1].y = 432.0f;
        verts[2].x = 0.0f;
        verts[2].y = 480.0f;
        verts[3].x = 640.0f;
        verts[3].y = 480.0f;
        RwIm2DRenderIndexedPrimitive(4, verts, 4, indices, 4);
    }
    xTRCRender();
    RwCameraEndUpdate(cam);
    RwCameraShowRaster(cam, NULL, 0);
}

static void* iFMVmalloc(unsigned long size)
{
    unsigned int used = sWorkUsed;
    unsigned int next = used + ((size + 0x1F) & ~0x1F);
    unsigned char* base = sWorkAligned;
    if (next > 0x3E0000)
    {
        return NULL;
    }
    void* mem = base + used;
    sWorkUsed = next;
    return mem;
}

static void iFMVfree(void* mem)
{
}

static void* arammalloc(unsigned long size)
{
    return (void*)ARAlloc(size);
}

static void aramfree(void* mem)
{
    unsigned long length;
    ARFree(&length);
}

static signed int iFMVResEntryDestroy(RwResEntry* entry)
{
    return 0;
}

static inline void iFMVFlushScreen()
{
    RwCamera* cam = globals.camera.lo_cam;
    signed int i = 2;
    do
    {
        RwCameraBeginUpdate(cam);
        RwCameraEndUpdate(cam);
        RwCameraShowRaster(cam, NULL, 1);
    } while (i-- > 0);
}

static void PlayFMV(char* fname, unsigned int buttons, float time)
{
    tag_xFile file;

    iFMVFlushScreen();
    fn_80014A34(0);
    iPadStopRumble(globals.pad0);
    fn_800167F0();

    RwResEntry* entry = RwResourcesAllocateResEntry(NULL, NULL, 0x3E0000, iFMVResEntryDestroy);
    if (entry == NULL)
    {
        OSReport("### Error allocating Bink work buffer\n");
        return;
    }
    sResEntry = entry;
    OSReport("RW resource arena size:  %d bytes\n", RwResourcesGetArenaSize());
    OSReport("RW rep entry:        %08X-%08X\n", (unsigned char*)entry + 0x18,
             (unsigned char*)entry + 0x3E0017);
    sWorkBase = (unsigned char*)entry + 0x18;
    sWorkAligned = (unsigned char*)(((unsigned int)sWorkBase + 0x1F) & ~0x1F);
    unsigned char* mem;
    unsigned int used = sWorkUsed + 0x12C400;
    if (used > 0x3E0000)
    {
        mem = NULL;
    }
    else
    {
        mem = sWorkAligned + sWorkUsed;
        sWorkUsed = used;
    }
    OSReport("RW rep entry round:  %08X-%08X\n", mem, mem + 0x12C3FF);
    sFMVWorkArea = mem;
    OSReport("### Allocated %d bytes at %08X for Bink work buffer\n", 0x12C400, mem);
    OSReport("FMV work area:  %08X - %08X\n", sFMVWorkArea, sFMVWorkArea + 0x12C3FF);

    RADSetAudioMemory(arammalloc, aramfree);
    RADSetMemory(iFMVmalloc, iFMVfree);
    Bink_surface_type[0] = 3;
    Bink_surface_type[1] = 5;
    Bink_surface_type[2] = 10;
    Bink_surface_type[3] = 7;
    Bink_surface_type[4] = 13;

    do
    {
        Bink = BinkOpen(fname, 0);
        if (Bink == NULL)
        {
            iFileOpen(fname, 0, &file);
            DVDSeekAsyncPrio(&file.fileInfo, 0, NULL, 2);
            iTRCDisk::CheckDVDAndResetState();
            DVDCancelAsync(&file.fileInfo.cb, NULL);
        }
        unsigned int err = fn_800443F8();
        if (err)
        {
            fn_800443AC();
        }
        sDiskError = err;
    } while (Bink == NULL && sDiskError != 0);

    if (Bink != NULL)
    {
        if (Bink->NumTracks != 0)
        {
            for (ip = 0; ip <= Bink->NumTracks; ip++)
            {
                vol = (float)__fabs(gSnd.mastervol.left > gSnd.mastervol.right ? gSnd.mastervol.left
                                                                              : gSnd.mastervol.right) *
                      (float)__fabs(gSnd.streamvol.left > gSnd.streamvol.right ? gSnd.streamvol.left
                                                                              : gSnd.streamvol.right);
                vol = vol * vol;
                vol = vol * 32768.0f;
                BinkSetVolume(Bink, ip, vol);
            }
        }

        Image = Open_RAD_3D_image(NULL, Bink->Width, Bink->Height, 0);
        if (Image != NULL)
        {
            if (frame_num != 0)
            {
                BinkGoto(Bink, frame_num, 0);
            }
            sDiskError = 0;
            while (1)
            {
                iTRCDisk::CheckDVDAndResetState();
                unsigned int err = fn_800443F8();
                if (err)
                {
                    fn_800443AC();
                }
                if (err)
                {
                    break;
                }
                if (BinkWait(Bink) == 0)
                {
                    HBINK bnk = Bink;
                    HRAD3DIMAGE img = Image;
                    unsigned int pitch;
                    unsigned int type;
                    BinkDoFrame(bnk);
                    if (Lock_RAD_3D_image(img, &pixels, &pitch, &type))
                    {
                        BinkCopyToBuffer(bnk, pixels, pitch, bnk->Height, 0, 0,
                                         Bink_surface_type[type]);
                        Unlock_RAD_3D_image(img);
                    }
                    Show_frame();
                    BinkNextFrame(Bink);
                }
                xPadUpdate(0, 0.0f);
                float t = (float)Bink->FrameNum / (float)(Bink->FrameRate / Bink->FrameRateDiv);
                if (buttons && t >= time && (*(unsigned int*)((unsigned char*)&mPad + 0x30) & buttons))
                {
                    frame_num = -1;
                    break;
                }
                if (Bink->FrameNum == Bink->Frames - 1)
                {
                    frame_num = -1;
                    break;
                }
            }
        }
        if (frame_num != -1)
        {
            frame_num = Bink->FrameNum;
        }
        Close_RAD_3D_image(Image);
        Image = NULL;
        BinkClose(Bink);
        Bink = NULL;
    }
    else
    {
        sBinkError = BinkGetError();
    }

    iFMVFlushScreen();
    RwResourcesFreeResEntry(entry);
    sWorkBase = NULL;
    sWorkAligned = NULL;
    sWorkUsed = 0;
}
