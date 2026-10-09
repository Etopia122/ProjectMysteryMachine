#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// Small game functions that are still unsplit assembly in the decomp
// (fn_XXXXXXXX gaps). Written from the GameCube assembly.

class xMorphSeqFile
{
public:
    unsigned int Magic;
    unsigned int Flags;
    unsigned int TimeCount;
    unsigned int ModelCount;
};

// 0x80030F28: duration = last entry of the time list that follows the header.
float xMorphSeqDuration(xMorphSeqFile* seq)
{
    return ((float*)(seq + 1))[seq->TimeCount - 1];
}

// ---------------------------------------------------------------- iBoxForModel (0x80007738/0x800077C0)
class _xVec3
{
public:
    float x, y, z;
};
class _xBox
{
public:
    _xVec3 upper, lower;
};
class xModelInstance;
void iBoxInitBoundVec(_xBox* b, const _xVec3* v);
void iBoxBoundVec(_xBox* o, const _xBox* b, const _xVec3* v);

struct PcMorphTarget { void* parentGeom; float sphere[4]; _xVec3* verts; _xVec3* normals; };
struct PcGeometryView { unsigned char pad[0x14]; int numVertices; int numMorphTargets; unsigned char pad2[0x68 - 0x1C]; PcMorphTarget* morphTarget; };
struct PcAtomicView { unsigned char pad[0x18]; PcGeometryView* geometry; };
struct PcModelView { unsigned char pad[0x10]; PcAtomicView* Data; unsigned char pad2[0x20 - 0x14]; float* Mat; };

// Box around the model's vertices in model space.
extern "C" void fn_80007738(void* bound, xModelInstance* model);
extern "C" void fn_800077C0(void* bound, xModelInstance* model);

void fn_80007738(void* bound, xModelInstance* model)
{
    _xBox* box = (_xBox*)bound;
    PcGeometryView* geom = ((PcModelView*)model)->Data->geometry;
    if (geom->numMorphTargets >= 1)
    {
        _xVec3* verts = geom->morphTarget->verts;
        iBoxInitBoundVec(box, &verts[0]);
        for (unsigned short i = 1; i < geom->numVertices; i++)
        {
            iBoxBoundVec(box, box, &verts[i]);
        }
    }
}

// Same, offset by the model's position.
void fn_800077C0(void* bound, xModelInstance* model)
{
    _xBox* box = (_xBox*)bound;
    fn_80007738(bound, model);
    float* mat = ((PcModelView*)model)->Mat;
    box->upper.x += mat[12];
    box->upper.y += mat[13];
    box->upper.z += mat[14];
    box->lower.x += mat[12];
    box->lower.y += mat[13];
    box->lower.z += mat[14];
}

float iurand();
void iParMgrRender();
static void pc_call_iParMgrRender() { iParMgrRender(); }

// ---------------------------------------------------------------- volumes (0x800055E0-0x80005C74)
class xBase;
class xSerial;
class p2BaseAsset;
void xBaseInit(xBase* xb, p2BaseAsset* asset);
void xBaseReset(xBase* xb, p2BaseAsset* asset);
void xBaseLoad(xBase* ent, xSerial* s);
void xBaseSave(xBase* ent, xSerial* s);
unsigned int xSTAssetCountByType(unsigned int type);
void* xSTFindAssetByType(unsigned int type, int idx, unsigned int* size);
void* xMemAlloc(unsigned int heapID, unsigned int size);
extern unsigned int gActiveHeap;

struct PcVolume // _zVolume: xBase (0xC) + asset
{
    unsigned int id;
    unsigned char baseType, linkCount;
    unsigned short baseFlags;
    void* link;
    void* eventFunc;
    unsigned char* asset;
};

static PcVolume* s_volumes;           // lbl_8025E860
static unsigned short s_volumeCount;  // lbl_8025E864
static xBase* s_occluders[10];        // 0x801DF1C0
extern "C" { signed int lbl_8025E868; } // occluder count

// zVolumeEventCB (0x800055E0)
static int zVolumeEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam, xBase* toParamWidget)
{
    switch (toEvent)
    {
    case 10:
        xBaseReset(to, (p2BaseAsset*)((PcVolume*)to)->asset);
        break;
    case 0x108: // occluder on
    {
        int n = lbl_8025E868;
        if (n == 10)
        {
            return 1;
        }
        for (int i = 0; i < n; i++)
        {
            if (s_occluders[i] == to)
            {
                return 1;
            }
        }
        s_occluders[n] = to;
        lbl_8025E868 = n + 1;
        break;
    }
    case 0x109: // occluder off (the original drops the last entry, not the matched one)
    {
        int n = lbl_8025E868;
        for (int i = 0; i < n; i++)
        {
            if (s_occluders[i] == to)
            {
                lbl_8025E868 = n - 1;
                s_occluders[n] = s_occluders[n - 1];
                return 1;
            }
        }
        break;
    }
    }
    return 1;
}

