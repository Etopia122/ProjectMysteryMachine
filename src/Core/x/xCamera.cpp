#include "xCamera.h"
#include "xCollide.h"
#include "xEnt.h"
#include "xEnv.h"
#include "xMath.h"
#include "xMemMgr.h"
#include "xstransvc.h"
#include "iMath.h"
#include "zGlobals.h"

// Functions are in reverse DOL order (-inline deferred emits last to first).
extern "C" void* memcpy(void* dst, const void* src, unsigned int n);
extern "C" void* memset(void* dst, int c, unsigned int n);
extern "C" double acos(double);

class _xScene;
#ifndef NULL
#define NULL 0
#endif
class iEnv;

class p2MarkerAsset
{
public:
    _xVec3 pos;
};

void xMat3x3LookVec(_xMat3x3* m, _xVec3* at);
void xMat3x3RotC(_xMat3x3* m, float x, float y, float z, float t);
void xMat4x3Orthonormalize(_xMat4x3* o, _xMat4x3* m);
void xMat4x3MoveLocalRight(_xMat4x3* m, float mag);
void xMat4x3MoveLocalAt(_xMat4x3* m, float mag);
void xMat4x3MoveLocalUp(_xMat4x3* m, float mag);
float xVec3Normalize(_xVec3* o, _xVec3* v);
void xQuatFromMat(_xQuat* q, _xMat3x3* m);
void xQuatToMat(_xQuat* q, _xMat3x3* m);
void xQuatNormalize(_xQuat* o, _xQuat* q);
void xQuatSlerp(_xQuat* o, _xQuat* a, _xQuat* b, float t);
void xRayHitsCameraScene(_xScene* sc, _xRay3* r, void (*cb)(_xEnt*));
void iRayHitsCamEnv(const _xRay3* r, const _xEnv* env, _xCollis* coll);
RwCamera* iCameraCreate(int width, int height);
void iCameraBegin(RwCamera* cam, signed int clear);
void iCameraEnd(RwCamera* cam);
void iCameraShowRaster(RwCamera* cam);
void iCameraUpdatePos(RwCamera* cam, _xMat4x3* mat);
void iCameraSetFOV(RwCamera* cam, float fov);
void iCameraAssignEnv(RwCamera* cam, iEnv* env);
void iCameraRemoveEnv(RwCamera* cam, iEnv* env);
unsigned int iCameraFrustumPlanes(RwCamera* cam, _xVec4* frustplane);
void iCameraUpdateFog(RwCamera* cam, long long t);
void xScrFxUpdate(RwCamera* cam, float dt);
signed int zGameIsPaused();

// Interpolation curves (GameCube-only code after xFFX).
float fn_80029458(float t);
float fn_8002947C(float t);
float fn_800294A0(float t);
float fn_800294C4(float t);

extern _xScene* g_xSceneCur;
extern unsigned int gTransitionSceneID;
extern signed int lbl_8025ECC0;
extern signed int lbl_8025ECC4;
extern _xVec3 lbl_80229088;
extern _xVec3 lbl_80229094;

#define SCENE_ID(s) ((((s)[0] & ~0x20) << 24) | ((s)[1] << 16) | ((s)[2] << 8) | (s)[3])

static _xVec3 sTargetPos = { 0.0f, 0.0f, 0.0f };
static _xVec3 sLastTargetPos = { 0.0f, 0.0f, 0.0f };
static _xVec3 sLastCamPos = { 0.0f, 0.0f, 0.0f };
static signed int sOnGround;
static float sGroundHeight;
static float sCurXOffset;
static float sCurYOffset;
static signed int sControllerStill;
float gCameraLastFov;
static unsigned int sCamDebugFixInterp = 1;
_xVec3 sDummyTargetPos = { 0.0f, 0.0f, 0.0f };
_xMat4x3 sDummyTargetFrame;

inline void xMat4x3Copy(_xMat4x3* o, const _xMat4x3* m)
{
    memcpy(o, m, sizeof(_xMat4x3));
}

inline float xQuatDot(const _xQuat* a, const _xQuat* b)
{
    return xVec3Dot(&a->v, &b->v) + a->s * b->s;
}

inline void xQuatFlip(_xQuat* o, const _xQuat* q)
{
    o->s = -q->s;
    xVec3SMul(&o->v, &q->v, -1.0f);
}

inline void xQuatCopy(_xQuat* o, const _xQuat* q)
{
    o->s = q->s;
    o->v.x = q->v.x;
    o->v.y = q->v.y;
    o->v.z = q->v.z;
}

inline float xCamLerp(float a, float b, float t)
{
    if (a == b)
    {
        return a;
    }
    float r = b - a;
    r *= t;
    r += a;
    return r;
}

inline float xCamClamp(float x, float max)
{
    return x < max ? x : max;
}

static inline _tagxCam* xCameraCur(_tagxCamFrame* cam)
{
    _tagxCam* c = &cam->hi_cam;
    if (cam->t_cam)
    {
        c = cam->t_cam;
    }
    return c;
}

static inline _tagxCam* xCameraCurT(_tagxCamFrame* cam)
{
    return cam->t_cam ? cam->t_cam : &cam->hi_cam;
}

static inline void xCameraSetTargetFrame(_tagxCamFrame* cam, _xMat4x3* frame_target)
{
    cam->target_frame = frame_target;
}

static inline void xCameraSetTargetPos(_tagxCamFrame* cam, _xVec3* new_target)
{
    sLastTargetPos = sTargetPos;
    cam->target_pos = new_target;
    sTargetPos = *new_target;
}

static inline void xCameraResetState(_tagxCamFrame* cam, _tagxCam* c)
{
    switch (c->cam_type)
    {
    case eCamType_Follow:
    case eCamType_FollowWithOffset:
    case eCamType_Static:
    case eCamType_Path:
    case eCamType_StaticFollow:
        break;
    case eCamType_Shoulder:
        c->u.sh.state = 0;
        cam->pre_align_dir.x = 0.0f;
        cam->pre_align_dir.y = 0.0f;
        cam->pre_align_dir.z = 0.0f;
        break;
    }
}

