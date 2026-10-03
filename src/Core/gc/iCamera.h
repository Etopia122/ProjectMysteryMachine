#ifndef ICAMERA_H
#define ICAMERA_H

#include "rwtypes.h"

enum RwFogType
{
    rwFOGTYPENAFOGTYPE = 0,
    rwFOGTYPELINEAR = 1,
    rwFOGTYPEEXPONENTIAL = 2,
    rwFOGTYPEEXPONENTIAL2 = 3,
};

class iFogParams
{
public:
    RwFogType type;
    float start;
    float stop;
    float density;
    RwRGBA fogcolor;
    RwRGBA bgcolor;
    unsigned char* table;
};

void iCameraSetFogParams(iFogParams* fp, float time);

#endif
