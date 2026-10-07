/*
 * Rasters: creation and destruction, locking and the raster context stack.
 */

#include "rwsdk/rwcore.h"

#define rwSTANDARDRASTERCREATE 4
#define rwSTANDARDRASTERDESTROY 5
#define rwSTANDARDRASTERLOCK 15
#define rwSTANDARDRASTERUNLOCK 16
#define rwSTANDARDRASTERSHOWRASTER 20
#define rwSTANDARDRASTERLOCKPALETTE 23
#define rwSTANDARDRASTERUNLOCKPALETTE 24
#define rwSTANDARDRASTERGETMIPLEVELS 28

#define rwRASTERFORMATMIPMAP 0x8000
#define rwRASTERDONTALLOCATE 0x80
#define rwRASTERTYPENORMAL 0x00
#define rwRASTERPALETTELOCKED 0x18

#define rwRASTERCONTEXTSTACKSIZE 10

typedef struct rwRasterGlobals {
    RwRaster* rasterStack[rwRASTERCONTEXTSTACKSIZE];
    RwInt32 rasterSP;
    RwRaster dummyRaster;
    RwFreeList* rasterFreeList;
} rwRasterGlobals;

RwPluginRegistry rasterTKList = { sizeof(RwRaster), sizeof(RwRaster), 0, 0, NULL, NULL };

RwModuleInfo rasterModule;

#define RWRASTERGLOBAL(var) (RWPLUGINOFFSET(rwRasterGlobals, RwEngineInstance, rasterModule.globalsOffset)->var)

void* memset(void* dst, int c, unsigned long n);
void _rwResourcesPurge(void);

RwRaster* RwRasterUnlock(RwRaster* raster) {
    RWSRCGLOBAL(stdFunc)[rwSTANDARDRASTERUNLOCK](NULL, raster, 0);

    return raster;
}

RwRaster* RwRasterUnlockPalette(RwRaster* raster) {
    RWSRCGLOBAL(stdFunc)[rwSTANDARDRASTERUNLOCKPALETTE](NULL, raster, 0);
    raster->privateFlags &= ~rwRASTERPALETTELOCKED;

    return raster;
}

RwBool RwRasterDestroy(RwRaster* raster) {
    _rwPluginRegistryDeInitObject(&rasterTKList, raster);
    RWSRCGLOBAL(stdFunc)[rwSTANDARDRASTERDESTROY](NULL, raster, 0);
    RwFreeListFree(RWRASTERGLOBAL(rasterFreeList), raster);

    return TRUE;
}

RwInt32 RwRasterRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB,
                               RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB) {
    return _rwPluginRegistryAddPlugin(&rasterTKList, size, pluginID, constructCB, destructCB, copyCB);
}

RwUInt8* RwRasterLockPalette(RwRaster* raster, RwInt32 lockMode) {
    RwUInt8* palette;

    if (RWSRCGLOBAL(stdFunc)[rwSTANDARDRASTERLOCKPALETTE](&palette, raster, lockMode)) {
        return palette;
    }

    return NULL;
}

RwInt32 RwRasterGetNumLevels(RwRaster* raster) {
    RwInt32 numMipLevels;

    if (!(RwRasterGetFormat(raster) & rwRASTERFORMATMIPMAP)) {
        return 1;
    }

    if (RWSRCGLOBAL(stdFunc)[rwSTANDARDRASTERGETMIPLEVELS](&numMipLevels, raster, 0)) {
        return numMipLevels;
    }

    return -1;
}

RwRaster* RwRasterShowRaster(RwRaster* raster, void* dev, RwUInt32 flags) {
    RwStandardFunc showRaster = RWSRCGLOBAL(stdFunc)[rwSTANDARDRASTERSHOWRASTER];

    _rwResourcesPurge();

    if (showRaster(raster, dev, flags)) {
        return raster;
    }

    return NULL;
}

RwRaster* RwRasterCreate(RwInt32 width, RwInt32 height, RwInt32 depth, RwInt32 flags) {
    RwRaster* raster = (RwRaster*)RwFreeListAlloc(RWRASTERGLOBAL(rasterFreeList));

    if (raster) {
        RwStandardFunc rasterCreate = RWSRCGLOBAL(stdFunc)[rwSTANDARDRASTERCREATE];

        raster->privateFlags = 0;
        raster->cFlags = 0;
        raster->width = width;
        raster->height = height;
        raster->nOffsetX = 0;
        raster->nOffsetY = 0;
        raster->depth = depth;
        raster->parent = raster;
        raster->cpPixels = NULL;
        raster->palette = NULL;

        if (!rasterCreate(NULL, raster, flags)) {
            RwFreeListFree(RWRASTERGLOBAL(rasterFreeList), raster);
            return NULL;
        }

        _rwPluginRegistryInitObject(&rasterTKList, raster);

        return raster;
    }

    return NULL;
}

RwUInt8* RwRasterLock(RwRaster* raster, RwUInt8 level, RwInt32 lockMode) {
    RwUInt8* pixels;

    if (RWSRCGLOBAL(stdFunc)[rwSTANDARDRASTERLOCK](&pixels, raster, lockMode + ((RwInt32)level << 8))) {
        return pixels;
    }

    return NULL;
}

void* _rwRasterClose(void* instance, RwInt32 offset, RwInt32 size) {
    if (RWRASTERGLOBAL(rasterFreeList)) {
        RwFreeListDestroy(RWRASTERGLOBAL(rasterFreeList));
        RWRASTERGLOBAL(rasterFreeList) = NULL;
    }

    rasterModule.numInstances--;

    return instance;
}

void* _rwRasterOpen(void* instance, RwInt32 offset, RwInt32 size) {
    rasterModule.globalsOffset = offset;

    memset(&RWRASTERGLOBAL(dummyRaster), 0, sizeof(RwRaster));
    RWRASTERGLOBAL(dummyRaster).width = 0;
    RWRASTERGLOBAL(dummyRaster).height = 0;
    RWRASTERGLOBAL(dummyRaster).depth = 0;
    RWRASTERGLOBAL(dummyRaster).cFlags = rwRASTERDONTALLOCATE;
    RWRASTERGLOBAL(dummyRaster).cpPixels = NULL;
    RWRASTERGLOBAL(dummyRaster).palette = NULL;
    RWRASTERGLOBAL(dummyRaster).cType = rwRASTERTYPENORMAL;

    RWRASTERGLOBAL(rasterSP) = 0;
    RWRASTERGLOBAL(rasterStack)[0] = &RWRASTERGLOBAL(dummyRaster);

    RWRASTERGLOBAL(rasterFreeList) = RwFreeListCreate(rasterTKList.sizeOfStruct, 20, 0);
    if (!RWRASTERGLOBAL(rasterFreeList)) {
        return NULL;
    }

    rasterModule.numInstances++;

    return instance;
}