static inline _tagxCam* xCameraFind(_tagxCamFrame* cam, unsigned int id)
{
    for (signed int i = 0; i < (signed int)cam->total_cameras; i++)
    {
        _tagxCam* c = cam->cameras[i];
        if (c->id == id)
        {
            return c;
        }
    }
    return NULL;
}

static void xCameraSeeThruCallback(_xEnt* ent);

static inline void xCameraAdjustTargetPos(_tagxCamFrame* cam)
{
    _tagxCam* c = xCameraCur(cam);

    if (c->flags & 0x40)
    {
        _xVec3 desired_target_pos = *cam->target_pos;
        if (!sOnGround)
        {
            if (desired_target_pos.y > sGroundHeight)
            {
                desired_target_pos.y = sGroundHeight;
            }
            else
            {
                sGroundHeight = desired_target_pos.y;
            }
        }
        else
        {
            sGroundHeight = desired_target_pos.y;
        }
        _xVec3 cur_pos = sTargetPos;
        sTargetPos.x = xCamLerp(cur_pos.x, desired_target_pos.x, 0.5f);
        sTargetPos.y = xCamLerp(cur_pos.y, desired_target_pos.y, 0.5f);
        sTargetPos.z = xCamLerp(cur_pos.z, desired_target_pos.z, 0.5f);
    }
    else
    {
        sTargetPos = *cam->target_pos;
    }

    if (lbl_8025ECC4)
    {
        sTargetPos = lbl_80229094;
    }
    if (lbl_8025ECC0)
    {
        sTargetPos = lbl_80229088;
    }
}

static inline void xCameraSeeThru(_tagxCamFrame* cam, _tagxCam* c)
{
    _xCollis collis;
    _xRay3 ray;
    _xMat3x3 rot;

    memset(&collis, 0, sizeof(_xCollis));
    ray.origin = c->mat.pos;
    ray.dir.x = sTargetPos.x - ray.origin.x;
    ray.dir.y = sTargetPos.y - ray.origin.y;
    ray.dir.z = sTargetPos.z - ray.origin.z;
    ray.max_t = xVec3Normalize(&ray.dir, &ray.dir);
    ray.min_t = 0.0f;
    ray.flags = 0xC00;
    ray.dir = cam->hi_cam.mat.at;
    xRayHitsCameraScene(g_xSceneCur, &ray, xCameraSeeThruCallback);

    {
        _xVec3 offset;
        xVec3SMul(&offset, &cam->hi_cam.mat.right, 0.5f);
        xMat3x3RotC(&rot, ray.dir.x, ray.dir.y, ray.dir.z, 6.2831855f * iurand());
        xMat3x3RMulVec(&offset, &rot, &offset);
        xVec3AddTo(&ray.dir, &offset);
        ray.max_t *= xVec3Normalize(&ray.dir, &ray.dir);
        xRayHitsCameraScene(g_xSceneCur, &ray, xCameraSeeThruCallback);
    }
}

static inline _tagxCam* xCameraStaticFollow(_tagxCamFrame* cam)
{
    _tagxCam* c = xCameraCurT(cam);
    _xVec3 dir;
    dir.x = c->mat.pos.x - sTargetPos.x;
    dir.y = c->mat.pos.y - sTargetPos.y;
    dir.z = c->mat.pos.z - sTargetPos.z;
    xMat3x3LookVec((_xMat3x3*)&c->mat, &dir);
    c->mat.pos = c->asset->pos;
    return c;
}

static void ShoulderStateRealign(_tagxCamFrame* cam, _tagxCam* c, float seconds);
static void ShoulderStateFollow(_tagxCamFrame* cam, _tagxCam* c, float seconds);
static void ShoulderStatePause(_tagxCamFrame* cam, _tagxCam* c, float seconds);

static inline void ShoulderGetDesiredPos(_tagxCam* c, _xVec3* pos, float x, float z)
{
    xVec3Copy(pos, &sTargetPos);
    pos->y += c->u.sh.height;
    pos->x -= x * c->u.sh.distance;
    pos->z -= z * c->u.sh.distance;
}

static inline void ShoulderMoveTo(_tagxCam* c, _xVec3* desired_pos, float t)
{
    c->mat.pos.x = xCamLerp(c->mat.pos.x, desired_pos->x, t);
    c->mat.pos.y = xCamLerp(c->mat.pos.y, desired_pos->y, t);
    c->mat.pos.z = xCamLerp(c->mat.pos.z, desired_pos->z, t);
}

static inline void xMat3x3LookAtPos(_xMat4x3* m, const _xVec3* at)
{
    _xVec3 vec;
    vec.x = m->pos.x - at->x;
    vec.y = m->pos.y - at->y;
    vec.z = m->pos.z - at->z;
    xMat3x3LookVec((_xMat3x3*)m, &vec);
}

static inline void ShoulderLookAtTarget(_tagxCamFrame* cam, _tagxCam* c)
{
    _xVec3 target_with_y = sTargetPos;
    target_with_y.y += cam->hi_cam.view_offset.y;
    xMat3x3LookAtPos(&c->mat, &target_with_y);
}

static inline unsigned int xCameraEntIsSeeThru(_xEnt* ent)
{
    if (!ent->asset)
    {
        return 0;
    }
    if (ent->asset->seeThru >= 1.0f)
    {
        return 0;
    }
    return 1;
}

static inline void xVec2NormalizeXZ(float& x, float& z)
{
    float len2 = x * x + z * z;
    if (len2 != 1.0f && len2 != 0.0f)
    {
        float inv = 1.0f / xsqrt(len2);
        x *= inv;
        z *= inv;
    }
}

static inline signed int xCamFPClassify(float x)
{
    switch (*(signed int*)&x & 0x7F800000)
    {
    case 0x7F800000:
        if (*(signed int*)&x & 0x7FFFFF)
        {
            return 1;
        }
        return 2;
    case 0:
        if (*(signed int*)&x & 0x7FFFFF)
        {
            return 5;
        }
        return 3;
    }
    return 4;
}

