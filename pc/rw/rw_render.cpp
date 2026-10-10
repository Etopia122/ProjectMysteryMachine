// Shared RenderWare renderer: render states, cameras, lighting, skinning,
// world sectors, Im2D/Im3D and the debug overlay. Drawing goes through the
// graphics backend (gpu.h): OpenGL on PC, citro3d on the New 3DS.
#include "rw_internal.h"
#include "gpu.h"
#include "pc_platform.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Logical framebuffer the game renders to.
static const int FB_W = 640, FB_H = 480;

// Current RenderWare render states.
struct RenderStates
{
    RwRaster* raster;
    int addrU, addrV, filter;
    int zTest, zWrite;
    int srcBlend, dstBlend, vertexAlpha;
    int fogEnable;
    RwRGBA fogColor;
    int cullMode;
    int alphaFunc, alphaRef;
};
static RenderStates s_rs;

// GX state the game sets directly, outside RenderWare: colour/alpha writes
// (depth-only passes) and an alpha compare that replaces RenderWare's until
// the next RW alpha state flush (src/Core/gc/iEnv.cpp).
static bool s_colorUpdate = true, s_alphaUpdate = true;
static int s_gxAlphaFunc = -1, s_gxAlphaRef;

// Debug statistics (PC_STATS=1): draws per frame, printed every 120 frames.
static struct { int cams, atomics, atomicsDrawn, sectors, im2d, im3d, clears; } s_st;
static bool stats_on() { static int on = -1; if (on < 0) on = getenv("PC_STATS") != NULL; return on; }

// PC_PROF=1: per-second profile of the renderer's CPU work.
enum { PROF_SKIN, PROF_LIGHT, PROF_COLOUR, PROF_DRAW, PROF_WORLD, PROF_IM, PROF_COUNT };
static unsigned long long s_prof[PROF_COUNT];
// Vertex colour work per frame: lit/prelit meshes and vertices, light types seen.
static struct { int litMeshes, litVerts, prelitMeshes, prelitVerts, dirLights, pointLights; } s_profCol;
static bool prof_on() { static int on = -1; if (on < 0) on = getenv("PC_PROF") != NULL; return on; }
struct ProfScope
{
    int slot;
    unsigned long long t0;
    ProfScope(int s) : slot(s), t0(prof_on() ? pc_perf_counter() : 0) {}
    ~ProfScope()
    {
        if (t0)
        {
            s_prof[slot] += pc_perf_counter() - t0;
        }
    }
};

static void prof_frame()
{
    static unsigned long long start;
    static int frames;
    unsigned long long now = pc_perf_counter();
    if (!start)
    {
        start = now;
    }
    frames++;
    double secs = pc_perf_seconds(now - start);
    if (secs < 1.0)
    {
        return;
    }
    double ms[PROF_COUNT];
    for (int i = 0; i < PROF_COUNT; i++)
    {
        ms[i] = pc_perf_seconds(s_prof[i]) * 1000.0 / frames;
        s_prof[i] = 0;
    }
    pc_log("[prof] %.1f fps, %.1f ms/frame: skin %.1f light %.1f colour %.1f draw %.1f (world %.1f) im2d/3d %.1f",
           frames / secs, secs * 1000.0 / frames, ms[PROF_SKIN], ms[PROF_LIGHT], ms[PROF_COLOUR], ms[PROF_DRAW],
           ms[PROF_WORLD], ms[PROF_IM]);
    pc_log("[prof] colours/frame: lit %d meshes %d verts, prelit %d meshes %d verts; lights dir %d point/spot %d",
           s_profCol.litMeshes / frames, s_profCol.litVerts / frames, s_profCol.prelitMeshes / frames,
           s_profCol.prelitVerts / frames, s_profCol.dirLights / frames, s_profCol.pointLights / frames);
    memset(&s_profCol, 0, sizeof(s_profCol));
    start = now;
    frames = 0;
}

// PC_DBG=... render toggles.
static bool dbg_has(const char* what)
{
    static const char* dbg = getenv("PC_DBG");
    return dbg && strstr(dbg, what);
}

static bool s_open;

void pc_rw_device_open(void* displayID)
{
    if (s_open)
    {
        return;
    }
    s_open = gpu_open();
    s_rs.zTest = s_rs.zWrite = 1;
    s_rs.srcBlend = 5;
    s_rs.dstBlend = 6;
    s_rs.vertexAlpha = 0;
    s_rs.cullMode = 1;
    s_rs.filter = 2;
    s_rs.addrU = s_rs.addrV = 1;
    s_rs.alphaFunc = 5; // greater
    s_rs.alphaRef = 0;
}

void pc_rw_device_close()
{
    if (s_open)
    {
        gpu_close();
    }
    s_open = false;
}

void pc_render_raster_free(PcRaster* r)
{
    if (s_open)
    {
        gpu_raster_free(r);
    }
}

void pc_raster_scan_alpha(PcRaster* p)
{
    p->hasAlpha = 0;
    for (size_t i = 3, n = (size_t)p->r.width * p->r.height * 4; p->rgba && i < n; i += 4)
    {
        if (p->rgba[i] != 255)
        {
            p->hasAlpha = 1;
            break;
        }
    }
}

// ---------------------------------------------------------------- render state
// RenderWare blends when vertex alpha is enabled or the texture has alpha
// (call after the raster is prepared, which uploads it).
static bool raster_has_alpha(RwRaster* r)
{
    if (!r)
    {
        return false;
    }
    PcRaster* p = pc_raster(r);
    return p->isTarget || p->hasAlpha;
}

// The texture to draw with, uploaded; NULL when there is none to use.
static PcRaster* use_texture(RwRaster* r)
{
    if (!r || dbg_has("notex"))
    {
        return NULL;
    }
    PcRaster* p = pc_raster(r);
    gpu_prepare_texture(p);
    return p->rgba || p->isTarget ? p : NULL;
}

// Resolves the current RenderWare states for a draw. 'alpha' forces blending on.
static GpuState draw_state(bool alpha, PcRaster* tex, int filter, int addrU, int addrV)
{
    GpuState s;
    s.zTest = s_rs.zTest && !dbg_has("nodepth");
    s.zWrite = s_rs.zWrite != 0;
    s.blend = (alpha || s_rs.vertexAlpha) && !dbg_has("noblend");
    s.srcBlend = s_rs.srcBlend;
    s.dstBlend = s_rs.dstBlend;
    s.colorWrite = s_colorUpdate;
    s.alphaWrite = s_alphaUpdate;
    if (s_gxAlphaFunc >= 0)
    {
        s.alphaFunc = s_gxAlphaFunc;
        s.alphaRef = s_gxAlphaRef;
    }
    else
    {
        s.alphaFunc = s_rs.alphaFunc;
        s.alphaRef = s_rs.alphaRef;
    }
    if (dbg_has("noat"))
    {
        s.alphaFunc = PC_ALPHA_ALWAYS;
    }
    s.cull = dbg_has("nocull") ? PC_CULL_NONE : s_rs.cullMode;
    s.fog = s_rs.fogEnable && !dbg_has("nofog");
    s.fogColor = s_rs.fogColor;
    RwCamera* cam = rwGlobals()->curCamera;
    s.fogStart = cam ? cam->fogPlane : 0.0f;
    s.fogEnd = cam ? cam->farPlane : 1.0f;
    s.tex = tex;
    s.filter = filter;
    s.addrU = addrU;
    s.addrV = addrV;
    s.matColor = RwRGBA{ 255, 255, 255, 255 };
    s.addColor = RwRGBA{ 0, 0, 0, 0 };
    return s;
}

