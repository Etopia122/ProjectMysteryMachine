/*
 * Image and raster conversion through the device's standard functions.
 */

#include "rwsdk/rwcore.h"

#define rwSTANDARDIMAGEGETRASTER 6
#define rwSTANDARDRASTERSETIMAGE 7
#define rwSTANDARDIMAGEFINDRASTERFORMAT 9

#define rwIMAGEGAMMACORRECTED 0x02
#define rwRASTERGAMMACORRECTED 0x01

RwImage* RwImageSetFromRaster(RwImage* image, RwRaster* raster) {
    if (RWSRCGLOBAL(stdFunc)[rwSTANDARDIMAGEGETRASTER](image, raster, 0)) {
        if (raster->privateFlags & rwRASTERGAMMACORRECTED) {
            image->flags |= rwIMAGEGAMMACORRECTED;
        }

        return image;
    }

    return NULL;
}

RwRaster* RwRasterSetFromImage(RwRaster* raster, RwImage* image) {
    if (RWSRCGLOBAL(stdFunc)[rwSTANDARDRASTERSETIMAGE](raster, image, 0)) {
        if (image->flags & rwIMAGEGAMMACORRECTED) {
            raster->privateFlags |= rwRASTERGAMMACORRECTED;
        }

        return raster;
    }

    return NULL;
}

RwImage* RwImageFindRasterFormat(RwImage* ipImage, RwInt32 nRasterType, RwInt32* npWidth, RwInt32* npHeight,
                                 RwInt32* npDepth, RwInt32* npFormat) {
    RwRaster raster;

    if (!RWSRCGLOBAL(stdFunc)[rwSTANDARDIMAGEFINDRASTERFORMAT](&raster, ipImage, nRasterType)) {
        return NULL;
    }

    *npFormat = RwRasterGetFormat(&raster) | RwRasterGetType(&raster);
    *npWidth = raster.width;
    *npHeight = raster.height;
    *npDepth = raster.depth;

    return ipImage;
}