static inline void GetPointOnLine(_xVec3* C, unsigned int marker1id, unsigned int marker2id,
                                  _xVec3* result)
{
    _xVec3 AB;
    _xVec3 normAB;
    p2MarkerAsset* m1 = (p2MarkerAsset*)xSTFindAsset(marker1id, NULL);
    p2MarkerAsset* m2 = (p2MarkerAsset*)xSTFindAsset(marker2id, NULL);

    AB.x = m2->pos.x - m1->pos.x;
    AB.y = m2->pos.y - m1->pos.y;
    AB.z = m2->pos.z - m1->pos.z;
    _xVec3 AC;
    AC.x = C->x - m1->pos.x;
    AC.y = C->y - m1->pos.y;
    AC.z = C->z - m1->pos.z;
    float dot = xVec3Dot(&AC, &AB);
    float len = xVec3Normalize(&normAB, &AB);
    float r = dot / (len * len);
    if (xCamFPClassify(r) == 1)
    {
        r = 0.0f;
    }

    if (r <= 0.0f)
    {
        *result = m1->pos;
    }
    else if (r >= 1.0f)
    {
        *result = m2->pos;
    }
    else
    {
        r *= len;
        result->x = r * normAB.x + m1->pos.x;
        result->y = r * normAB.y + m1->pos.y;
        result->z = r * normAB.z + m1->pos.z;
    }
}

void xCameraCreate(_tagxCamFrame* cam, unsigned int width, unsigned int height)
{
    static signed int first_time = 1;

    if (first_time)
    {
        first_time = 0;
        cam->lo_cam = iCameraCreate(width, height);
        xMat4x3Copy(&cam->hi_cam.mat, (const _xMat4x3*)&g_I3);
        cam->t_cam = NULL;
    }
}

void xCameraDestroy(_tagxCamFrame* cam)
{
}

void xCameraBegin(_tagxCamFrame* cam, signed int clear)
{
    iCameraBegin(cam->lo_cam, clear);
    iCameraFrustumPlanes(cam->lo_cam, cam->frustplane);
    iCameraUpdateFog(cam->lo_cam, 0);
}

_tagxCam* xCameraFollowWithOffsetState(_tagxCamFrame* cam, float seconds)
{
    _xMat3x3 mrot;
    _xVec3 to_vec;
    _xVec3 dirvec;
    _xVec3 tmp;
    _xVec3 new_pos;
    _tagxCam* c;

    if (cam->t_cam)
    {
        c = cam->t_cam;
    }
    else
    {
        c = &cam->hi_cam;
    }

    dirvec.x = 0.0f;
    dirvec.y = c->u.f.height;
    dirvec.z = c->u.f.distance;
    xMat3x3Euler(&mrot, (PI * c->u.f.rotation) / 180.0f, 0.0f, 0.0f);
    xMat3x3LMulVec(&dirvec, &mrot, &dirvec);
    xVec3Add(&new_pos, &sTargetPos, &dirvec);
    xVec3Sub(&to_vec, &new_pos, &sTargetPos);
    xMat3x3LookVec((_xMat3x3*)&c->mat, &to_vec);

    tmp = c->mat.right;
    xVec3Normalize(&tmp, &tmp);
    xVec3SMulBy(&tmp, sCurXOffset);
    xVec3AddTo(&new_pos, &tmp);
    tmp = c->mat.up;
    xVec3Normalize(&tmp, &tmp);
    xVec3SMulBy(&tmp, cam->hi_cam.view_offset.y);
    xVec3AddTo(&new_pos, &tmp);
    tmp = to_vec;
    xVec3Normalize(&tmp, &tmp);
    xVec3SMulBy(&tmp, cam->hi_cam.view_offset.z);
    xVec3AddTo(&new_pos, &tmp);

    signed int trans = 0;
    if (cam->t_cam || seconds < 0.0f)
    {
        trans = 1;
    }
    float cpa = trans ? 1.0f : c->u.f.rubber_band;
    unsigned int flags = c->flags;
    float cpb = 1.0f - cpa;

    _xVec3 preline_test_pos = c->mat.pos;
    _xVec3* p = &preline_test_pos;
    if (!(flags & 0x1))
    {
        p->x = new_pos.x * cpa + c->mat.pos.x * cpb;
    }
    if (!(flags & 0x2))
    {
        p->y = new_pos.y * cpa + c->mat.pos.y * cpb;
    }
    if (!(flags & 0x4))
    {
        p->z = new_pos.z * cpa + c->mat.pos.z * cpb;
    }

    if (c->asset->markerid[0] && c->asset->markerid[1])
    {
        GetPointOnLine(p, c->asset->markerid[0], c->asset->markerid[1],
                       &c->mat.pos);
    }
    else
    {
        c->mat.pos = *p;
    }

    return c;
}

signed int sDXLeft;
signed int sDXRight;
signed int sDYUp;
signed int sDYDown;

void xCameraSetTargetDX(_tagxCamFrame* cam, float dx, float seconds)
{
    float start;
    float end;
    float desired;
    _tagxCam* c = xCameraCur(cam);

    if (c->cam_type == eCamType_FollowWithOffset)
    {
        start = c->u.f.start_speed * seconds;
        end = c->u.f.end_speed * seconds;
    }
    else if (c->cam_type == eCamType_Shoulder)
    {
        start = 0.04f;
        end = 0.07f;
    }
    else
    {
        return;
    }

    if (dx < -32.0f || dx > 32.0f)
    {
        if (dx < 0.0f)
        {
            sDXRight = 0;
            sDXLeft++;
            if (sDXLeft >= c->asset->offset_start_frames)
            {
                desired = -c->view_offset.x;
            }
            else
            {
                return;
            }
        }
        else
        {
            sDXLeft = 0;
            sDXRight++;
            if (sDXRight >= c->asset->offset_start_frames)
            {
                desired = c->view_offset.x;
            }
            else
            {
                return;
            }
        }

        if (sDXRight > c->asset->offset_end_frames)
        {
            sDXRight = c->asset->offset_end_frames;
        }
        if (sDXLeft > c->asset->offset_end_frames)
        {
            sDXLeft = c->asset->offset_end_frames;
        }

        if (desired > sCurXOffset)
        {
            if (sDXRight >= c->asset->offset_end_frames || sDXLeft >= c->asset->offset_end_frames)
            {
                sCurXOffset += end;
            }
            else
            {
                sCurXOffset += start;
            }
            if (desired < sCurXOffset)
            {
                sCurXOffset = desired;
            }
        }
        else if (desired < sCurXOffset)
        {
            if (sDXRight >= c->asset->offset_end_frames || sDXLeft >= c->asset->offset_end_frames)
            {
                sCurXOffset -= end;
            }
            else
            {
                sCurXOffset -= start;
            }
            if (desired > sCurXOffset)
            {
                sCurXOffset = desired;
            }
        }
    }
    else
    {
        return;
    }
}

