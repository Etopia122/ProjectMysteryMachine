#include "xstransvc.h"

enum _SDRenderState
{
    SDRS_Unknown = 0,
    SDRS_Default = 1,
    SDRS_OpaqueModels = 2,
    SDRS_AlphaModels = 3,
    SDRS_Bubble = 4,
    SDRS_Projectile = 5,
    SDRS_Font = 6,
    SDRS_HUD = 7,
    SDRS_Particles = 8,
    SDRS_Lightning = 9,
    SDRS_SkidMark = 10,
    SDRS_Tide = 11,
    SDRS_SkyBack = 12,
    SDRS_Total = -1,
};

enum RwRenderState
{
    rwRENDERSTATENARENDERSTATE = 0,
    rwRENDERSTATETEXTURERASTER = 1,
    rwRENDERSTATETEXTUREADDRESS = 2,
    rwRENDERSTATETEXTUREADDRESSU = 3,
    rwRENDERSTATETEXTUREADDRESSV = 4,
    rwRENDERSTATETEXTUREPERSPECTIVE = 5,
    rwRENDERSTATEZTESTENABLE = 6,
    rwRENDERSTATESHADEMODE = 7,
    rwRENDERSTATEZWRITEENABLE = 8,
    rwRENDERSTATETEXTUREFILTER = 9,
    rwRENDERSTATESRCBLEND = 10,
    rwRENDERSTATEDESTBLEND = 11,
    rwRENDERSTATEVERTEXALPHAENABLE = 12,
    rwRENDERSTATEBORDERCOLOR = 13,
    rwRENDERSTATEFOGENABLE = 14,
    rwRENDERSTATEFOGCOLOR = 15,
    rwRENDERSTATEFOGTYPE = 16,
    rwRENDERSTATEFOGDENSITY = 17,
    rwRENDERSTATECULLMODE = 20,
};

enum GXCompare
{
    GX_NEVER,
    GX_LESS,
    GX_EQUAL,
    GX_LEQUAL,
    GX_GREATER,
    GX_NEQUAL,
    GX_GEQUAL,
    GX_ALWAYS
};

enum GXAlphaOp
{
    GX_AOP_AND,
    GX_AOP_OR,
    GX_AOP_XOR,
    GX_AOP_XNOR,
    GX_MAX_ALPHAOP
};

class RwRaster;

class RwTexture
{
public:
    RwRaster* raster;
};

extern "C" {
signed int RwRenderStateSet(RwRenderState state, void* value);
void GXSetAlphaCompare(GXCompare comp0, unsigned char ref0, GXAlphaOp op, GXCompare comp1,
                       unsigned char ref1);
}

static _SDRenderState sRS;
static signed int sBlendTable[11] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

// Functions are in reverse DOL order (-inline deferred emits last to first).

void zRenderStateInit()
{
    sRS = SDRS_Unknown;
}

void zRenderState(_SDRenderState next)
{
    if (next == sRS)
    {
        return;
    }
    if (next != SDRS_Default)
    {
        zRenderState(SDRS_Default);
    }
    sRS = next;

    switch (next)
    {
    case SDRS_Default:
        RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)2);
        RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)1);
        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);
        RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)1);
        RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)1);
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)1);
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)2);
        RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)2);
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)1);
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)1);
        break;
    case SDRS_Particles:
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)0);
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)1);
        RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)2);
        RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)0);
        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);
        RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)1);
        RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)1);
        RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)1);
        break;
    case SDRS_Tide:
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)1);
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)1);
        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)5);
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)6);
        RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)2);
        break;
    case SDRS_OpaqueModels:
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)1);
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)1);
        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)0);
        RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)1);
        RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)2);
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)2);
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)1);
        break;
    case SDRS_AlphaModels:
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)1);
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)1);
        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);
        RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)1);
        RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)2);
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)5);
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)6);
        GXSetAlphaCompare(GX_GEQUAL, 0x80, GX_AOP_AND, GX_LEQUAL, 0xFF);
        break;
    case SDRS_Lightning:
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)0);
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)1);
        RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)1);
        RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)1);
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)5);
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)6);
        break;
    case SDRS_SkidMark:
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)0);
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)1);
        RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)2);
        RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)1);
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)5);
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)6);
        break;
    case SDRS_Font:
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)0);
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)0);
        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)0);
        RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)1);
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)2);
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)1);
        break;
    case SDRS_HUD:
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)1);
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)1);
        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);
        RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)2);
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)5);
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)6);
        break;
    case SDRS_Bubble:
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)1);
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)1);
        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)0);
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)5);
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)6);
        RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)2);
        break;
    case SDRS_SkyBack:
        RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)0);
        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)0);
        RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)1);
        RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)1);
        RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)1);
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)0);
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)0);
        break;    }
}

void zRenderStateSetTexture(unsigned int assetID)
{
    RwTexture* tex;

    if (assetID == 0)
    {
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, 0);
        return;
    }
    tex = (RwTexture*)xSTFindAsset(assetID, 0);
    if (tex && tex->raster)
    {
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, tex->raster);
    }
}

void zRenderStateSetSrcBlendMode(signed int xmode)
{
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)sBlendTable[xmode]);
}

void zRenderStateSetDstBlendMode(signed int xmode)
{
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)sBlendTable[xmode]);
}
