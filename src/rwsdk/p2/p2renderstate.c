/*
 * PowerPipe render state vectors: snapshots of the driver render state that
 * pipelines can save, restore and default.
 */

#include "rwsdk/p2core_int.h"

#define rwPLUGIN_ID 1 /* core */

#define E_RX_INVALIDRENDERSTATEVECTOR 0x80000016
#define E_RX_ENGINENOTSTARTED 0x80000018

#define rwENGINESTATUSSTARTED 3

#define rxRENDERSTATEFLAG_TEXTUREPERSPECTIVE 0x01
#define rxRENDERSTATEFLAG_ZTESTENABLE 0x02
#define rxRENDERSTATEFLAG_ZWRITEENABLE 0x04
#define rxRENDERSTATEFLAG_VERTEXALPHAENABLE 0x08
#define rxRENDERSTATEFLAG_ALPHAPRIMITIVEBUFFER 0x20

#define rwRENDERSTATETEXTURERASTER 1
#define rwRENDERSTATETEXTUREADDRESS 2
#define rwRENDERSTATETEXTUREADDRESSU 3
#define rwRENDERSTATETEXTUREADDRESSV 4
#define rwRENDERSTATETEXTUREPERSPECTIVE 5
#define rwRENDERSTATEZTESTENABLE 6
#define rwRENDERSTATESHADEMODE 7
#define rwRENDERSTATEZWRITEENABLE 8
#define rwRENDERSTATETEXTUREFILTER 9
#define rwRENDERSTATESRCBLEND 10
#define rwRENDERSTATEDESTBLEND 11
#define rwRENDERSTATEVERTEXALPHAENABLE 12
#define rwRENDERSTATEBORDERCOLOR 13
#define rwRENDERSTATEFOGCOLOR 15
#define rwRENDERSTATEFOGTYPE 16
#define rwRENDERSTATEFOGTABLE 18
#define rwRENDERSTATEALPHAPRIMITIVEBUFFER 19

RwBool RwRenderStateGet(RwRenderState state, void* value);

RxRenderStateVector* RxRenderStateVectorLoadDriverState(RxRenderStateVector* rsvp);
void RxRenderStateVectorDestroy(RxRenderStateVector* rsvp);

RxRenderStateVector* RxRenderStateVectorSetDefaultRenderStateVector(RxRenderStateVector* rsvp)
{
    if (rsvp != NULL) {
        if (RWSRCGLOBAL(engineStatus) == rwENGINESTATUSSTARTED) {
            *rsvp = RXPIPELINEGLOBAL(defaultRenderState);
            return rsvp;
        }

        if (rsvp != &RXPIPELINEGLOBAL(defaultRenderState)) {
            RWERROR((E_RX_ENGINENOTSTARTED));
            return NULL;
        }

        {
            const RwRGBA white = { 255, 255, 255, 255 };

            rsvp->Flags = rxRENDERSTATEFLAG_TEXTUREPERSPECTIVE | rxRENDERSTATEFLAG_ZTESTENABLE |
                          rxRENDERSTATEFLAG_ZWRITEENABLE;
            rsvp->ShadeMode = 2;
            rsvp->SrcBlend = 5;
            rsvp->DestBlend = 6;
            rsvp->TextureRaster = NULL;
            rsvp->AddressModeU = 1;
            rsvp->AddressModeV = 1;
            rsvp->FilterMode = 2;
            rsvp->BorderColor = white;
            rsvp->FogType = 0;
            rsvp->FogColor = white;
            rsvp->FogTable = NULL;
        }
        return rsvp;
    }

    RWERROR((E_RX_INVALIDRENDERSTATEVECTOR));
    return NULL;
}

RxRenderStateVector* RxRenderStateVectorCreate(RwBool current)
{
    RxRenderStateVector* rsvp;

    if (RWSRCGLOBAL(engineStatus) == rwENGINESTATUSSTARTED) {
        rsvp = (RxRenderStateVector*)RwMalloc(sizeof(RxRenderStateVector));
        if (rsvp != NULL) {
            if (current) {
                if (RxRenderStateVectorLoadDriverState(rsvp) == NULL) {
                    RxRenderStateVectorDestroy(rsvp);
                }
            } else {
                *rsvp = RXPIPELINEGLOBAL(defaultRenderState);
            }
            return rsvp;
        }

        RWERROR((E_RW_NOMEM, sizeof(RxRenderStateVector)));
        return NULL;
    }

    RWERROR((E_RX_ENGINENOTSTARTED));
    return NULL;
}

void RxRenderStateVectorDestroy(RxRenderStateVector* rsvp)
{
    if (rsvp != NULL) {
        RwFree(rsvp);
        return;
    }

    RWERROR((E_RX_INVALIDRENDERSTATEVECTOR));
}

RxRenderStateVector* RxRenderStateVectorLoadDriverState(RxRenderStateVector* rsvp)
{
    RwInt32 state;

    if (rsvp != NULL) {
        rsvp->Flags = 0;

        RwRenderStateGet(rwRENDERSTATETEXTUREPERSPECTIVE, &state);
        if (state) {
            rsvp->Flags |= rxRENDERSTATEFLAG_TEXTUREPERSPECTIVE;
        }
        RwRenderStateGet(rwRENDERSTATEZTESTENABLE, &state);
        if (state) {
            rsvp->Flags |= rxRENDERSTATEFLAG_ZTESTENABLE;
        }
        RwRenderStateGet(rwRENDERSTATEZWRITEENABLE, &state);
        if (state) {
            rsvp->Flags |= rxRENDERSTATEFLAG_ZWRITEENABLE;
        }
        RwRenderStateGet(rwRENDERSTATEALPHAPRIMITIVEBUFFER, &state);
        if (state) {
            rsvp->Flags |= rxRENDERSTATEFLAG_ALPHAPRIMITIVEBUFFER;
        }
        RwRenderStateGet(rwRENDERSTATEVERTEXALPHAENABLE, &state);
        if (state) {
            rsvp->Flags |= rxRENDERSTATEFLAG_VERTEXALPHAENABLE;
        }

        RwRenderStateGet(rwRENDERSTATESHADEMODE, &rsvp->ShadeMode);
        RwRenderStateGet(rwRENDERSTATESRCBLEND, &rsvp->SrcBlend);
        RwRenderStateGet(rwRENDERSTATEDESTBLEND, &rsvp->DestBlend);
        RwRenderStateGet(rwRENDERSTATETEXTURERASTER, &rsvp->TextureRaster);
        if (RwRenderStateGet(rwRENDERSTATETEXTUREADDRESS, &rsvp->AddressModeU)) {
            rsvp->AddressModeV = rsvp->AddressModeU;
        } else {
            RwRenderStateGet(rwRENDERSTATETEXTUREADDRESSU, &rsvp->AddressModeU);
            RwRenderStateGet(rwRENDERSTATETEXTUREADDRESSV, &rsvp->AddressModeV);
        }
        RwRenderStateGet(rwRENDERSTATETEXTUREFILTER, &rsvp->FilterMode);
        RwRenderStateGet(rwRENDERSTATEBORDERCOLOR, &rsvp->BorderColor);
        RwRenderStateGet(rwRENDERSTATEFOGTYPE, &rsvp->FogType);
        RwRenderStateGet(rwRENDERSTATEFOGCOLOR, &rsvp->FogColor);
        RwRenderStateGet(rwRENDERSTATEFOGTABLE, &rsvp->FogTable);
        return rsvp;
    }

    RWERROR((E_RX_INVALIDRENDERSTATEVECTOR));
    return NULL;
}