void xCameraSetTargetDY(_tagxCamFrame* cam, float dy, float seconds)
{
    float start;
    float end;
    float desired;
    _tagxCam* c = xCameraCur(cam);

    if (c->cam_type == eCamType_FollowWithOffset)
    {
        start = c->u.f.start_speed * seconds;
        end = c->u.f.end_speed * seconds;
    }
    else if (c->cam_type == eCamType_Shoulder)
    {
        start = 0.04f;
        end = 0.07f;
    }
    else
    {
        return;
    }

    if (dy < -32.0f || dy > 32.0f)
    {
        if (dy < 0.0f)
        {
            sDYDown = 0;
            sDYUp++;
            if (sDYUp >= c->asset->offset_start_frames)
            {
                desired = -c->view_offset.z;
            }
            else
            {
                return;
            }
        }
        else
        {
            sDYUp = 0;
            sDYDown++;
            if (sDYDown >= c->asset->offset_start_frames)
            {
                desired = c->view_offset.z;
            }
            else
            {
                return;
            }
        }

        if (sDYDown > c->asset->offset_end_frames)
        {
            sDYDown = c->asset->offset_end_frames;
        }
        if (sDYUp > c->asset->offset_end_frames)
        {
            sDYUp = c->asset->offset_end_frames;
        }

        if (desired > sCurYOffset)
        {
            if (sDYDown >= c->asset->offset_end_frames || sDYUp >= c->asset->offset_end_frames)
            {
                sCurYOffset += end;
            }
            else
            {
                sCurYOffset += start;
            }
            if (desired < sCurYOffset)
            {
                sCurYOffset = desired;
            }
        }
        else if (desired < sCurYOffset)
        {
            if (sDYDown >= c->asset->offset_end_frames || sDYUp >= c->asset->offset_end_frames)
            {
                sCurYOffset -= end;
            }
            else
            {
                sCurYOffset -= start;
            }
            if (desired > sCurYOffset)
            {
                sCurYOffset = desired;
            }
        }
    }
    else
    {
        return;
    }
}

void xCameraSetController(_tagxCamFrame* cam, float x, float y, float seconds)
{
    xCameraSetTargetDX(cam, x, seconds);
    xCameraSetTargetDY(cam, y, seconds);
    if (x > 32.0f || x < -32.0f || y > 32.0f || y < -32.0f)
    {
        sControllerStill = 0;
    }
    else
    {
        sControllerStill++;
    }
}

_tagxCam* xCameraFollowState(_tagxCamFrame* cam, float seconds)
{
    _xMat3x3 mrot;
    _xVec3 to_vec;
    _xVec3 dirvec;
    _xVec3 tmp;
    _xVec3 new_pos;
    _tagxCam* c;

    if (cam->t_cam)
    {
        c = cam->t_cam;
    }
    else
    {
        c = &cam->hi_cam;
    }

    dirvec.x = 0.0f;
    dirvec.y = c->u.f.height;
    dirvec.z = c->u.f.distance;
    xMat3x3Euler(&mrot, (PI * c->u.f.rotation) / 180.0f, 0.0f, 0.0f);
    xMat3x3LMulVec(&dirvec, &mrot, &dirvec);
    xVec3Add(&new_pos, &sTargetPos, &dirvec);
    xVec3Sub(&to_vec, &new_pos, &sTargetPos);
    xMat3x3LookVec((_xMat3x3*)&c->mat, &to_vec);

    tmp = c->mat.right;
    xVec3Normalize(&tmp, &tmp);
    xVec3SMulBy(&tmp, cam->hi_cam.view_offset.x);
    xVec3AddTo(&new_pos, &tmp);
    tmp = c->mat.up;
    xVec3Normalize(&tmp, &tmp);
    xVec3SMulBy(&tmp, cam->hi_cam.view_offset.y);
    xVec3AddTo(&new_pos, &tmp);
    tmp = to_vec;
    xVec3Normalize(&tmp, &tmp);
    xVec3SMulBy(&tmp, cam->hi_cam.view_offset.z);
    xVec3AddTo(&new_pos, &tmp);

    signed int trans = 0;
    if (cam->t_cam || seconds < 0.0f)
    {
        trans = 1;
    }
    float cpa = trans ? 1.0f : c->u.f.rubber_band;
    unsigned int flags = c->flags;
    float cpb = 1.0f - cpa;

    _xVec3 preline_test_pos = c->mat.pos;
    _xVec3* p = &preline_test_pos;
    if (!(flags & 0x1))
    {
        p->x = new_pos.x * cpa + c->mat.pos.x * cpb;
    }
    if (!(flags & 0x2))
    {
        p->y = new_pos.y * cpa + c->mat.pos.y * cpb;
    }
    if (!(flags & 0x4))
    {
        p->z = new_pos.z * cpa + c->mat.pos.z * cpb;
    }

    if (c->asset->markerid[0] && c->asset->markerid[1])
    {
        GetPointOnLine(p, c->asset->markerid[0], c->asset->markerid[1], &new_pos);
        c->mat.pos = new_pos;
    }
    else
    {
        c->mat.pos = *p;
    }

    return c;
}