extern "C" {

RwInt32 RwRenderStateSet(RwInt32 state, void* value)
{
    intptr_t v = (intptr_t)value;
    switch (state)
    {
    case rwRENDERSTATETEXTURERASTER: s_rs.raster = (RwRaster*)value; break;
    case rwRENDERSTATETEXTUREADDRESS: s_rs.addrU = s_rs.addrV = (int)v; break;
    case rwRENDERSTATETEXTUREADDRESSU: s_rs.addrU = (int)v; break;
    case rwRENDERSTATETEXTUREADDRESSV: s_rs.addrV = (int)v; break;
    case rwRENDERSTATEZTESTENABLE: s_rs.zTest = (int)v; break;
    case rwRENDERSTATEZWRITEENABLE: s_rs.zWrite = (int)v; break;
    case rwRENDERSTATETEXTUREFILTER: s_rs.filter = (int)v; break;
    case rwRENDERSTATESRCBLEND: s_rs.srcBlend = (int)v; break;
    case rwRENDERSTATEDESTBLEND: s_rs.dstBlend = (int)v; break;
    case rwRENDERSTATEVERTEXALPHAENABLE: s_rs.vertexAlpha = (int)v; break;
    case rwRENDERSTATEFOGENABLE: s_rs.fogEnable = (int)v; break;
    case rwRENDERSTATEFOGCOLOR: memcpy(&s_rs.fogColor, &v, 4); break;
    case rwRENDERSTATECULLMODE: s_rs.cullMode = (int)v; break;
    case rwRENDERSTATEALPHATESTFUNCTION: s_rs.alphaFunc = (int)v; break;
    case rwRENDERSTATEALPHATESTFUNCTIONREF: s_rs.alphaRef = (int)v; break;
    default: break;
    }
    return 1;
}

RwInt32 RwRenderStateGet(RwInt32 state, void* value)
{
    intptr_t v = 0;
    switch (state)
    {
    case rwRENDERSTATETEXTURERASTER: *(RwRaster**)value = s_rs.raster; return 1;
    case rwRENDERSTATETEXTUREADDRESS: case rwRENDERSTATETEXTUREADDRESSU: v = s_rs.addrU; break;
    case rwRENDERSTATETEXTUREADDRESSV: v = s_rs.addrV; break;
    case rwRENDERSTATEZTESTENABLE: v = s_rs.zTest; break;
    case rwRENDERSTATEZWRITEENABLE: v = s_rs.zWrite; break;
    case rwRENDERSTATETEXTUREFILTER: v = s_rs.filter; break;
    case rwRENDERSTATESRCBLEND: v = s_rs.srcBlend; break;
    case rwRENDERSTATEDESTBLEND: v = s_rs.dstBlend; break;
    case rwRENDERSTATEVERTEXALPHAENABLE: v = s_rs.vertexAlpha; break;
    case rwRENDERSTATEFOGENABLE: v = s_rs.fogEnable; break;
    case rwRENDERSTATEFOGCOLOR: memcpy(&v, &s_rs.fogColor, 4); break;
    case rwRENDERSTATECULLMODE: v = s_rs.cullMode; break;
    case rwRENDERSTATEALPHATESTFUNCTION: v = s_rs.alphaFunc; break;
    case rwRENDERSTATEALPHATESTFUNCTIONREF: v = s_rs.alphaRef; break;
    default: break;
    }
    *(RwInt32*)value = (RwInt32)v;
    return 1;
}

RwInt32 RxRenderStateVectorSetDefaultRenderStateVector(RxRenderStateVector* r)
{
    memset(r, 0, sizeof(*r));
    r->Flags = 0x7;
    r->ShadeMode = 2;
    r->SrcBlend = 5;
    r->DestBlend = 6;
    r->AddressModeU = r->AddressModeV = 1;
    r->FilterMode = 2;
    r->FogType = 1;
    return 1;
}

}

// Debug: PC_TRACE_AT=<seconds> logs the next ~60 clears/camera begins/presents.
static bool trace_frame()
{
    static const char* at = getenv("PC_TRACE_AT");
    static int left = 200;
    return at && left > 0 && pc_ticks_ms() >= (unsigned)(atof(at) * 1000) && left--;
}

// ---------------------------------------------------------------- matrices
static void matrix16(const RwMatrix* m, float* o)
{
    o[0] = m->right.x; o[1] = m->right.y; o[2] = m->right.z; o[3] = 0;
    o[4] = m->up.x; o[5] = m->up.y; o[6] = m->up.z; o[7] = 0;
    o[8] = m->at.x; o[9] = m->at.y; o[10] = m->at.z; o[11] = 0;
    o[12] = m->pos.x; o[13] = m->pos.y; o[14] = m->pos.z; o[15] = 1;
}

// o = a * b (column-major).
static void mul16(const float* a, const float* b, float* o)
{
    for (int c = 0; c < 4; c++)
    {
        for (int r = 0; r < 4; r++)
        {
            o[c * 4 + r] = a[r] * b[c * 4] + a[4 + r] * b[c * 4 + 1] + a[8 + r] * b[c * 4 + 2] + a[12 + r] * b[c * 4 + 3];
        }
    }
}

static const float kIdentity[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };

static void ortho(float* p, float l, float r, float b, float t)
{
    memset(p, 0, 16 * sizeof(float));
    p[0] = 2.0f / (r - l);
    p[5] = 2.0f / (t - b);
    p[10] = -1.0f;
    p[12] = -(r + l) / (r - l);
    p[13] = -(t + b) / (t - b);
    p[15] = 1.0f;
}

// ---------------------------------------------------------------- cameras
static float s_view[16];
static float s_proj[16];
static RwCamera* s_cam;

static void load_projection(RwCamera* c)
{
    float vx = c->viewWindow.x, vy = c->viewWindow.y, n = c->nearPlane, f = c->farPlane;
    float ox = c->viewOffset.x, oy = c->viewOffset.y;
    float* p = s_proj;
    memset(p, 0, 16 * sizeof(float));
    if (c->projectionType == rwPARALLEL)
    {
        p[0] = 1.0f / vx;
        p[5] = 1.0f / vy;
        p[10] = 2.0f / (f - n);
        p[12] = -ox / vx;
        p[13] = oy / vy;
        p[14] = -(f + n) / (f - n);
        p[15] = 1.0f;
    }
    else
    {
        // Camera space looks down +Z (x already flipped to point right).
        p[0] = 1.0f / vx;
        p[5] = 1.0f / vy;
        // View offset, as RenderWare applies it: the window moves by the offset at
        // distance 1 and the eye with it, so points shift by offset * (1 - 1/z).
        // A positive offset moves the image right and up (the HUD's xModelRender2D
        // puts a model on the axis and offsets it to its screen spot).
        p[8] = ox / vx;
        p[9] = oy / vy;
        p[12] = -ox / vx;
        p[13] = -oy / vy;
        p[10] = (f + n) / (f - n);
        p[11] = 1.0f;
        p[14] = -2.0f * f * n / (f - n);
    }
    gpu_set_projection(p);
}

static void target_camera(RwCamera* c)
{
    RwRaster* fb = c ? c->frameBuffer : NULL;
    if (fb && fb->cType == rwRASTERTYPECAMERATEXTURE && fb->width > 0 && fb->height > 0)
    {
        gpu_set_target(pc_raster(fb));
    }
    else
    {
        gpu_set_target(NULL);
    }
}

void pc_render_begin_camera(RwCamera* c)
{
    if (!s_open)
    {
        return;
    }
    s_cam = c;
    s_st.cams++;
    if (trace_frame())
    {
        pc_log("[trace] begin cam %p fb %p vw %.3f %.3f off %.3f %.3f", c, c->frameBuffer, c->viewWindow.x,
               c->viewWindow.y, c->viewOffset.x, c->viewOffset.y);
    }
    target_camera(c);
    if (stats_on())
    {
        static void* seen[64];
        static int nseen;
        bool known = false;
        for (int i = 0; i < nseen; i++)
            known |= seen[i] == c;
        if (!known && nseen < 64)
        {
            seen[nseen++] = c;
            RwRaster* fb = c->frameBuffer;
            pc_log("[cam] new camera %p fb %p type %d %dx%d parent %p zb %p", c, fb, fb ? fb->cType : -1,
                   fb ? fb->width : 0, fb ? fb->height : 0, fb ? fb->parent : NULL, c->zBuffer);
        }
    }
    load_projection(c);
    // View = inverse(camera LTM) with X flipped (RW's +X points left on screen).
    RwMatrix v = c->viewMatrix;
    v.right.x = -v.right.x;
    v.up.x = -v.up.x;
    v.at.x = -v.at.x;
    v.pos.x = -v.pos.x;
    matrix16(&v, s_view);
    gpu_set_modelview(s_view);
    if (stats_on())
    {
        static int n;
        if (n++ % 600 == 0)
        {
            RwFrame* f = (RwFrame*)c->object.object.parent;
            const RwMatrix* m = f ? &f->ltm : NULL;
            pc_log("[cam] proj %d vw %.3f %.3f off %.3f %.3f near %.3f far %.3f", c->projectionType, c->viewWindow.x,
                   c->viewWindow.y, c->viewOffset.x, c->viewOffset.y, c->nearPlane, c->farPlane);
            if (m)
            {
                pc_log("[cam] pos %.2f %.2f %.2f at %.2f %.2f %.2f up %.2f %.2f %.2f right %.2f %.2f %.2f", m->pos.x,
                       m->pos.y, m->pos.z, m->at.x, m->at.y, m->at.z, m->up.x, m->up.y, m->up.z, m->right.x,
                       m->right.y, m->right.z);
            }
        }
    }
}

void pc_render_end_camera(RwCamera* c)
{
    s_cam = NULL;
    gpu_set_target(NULL);
}

// The HUD (zHud_Render) on a second screen, when the backend has one.
extern "C" int pc_render_hud_second_screen()
{
    return s_open && gpu_second_screen();
}

extern "C" void pc_render_hud_screen(int on)
{
    if (pc_render_hud_second_screen())
    {
        gpu_select_screen(on ? 1 : 0);
    }
}

