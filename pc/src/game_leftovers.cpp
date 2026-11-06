#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// Small game functions that are still unsplit assembly in the decomp
// (fn_XXXXXXXX gaps). Written from the GameCube assembly. Anything the decomp
// has since implemented is gone from here: its own copy is used instead.

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

// ---------------------------------------------------------------- camera easing
// Camera ease curves: a*t^3 + b*t - c*t^2 (constants from .sdata2)
static float ease(float t, float a, float b, float c) { return a * t * t * t + (b * t - c * t * t); }
extern "C" float fn_80029458(float t) { return ease(t, 1.1692308187484741f, 0.38564103841781616f, 0.5548717975616455f); }
extern "C" float fn_8002947C(float t) { return ease(t, 1.3727272748947144f, 3.149545431137085f, 3.522272825241089f); }
extern "C" float fn_800294A0(float t) { return ease(t, 0.523809552192688f, 2.459523916244507f, 1.9833333492279053f); }
extern "C" float fn_800294C4(float t) { return ease(t, 0.41830065846443176f, 1.8470587730407715f, 1.2653595209121704f); }

// ---------------------------------------------------------------- debugging
extern "C" void pc_log(const char* fmt, ...);
class _zPortal;
void zSceneSwitch(_zPortal* p);
extern unsigned char globals[];

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