static float sShoulderTimer;

static void ShoulderStatePause(_tagxCamFrame* cam, _tagxCam* c, float seconds)
{
    _xVec3 desired_pos;
    _xVec3 dir;
    dir.x = cam->hi_cam.mat.at.x;
    dir.z = cam->hi_cam.mat.at.z;
    xVec2NormalizeXZ(dir.x, dir.z);
    ShoulderGetDesiredPos(c, &desired_pos, dir.x, dir.z);
    ShoulderMoveTo(c, &desired_pos, 1.0f);
    ShoulderLookAtTarget(cam, c);

    if (sControllerStill > 0 && seconds > 0.0f)
    {
        sShoulderTimer = 0.0f;
        c->u.sh.state = 1;
    }
}

static void ShoulderStateFollow(_tagxCamFrame* cam, _tagxCam* c, float seconds)
{
    _xVec3 desired_pos;
    _xVec3 dir;
    dir.x = cam->hi_cam.mat.at.x;
    dir.z = cam->hi_cam.mat.at.z;
    xVec2NormalizeXZ(dir.x, dir.z);
    ShoulderGetDesiredPos(c, &desired_pos, dir.x, dir.z);
    ShoulderMoveTo(c, &desired_pos, 1.0f);

    sShoulderTimer += seconds;
    if (!sControllerStill)
    {
        c->u.sh.state = 0;
    }

    if (sShoulderTimer >= c->u.sh.realign_delay)
    {
        sShoulderTimer = c->u.sh.realign_delay;
        if (cam->pre_align_dir.x == 0.0f && cam->pre_align_dir.y == 0.0f &&
            cam->pre_align_dir.z == 0.0f)
        {
            cam->pre_align_dir.x = cam->target_frame->at.x;
            cam->pre_align_dir.y = cam->target_frame->at.y;
            cam->pre_align_dir.z = cam->target_frame->at.z;
            c->u.sh.state = 2;
            return;
        }
        _xVec3 pre;
        pre.x = cam->pre_align_dir.x;
        pre.z = cam->pre_align_dir.z;
        xVec2NormalizeXZ(pre.x, pre.z);
        _xVec3 at;
        at.x = cam->target_frame->at.x;
        at.z = cam->target_frame->at.z;
        xVec2NormalizeXZ(at.x, at.z);
        if (pre.x * at.x + pre.z * at.z <= 0.5f)
        {
            cam->pre_align_dir.x = cam->target_frame->at.x;
            cam->pre_align_dir.y = cam->target_frame->at.y;
            cam->pre_align_dir.z = cam->target_frame->at.z;
            c->u.sh.state = 2;
        }
    }
}

static void ShoulderStateRealign(_tagxCamFrame* cam, _tagxCam* c, float seconds)
{
    _xMat3x3 rot_matrix;
    _xVec3 vec;
    _xVec3 desired_pos;
    float rad_rot = (PI * c->u.sh.realign_speed) / 180.0f;
    float dp1 = cam->hi_cam.mat.at.x * cam->target_frame->right.x +
                cam->hi_cam.mat.at.z * cam->target_frame->right.z;

    if (dp1 < 0.0f)
    {
        xMat3x3Euler(&rot_matrix, -rad_rot, 0.0f, 0.0f);
    }
    else
    {
        xMat3x3Euler(&rot_matrix, rad_rot, 0.0f, 0.0f);
    }

    vec.x = cam->hi_cam.mat.at.x;
    vec.y = 0.0f;
    vec.z = cam->hi_cam.mat.at.z;
    xVec3Normalize(&vec, &vec);
    xMat3x3LMulVec(&vec, &rot_matrix, &vec);
    ShoulderGetDesiredPos(c, &desired_pos, vec.x, vec.z);
    ShoulderMoveTo(c, &desired_pos, 1.0f);
    ShoulderLookAtTarget(cam, c);

    float dp2 = cam->hi_cam.mat.at.x * cam->target_frame->right.x +
                cam->hi_cam.mat.at.z * cam->target_frame->right.z;

    if ((dp1 <= 0.0f && dp2 >= 0.0f) || (dp1 >= 0.0f && dp2 <= 0.0f))
    {
        ShoulderGetDesiredPos(c, &c->mat.pos, cam->target_frame->at.x, cam->target_frame->at.z);
        ShoulderLookAtTarget(cam, c);
        c->u.sh.state = 3;
    }

    if (!sControllerStill)
    {
        c->u.sh.state = 0;
    }
}

// TODO: 88.6% - the scene-ID load is scheduled later than in the target (same as
// zLobMaster ZLOB_fadeSetup); the SCENE_ID expression shape is probably different.
static void xCameraSeeThruCallback(_xEnt* ent)
{
    if (xCameraEntIsSeeThru(ent) && ent->baseType != 3)
    {
        ent->flags |= 4;
        ent->fade_tmr = 1.25f;
        if ((gTransitionSceneID ^ 0x20000000) == SCENE_ID("o004") &&
            (ent->id == 0xF933F89E || ent->id == 0xFFF2B7AA))
        {
            ent->fade_tmr = 0.25f;
        }
    }
}