void pc_render_clear(RwCamera* c, const RwRGBA* col, RwInt32 mode)
{
    if (!s_open)
    {
        return;
    }
    s_st.clears++;
    if (trace_frame())
    {
        pc_log("[trace] clear cam %p mode %d col %02x%02x%02x%02x", c, mode, col ? col->red : 0, col ? col->green : 0,
               col ? col->blue : 0, col ? col->alpha : 0);
    }
    if (stats_on())
    {
        static int n;
        if (n++ % 400 == 0)
        {
            pc_log("[clear] cam %p mode %d col %02x%02x%02x%02x target %p", c, mode, col ? col->red : 0,
                   col ? col->green : 0, col ? col->blue : 0, col ? col->alpha : 0, gpu_target());
        }
    }
    // RwCameraClear clears the camera's own raster, and may be called outside
    // its Begin/EndUpdate.
    RwRaster* fb = c ? c->frameBuffer : NULL;
    PcRaster* target = NULL;
    if (fb && fb->cType == rwRASTERTYPECAMERATEXTURE && fb->width > 0 && fb->height > 0)
    {
        target = pc_raster(fb);
    }
    const RwRGBA* image = (mode & rwCAMERACLEARIMAGE) ? col : NULL;
    bool depth = (mode & rwCAMERACLEARZ) != 0;
    if (image || depth)
    {
        gpu_clear(target, image, depth);
    }
}

void pc_events_pump();

static void draw_overlay();
static void skin_cache_trim();

static unsigned long long s_stall[PC_STALL_COUNT];
static int s_fileReads;

void pc_stall_note(int kind, unsigned long long ticks)
{
    if (kind >= 0 && kind < PC_STALL_COUNT)
    {
        s_stall[kind] += ticks;
        s_fileReads += kind == PC_STALL_FILE;
    }
}

// PC_PROF=1: frames over 150 ms (from present to present) with where the time
// went, to tell game work from waiting.
static void long_frame_check()
{
    if (!prof_on())
    {
        return;
    }
    static unsigned long long last;
    unsigned long long now = pc_perf_counter();
    double ms = last ? pc_perf_seconds(now - last) * 1000.0 : 0;
    last = now;
    if (ms > 150.0)
    {
        double vi = pc_perf_seconds(s_stall[PC_STALL_VI]) * 1000.0;
        double file = pc_perf_seconds(s_stall[PC_STALL_FILE]) * 1000.0;
        double present = pc_perf_seconds(s_stall[PC_STALL_PRESENT]) * 1000.0;
        pc_log("[long] frame %.0f ms: retrace wait %.0f, file reads %.0f (%d), present %.0f, rest %.0f", ms, vi, file,
               s_fileReads, present, ms - vi - file - present);
    }
    memset(s_stall, 0, sizeof(s_stall));
    s_fileReads = 0;
}

void pc_render_present()
{
    if (!s_open)
    {
        return;
    }
    skin_cache_trim();
    long_frame_check();
    if (trace_frame())
    {
        pc_log("[trace] present");
    }
    draw_overlay();
    if (stats_on())
    {
        static int frame;
        if (++frame % 120 == 0)
        {
            pc_log("[stats] %.1fs frame %d: cams %d atomics %d/%d sectors %d im2d %d im3d %d clears %d",
                   pc_ticks_ms() / 1000.0, frame, s_st.cams, s_st.atomicsDrawn, s_st.atomics, s_st.sectors, s_st.im2d,
                   s_st.im3d, s_st.clears);
        }
        memset(&s_st, 0, sizeof(s_st));
    }
    gpu_present();
    if (prof_on())
    {
        prof_frame();
    }
    pc_events_pump();
}

// ---------------------------------------------------------------- lighting
// Lighting is done on the CPU with RenderWare's rules: per vertex,
//   colour = prelit + materialColour * (ambient * ka + sum(lights) * kd)
//   alpha  = prelit.alpha * materialAlpha
// (fixed-function lighting can't add the prelit colour as well).
static std::vector<float> s_light; // per-vertex accumulated direct light (rgb), or empty when unlit
static float s_amb[3];              // accumulated ambient light
static bool s_lightUniform;         // only ambient light: every vertex gets the same light

// Whether only ambient light reaches atomics (the same for every vertex): sets
// s_amb and s_lightUniform as light_vertices would, without any vertices.
static bool ambient_light_only()
{
    RpWorld* w = (RpWorld*)rwGlobals()->curWorld;
    s_lightUniform = true;
    s_amb[0] = s_amb[1] = s_amb[2] = 0;
    if (!w)
    {
        return true;
    }
    RwLinkList* lists[2] = { &w->directionalLightList, &w->lightList };
    for (int li = 0; li < 2; li++)
    {
        RwLLLink* end = &lists[li]->link;
        for (RwLLLink* l = end->next; l != end; l = l->next)
        {
            RpLight* lt = rwLLLinkGetData(l, RpLight, inWorld);
            if (!(lt->object.object.flags & rpLIGHTLIGHTATOMICS))
            {
                continue;
            }
            if (lt->object.object.subType == rpLIGHTAMBIENT)
            {
                s_amb[0] += lt->color.red;
                s_amb[1] += lt->color.green;
                s_amb[2] += lt->color.blue;
            }
            else if (lt->object.object.parent)
            {
                return false;
            }
        }
    }
    return true;
}

static void light_vertices(RpGeometry* g, const RwMatrix* ltm, const RwV3d* pos, const RwV3d* nrm)
{
    ProfScope prof(PROF_LIGHT);
    RpWorld* w = (RpWorld*)rwGlobals()->curWorld;
    int nv = g->numVertices;
    s_light.assign(nv * 3, 0.0f);
    s_lightUniform = true;
    s_amb[0] = s_amb[1] = s_amb[2] = 0;
    if (!w)
    {
        return;
    }
    RwLinkList* lists[2] = { &w->directionalLightList, &w->lightList };
    static int dumps;
    static int calls;
    bool dump = stats_on() && g->numVertices > 1000 && ++calls % 400 == 0 && dumps < 3;
    dumps += dump;
    for (int li = 0; li < 2; li++)
    {
        RwLLLink* end = &lists[li]->link;
        for (RwLLLink* l = end->next; l != end; l = l->next)
        {
            RpLight* lt = rwLLLinkGetData(l, RpLight, inWorld);
            if (dump)
            {
                RwFrame* lf = (RwFrame*)lt->object.object.parent;
                pc_log("[light] type %x flags %x col %.2f %.2f %.2f radius %.1f pos %.1f %.1f %.1f at %.2f %.2f %.2f",
                       lt->object.object.subType, lt->object.object.flags, lt->color.red, lt->color.green,
                       lt->color.blue, lt->radius, lf ? lf->ltm.pos.x : 0, lf ? lf->ltm.pos.y : 0,
                       lf ? lf->ltm.pos.z : 0, lf ? lf->ltm.at.x : 0, lf ? lf->ltm.at.y : 0, lf ? lf->ltm.at.z : 0);
            }
            if (!(lt->object.object.flags & rpLIGHTLIGHTATOMICS))
            {
                continue;
            }
            RwFrame* f = (RwFrame*)lt->object.object.parent;
            const RwRGBAReal& c = lt->color;
            int type = lt->object.object.subType;
            if (type == rpLIGHTAMBIENT)
            {
                s_amb[0] += c.red;
                s_amb[1] += c.green;
                s_amb[2] += c.blue;
                continue;
            }
            if (!f)
            {
                continue;
            }
            const RwMatrix& lm = f->ltm;
            s_lightUniform = false;
            if (type == rpLIGHTDIRECTIONAL)
            {
                s_profCol.dirLights++;
            }
            else
            {
                s_profCol.pointLights++;
            }
            for (int i = 0; i < nv; i++)
            {
                // World-space normal.
                const RwV3d& n = nrm[i];
                float nx = ltm->right.x * n.x + ltm->up.x * n.y + ltm->at.x * n.z;
                float ny = ltm->right.y * n.x + ltm->up.y * n.y + ltm->at.y * n.z;
                float nz = ltm->right.z * n.x + ltm->up.z * n.y + ltm->at.z * n.z;
                float nl = sqrtf(nx * nx + ny * ny + nz * nz);
                if (nl > 0)
                {
                    nx /= nl, ny /= nl, nz /= nl;
                }
                float k;
                if (type == rpLIGHTDIRECTIONAL)
                {
                    // Directional lights shine along their frame's at vector.
                    k = -(nx * lm.at.x + ny * lm.at.y + nz * lm.at.z);
                }
                else
                {
                    const RwV3d& p = pos[i];
                    float wx = ltm->right.x * p.x + ltm->up.x * p.y + ltm->at.x * p.z + ltm->pos.x;
                    float wy = ltm->right.y * p.x + ltm->up.y * p.y + ltm->at.y * p.z + ltm->pos.y;
                    float wz = ltm->right.z * p.x + ltm->up.z * p.y + ltm->at.z * p.z + ltm->pos.z;
                    float dx = lm.pos.x - wx, dy = lm.pos.y - wy, dz = lm.pos.z - wz;
                    float d = sqrtf(dx * dx + dy * dy + dz * dz);
                    if (d >= lt->radius || d <= 0)
                    {
                        continue;
                    }
                    dx /= d, dy /= d, dz /= d;
                    k = (nx * dx + ny * dy + nz * dz) * (1.0f - d / lt->radius);
                    if (type != rpLIGHTPOINT)
                    {
                        // Spot: outside the cone gets nothing.
                        float cosA = -(dx * lm.at.x + dy * lm.at.y + dz * lm.at.z);
                        if (-cosA > lt->minusCosAngle)
                        {
                            continue;
                        }
                    }
                }
                if (k > 0)
                {
                    s_light[i * 3] += c.red * k;
                    s_light[i * 3 + 1] += c.green * k;
                    s_light[i * 3 + 2] += c.blue * k;
                }
            }
        }
    }
}