extern "C" {

// zVolumeInit
void fn_80005B58()
{
    s_volumeCount = (unsigned short)xSTAssetCountByType('VOLU');
    if (!s_volumeCount)
    {
        s_volumes = 0;
        return;
    }
    s_volumes = (PcVolume*)xMemAlloc(gActiveHeap, s_volumeCount * sizeof(PcVolume));
    for (unsigned short i = 0; i < s_volumeCount; i++)
    {
        unsigned int size;
        unsigned char* asset = (unsigned char*)xSTFindAssetByType('VOLU', i, &size);
        PcVolume* v = &s_volumes[i];
        xBaseInit((xBase*)v, (p2BaseAsset*)asset);
        v->eventFunc = (void*)zVolumeEventCB;
        v->asset = asset;
        v->link = v->linkCount ? asset + 0x50 : 0;
    }
}

// zVolumeGetVolume
void* fn_80005AE8(unsigned short idx) { return &s_volumes[idx]; }

// bound of a volume (asset + 0xC)
void* fn_80005AFC(void* vol) { return ((PcVolume*)vol)->asset + 0xC; }

// zVolumeSetup: reset the cached partition indices of every volume
void fn_80005B08()
{
    for (unsigned int i = 0; i < s_volumeCount; i++)
    {
        unsigned char* a = s_volumes[i].asset;
        *(int*)(a + 0x40) = -1;
        *(int*)(a + 0x3C) = -1;
        *(int*)(a + 0x38) = -1;
    }
}

void fn_80005C34(xBase* b, xSerial* s) { xBaseLoad(b, s); }
void fn_80005C54(xBase* b, xSerial* s) { xBaseSave(b, s); }

// Occluder plane precalc from the camera position (0x800056F8). TODO: port the
// real plane setup; with no planes nothing is occluded (everything is drawn).
signed int lbl_8025E86C;
void fn_800056F8(void* pos) { lbl_8025E86C = 0; }

// ---------------------------------------------------------------- misc
void fn_80021B1C() {} // xDebugExit
void fn_80021B20() {}

// Camera ease curves: a*t^3 + b*t - c*t^2 (constants from .sdata2)
static float ease(float t, float a, float b, float c) { return a * t * t * t + (b * t - c * t * t); }
float fn_80029458(float t) { return ease(t, 1.1692308187484741f, 0.38564103841781616f, 0.5548717975616455f); }
float fn_8002947C(float t) { return ease(t, 1.3727272748947144f, 3.149545431137085f, 3.522272825241089f); }
float fn_800294A0(float t) { return ease(t, 0.523809552192688f, 2.459523916244507f, 1.9833333492279053f); }
float fn_800294C4(float t) { return ease(t, 0.41830065846443176f, 1.8470587730407715f, 1.2653595209121704f); }

void fn_80032D28() { pc_call_iParMgrRender(); }

// Is a screen fade active (mFade.active, xScrFx)
extern int mFade;
signed int fn_800394B4() { return mFade; }

// Random step (0..359) whose magnitude differs from the previous one by more than 10;
// *pitch gets step / 360 * scale, truncated. (range is unused, as in the original.)
signed short fn_800C751C(signed short old, float* pitch, signed int range, float scale)
{
    int n;
    float o = __builtin_fabsf((float)old);
    do
    {
        n = (int)(360.0f * iurand());
    } while (o >= __builtin_fabsf((float)n) - 10.0f && o <= __builtin_fabsf((float)n) + 10.0f);
    *pitch = (float)(int)((float)n / 360.0f * scale);
    return (signed short)n;
}

}

// ---------------------------------------------------------------- cutscenes and FMV
class xCutscene;
signed int xCutscene_Destroy(xCutscene* csn) { return 1; }
void xCutscene_Init(void* toc) {}

enum eFMVFile
{
    eFMVFile_Dummy
};
static const char* const s_fmvNames[41] = {
    "HILogo", "Intro", "demo/demo1", "demo/demo2", "Intro", "demo/demo1", "demo/demo2", "Intro", "demo/demo1",
    "demo/demo2", "Intro", "demo/demo1", "demo/demo2", "Intro", "demo/demo1", "demo/demo2", "Intro", "THQLogo",
    "WBLogo", "SDTrailr", "HWTrailr", "HILogo", "HILogo", "HILogo", "Credits1", "Credits2", "RW4", "cin1", "cin2",
    "cin3", "cin4", "cin5", "cin6", "cin7", "cin8", "cin9", "cin10", "cin11", "cin_mmpd", "cin_mmsc", "cin_shbr",
};
char* zFMVFileGetName(eFMVFile f) { return (char*)s_fmvNames[(unsigned)f < 41 ? (unsigned)f : 0]; }