// TODO: 99.8% - case 3 stack slots: target puts target_with_y above dir (0x20/0x14/0x8).
_tagxCam* xCameraShoulderState(_tagxCamFrame* cam, float seconds)
{
    static _xMat4x3 old_matrix = { 0.0f, 0.0f, 0.0f };
    _xCollis collis_env;
    _xMat4x3 target_pos_matrix;
    _xMat4x3 desired_pos_matrix;
    _xRay3 ray;
    _tagxCam* c;

    if (cam->t_cam)
    {
        c = cam->t_cam;
    }
    else
    {
        c = &cam->hi_cam;
    }

    if (old_matrix.at.x + old_matrix.at.y + old_matrix.at.z != 0.0f)
    {
        xMat4x3Copy(&c->mat, &old_matrix);
    }

    switch (c->u.sh.state)
    {
    case 0:
        ShoulderStatePause(cam, c, seconds);
        break;
    case 1:
        ShoulderStateFollow(cam, c, seconds);
        break;
    case 2:
        ShoulderStateRealign(cam, c, seconds);
        break;
    case 3:
    {
        _xVec3 desired_pos;
        _xVec3 dir;
        dir.x = cam->hi_cam.mat.at.x;
        dir.y = 0.0f;
        dir.z = cam->hi_cam.mat.at.z;
        xVec3Normalize(&dir, &dir);
        ShoulderGetDesiredPos(c, &desired_pos, dir.x, dir.z);
        ShoulderMoveTo(c, &desired_pos, 0.2f);
        ShoulderLookAtTarget(cam, c);
        if (!sControllerStill)
        {
            c->u.sh.state = 0;
        }
        break;
    }
    }

    xMat4x3Copy(&old_matrix, &c->mat);

    if (c->flags & 0x80)
    {
        xMat4x3Copy(&target_pos_matrix, &c->mat);
        xVec3Copy(&target_pos_matrix.pos, &sTargetPos);
        target_pos_matrix.pos.y += cam->hi_cam.view_offset.y;
        xMat4x3Copy(&desired_pos_matrix, &c->mat);
        xMat4x3MoveLocalRight(&desired_pos_matrix, sCurXOffset);
        xMat4x3MoveLocalAt(&desired_pos_matrix, sCurYOffset);
        xMat4x3MoveLocalUp(&desired_pos_matrix, 0.25f * sCurYOffset);

        xVec3Copy(&ray.origin, &target_pos_matrix.pos);
        xVec3Sub(&ray.dir, &desired_pos_matrix.pos, &target_pos_matrix.pos);
        ray.max_t = xVec3Normalize(&ray.dir, &ray.dir);
        ray.min_t = 0.1f;
        ray.flags = 0xC00;
        memset(&collis_env, 0, sizeof(_xCollis));
        iRayHitsCamEnv(&ray, cam->env, &collis_env);

        if ((collis_env.flags & 0x1) && collis_env.dist < 1e19f)
        {
            collis_env.dist -= 0.1f;
            if (collis_env.dist < 0.0f)
            {
                collis_env.dist = 0.0f;
            }
            xMat4x3Copy(&c->mat, &target_pos_matrix);
            c->mat.pos.x += ray.dir.x * collis_env.dist;
            c->mat.pos.y += ray.dir.y * collis_env.dist;
            c->mat.pos.z += ray.dir.z * collis_env.dist;
            c->mat.pos.y += 1.5f * (1.0f - collis_env.dist / ray.max_t);
        }
        else
        {
            xMat4x3Copy(&c->mat, &desired_pos_matrix);
        }
    }
    else
    {
        xMat4x3MoveLocalRight(&c->mat, sCurXOffset);
        xMat4x3MoveLocalAt(&c->mat, sCurYOffset);
        xMat4x3MoveLocalUp(&c->mat, 0.25f * sCurYOffset);
    }

    return c;
}

void xCameraRunState(_tagxCamFrame* cam, float seconds)
{
    _tagxCam* c;

    if (seconds > 0.0f)
    {
        cam->g_time += seconds;
    }

    xCameraAdjustTargetPos(cam);

    switch (cam->hi_cam.cam_type)
    {
    case eCamType_Follow:
        c = xCameraFollowState(cam, seconds);
        break;
    case eCamType_FollowWithOffset:
        c = xCameraFollowWithOffsetState(cam, seconds);
        break;
    case eCamType_Static:
        c = xCameraCurT(cam);
        break;
    case eCamType_StaticFollow:
        c = xCameraStaticFollow(cam);
        break;
    case eCamType_Path:
        c = NULL;
        break;
    case eCamType_Shoulder:
        c = xCameraShoulderState(cam, seconds);
        break;
    }

    xCameraSeeThru(cam, c);
}

void xCameraUpdate(_tagxCamFrame* cam, float seconds)
{
    if (!zGameIsPaused())
    {
        if (cam->t_cam && cam->t_cam->trans_type)
        {
            xCameraInterpValid(cam, cam->t_cam->trans_type, seconds);
        }
        xCameraRunState(cam, seconds);
        iCameraUpdatePos(cam->lo_cam, &cam->hi_cam.mat);
        gCameraLastFov = cam->hi_cam.fov;
        iCameraSetFOV(cam->lo_cam, cam->hi_cam.fov);
        sLastCamPos = cam->hi_cam.mat.pos;
    }
}

void xCameraEnd(_tagxCamFrame* cam, float seconds, signed int update_scrn_fx)
{
    if (update_scrn_fx)
    {
        xScrFxUpdate(cam->lo_cam, seconds);
    }
    iCameraEnd(cam->lo_cam);
}

void xCameraShowRaster(_tagxCamFrame* cam)
{
    iCameraShowRaster(cam->lo_cam);
}

void xCameraSetTarget(_tagxCamFrame* cam, _xVec3* new_target)
{
    xCameraSetTargetPos(cam, new_target);
}

void xCameraSetTarget(_tagxCamFrame* cam, _xMat4x3* frame_target)
{
    xCameraSetTargetFrame(cam, frame_target);
}

void xCameraAssignEnv(_tagxCamFrame* camera, _xEnv* env)
{
    camera->env = env;
    iCameraAssignEnv(camera->lo_cam, *(iEnv**)camera->env);
}

void xCameraRemoveEnv(_tagxCamFrame* camera)
{
    iCameraRemoveEnv(camera->lo_cam, *(iEnv**)camera->env);
}

void xCameraInit(_tagxCamFrame* cam, unsigned int total)
{
    xCameraSetTargetPos(cam, &sDummyTargetPos);
    xCameraSetTargetFrame(cam, &sDummyTargetFrame);
    cam->total_cameras = total;
    cam->cameras = (_tagxCam**)xMemAlloc(gActiveHeap, cam->total_cameras * sizeof(_tagxCam*));
    _tagxCam* array = (_tagxCam*)xMemAlloc(gActiveHeap, cam->total_cameras * sizeof(_tagxCam));
    for (signed int i = 0; i < (signed int)total; i++)
    {
        cam->cameras[i] = &array[i];
    }
}

