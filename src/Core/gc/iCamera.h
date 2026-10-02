#ifndef ICAMERA_H
#define ICAMERA_H

enum RwFogType
{
    rwFOGTYPENAFOGTYPE = 0,
    rwFOGTYPELINEAR = 1,
    rwFOGTYPEEXPONENTIAL = 2,
    rwFOGTYPEEXPONENTIAL2 = 3,
};

class RwRGBA
{
public:
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char alpha;
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
