#include "iTime.h"
#include "xMemMgr.h"

extern "C" int sprintf(char* s, const char* format, ...);
extern "C" void* malloc(unsigned int size);
extern "C" void free(void* ptr);

class tag_xFile;
unsigned int iFileOpen(char* name, signed int flags, tag_xFile* file);
unsigned int iFileClose(tag_xFile* file);
// The GameCube iFileWrite is a stub (see iFile.cpp).
extern "C" signed int fn_8000A5A4(tag_xFile* file, void* buf, unsigned int size);
float iTimeDiffSec(iTime t0, iTime t1);

class RwRaster
{
public:
    unsigned char pad0[0x4];
    unsigned char* cpPixels; // 0x4
    unsigned char pad8[0x4];
    signed int width;  // 0xC
    signed int height; // 0x10
    signed int depth;  // 0x14
};

class RwImage
{
public:
    unsigned char pad0[0x14];
    unsigned char* cpPixels; // 0x14
};

class RwCamera
{
public:
    unsigned char pad0[0x60];
    RwRaster* frameBuffer; // 0x60
};

class RwGlobals
{
public:
    RwCamera* curCamera; // 0x0
};

extern RwGlobals* RwEngineInstance;

extern "C" {
RwRaster* RwRasterLock(RwRaster* raster, unsigned char level, signed int lockMode);
RwRaster* RwRasterUnlock(RwRaster* raster);
RwImage* RwImageCreate(signed int width, signed int height, signed int depth);
RwImage* RwImageAllocatePixels(RwImage* image);
RwImage* RwImageSetFromRaster(RwImage* image, RwRaster* raster);
signed int RwImageDestroy(RwImage* image);
}

class tagBITMAPFILEHEADER
{
public:
    signed int bfSize;         // 0x0
    signed short bfReserved1;  // 0x4
    signed short bfReserved2;  // 0x6
    signed int bfOffBits;      // 0x8
};

class tagBITMAPINFOHEADER
{
public:
    signed int biSize;          // 0x0
    signed int biWidth;         // 0x4
    signed int biHeight;        // 0x8
    signed short biPlanes;      // 0xC
    signed short biBitCount;    // 0xE
    signed int biCompression;   // 0x10
    signed int biSizeImage;     // 0x14
    signed int biXPelsPerMeter; // 0x18
    signed int biYPelsPerMeter; // 0x1C
    signed int biClrUsed;       // 0x20
    signed int biClrImportant;  // 0x24
};

class BmpMegaHeader
{
public:
    char prepad[2];                   // 0x0
    char bfType[2];                   // 0x2
    tagBITMAPFILEHEADER bmFile;       // 0x4
    tagBITMAPINFOHEADER bmInfo;       // 0x10
};

class tag_xFile
{
public:
    unsigned char pad[0xFC];
};

unsigned int gScreenDump;
static signed int sDumpFileNumber;
char gScreenDumpPath[256];

// TODO: one instruction short. The original has a second, unreachable `b` to the
// epilogue right after the image-create failure path's return.
void iDebugScreenDump()
{
    RwRaster* camRas;
    signed int width;
    signed int height;
    RwImage* image;
    unsigned char* oldCamPixels;
    unsigned char* pixels;
    tag_xFile file;
    unsigned int x;
    unsigned int y;
    char fname[64];
    unsigned char* bp;
    unsigned char* buf;
    unsigned char* imp;
    iTime t0;
    iTime t1;
    BmpMegaHeader mhdr;
    unsigned char* p;

    camRas = RwEngineInstance->curCamera->frameBuffer;
    if (!camRas)
    {
        return;
    }

    width = camRas->width;
    height = camRas->height;
    oldCamPixels = camRas->cpPixels;
    pixels = (unsigned char*)malloc((camRas->depth >> 3) * (width * height));
    if (pixels)
    {
        camRas->cpPixels = pixels;
    }
    else
    {
        return;
    }

    if (!RwRasterLock(camRas, 0, 2))
    {
        camRas->cpPixels = oldCamPixels;
        free(pixels);
    }

    image = RwImageCreate(width, height, 32);
    if (image)
    {
        RwImageAllocatePixels(image);
        RwImageSetFromRaster(image, camRas);
        RwRasterUnlock(camRas);
        camRas->cpPixels = oldCamPixels;
        free(pixels);
    }
    else
    {
        RwRasterUnlock(camRas);
        camRas->cpPixels = oldCamPixels;
        free(pixels);
        return;
    }

    t0 = iTimeGet();

    if (gScreenDumpPath[0])
    {
        sprintf(fname, "host:%sdump%04d.bmp", gScreenDumpPath, sDumpFileNumber++);
        iFileOpen(fname, 0xE, &file);
    }
    else
    {
        sprintf(fname, "dump%04d.bmp", sDumpFileNumber++);
        iFileOpen(fname, 0xE, &file);
    }

    mhdr.bfType[0] = 'B';
    mhdr.bfType[1] = 'M';
    mhdr.bmFile.bfSize = 640 * 480 * 3 + 0x36;
    mhdr.bmFile.bfReserved1 = 0;
    mhdr.bmFile.bfReserved2 = 0;
    mhdr.bmFile.bfOffBits = 0x36;
    mhdr.bmInfo.biSize = 0x28;
    mhdr.bmInfo.biWidth = 640;
    mhdr.bmInfo.biHeight = 480;
    mhdr.bmInfo.biPlanes = 1;
    mhdr.bmInfo.biBitCount = 24;
    mhdr.bmInfo.biCompression = 0;
    mhdr.bmInfo.biSizeImage = 0;
    mhdr.bmInfo.biXPelsPerMeter = 0;
    mhdr.bmInfo.biYPelsPerMeter = 0;
    mhdr.bmInfo.biClrUsed = 0;
    mhdr.bmInfo.biClrImportant = 0;
    fn_8000A5A4(&file, mhdr.bfType, 0x36);

    buf = (unsigned char*)xMemPushTemp(0x7800);
    imp = image->cpPixels;
    bp = buf;
    for (y = 0; y < 480; y++)
    {
        p = imp + (480 - y) * (640 * 4);
        for (x = 0; x < 640; x++)
        {
            bp[0] = p[2];
            bp[1] = p[1];
            bp[2] = p[0];
            p += 4;
            bp += 3;
        }
        if ((unsigned int)(bp - buf) >= 0x7800)
        {
            fn_8000A5A4(&file, buf, 0x7800);
            bp = buf;
        }
    }

    xMemPopTemp(buf);
    RwImageDestroy(image);
    iFileClose(&file);
    t1 = iTimeGet();
    iTimeDiffSec(t0, t1);
    gScreenDump = 0;
}