_tagxCam* xCameraSetAsset(_tagxCamFrame* cam, xCamAsset* src, signed int idx)
{
    _tagxCam* dst = cam->cameras[idx];

    dst->asset = src;
    dst->id = src->id;
    dst->link = (p2LinkAsset*)&src->baseType;
    dst->linkCount = src->linkCount;
    dst->baseType = src->baseType;
    dst->cam_type = (_tagCamType)src->cam_type;
    dst->mat.right = src->right;
    dst->mat.up = src->up;
    dst->mat.at = src->at;
    dst->mat.pos = src->pos;
    dst->fov = src->fov;
    xMat4x3Orthonormalize(&dst->mat, &dst->mat);
    dst->trans_time = src->trans_time;
    dst->trans_type = src->trans_type;
    dst->flags = src->flags;
    dst->view_offset.x = -1.0f * src->view_offset.x;
    dst->view_offset.y = src->view_offset.y;
    dst->view_offset.z = -1.0f * src->view_offset.z;
    dst->valid_flags = 0x100C0;

    switch (src->cam_type)
    {
    case eCamType_FollowWithOffset:
        dst->valid_flags |= 0x400000;
        dst->u.f.start_speed = src->cam_follow.start_speed;
        dst->u.f.end_speed = src->cam_follow.end_speed;
    case eCamType_Follow:
        dst->u.f.rotation = src->cam_follow.rotation;
        dst->u.f.distance = src->cam_follow.distance;
        dst->u.f.height = src->cam_follow.height;
        dst->u.f.rubber_band = xCamClamp(src->cam_follow.rubber_band, 0.5f);
        dst->valid_flags |= 0x30F;
        dst->view_offset.z = 0.0f;
        break;
    case eCamType_Shoulder:
        dst->u.sh.distance = src->cam_shoulder.distance;
        dst->u.sh.height = src->cam_shoulder.height;
        dst->u.sh.realign_speed = src->cam_shoulder.realign_speed;
        dst->u.sh.realign_delay = src->cam_shoulder.realign_delay;
        dst->valid_flags |= 0x1E0300;
        break;
    case eCamType_StaticFollow:
        dst->u.sf.rubber_band = xCamClamp(src->cam_staticFollow.rubber_band, 0.5f);
        dst->valid_flags |= 0x30F;
        dst->view_offset.z = 0.0f;
        break;
    case eCamType_Static:
        dst->u.s.unused = src->cam_static.unused;
        dst->valid_flags |= 0x300;
        dst->view_offset.z = 0.0f;
        break;
    case eCamType_Path:
        dst->u.p.assetID = src->cam_path.assetID;
        dst->valid_flags |= 0x300;
        dst->view_offset.z = 0.0f;
        break;
    }

    return cam->cameras[idx];
}

void xCameraInterpValid(_tagxCamFrame* cam, _tagTransType trans_type, float seconds)
{
    _xQuat cur_fm;
    _xQuat dst_fm;
    float t;
    _tagxCam* orig = &cam->orig_cam;
    _tagxCam* dest = cam->t_cam;

    if (seconds < 0.0f)
    {
        t = 1.0f;
    }
    else
    {
        t = cam->t_time / dest->trans_time;
    }

    if (t >= 1.0f)
    {
        t = 1.0f;
        cam->t_time = 0.0f;
        cam->t_cam = NULL;
    }
    else
    {
        cam->t_time += seconds;
    }

    switch (trans_type)
    {
    case eTransType_Interp1:
        t = fn_800294C4(t);
        break;
    case eTransType_Interp2:
        t = fn_800294A0(t);
        break;
    case eTransType_Interp3:
        t = fn_8002947C(t);
        break;
    case eTransType_Interp4:
        t = fn_80029458(t);
        break;
    case eTransType_Interp1Rev:
        t = 1.0f - fn_800294C4(1.0f - t);
        break;
    case eTransType_Interp2Rev:
        t = 1.0f - fn_800294A0(1.0f - t);
        break;
    case eTransType_Interp3Rev:
        t = 1.0f - fn_8002947C(1.0f - t);
        break;
    case eTransType_Interp4Rev:
        t = 1.0f - fn_80029458(1.0f - t);
        break;
    }

    if (dest->cam_type == orig->cam_type)
    {
        if (dest->valid_flags & 0x1)
        {
            cam->hi_cam.u.f.rotation = xCamLerp(orig->u.f.rotation, dest->u.f.rotation, t);
        }
        if (dest->valid_flags & 0x2)
        {
            cam->hi_cam.u.f.distance = xCamLerp(orig->u.f.distance, dest->u.f.distance, t);
        }
        if (dest->valid_flags & 0x4)
        {
            cam->hi_cam.u.f.height = xCamLerp(orig->u.f.height, dest->u.f.height, t);
        }
        if (dest->valid_flags & 0x8)
        {
            cam->hi_cam.u.f.rubber_band =
                xCamLerp(orig->u.f.rubber_band, dest->u.f.rubber_band, t);
        }
    }

    if (dest->valid_flags & 0x20000)
    {
        cam->hi_cam.u.sh.distance = xCamLerp(orig->u.sh.distance, dest->u.sh.distance, t);
    }
    if (dest->valid_flags & 0x40000)
    {
        cam->hi_cam.u.sh.height = xCamLerp(orig->u.sh.height, dest->u.sh.height, t);
    }
    if (dest->valid_flags & 0x80000)
    {
        cam->hi_cam.u.sh.realign_speed =
            xCamLerp(orig->u.sh.realign_speed, dest->u.sh.realign_speed, t);
    }
    if (dest->valid_flags & 0x100000)
    {
        cam->hi_cam.u.sh.realign_delay =
            xCamLerp(orig->u.sh.realign_delay, dest->u.sh.realign_delay, t);
    }

    if (dest->valid_flags & 0x40)
    {
        cam->hi_cam.mat.pos.x = xCamLerp(orig->mat.pos.x, dest->mat.pos.x, t);
        cam->hi_cam.mat.pos.y = xCamLerp(orig->mat.pos.y, dest->mat.pos.y, t);
        cam->hi_cam.mat.pos.z = xCamLerp(orig->mat.pos.z, dest->mat.pos.z, t);
    }

    if (dest->valid_flags & 0x80)
    {
        if (t != 1.0f)
        {
            if (sCamDebugFixInterp)
            {
                xQuatFromMat(&dest->rot, (_xMat3x3*)&dest->mat);
                xQuatNormalize(&dest->rot, &dest->rot);
                if (xQuatDot(&dest->rot, &cam->hi_cam.rot) < 0.0f)
                {
                    xQuatFlip(&dest->rot, &dest->rot);
                }
                xQuatSlerp(&cam->hi_cam.rot, &orig->rot, &dest->rot, t);
                xQuatToMat(&cam->hi_cam.rot, (_xMat3x3*)&cam->hi_cam.mat);
            }
            else
            {
                _xQuat q;
                _xQuat b;
                _xQuat a;
                float x = cam->hi_cam.mat.pos.x;
                float y = cam->hi_cam.mat.pos.y;
                float z = cam->hi_cam.mat.pos.z;
                xQuatFromMat(&a, (_xMat3x3*)&orig->mat);
                xQuatFromMat(&b, (_xMat3x3*)&dest->mat);
                xQuatSlerp(&q, &a, &b, t);
                xQuatToMat(&q, (_xMat3x3*)&cam->hi_cam.mat);
                cam->hi_cam.mat.pos.x = x;
                cam->hi_cam.mat.pos.y = y;
                cam->hi_cam.mat.pos.z = z;
            }
        }
        else
        {
            acos(xQuatDot(&cam->hi_cam.rot, &dest->rot));
            xQuatFromMat(&cur_fm, (_xMat3x3*)&cam->hi_cam.mat);
            xQuatFromMat(&dst_fm, (_xMat3x3*)&dest->mat);
            acos(xQuatDot(&cur_fm, &dst_fm));
            acos(xQuatDot(&cur_fm, &cam->hi_cam.rot));
            acos(xQuatDot(&dst_fm, &dest->rot));
            cam->hi_cam.mat = dest->mat;
        }
    }

    if (dest->valid_flags & 0x100)
    {
        cam->hi_cam.fov = xCamLerp(orig->fov, dest->fov, t);
    }

    cam->hi_cam.view_offset.x = xCamLerp(orig->view_offset.x, dest->view_offset.x, t);
    cam->hi_cam.view_offset.y = xCamLerp(orig->view_offset.y, dest->view_offset.y, t);
    cam->hi_cam.view_offset.z = xCamLerp(orig->view_offset.z, dest->view_offset.z, t);
}