int xStricmp(char* a, char* b);
unsigned int iFMVPlay(char* filename, unsigned int buttons, float time);
void xSndSuspend();
void xSndResume();
extern "C" signed int lbl_8025EEC8;
struct PcSceneView { unsigned char pad[0x6C]; unsigned int sceneID; };
extern unsigned char globals[];

unsigned int zFMVPlay(char* filename, unsigned int buttons, float time)
{
    char path[64];
    sprintf(path, "%s/%s%s", "FMV", filename, ".bik");
    xSndSuspend();
    unsigned int result = iFMVPlay(path, buttons, time);
    xSndResume();
    if (!xStricmp(filename, (char*)"Credits2"))
    {
        PcSceneView* scene = *(PcSceneView**)(globals + 0xB64);
        if (scene && scene->sceneID == 'S005')
        {
            lbl_8025EEC8 = 1;
        }
    }
    return result;
}

// ---------------------------------------------------------------- debugging
extern "C" void pc_log(const char* fmt, ...);
class _zPortal;
void zSceneSwitch(_zPortal* p);

// Debug (PC_PORTAL=<sec>:<index>): takes the index-th portal of the current
// scene, as if the player walked into it. Logs the scene's portals.
extern "C" void pc_debug_portal(int index)
{
    unsigned char* scene = *(unsigned char**)(globals + 0xB64);
    if (!scene)
    {
        return;
    }
    const int PORTAL_TYPE = 16, PORTAL_SIZE = 0x14;
    unsigned int count = ((unsigned int*)(scene + 0x7C))[PORTAL_TYPE];
    unsigned char* list = ((unsigned char**)(scene + 0x128))[PORTAL_TYPE];
    for (unsigned int i = 0; i < count; i++)
    {
        unsigned char* asset = *(unsigned char**)(list + i * PORTAL_SIZE + 0x10);
        pc_log("[portal] %u: to %.4s", i, asset ? (char*)asset + 0x14 : "?");
    }
    if (index >= 0 && (unsigned int)index < count)
    {
        zSceneSwitch((_zPortal*)(list + index * PORTAL_SIZE));
    }
}

// Debug (PC_SCENE=<id>): the scene New Game starts in, e.g. I001.
unsigned int pc_debug_start_scene(unsigned int sceneID)
{
    const char* s = getenv("PC_SCENE");
    if (s && strlen(s) == 4)
    {
        static bool used;
        if (!used)
        {
            used = true; // only the first game started
            sceneID = (unsigned int)s[0] << 24 | (unsigned int)s[1] << 16 | (unsigned int)s[2] << 8 | (unsigned char)s[3];
            pc_log("[debug] starting in scene %s", s);
        }
    }
    return sceneID;
}

// Debug (PC_SURFDUMP=<sec>): lists the world materials that map to special
// surfaces (friction, sticky, type) with the bounds of the floor using them.
class xSurface;
xSurface* zSurfaceGetSurface(unsigned int mat_id);
struct RpWorld;
extern "C" RpWorld* pc_current_world();
extern "C" int pc_world_material_bounds(RpWorld* w, int mat, float* out6);
extern "C" int pc_world_max_window_base(RpWorld* w);

extern "C" void pc_debug_surfaces()
{
    RpWorld* w = pc_current_world();
    if (!w)
    {
        return;
    }
    int nmat = *(int*)((unsigned char*)w + 0x14); // matList.numMaterials
    pc_log("[surf] %d world materials, max window base %d", nmat, pc_world_max_window_base(w));
    for (int i = 0; i < nmat; i++)
    {
        unsigned char* surf = (unsigned char*)zSurfaceGetSurface((unsigned int)i);
        unsigned char* props = *(unsigned char**)(surf + 0x24);
        unsigned char* asset = props ? *(unsigned char**)props : NULL;
        if (!asset)
        {
            continue;
        }
        float b[6] = { 0 };
        int n = pc_world_material_bounds(w, i, b);
        pc_log("[surf] mat %d -> surf %08x friction %.2f sticky %d type %d flags %x dmg %d: %d polys (%.1f %.1f %.1f)-(%.1f %.1f %.1f)",
               i, *(unsigned int*)surf, *(float*)(asset + 0x10), asset[9], asset[0xB], asset[0xF], asset[8], n, b[0], b[1],
               b[2], b[3], b[4], b[5]);
    }
}
