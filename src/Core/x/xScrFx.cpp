#include "xMath3.h"
#include "iDraw.h"
#include "zGlobals.h"
#include "rwtypes.h"

extern "C" {
void* memset(void* dst, int c, unsigned int n);
void* memcpy(void* dst, const void* src, unsigned int n);
}

class RwIm2DVertex
{
public:
    float x;                // 0x0
    float y;                // 0x4
    float z;                // 0x8
    unsigned char color[4]; // 0xC
    float u;                // 0x10
    float v;                // 0x14
    float recipZ;           // 0x18
};

class RwVideoMode
{
public:
    signed int width;  // 0x0
    signed int height; // 0x4
    signed int depth;  // 0x8
    signed int flags;  // 0xC
};

class _xFadeData
{
public:
    signed int active;    // 0x0
    signed int hold;      // 0x4
    iColor_tag src;       // 0x8
    iColor_tag dest;      // 0xC
    float time_passed;    // 0x10
    float time_total;     // 0x14
    void (*cb)();         // 0x18
    signed int fancy;     // 0x1C
};

// The player model as rendered by the fancy fade (body, then head).
class xModelInstanceView
{
public:
    xModelInstanceView* Next; // 0x0
    unsigned char pad4[0x20 - 0x4];
    _xMat4x3* Mat;            // 0x20
    _xVec3 Scale;             // 0x24
};

extern "C" {
signed int RwRenderStateSet(signed int state, void* value);
signed int RwEngineGetCurrentVideoMode();
RwVideoMode* RwEngineGetVideoModeInfo(RwVideoMode* modeinfo, signed int modeIndex);
signed int RwIm2DRenderIndexedPrimitive(signed int primType, RwIm2DVertex* vertices,
                                        signed int numVertices, unsigned short* indices,
                                        signed int numIndices);
RwCamera* RwCameraClear(RwCamera* camera, void* colour, signed int clearMode);
// TODO: unidentified (iEnv.cpp: z-buffer enable around the fancy fade)
void fn_8000A080(unsigned int mode);
// TODO: unidentified flag set while a scene loads
extern unsigned int lbl_8025EEC8;
}

extern const iColor_tag g_BLACK;

void iScrFxBegin();
void iScrFxEnd();
void iScrFxDrawFade(RwCamera* cam, iColor_tag* col);
void iScrFxDrawLetterbox(RwCamera* cam, signed int y_offset);
signed int zGameIsPaused();
signed int zMenuRunning();
void xModelRenderSingle(xModelInstance* modelInst);

#define PLAYER_MODEL ((xModelInstanceView*)globals.player.ent.model)

_xFadeData mFade;
static float mLetterboxO;
static float mLetterboxTO;

// Functions are in reverse DOL order (-inline deferred emits last to first).

void xScrFxInit()
{
    memset(&mFade, 0, sizeof(_xFadeData));
    mLetterboxO = 0.0f;
    mLetterboxTO = 0.0f;
}

void DrawScreenSizeRectangle()
{
    static RwIm2DVertex v[4];
    static unsigned short indices[4] = { 0, 1, 2, 3 };
    RwVideoMode video_mode;

    RwEngineGetVideoModeInfo(&video_mode, RwEngineGetCurrentVideoMode());
    v[0].x = 0.0f;
    v[0].y = 0.0f;
    v[0].z = 0.0f;
    v[1].x = video_mode.width;
    v[1].y = 0.0f;
    v[1].z = 0.0f;
    v[2].x = 0.0f;
    v[2].y = video_mode.height;
    v[2].z = 0.0f;
    v[3].x = video_mode.width;
    v[3].y = video_mode.height;
    v[3].z = 0.0f;
    v[0].color[0] = 0;
    v[0].color[1] = 0;
    v[0].color[2] = 0;
    v[0].color[3] = 255;
    v[1].color[0] = 0;
    v[1].color[1] = 0;
    v[1].color[2] = 0;
    v[1].color[3] = 255;
    v[2].color[0] = 0;
    v[2].color[1] = 0;
    v[2].color[2] = 0;
    v[2].color[3] = 255;
    v[3].color[0] = 0;
    v[3].color[1] = 0;
    v[3].color[2] = 0;
    v[3].color[3] = 255;
    RwIm2DRenderIndexedPrimitive(4, v, 4, indices, 4);
}