void xCameraNext(_tagxCamFrame* cam, _tagxCam* trans_cam)
{
    _xMat3x3 from_new_src;

    if (cam->hi_cam.asset->id == trans_cam->asset->id)
    {
        xSTAssetName(trans_cam->id);
        return;
    }

    xCameraResetState(cam, trans_cam);
    gCameraLastFov = 0.0f;
    sGroundHeight = sTargetPos.y;

    switch (trans_cam->trans_type)
    {
    case eTransType_None:
        memcpy(&cam->hi_cam, trans_cam, sizeof(_tagxCam));
        cam->t_cam = NULL;
        cam->t_time = -1.0f;
        cam->g_time = 0.0f;
        xCameraRunState(cam, -1.0f);
        break;
    case eTransType_Interp1:
    case eTransType_Interp2:
    case eTransType_Interp3:
    case eTransType_Interp4:
    case eTransType_Linear:
    case eTransType_Interp1Rev:
    case eTransType_Interp2Rev:
    case eTransType_Interp3Rev:
    case eTransType_Interp4Rev:
        memcpy(&cam->orig_cam, &cam->hi_cam, sizeof(_tagxCam));
        memcpy(&cam->hi_cam, trans_cam, sizeof(_tagxCam));
        if (sCamDebugFixInterp)
        {
            _xQuat* dst_rot = &cam->hi_cam.rot;
            _xQuat* src_rot = &trans_cam->rot;
            cam->hi_cam.mat = cam->orig_cam.mat;
            xQuatFromMat(dst_rot, (_xMat3x3*)&cam->hi_cam.mat);
            xQuatFromMat(src_rot, (_xMat3x3*)&trans_cam->mat);
            xQuatNormalize(dst_rot, dst_rot);
            xQuatNormalize(src_rot, src_rot);
            if (xQuatDot(dst_rot, src_rot) < 0.0f)
            {
                xQuatFlip(dst_rot, dst_rot);
                xQuatToMat(dst_rot, &from_new_src);
            }
            xQuatCopy(&cam->orig_cam.rot, dst_rot);
        }
        cam->t_cam = trans_cam;
        cam->t_time = 0.0f;
        break;
    }
    cam->g_time = 0.0f;
}

void xCameraPickInitial(_tagxCamFrame* cam, unsigned int init_id)
{
    if (init_id)
    {
        xCameraPickInitial(cam, xCameraFind(cam, init_id));
    }
}

void xCameraPickInitial(_tagxCamFrame* cam, _tagxCam* c)
{
    xCameraSetTargetPos(cam, &globals.player.ent.frame->mat.pos);
    xCameraSetTargetFrame(cam, (_xMat4x3*)globals.player.ent.model->Mat);
    memcpy(&cam->hi_cam, c, sizeof(_tagxCam));
    cam->t_cam = NULL;
    cam->t_time = -1.0f;
    cam->g_time = 0.0f;
    xCameraResetState(cam, &cam->hi_cam);
    gCameraLastFov = 0.0f;
    sGroundHeight = sTargetPos.y;
    xCameraRunState(cam, -1.0f);
}

void xCameraTargetOnGround(signed int on_ground)
{
    sOnGround = on_ground;
}
_xMat4x3* xCameraGetCurMatrix(_tagxCamFrame* cam)
{
    return &cam->hi_cam.mat;
}