// ---------------------------------------------------------------- geometry
static std::vector<RwRGBA> s_col;

// Whether a static prelit array has any alpha below 255, per cache key.
static std::unordered_map<const void*, bool> s_prelitAlpha;

static bool prelit_has_alpha(const void* key, const RwRGBA* prelit, int nv)
{
    std::unordered_map<const void*, bool>::iterator it = s_prelitAlpha.find(key);
    if (it != s_prelitAlpha.end())
    {
        return it->second;
    }
    bool a = false;
    for (int k = 0; k < nv && !a; k++)
    {
        a = prelit[k].alpha < 255;
    }
    s_prelitAlpha[key] = a;
    return a;
}

// Geometries the game has locked to rewrite their arrays.
static std::unordered_set<const void*> s_dynamicGeometry;

void pc_render_geometry_dynamic(const void* key)
{
    if (s_dynamicGeometry.insert(key).second && s_open)
    {
        gpu_forget(key);
    }
}

void pc_render_forget(const void* key)
{
    s_dynamicGeometry.erase(key);
    s_prelitAlpha.erase(key);
    if (s_open)
    {
        gpu_forget(key);
    }
}

// Positions and texture coordinates shared by the meshes of one geometry.
struct MeshArrays
{
    const RwV3d* pos;
    const RwTexCoords* uv;
};

// staticKey: the sector or unskinned geometry whose arrays these are (NULL
// when they change every frame).
// skin: the backend skins the meshes (arr holds the bind pose and staticKey the
// geometry, whose arrays the backend keeps).
static void draw_meshes(RpMeshHeader* mh, RpMaterialList* mats, const RwRGBA* prelit, bool modulate, int nv,
                        const RpTriangle* tris, int ntris, const MeshArrays& arr, const void* staticKey,
                        const GpuSkin* skin = NULL)
{
    GpuVerts v;
    v.pos = &arr.pos->x;
    v.posStride = sizeof(RwV3d);
    v.posSize = 3;
    v.uv = arr.uv ? &arr.uv->u : NULL;
    v.uvStride = sizeof(RwTexCoords);
    v.count = nv;
    v.cacheKey = NULL;
    // The meshes of one model share these arrays (the CPU colour path below
    // rewrites its buffer per mesh and says so).
    v.reuseKey = staticKey ? NULL : (const void*)arr.pos;
    if (mh && mh->numMeshes)
    {
        RpMesh* mesh = (RpMesh*)(mh + 1);
        GpuPrim prim = (mh->flags & rpMESHHEADERTRISTRIP) ? PRIM_TRISTRIP : PRIM_TRIANGLES;
        for (int i = 0; i < mh->numMeshes; i++)
        {
            RpMaterial* m = mesh[i].material;
            RwRGBA mc = m ? m->color : RwRGBA{ 255, 255, 255, 255 };
            if (!m || !modulate)
            {
                mc.red = mc.green = mc.blue = mc.alpha = 255;
            }
            bool alpha = mc.alpha < 255;
            bool lit = !s_light.empty();
            RwRGBA matColor = { 255, 255, 255, 255 }, addColor = { 0, 0, 0, 0 };
            v.cacheKey = NULL;
            // Debug: PC_DBG=cpucolour computes every vertex colour on the CPU.
            static bool cpuColour = dbg_has("cpucolour");
            static bool cpuStatic = cpuColour || dbg_has("cpustatic"), cpuUniform = cpuColour || dbg_has("cpuuniform");
            if (prelit && !lit && staticKey && !cpuStatic)
            {
                // Static prelit geometry: the GPU multiplies by the material colour
                // and the backend can keep the arrays.
                v.col = prelit;
                v.colStride = sizeof(RwRGBA);
                static bool noCache = dbg_has("nocache");
                v.cacheKey = noCache ? NULL : staticKey;
                matColor = mc;
                alpha |= prelit_has_alpha(staticKey, prelit, nv);
            }
            else if (lit && s_lightUniform && !cpuUniform)
            {
                // Only ambient light reaches it, the same for every vertex: the GPU
                // adds it and applies the material colour (unskinned geometry keeps
                // its arrays cached).
                float ka = m ? m->surfaceProps.ambient : 1.0f;
                if (m)
                {
                    mc = m->color;
                }
                float lc[3];
                for (int c = 0; c < 3; c++)
                {
                    lc[c] = s_amb[c] * ka * 255.0f;
                    lc[c] = lc[c] > 255 ? 255 : lc[c];
                }
                if (prelit)
                {
                    addColor = RwRGBA{ (RwUInt8)lc[0], (RwUInt8)lc[1], (RwUInt8)lc[2], 0 };
                    matColor = mc;
                    v.col = prelit;
                    v.colStride = sizeof(RwRGBA);
                    bool pa = false;
                    if (staticKey)
                    {
                        pa = prelit_has_alpha(staticKey, prelit, nv);
                    }
                    else
                    {
                        for (int k = 0; k < nv && !pa; k++)
                        {
                            pa = prelit[k].alpha < 255;
                        }
                    }
                    alpha |= pa;
                }
                else
                {
                    float r = lc[0] * mc.red / 255.0f, gr = lc[1] * mc.green / 255.0f, b = lc[2] * mc.blue / 255.0f;
                    matColor.red = (RwUInt8)(r > 255 ? 255 : r);
                    matColor.green = (RwUInt8)(gr > 255 ? 255 : gr);
                    matColor.blue = (RwUInt8)(b > 255 ? 255 : b);
                    matColor.alpha = mc.alpha;
                    v.col = NULL;
                    v.constCol = RwRGBA{ 255, 255, 255, 255 };
                }
                alpha |= mc.alpha < 255;
                v.cacheKey = staticKey;
            }
            else if (prelit || lit)
            {
                ProfScope prof(PROF_COLOUR);
                if (lit)
                {
                    s_profCol.litMeshes++;
                    s_profCol.litVerts += nv;
                }
                else
                {
                    s_profCol.prelitMeshes++;
                    s_profCol.prelitVerts += nv;
                }
                s_col.resize(nv);
                float ka = m ? m->surfaceProps.ambient : 1.0f, kd = m ? m->surfaceProps.diffuse : 1.0f;
                if (lit && m)
                {
                    // Lit geometry always takes the material colour.
                    mc = m->color;
                }
                for (int k = 0; k < nv; k++)
                {
                    float r, gr, b;
                    if (lit)
                    {
                        // (prelit + ambient + lights), then the material colour, as the
                        // GameCube renders it (Scooby's brown material on the title screen).
                        r = (s_amb[0] * ka + s_light[k * 3] * kd) * 255.0f;
                        gr = (s_amb[1] * ka + s_light[k * 3 + 1] * kd) * 255.0f;
                        b = (s_amb[2] * ka + s_light[k * 3 + 2] * kd) * 255.0f;
                        if (prelit)
                        {
                            r += prelit[k].red, gr += prelit[k].green, b += prelit[k].blue;
                        }
                        r = (r > 255 ? 255 : r) * mc.red / 255.0f;
                        gr = (gr > 255 ? 255 : gr) * mc.green / 255.0f;
                        b = (b > 255 ? 255 : b) * mc.blue / 255.0f;
                    }
                    else
                    {
                        r = prelit[k].red * mc.red / 255.0f;
                        gr = prelit[k].green * mc.green / 255.0f;
                        b = prelit[k].blue * mc.blue / 255.0f;
                    }
                    s_col[k].red = (RwUInt8)(r > 255 ? 255 : r);
                    s_col[k].green = (RwUInt8)(gr > 255 ? 255 : gr);
                    s_col[k].blue = (RwUInt8)(b > 255 ? 255 : b);
                    s_col[k].alpha = (RwUInt8)((prelit ? prelit[k].alpha : 255) * mc.alpha / 255);
                    alpha |= s_col[k].alpha < 255;
                }
                v.col = s_col.data();
                v.colStride = sizeof(RwRGBA);
                gpu_dynamic_changed();
            }
            else
            {
                v.col = NULL;
                v.constCol = mc;
            }
            RwTexture* t = m ? m->texture : NULL;
            PcRaster* tex = t ? use_texture(t->raster) : NULL;
            GpuState s = t ? draw_state(alpha || (t->raster && raster_has_alpha(t->raster)), tex, rwTexFilter(t),
                                        rwTexAddrU(t), rwTexAddrV(t))
                           : draw_state(alpha, NULL, 0, 0, 0);
            s.matColor = matColor;
            s.addColor = addColor;
            // Debug: PC_HILITE_MAT=<n> paints world material n bright red.
            static const char* hilite = getenv("PC_HILITE_MAT");
            RpWorld* cw = (RpWorld*)rwGlobals()->curWorld;
            if (hilite && cw && mats == &cw->matList && m && atoi(hilite) < cw->matList.numMaterials &&
                cw->matList.materials[atoi(hilite)] == m)
            {
                v.col = NULL;
                v.cacheKey = NULL;
                v.constCol = RwRGBA{ 255, 0, 0, 255 };
                s.matColor = RwRGBA{ 255, 255, 255, 255 };
                s.addColor = RwRGBA{ 0, 0, 0, 0 };
                s.tex = NULL;
                s.fog = false;
            }
            if (dbg_has("white"))
            {
                v.col = NULL;
                v.cacheKey = NULL;
                v.constCol = RwRGBA{ 255, 255, 255, 255 };
                s.matColor = RwRGBA{ 255, 255, 255, 255 };
                s.addColor = RwRGBA{ 0, 0, 0, 0 };
            }
            ProfScope prof(PROF_DRAW);
            if (skin)
            {
                // The backend keeps the arrays: a constant colour becomes part of the
                // material colour (clamp(colour + add) * material) over white vertices.
                GpuVerts sv = v;
                sv.cacheKey = staticKey;
                sv.reuseKey = NULL;
                if (!sv.col)
                {
                    const RwRGBA& c = v.constCol;
                    int add[3] = { c.red + s.addColor.red, c.green + s.addColor.green, c.blue + s.addColor.blue };
                    s.matColor.red = (RwUInt8)((add[0] > 255 ? 255 : add[0]) * s.matColor.red / 255);
                    s.matColor.green = (RwUInt8)((add[1] > 255 ? 255 : add[1]) * s.matColor.green / 255);
                    s.matColor.blue = (RwUInt8)((add[2] > 255 ? 255 : add[2]) * s.matColor.blue / 255);
                    s.matColor.alpha = (RwUInt8)(c.alpha * s.matColor.alpha / 255);
                    s.addColor = RwRGBA{ 0, 0, 0, 0 };
                }
                gpu_draw_skinned(s, sv, *skin, prim, mesh[i].indices, (int)mesh[i].numIndices);
            }
            else
            {
                gpu_draw(s, v, prim, mesh[i].indices, (int)mesh[i].numIndices);
            }
        }
        return;
    }
    // No mesh data: draw the triangle list directly.
    if (tris && ntris)
    {
        std::vector<RwUInt16> idx(ntris * 3);
        for (int i = 0; i < ntris; i++)
        {
            idx[i * 3] = tris[i].vertIndex[0];
            idx[i * 3 + 1] = tris[i].vertIndex[1];
            idx[i * 3 + 2] = tris[i].vertIndex[2];
        }
        v.col = NULL;
        v.constCol = RwRGBA{ 255, 255, 255, 255 };
        gpu_draw(draw_state(false, NULL, 0, 0, 0), v, PRIM_TRIANGLES, idx.data(), (int)idx.size());
    }
}