// TODO (99.2%): volatile float registers differ in the scale and colour math.
void xScrFxUpdate(RwCamera* cam, float seconds)
{
    if (lbl_8025EEC8)
    {
        iScrFxBegin();
        iScrFxDrawFade(cam, (iColor_tag*)&g_BLACK);
        iScrFxEnd();
        return;
    }

    iScrFxBegin();
    if (!zGameIsPaused())
    {
        if (mFade.active)
        {
            float t;
            if (mFade.hold == 2)
            {
                t = 1.0f;
            }
            else
            {
                if (mFade.time_passed == 0.0f)
                {
                    t = 0.0f;
                }
                else
                {
                    t = mFade.time_passed / mFade.time_total;
                }
                mFade.time_passed += seconds;
                if (t >= 1.0f)
                {
                    t = 1.0f;
                    if (mFade.hold == 1)
                    {
                        mFade.hold = 2;
                    }
                    else
                    {
                        mFade.active = 0;
                    }
                    if (mFade.cb)
                    {
                        mFade.cb();
                    }
                }
            }

            if (mFade.fancy)
            {
                signed int entering_scene = 0;
                if (mFade.src.a > mFade.dest.a)
                {
                    entering_scene = 1;
                }
                RwCameraClear(cam, 0, 2);
                RwRenderStateSet(6, (void*)0);
                RwRenderStateSet(8, (void*)1);
                RwRenderStateSet(10, (void*)1);
                RwRenderStateSet(11, (void*)1);
                if (!entering_scene && t >= 0.95f)
                {
                    DrawScreenSizeRectangle();
                }
                else
                {
                    float dog_y;
                    float dog_z;
                    float sc;
                    _xMat4x3 old;
                    _xMat4x3 old_head;

                    if (entering_scene)
                    {
                        sc = 300.0f * t;
                        dog_y = -5.5f;
                        dog_z = 25.0f;
                        PLAYER_MODEL->Scale.x = sc;
                        PLAYER_MODEL->Scale.y = sc;
                        PLAYER_MODEL->Scale.z = 1.0f;
                        PLAYER_MODEL->Next->Scale.x = sc;
                        PLAYER_MODEL->Next->Scale.y = sc;
                        PLAYER_MODEL->Next->Scale.z = 1.0f;
                    }
                    else
                    {
                        sc = (1.0f - t) * 300.0f;
                        dog_y = -20.5f;
                        dog_z = 25.0f;
                        PLAYER_MODEL->Scale.x = sc;
                        PLAYER_MODEL->Scale.y = sc;
                        PLAYER_MODEL->Scale.z = 1.0f;
                        PLAYER_MODEL->Next->Scale.x = sc;
                        PLAYER_MODEL->Next->Scale.y = sc;
                        PLAYER_MODEL->Next->Scale.z = 1.0f;
                    }

                    memcpy(&old, PLAYER_MODEL->Mat, sizeof(_xMat4x3));
                    memcpy(PLAYER_MODEL->Mat, &g_I3, sizeof(_xMat4x3));
                    PLAYER_MODEL->Mat->pos.y = dog_y;
                    PLAYER_MODEL->Mat->pos.z = dog_z;
                    memcpy(&old_head, PLAYER_MODEL->Next->Mat, sizeof(_xMat4x3));
                    memcpy(PLAYER_MODEL->Next->Mat, &g_I3, sizeof(_xMat4x3));
                    PLAYER_MODEL->Next->Mat->pos.y = dog_y;
                    PLAYER_MODEL->Next->Mat->pos.z = dog_z;
                    fn_8000A080(-1);
                    xModelRenderSingle((xModelInstance*)PLAYER_MODEL);
                    xModelRenderSingle((xModelInstance*)PLAYER_MODEL->Next);
                    memcpy(PLAYER_MODEL->Mat, &old, sizeof(_xMat4x3));
                    memcpy(PLAYER_MODEL->Next->Mat, &old_head, sizeof(_xMat4x3));
                    fn_8000A080(0);
                    PLAYER_MODEL->Scale.x = 1.0f;
                    PLAYER_MODEL->Scale.y = 1.0f;
                    PLAYER_MODEL->Scale.z = 1.0f;
                    PLAYER_MODEL->Next->Scale.x = 1.0f;
                    PLAYER_MODEL->Next->Scale.y = 1.0f;
                    PLAYER_MODEL->Next->Scale.z = 1.0f;
                    RwRenderStateSet(6, (void*)1);
                    RwRenderStateSet(8, (void*)0);
                    RwRenderStateSet(1, (void*)0);
                    DrawScreenSizeRectangle();
                }
            }
            else
            {
                iColor_tag col;
                float dr = (mFade.dest.r - mFade.src.r) * t;
                float dg = (mFade.dest.g - mFade.src.g) * t;
                float db = (mFade.dest.b - mFade.src.b) * t;
                float da = (mFade.dest.a - mFade.src.a) * t;
                col.r = dr + mFade.src.r;
                col.g = dg + mFade.src.g;
                col.b = db + mFade.src.b;
                col.a = da + mFade.src.a;
                iScrFxDrawFade(cam, &col);
            }
        }

        if (mLetterboxO < mLetterboxTO)
        {
            mLetterboxO += 100.0f * seconds;
            if (mLetterboxO > mLetterboxTO)
            {
                mLetterboxO = mLetterboxTO;
            }
        }
        else if (mLetterboxO > mLetterboxTO)
        {
            mLetterboxO -= 100.0f * seconds;
            if (mLetterboxO < mLetterboxTO)
            {
                mLetterboxO = mLetterboxTO;
            }
        }
    }

    if (mLetterboxO > 0.0f)
    {
        iScrFxDrawLetterbox(cam, mLetterboxO);
    }
    iScrFxEnd();
}

// TODO: the last argument is unused here (1 from most callers, 0 from zMenu).
void xScrFxFade(iColor_tag* base, iColor_tag* dest, float seconds, void (*callback)(),
                signed int hold, signed int unused)
{
    mFade.active = 1;
    mFade.src.r = base->r;
    mFade.src.g = base->g;
    mFade.src.b = base->b;
    mFade.src.a = base->a;
    mFade.dest.r = dest->r;
    mFade.dest.g = dest->g;
    mFade.dest.b = dest->b;
    mFade.dest.a = dest->a;
    mFade.time_passed = 0.0f;
    mFade.time_total = seconds;
    mFade.cb = callback;
    if (hold == 0)
    {
        mFade.hold = 0;
    }
    else
    {
        mFade.hold = 1;
    }
    mFade.fancy = 0;
}

void xScrFxLetterbox(signed int enable)
{
    if (zMenuRunning())
    {
        enable = 0;
    }
    mLetterboxTO = enable ? 16.0f : 0.0f;
}

signed int xScrFxIsLetterbox()
{
    return mLetterboxTO > 0.0f;
}

signed int xScrFxIsFading()
{
    return mFade.active;
}
