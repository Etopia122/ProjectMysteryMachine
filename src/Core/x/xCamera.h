#ifndef XCAMERA_H
#define XCAMERA_H

#include "xBase.h"
#include "xMath3.h"

class RwCamera;
class _xEnv;
class _xEnt;

// GameCube layouts: _xMat4x3 is only 4-aligned here, so everything after the
// first matrix sits 4 bytes lower than in the PS2 DWARF (_tagxCam 0xA0, frame 0x1F0).

enum _tagTransType
{
    eTransType_None = 0,
    eTransType_Interp1 = 1,
    eTransType_Interp2 = 2,
    eTransType_Interp3 = 3,
    eTransType_Interp4 = 4,
    eTransType_Linear = 5,
    eTransType_Interp1Rev = 6,
    eTransType_Interp2Rev = 7,
    eTransType_Interp3Rev = 8,
    eTransType_Interp4Rev = 9,
    eTransType_Total = 10,
};

enum _tagCamType
{
    eCamType_Follow = 0,
    eCamType_FollowWithOffset = 1,
    eCamType_Static = 2,
    eCamType_Path = 3,
    eCamType_StaticFollow = 4,
    eCamType_Shoulder = 5,
    eCamType_Total = 6,
};

class _tagxCamFollowAsset
{
public:
    float rotation;
    float distance;
    float height;
    float rubber_band;
    float start_speed;
    float end_speed;
};

class _tagxCamShoulderAsset
{
public:
    float distance;
    float height;
    float realign_speed;
    float realign_delay;
};

class _tagp2CamStaticAsset
{
public:
    unsigned int unused;
};

class _tagxCamPathAsset
{
public:
    unsigned int assetID;
    float time_end;
    float time_delay;
};

class _tagp2CamStaticFollowAsset
{
public:
    float rubber_band;
};

class xCamAsset : public p2BaseAsset
{
public:
    _xVec3 pos; // 0x8
    _xVec3 at; // 0x14
    _xVec3 up; // 0x20
    _xVec3 right; // 0x2C
    _xVec3 view_offset; // 0x38
    signed short offset_start_frames; // 0x44
    signed short offset_end_frames; // 0x46
    float fov; // 0x48
    float trans_time; // 0x4C
    _tagTransType trans_type; // 0x50
    unsigned int flags; // 0x54
    float fade_up; // 0x58
    float fade_down; // 0x5C
    union
    {
        _tagxCamFollowAsset cam_follow;
        _tagxCamShoulderAsset cam_shoulder;
        _tagp2CamStaticAsset cam_static;
        _tagxCamPathAsset cam_path;
        _tagp2CamStaticFollowAsset cam_staticFollow;
    }; // 0x60
    unsigned int valid_flags; // 0x78
    unsigned int markerid[2]; // 0x7C
    unsigned char cam_type; // 0x84
    unsigned char pad[3];
};

class _tagxCamFollow
{
public:
    float rotation;
    float distance;
    float height;
    float rubber_band;
    float start_speed;
    float end_speed;
};

class _tagxCamShoulder
{
public:
    float distance;
    float height;
    float realign_speed;
    float realign_delay;
    signed int state;
};

class _tagxCamPath
{
public:
    unsigned int assetID;
    float time_end;
};

class _tagxCamStatic
{
public:
    unsigned int unused;
};

class _tagxCamStaticFollow
{
public:
    float rubber_band;
};

class _tagCamInfo
{
public:
    union
    {
        _tagxCamFollow f;
        _tagxCamStatic s;
        _tagxCamShoulder sh;
        _tagxCamPath p;
        _tagxCamStaticFollow sf;
    };
};

class _tagxCam : public xBase
{
public:
    unsigned int valid_flags; // 0x10
    _tagCamType cam_type; // 0x14
    unsigned int flags; // 0x18
    _xMat4x3 mat; // 0x1C
    _xQuat rot; // 0x5C
    float fov; // 0x6C
    float trans_time; // 0x70
    _tagTransType trans_type; // 0x74
    _xVec3 view_offset; // 0x78
    _tagCamInfo u; // 0x84
    xCamAsset* asset; // 0x9C
};

class _tagxCamFrame
{
public:
    RwCamera* lo_cam; // 0x0
    _tagxCam orig_cam; // 0x4
    _tagxCam hi_cam; // 0xA4
    _xVec4 frustplane[8]; // 0x144
    _tagxCam* t_cam; // 0x1C4
    float t_time; // 0x1C8
    float g_time; // 0x1CC
    _xEnv* env; // 0x1D0
    _xVec3* target_pos; // 0x1D4
    _xMat4x3* target_frame; // 0x1D8
    _xVec3 pre_align_dir; // 0x1DC
    _tagxCam** cameras; // 0x1E8
    unsigned int total_cameras; // 0x1EC
};

_xMat4x3* xCameraGetCurMatrix(_tagxCamFrame* cam);
void xCameraTargetOnGround(signed int on_ground);
void xCameraPickInitial(_tagxCamFrame* cam, _tagxCam* c);
void xCameraPickInitial(_tagxCamFrame* cam, unsigned int init_id);
void xCameraNext(_tagxCamFrame* cam, _tagxCam* trans_cam);
void xCameraInterpValid(_tagxCamFrame* cam, _tagTransType trans_type, float seconds);
_tagxCam* xCameraSetAsset(_tagxCamFrame* cam, xCamAsset* src, signed int idx);
void xCameraInit(_tagxCamFrame* cam, unsigned int total);
void xCameraRemoveEnv(_tagxCamFrame* camera);
void xCameraAssignEnv(_tagxCamFrame* camera, _xEnv* env);
void xCameraSetTarget(_tagxCamFrame* cam, _xMat4x3* frame_target);
void xCameraSetTarget(_tagxCamFrame* cam, _xVec3* new_target);
void xCameraShowRaster(_tagxCamFrame* cam);
void xCameraEnd(_tagxCamFrame* cam, float seconds, signed int update_scrn_fx);
void xCameraUpdate(_tagxCamFrame* cam, float seconds);
void xCameraRunState(_tagxCamFrame* cam, float seconds);
_tagxCam* xCameraShoulderState(_tagxCamFrame* cam, float seconds);
_tagxCam* xCameraFollowState(_tagxCamFrame* cam, float seconds);
void xCameraSetController(_tagxCamFrame* cam, float x, float y, float seconds);
void xCameraSetTargetDY(_tagxCamFrame* cam, float dy, float seconds);
void xCameraSetTargetDX(_tagxCamFrame* cam, float dx, float seconds);
_tagxCam* xCameraFollowWithOffsetState(_tagxCamFrame* cam, float seconds);
void xCameraBegin(_tagxCamFrame* cam, signed int clear);
void xCameraDestroy(_tagxCamFrame* cam);
void xCameraCreate(_tagxCamFrame* cam, unsigned int width, unsigned int height);

#endif