static void skin_vertices(RpGeometry* g, PcSkin* sk, RpHAnimHierarchy* h, RwV3d* outPos, RwV3d* outNrm)
{
    ProfScope prof(PROF_SKIN);
    int nb = sk->numBones;
    static std::vector<RwMatrix> s_bones;
    if ((int)s_bones.size() < nb)
    {
        s_bones.resize(nb);
    }
    RwMatrix* m = s_bones.data();
    for (int b = 0; b < nb && b < h->numNodes; b++)
    {
        RwMatrixMultiply(&m[b], &sk->invMats[b], &h->pMatrixArray[b]);
    }
    const RwV3d* v = g->morphTarget->verts;
    const RwV3d* n = outNrm ? g->morphTarget->normals : NULL;
    for (int i = 0; i < g->numVertices; i++)
    {
        RwV3d p = { 0, 0, 0 }, q = { 0, 0, 0 };
        const RwUInt8* bi = (const RwUInt8*)&sk->indices[i];
        const float* w = &sk->weights[i * 4];
        for (int k = 0; k < 4; k++)
        {
            if (w[k] == 0.0f || bi[k] >= nb)
            {
                continue;
            }
            const RwMatrix& M = m[bi[k]];
            p.x += w[k] * (v[i].x * M.right.x + v[i].y * M.up.x + v[i].z * M.at.x + M.pos.x);
            p.y += w[k] * (v[i].x * M.right.y + v[i].y * M.up.y + v[i].z * M.at.y + M.pos.y);
            p.z += w[k] * (v[i].x * M.right.z + v[i].y * M.up.z + v[i].z * M.at.z + M.pos.z);
            if (n)
            {
                q.x += w[k] * (n[i].x * M.right.x + n[i].y * M.up.x + n[i].z * M.at.x);
                q.y += w[k] * (n[i].x * M.right.y + n[i].y * M.up.y + n[i].z * M.at.y);
                q.z += w[k] * (n[i].x * M.right.z + n[i].y * M.up.z + n[i].z * M.at.z);
            }
        }
        outPos[i] = p;
        if (outNrm)
        {
            outNrm[i] = q;
        }
    }
}

// Skinned vertices per atomic, kept while its pose is unchanged: a model drawn
// twice in a frame (its shadow, then itself) or standing still is skinned once.
struct SkinCache
{
    const RpGeometry* g;
    const RpHAnimHierarchy* h;
    std::vector<RwMatrix> pose; // the hierarchy's matrices the vertices were made from
    std::vector<RwV3d> pos, nrm;
    bool hasNrm;
    unsigned frame; // last frame it was used
};
static std::unordered_map<const RpAtomic*, SkinCache> s_skinCache;
static unsigned s_frameCount;

static SkinCache& skin_cached(RpAtomic* a, RpGeometry* g, PcSkin* sk, RpHAnimHierarchy* h, bool wantNrm)
{
    SkinCache& c = s_skinCache[a];
    int nb = sk->numBones < h->numNodes ? sk->numBones : h->numNodes;
    size_t poseBytes = (size_t)(nb > 0 ? nb : 0) * sizeof(RwMatrix);
    // Geometry the game rewrites (RpGeometryLock) is skinned again every frame.
    bool same = c.g == g && c.h == h && (int)c.pose.size() == nb && (int)c.pos.size() == g->numVertices &&
                (c.hasNrm || !wantNrm) && (!s_dynamicGeometry.count(g) || c.frame == s_frameCount) &&
                memcmp(c.pose.data(), h->pMatrixArray, poseBytes) == 0;
    if (!same)
    {
        // Debug: PC_LOG_SKINS=1 logs each skinned geometry's bones once.
        static bool logSkins = getenv("PC_LOG_SKINS") != NULL;
        static std::unordered_set<const void*> logged;
        if (logSkins && logged.insert(g).second)
        {
            int used = 0, maxMesh = 0;
            std::vector<char> seen(sk->numBones + 1, 0);
            for (int i = 0; i < g->numVertices; i++)
            {
                const RwUInt8* bi = (const RwUInt8*)&sk->indices[i];
                for (int k = 0; k < 4; k++)
                {
                    if (sk->weights[i * 4 + k] != 0.0f && bi[k] < sk->numBones && !seen[bi[k]] && (seen[bi[k]] = 1))
                    {
                        used++;
                    }
                }
            }
            if (g->mesh)
            {
                RpMesh* mesh = (RpMesh*)(g->mesh + 1);
                for (int m = 0; m < g->mesh->numMeshes; m++)
                {
                    std::vector<char> ms(sk->numBones + 1, 0);
                    int n = 0;
                    for (RwUInt32 j = 0; j < mesh[m].numIndices; j++)
                    {
                        int vi = mesh[m].indices[j];
                        const RwUInt8* bi = (const RwUInt8*)&sk->indices[vi];
                        for (int k = 0; k < 4; k++)
                        {
                            if (sk->weights[vi * 4 + k] != 0.0f && bi[k] < sk->numBones && !ms[bi[k]] && (ms[bi[k]] = 1))
                            {
                                n++;
                            }
                        }
                    }
                    maxMesh = n > maxMesh ? n : maxMesh;
                }
            }
            pc_log("[skin] geometry %p: %d verts, %d bones (%d used), %d meshes, most bones in one mesh %d, max weights %d",
                   g, g->numVertices, sk->numBones, used, g->mesh ? g->mesh->numMeshes : 0, maxMesh, sk->maxWeights);
        }
        c.g = g;
        c.h = h;
        c.pose.assign(h->pMatrixArray, h->pMatrixArray + nb);
        c.pos.resize(g->numVertices);
        if (wantNrm)
        {
            c.nrm.resize(g->numVertices);
        }
        c.hasNrm = wantNrm;
        skin_vertices(g, sk, h, c.pos.data(), wantNrm ? c.nrm.data() : NULL);
        gpu_dynamic_changed();
    }
    c.frame = s_frameCount;
    return c;
}

// Drops the skinned vertices of atomics not drawn for a while (and of destroyed ones).
static void skin_cache_trim()
{
    if (++s_frameCount % 60)
    {
        return;
    }
    for (std::unordered_map<const RpAtomic*, SkinCache>::iterator it = s_skinCache.begin(); it != s_skinCache.end();)
    {
        if (s_frameCount - it->second.frame > 60)
        {
            it = s_skinCache.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void pc_render_atomic(RpAtomic* a)
{
    RpGeometry* g = a->geometry;
    RwFrame* f = (RwFrame*)a->object.object.parent;
    s_st.atomics++;
    if (!s_open || !g || !f || !g->morphTarget || !g->morphTarget->verts || g->numVertices <= 0)
    {
        return;
    }
    if (dbg_has("noatomic"))
    {
        return;
    }
    s_st.atomicsDrawn++;
    if (trace_frame())
    {
        pc_log("[trace] atomic verts %d pos %.1f %.1f %.1f blend %d/%d zt %d", g->numVertices, f->ltm.pos.x,
               f->ltm.pos.y, f->ltm.pos.z, s_rs.srcBlend, s_rs.dstBlend, s_rs.zTest);
    }
    if (stats_on() && dbg_has("atomlog"))
    {
        // Each distinct geometry once: flags, prelit sample, materials and textures.
        static std::vector<RpGeometry*> seen;
        if (std::find(seen.begin(), seen.end(), g) == seen.end() && seen.size() < 400)
        {
            seen.push_back(g);
            pc_log("[atomic] geo %p verts %d flags %x pos %.1f %.1f %.1f prelit %s %02x%02x%02x%02x skin %d", g,
                   g->numVertices, g->flags, f->ltm.pos.x, f->ltm.pos.y, f->ltm.pos.z, g->preLitLum ? "yes" : "no",
                   g->preLitLum ? g->preLitLum[0].red : 0, g->preLitLum ? g->preLitLum[0].green : 0,
                   g->preLitLum ? g->preLitLum[0].blue : 0, g->preLitLum ? g->preLitLum[0].alpha : 0,
                   pc_geometry(g)->skin != NULL);
            for (int i = 0; i < g->matList.numMaterials; i++)
            {
                RpMaterial* m = g->matList.materials[i];
                RwTexture* t = m ? m->texture : NULL;
                pc_log("[atomic]   mat %d col %02x%02x%02x%02x tex %s raster %p", i, m ? m->color.red : 0,
                       m ? m->color.green : 0, m ? m->color.blue : 0, m ? m->color.alpha : 0, t ? t->name : "-",
                       t ? t->raster : NULL);
            }
        }
    }
    float model[16], mv[16];
    matrix16(&f->ltm, model);
    mul16(s_view, model, mv);
    gpu_set_modelview(mv);

    const RwV3d* pos = g->morphTarget->verts;
    const RwV3d* nrm = g->morphTarget->normals;
    PcSkin* sk = pc_geometry(g)->skin;
    RpHAnimHierarchy* h = pc_atomic(a)->skinHierarchy;
    bool skinned = false;
    {
        static int geolog0 = getenv("PC_GEOLOG") ? atoi(getenv("PC_GEOLOG")) : -1;
        static int n0;
        if (g->numVertices == geolog0 && n0++ % 300 == 0)
        {
            pc_log("[geo] atomic %p skin %p hierarchy %p matrices %p ltm pos %.1f %.1f %.1f right %.2f %.2f %.2f", a, sk, h,
                   h ? h->pMatrixArray : NULL, f->ltm.pos.x, f->ltm.pos.y, f->ltm.pos.z, f->ltm.right.x, f->ltm.right.y,
                   f->ltm.right.z);
        }
    }
    if (sk && h && h->pMatrixArray && g->mesh && gpu_skinning() && !s_dynamicGeometry.count(g) &&
        (!nrm || !(g->flags & rpGEOMETRYLIGHT) || ambient_light_only()))
    {
        // The GPU skins it: only the pose's skin matrices are made here.
        static std::vector<RwMatrix> s_bones;
        {
            ProfScope prof(PROF_SKIN);
            if ((int)s_bones.size() < sk->numBones)
            {
                s_bones.resize(sk->numBones);
            }
            for (int b = 0; b < sk->numBones; b++)
            {
                if (b < h->numNodes)
                {
                    RwMatrixMultiply(&s_bones[b], &sk->invMats[b], &h->pMatrixArray[b]);
                }
                else
                {
                    memset(&s_bones[b], 0, sizeof(RwMatrix));
                    s_bones[b].right.x = s_bones[b].up.y = s_bones[b].at.z = 1.0f;
                }
            }
        }
        GpuSkin gs = { (const RwUInt8*)sk->indices, sk->weights, s_bones.data(), sk->numBones };
        if (nrm && (g->flags & rpGEOMETRYLIGHT))
        {
            s_light.assign(1, 0.0f); // lit, by ambient light only (ambient_light_only set it)
        }
        else
        {
            s_light.clear();
        }
        MeshArrays arr = { g->morphTarget->verts, g->texCoords[0] };
        draw_meshes(g->mesh, &g->matList, g->preLitLum, (g->flags & rpGEOMETRYMODULATEMATERIALCOLOR) != 0,
                    g->numVertices, g->triangles, g->numTriangles, arr, g, &gs);
        s_light.clear();
        gpu_set_modelview(s_view);
        return;
    }
    if (sk && h && h->pMatrixArray)
    {
        // Normals only feed the lighting.
        bool wantNrm = nrm && (g->flags & rpGEOMETRYLIGHT);
        SkinCache& sc = skin_cached(a, g, sk, h, wantNrm);
        pos = sc.pos.data();
        skinned = true;
        // Debug: PC_GEOLOG=<verts> logs the skinned bounds of models with that many vertices.
        static int geolog = getenv("PC_GEOLOG") ? atoi(getenv("PC_GEOLOG")) : -1;
        static int geologged;
        if (g->numVertices == geolog && geologged++ % 300 == 0)
        {
            float mn[3] = { 1e9f, 1e9f, 1e9f }, mx[3] = { -1e9f, -1e9f, -1e9f };
            for (int i = 0; i < g->numVertices; i++)
            {
                const float* v = &pos[i].x;
                for (int k = 0; k < 3; k++) { mn[k] = fminf(mn[k], v[k]); mx[k] = fmaxf(mx[k], v[k]); }
            }
            const RwV3d* b0 = g->morphTarget->verts;
            pc_log("[geo] skinned bounds (%.2f %.2f %.2f)-(%.2f %.2f %.2f) bind v0 (%.2f %.2f %.2f) skinned v0 (%.2f %.2f %.2f) "
                   "bones %d nodes %d ltm pos %.1f %.1f %.1f alpha %d", mn[0], mn[1], mn[2], mx[0], mx[1], mx[2], b0->x, b0->y, b0->z,
                   pos[0].x, pos[0].y, pos[0].z, sk->numBones, h->numNodes, f->ltm.pos.x, f->ltm.pos.y, f->ltm.pos.z,
                   g->matList.numMaterials ? g->matList.materials[0]->color.alpha : -1);
        }
        nrm = wantNrm ? sc.nrm.data() : NULL;
    }
    if (nrm && (g->flags & rpGEOMETRYLIGHT))
    {
        light_vertices(g, &f->ltm, pos, nrm);
    }
    else
    {
        s_light.clear();
    }
    MeshArrays arr = { pos, g->texCoords[0] };
    draw_meshes(g->mesh, &g->matList, g->preLitLum, (g->flags & rpGEOMETRYMODULATEMATERIALCOLOR) != 0,
                g->numVertices, g->triangles, g->numTriangles, arr,
                !skinned && !s_dynamicGeometry.count(g) ? (const void*)g : NULL);
    s_light.clear();
    gpu_set_modelview(s_view);
}

static bool sector_visible(RpWorldSector* s)
{
    RwCamera* c = rwGlobals()->curCamera;
    if (!c)
    {
        return true;
    }
    const RwBBox& b = s->boundingBox;
    for (int i = 0; i < 6; i++)
    {
        const RwPlane& p = c->frustumPlanes[i].plane;
        // nearest corner to the plane's inside
        RwV3d v = { p.normal.x > 0 ? b.inf.x : b.sup.x, p.normal.y > 0 ? b.inf.y : b.sup.y,
                    p.normal.z > 0 ? b.inf.z : b.sup.z };
        if (p.normal.x * v.x + p.normal.y * v.y + p.normal.z * v.z - p.distance > 0)
        {
            return false;
        }
    }
    return true;
}

void pc_render_world(RpWorld* w, std::vector<RpWorldSector*>& sectors)
{
    if (!s_open || dbg_has("nosector"))
    {
        return;
    }
    ProfScope prof(PROF_WORLD);
    gpu_set_modelview(s_view);
    for (size_t i = 0; i < sectors.size(); i++)
    {
        RpWorldSector* s = sectors[i];
        if (!s->numVertices || !s->mesh || !sector_visible(s))
        {
            continue;
        }
        s_st.sectors++;
        if (stats_on() && s_st.sectors == 1)
        {
            static int n;
            if (n++ % 600 == 0)
            {
                pc_log("[sector] verts %d bbox %.1f %.1f %.1f - %.1f %.1f %.1f v0 %.2f %.2f %.2f prelit %p (%02x%02x%02x%02x) tex %p",
                       s->numVertices, s->boundingBox.inf.x, s->boundingBox.inf.y, s->boundingBox.inf.z,
                       s->boundingBox.sup.x, s->boundingBox.sup.y, s->boundingBox.sup.z, s->vertices[0].x,
                       s->vertices[0].y, s->vertices[0].z, s->preLitLum, s->preLitLum ? s->preLitLum[0].red : 0,
                       s->preLitLum ? s->preLitLum[0].green : 0, s->preLitLum ? s->preLitLum[0].blue : 0,
                       s->preLitLum ? s->preLitLum[0].alpha : 0, s->texCoords[0]);
            }
        }
        MeshArrays arr = { s->vertices, s->texCoords[0] };
        draw_meshes(s->mesh, &w->matList, s->preLitLum, (w->flags & rpGEOMETRYMODULATEMATERIALCOLOR) != 0,
                    s->numVertices, NULL, 0, arr, s);
    }
}

// ---------------------------------------------------------------- immediate mode
static GpuPrim gpu_prim(int t)
{
    switch (t)
    {
    case rwPRIMTYPELINELIST: return PRIM_LINES;
    case rwPRIMTYPEPOLYLINE: return PRIM_LINESTRIP;
    case rwPRIMTYPETRISTRIP: return PRIM_TRISTRIP;
    case rwPRIMTYPETRIFAN: return PRIM_TRIFAN;
    case rwPRIMTYPEPOINTLIST: return PRIM_POINTS;
    default: return PRIM_TRIANGLES;
    }
}

// Blending for immediate-mode geometry, as RenderWare decides it: a vertex with
// alpha below 255 or a texture with alpha (vertexAlpha state is checked too).
static bool im2d_alpha(const RwIm2DVertex* v, int n)
{
    for (int i = 0; i < n; i++)
    {
        if (v[i].emissiveColor.alpha != 255)
        {
            return true;
        }
    }
    return raster_has_alpha(s_rs.raster);
}

// Screen-space draws: pixels of the current target (the 640x480 game area for
// the screen), y down.
static void begin_2d()
{
    float p[16];
    PcRaster* t = gpu_target();
    ortho(p, 0, t ? (float)t->r.width : (float)FB_W, t ? (float)t->r.height : (float)FB_H, 0);
    gpu_set_projection(p);
    gpu_set_modelview(kIdentity);
}

static void end_2d()
{
    gpu_set_projection(s_proj);
    gpu_set_modelview(s_view);
}

static GpuVerts im2d_verts(RwIm2DVertex* v, int n)
{
    GpuVerts gv;
    gv.pos = &v->x;
    gv.posStride = sizeof(RwIm2DVertex);
    gv.posSize = 2;
    gv.col = &v->emissiveColor;
    gv.colStride = sizeof(RwIm2DVertex);
    gv.uv = &v->u;
    gv.uvStride = sizeof(RwIm2DVertex);
    gv.count = n;
    gv.cacheKey = NULL;
    gv.reuseKey = NULL;
    return gv;
}

// Im2D: screen-space quads with no depth test; the y-down projection flips
// their winding, so no culling either.
static GpuState im2d_state(RwIm2DVertex* v, int n)
{
    // RenderWare's 2D path sets its own alpha compare on GameCube, ending one
    // the game set through GX (the alpha models' "alpha >= 128" would hide the
    // half-transparent unselected menu items).
    pc_render_reset_gx_alpha();
    PcRaster* tex = use_texture(s_rs.raster);
    GpuState s = draw_state(im2d_alpha(v, n), tex, s_rs.filter, s_rs.addrU, s_rs.addrV);
    s.zTest = false;
    s.cull = PC_CULL_NONE;
    return s;
}

// Im3D: the last transformed batch.
static RwIm3DVertex* s_im3dVerts;
static RwUInt32 s_im3dCount;
static RwMatrix s_im3dLtm;
static bool s_im3dHasLtm;

extern "C" {

float RwIm2DGetNearScreenZ() { return 0.0f; }
float RwIm2DGetFarScreenZ() { return 1.0f; }

RwInt32 RwIm2DRenderPrimitive(RwInt32 primType, RwIm2DVertex* v, RwInt32 n)
{
    if (!s_open || n <= 0 || dbg_has("noim2d"))
    {
        return 1;
    }
    static int logged;
    static const char* logIm2d = getenv("PC_LOG_IM2D");
    if (logIm2d && s_st.im2d == 0 && logged++ < atoi(logIm2d))
    {
        pc_log("[im2d] prim %d n %d raster %p v0 (%.1f %.1f %.3f) col %02x%02x%02x%02x uv (%.3f %.3f) v1 (%.1f %.1f)",
               primType, n, s_rs.raster, v[0].x, v[0].y, v[0].z, v[0].emissiveColor.red, v[0].emissiveColor.green,
               v[0].emissiveColor.blue, v[0].emissiveColor.alpha, v[0].u, v[0].v, n > 1 ? v[1].x : 0, n > 1 ? v[1].y : 0);
    }
    s_st.im2d++;
    if (trace_frame())
    {
        pc_log("[trace] im2d prim %d n %d raster %p (%.0f,%.0f)-(%.0f,%.0f) col %02x%02x%02x%02x blend %d/%d va %d "
               "atest %d/%d gx %d/%d",
               primType, n, s_rs.raster, v[0].x, v[0].y, v[n - 1].x, v[n - 1].y, v[0].emissiveColor.red,
               v[0].emissiveColor.green, v[0].emissiveColor.blue, v[0].emissiveColor.alpha, s_rs.srcBlend,
               s_rs.dstBlend, s_rs.vertexAlpha, s_rs.alphaFunc, s_rs.alphaRef, s_gxAlphaFunc, s_gxAlphaRef);
    }
    ProfScope prof(PROF_IM);
    begin_2d();
    gpu_draw(im2d_state(v, n), im2d_verts(v, n), gpu_prim(primType), NULL, 0);
    end_2d();
    return 1;
}

RwInt32 RwIm2DRenderIndexedPrimitive(RwInt32 primType, void* vertices, RwInt32 n, RwUInt16* idx, RwInt32 ni)
{
    if (!s_open || n <= 0)
    {
        return 1;
    }
    RwIm2DVertex* v = (RwIm2DVertex*)vertices;
    if (trace_frame())
    {
        pc_log("[trace] im2d-idx prim %d n %d raster %p (%.0f,%.0f) col %02x%02x%02x%02x blend %d/%d", primType, n,
               s_rs.raster, v[0].x, v[0].y, v[0].emissiveColor.red, v[0].emissiveColor.green,
               v[0].emissiveColor.blue, v[0].emissiveColor.alpha, s_rs.srcBlend, s_rs.dstBlend);
    }
    begin_2d();
    gpu_draw(im2d_state(v, n), im2d_verts(v, n), gpu_prim(primType), idx, ni);
    end_2d();
    return 1;
}

RwInt32 RwIm2DRenderLine(RwIm2DVertex* v, RwInt32 n, RwInt32 a, RwInt32 b)
{
    RwUInt16 idx[2] = { (RwUInt16)a, (RwUInt16)b };
    return RwIm2DRenderIndexedPrimitive(rwPRIMTYPELINELIST, v, n, idx, 2);
}

void* RwIm3DTransform(RwIm3DVertex* v, RwUInt32 n, RwMatrix* ltm, RwUInt32 flags)
{
    s_im3dVerts = v;
    s_im3dCount = n;
    s_im3dHasLtm = ltm != NULL;
    if (ltm)
    {
        s_im3dLtm = *ltm;
    }
    return v;
}

}

static void im3d_draw(GpuPrim prim, const RwUInt16* idx, int n)
{
    if (s_im3dHasLtm)
    {
        float m[16], mv[16];
        matrix16(&s_im3dLtm, m);
        mul16(s_view, m, mv);
        gpu_set_modelview(mv);
    }
    else
    {
        gpu_set_modelview(s_view);
    }
    static const char* logIm3d = getenv("PC_LOG_IM3D");
    static int logged;
    if (logIm3d && logged < atoi(logIm3d) && rwGlobals()->renderFrame > 2000)
    {
        logged++;
        RwIm3DVertex* v0 = s_im3dVerts;
        PcRaster* pr = s_rs.raster ? pc_raster(s_rs.raster) : NULL;
        pc_log("[im3d] n %u raster %p %dx%d fmt %x blend %d/%d valpha %d zw %d zt %d atest %d/%d col %02x%02x%02x%02x "
               "uv %.2f %.2f px0 %08x corner %08x gx %d",
               s_im3dCount, s_rs.raster, s_rs.raster ? s_rs.raster->width : 0, s_rs.raster ? s_rs.raster->height : 0,
               s_rs.raster ? s_rs.raster->cFormat : 0, s_rs.srcBlend, s_rs.dstBlend, s_rs.vertexAlpha, s_rs.zWrite,
               s_rs.zTest, s_rs.alphaFunc, s_rs.alphaRef, v0->color.red, v0->color.green, v0->color.blue,
               v0->color.alpha, v0->u, v0->v,
               pr && pr->rgba ? *(unsigned int*)(pr->rgba + (s_rs.raster->width * (s_rs.raster->height / 2) + s_rs.raster->width / 2) * 4) : 0,
               pr && pr->rgba ? *(unsigned int*)pr->rgba : 0, pr ? pr->ext[0xC] : -1);
    }
    PcRaster* tex = use_texture(s_rs.raster);
    bool alpha = raster_has_alpha(s_rs.raster);
    for (RwUInt32 i = 0; i < s_im3dCount && !alpha; i++)
    {
        alpha = s_im3dVerts[i].color.alpha != 255;
    }
    GpuVerts v;
    RwIm3DVertex* iv = s_im3dVerts;
    v.pos = &iv->objVertex.x;
    v.posStride = sizeof(RwIm3DVertex);
    v.posSize = 3;
    v.col = &iv->color;
    v.colStride = sizeof(RwIm3DVertex);
    v.uv = &iv->u;
    v.uvStride = sizeof(RwIm3DVertex);
    v.count = (int)s_im3dCount;
    v.cacheKey = NULL;
    v.reuseKey = NULL;
    gpu_draw(draw_state(alpha, tex, s_rs.filter, s_rs.addrU, s_rs.addrV), v, prim, idx, n);
    gpu_set_modelview(s_view);
}

extern "C" {

RwInt32 RwIm3DRenderPrimitive(RwInt32 primType)
{
    if (!s_open || !s_im3dVerts || dbg_has("noim3d"))
    {
        return 1;
    }
    s_st.im3d++;
    im3d_draw(gpu_prim(primType), NULL, 0);
    return 1;
}

RwInt32 RwIm3DRenderIndexedPrimitive(RwInt32 primType, RwUInt16* idx, RwInt32 n)
{
    if (!s_open || !s_im3dVerts || dbg_has("noim3d"))
    {
        return 1;
    }
    im3d_draw(gpu_prim(primType), idx, n);
    return 1;
}

RwInt32 RwIm3DEnd()
{
    s_im3dVerts = NULL;
    return 1;
}

}

// ---------------------------------------------------------------- movies
extern "C" void pc_render_fmv_frame(const unsigned char* rgba, int w, int h, int letterbox)
{
    if (!s_open)
    {
        return;
    }
    (void)letterbox;
    gpu_fmv_frame(rgba, w, h);
    pc_events_pump();
}

// ---------------------------------------------------------------- debug overlay
// A tiny 5x7 bitmap font for the debug readout (F3). Each glyph is 7 rows of
// 5 bits, most significant bit on the left.
static const struct { char c; unsigned char rows[7]; } s_font[] = {
    { '0', { 0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E } }, { '1', { 0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E } },
    { '2', { 0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F } }, { '3', { 0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E } },
    { '4', { 0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02 } }, { '5', { 0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E } },
    { '6', { 0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E } }, { '7', { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08 } },
    { '8', { 0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E } }, { '9', { 0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C } },
    { 'A', { 0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 } }, { 'B', { 0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E } },
    { 'C', { 0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E } }, { 'D', { 0x1C, 0x12, 0x11, 0x11, 0x11, 0x12, 0x1C } },
    { 'E', { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F } }, { 'F', { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10 } },
    { 'G', { 0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F } }, { 'H', { 0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 } },
    { 'I', { 0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E } }, { 'J', { 0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C } },
    { 'K', { 0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11 } }, { 'L', { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F } },
    { 'M', { 0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11 } }, { 'N', { 0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11 } },
    { 'O', { 0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E } }, { 'P', { 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10 } },
    { 'Q', { 0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D } }, { 'R', { 0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11 } },
    { 'S', { 0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E } }, { 'T', { 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 } },
    { 'U', { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E } }, { 'V', { 0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04 } },
    { 'W', { 0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A } }, { 'X', { 0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11 } },
    { 'Y', { 0x11, 0x11, 0x11, 0x0A, 0x04, 0x04, 0x04 } }, { 'Z', { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F } },
    { '-', { 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00 } }, { '.', { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C } },
    { ':', { 0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00 } }, { ',', { 0x00, 0x00, 0x00, 0x00, 0x0C, 0x04, 0x08 } },
    { '!', { 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04 } }, { '?', { 0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04 } },
    { '\'', { 0x04, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00 } }, { '&', { 0x0C, 0x12, 0x14, 0x08, 0x15, 0x12, 0x0D } },
    { '(', { 0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02 } }, { ')', { 0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08 } },
    { '>', { 0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08 } }, { '/', { 0x01, 0x01, 0x02, 0x04, 0x08, 0x10, 0x10 } },
    { '+', { 0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00 } },
};

static char s_overlay[4096];

extern "C" void pc_render_set_overlay(const char* text)
{
    snprintf(s_overlay, sizeof(s_overlay), "%s", text ? text : "");
}

static void overlay_rect(std::vector<float>& q, int x0, int y0, int x1, int y1)
{
    const float v[12] = { (float)x0, (float)y0, (float)x1, (float)y0, (float)x1, (float)y1,
                          (float)x0, (float)y0, (float)x1, (float)y1, (float)x0, (float)y1 };
    q.insert(q.end(), v, v + 12);
}

static void overlay_draw(const std::vector<float>& q, RwRGBA col)
{
    if (q.empty())
    {
        return;
    }
    GpuState s;
    memset(&s, 0, sizeof(s));
    s.blend = true;
    s.srcBlend = PC_BLEND_SRCALPHA;
    s.dstBlend = PC_BLEND_INVSRCALPHA;
    s.alphaFunc = PC_ALPHA_ALWAYS;
    s.cull = PC_CULL_NONE;
    s.colorWrite = s.alphaWrite = true;
    s.matColor = RwRGBA{ 255, 255, 255, 255 };
    s.addColor = RwRGBA{ 0, 0, 0, 0 };
    GpuVerts v;
    memset(&v, 0, sizeof(v));
    v.pos = q.data();
    v.posStride = 2 * sizeof(float);
    v.posSize = 2;
    v.constCol = col;
    v.count = (int)q.size() / 2;
    gpu_draw(s, v, PRIM_TRIANGLES, NULL, 0);
}

static void draw_overlay()
{
    if (!s_overlay[0])
    {
        return;
    }
    gpu_set_target(NULL);
    int w, h;
    const int S = gpu_overlay_begin(&w, &h); // pixels per font pixel
    float p[16];
    ortho(p, 0, (float)w, (float)h, 0);
    gpu_set_projection(p);
    gpu_set_modelview(kIdentity);
    // Backing box, then the text.
    int lines = 1, cols = 0, col = 0;
    for (const char* c = s_overlay; *c; c++)
    {
        if (*c == '\n') { lines++; col = 0; } else if (++col > cols) cols = col;
    }
    std::vector<float> q;
    overlay_rect(q, 4, 4, 12 + cols * 6 * S, 12 + lines * 9 * S);
    overlay_draw(q, RwRGBA{ 0, 0, 0, 153 });
    q.clear();
    int x0 = 8, x = x0, y = 8;
    for (const char* c = s_overlay; *c; c++)
    {
        char ch = *c >= 'a' && *c <= 'z' ? *c - 32 : *c;
        if (ch == '\n')
        {
            x = x0;
            y += 9 * S;
            continue;
        }
        for (size_t g = 0; g < sizeof(s_font) / sizeof(s_font[0]); g++)
        {
            if (s_font[g].c != ch)
            {
                continue;
            }
            for (int r = 0; r < 7; r++)
            {
                for (int b = 0; b < 5; b++)
                {
                    if (s_font[g].rows[r] & (0x10 >> b))
                    {
                        int px = x + b * S, py = y + r * S;
                        overlay_rect(q, px, py, px + S, py + S);
                    }
                }
            }
            break;
        }
        x += 6 * S;
    }
    overlay_draw(q, RwRGBA{ 255, 255, 77, 255 });
}

// ---------------------------------------------------------------- direct GX calls
// GX calls the game makes directly (src/Core/gc/iEnv.cpp, iSystem.cpp).
extern "C" void GXSetColorUpdate(int enable)
{
    s_colorUpdate = enable != 0;
}

extern "C" void GXSetAlphaUpdate(int enable)
{
    s_alphaUpdate = enable != 0;
}

// GX tests (alpha comp0 ref0) op (alpha comp1 ref1); fixed-function GPUs have
// one comparison, so keep the side that can fail. The game only uses
// "alpha >= 0 AND alpha <= ref". GX compares are RenderWare's minus one.
static bool gx_compare_always(int comp, int ref)
{
    return comp == 7 || (comp == 6 && ref == 0) || (comp == 3 && ref == 255);
}

extern "C" void GXSetAlphaCompare(int comp0, unsigned char ref0, int op, int comp1, unsigned char ref1)
{
    bool always0 = gx_compare_always(comp0, ref0), always1 = gx_compare_always(comp1, ref1);
    if (op == 1 && (always0 || always1)) // OR
    {
        comp0 = 7;
    }
    else if (always0)
    {
        comp0 = comp1;
        ref0 = ref1;
    }
    s_gxAlphaFunc = comp0 + 1;
    s_gxAlphaRef = ref0;
}

// RenderWare re-applies its own alpha test state.
void pc_render_reset_gx_alpha()
{
    s_gxAlphaFunc = -1;
}

// Z before or after texturing: alpha-tested fragments never write depth on the
// PC or 3DS GPUs either way.
extern "C" void GXSetZCompLoc(int beforeTex) {}
extern "C" void GXSetMisc(int token, int value) {}
extern "C" void GXAbortFrame() {}
extern "C" void GXDrawDone() {}
