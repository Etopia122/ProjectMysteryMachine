// citro3d graphics backend for the New 3DS port (see pc/rw/gpu.h).
//
// The game's 640x480 frame is drawn into a 320x240 4:3 area in the middle of
// the 400x240 top screen. The top screen's framebuffer is mounted sideways,
// so screen draws get a quarter-turn rotation on top of the projection.
// The HUD goes to the bottom screen instead (gpu_select_screen): the same
// 640x480 space at half size fills its 320x240. PC_HUD_TOP=1 keeps it on top
// (and the bottom screen for the text console).
// Vertex data is copied per draw into a per-frame arena in linear memory.
#include <3ds.h>
#include <citro3d.h>
#include <stdlib.h>
#include <string.h>
#include <unordered_map>

#include "gpu.h"
#include "pc_platform.h"
#include "scene_shbin.h"
#include "skin_shbin.h"

extern "C" void n3ds_console_off(); // main.cpp

#define DISPLAY_TRANSFER_FLAGS \
    (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
     GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
     GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

enum { SCREEN_W = 400, SCREEN_H = 240, GAME_W = 320, GAME_H = 240, BOTTOM_W = 320 };

struct Vert
{
    float x, y, z;
    u8 col[4];
    float u, v;
};

// Texture or render target behind a raster.
struct C3dRaster
{
    C3D_Tex tex;
    bool hasTex;
    float su, sv;              // UV scale: image size / padded texture size
    C3D_RenderTarget* target;  // camera texture rasters
};

static C3D_RenderTarget* s_top;
static DVLB_s* s_skinDvlb;
static shaderProgram_s s_skinProgram;
static int s_uBones = -1;      // < 0: no GPU skinning
static C3D_AttrInfo s_attrPlain, s_attrSkin;
static int s_curProgram;       // 0 scene.v.pica, 1 skin.v.pica
static C3D_RenderTarget* s_bottom; // NULL: the HUD stays on the top screen
static int s_screen;               // where screen draws go: 0 top, 1 bottom
static int s_gameW = 400;          // the top screen's game area in play: all of it (PC_ASPECT43=1: 320, 4:3)
static bool s_wide;                // in play now (gpu_set_wide): else the game area is 4:3

static int game_w() { return s_wide ? s_gameW : GAME_W; }
static DVLB_s* s_dvlb;
static shaderProgram_s s_program;
static int s_uMvp, s_uUvScale;
static bool s_inFrame;
static PcRaster* s_target;

// Per-frame vertex/index arena, double-buffered (the GPU reads one frame's
// data while the next is written).
static const size_t ARENA_SIZE = 6 * 1024 * 1024;
static u8* s_arena[2];
static int s_arenaIdx;
static size_t s_arenaUsed;
static bool s_arenaFull;

static float s_proj[16], s_mv[16];

// Static geometry (world sectors, unskinned models) converted once into linear
// memory, with its index lists, until gpu_forget.
// A skinned mesh for skin.v.pica: its triangles split into batches that use at
// most SKIN_MAX_BONES bones each (the vertex shader's uniform space), every
// batch with its own vertices (bone indices local to the batch) and indices.
enum { SKIN_MAX_BONES = 28 };

struct SkinVert
{
    float x, y, z;
    u8 col[4];
    float u, v;
    u8 bone[4];
    u8 weight[4];
};

struct SkinBatch
{
    int bones[SKIN_MAX_BONES];
    int numBones;
    SkinVert* verts;
    u16* idx;
    int numIdx;
};

struct SkinMesh
{
    const void* col; // the colour array it was built with
    std::vector<SkinBatch> batches;
};

struct StaticBuf
{
    Vert* verts;
    int count;
    std::unordered_map<const RwUInt16*, u16*> indices;
    std::unordered_map<const RwUInt16*, SkinMesh> skinned;
};
static std::unordered_map<const void*, StaticBuf> s_static;
static size_t s_staticBytes;

// The last dynamic conversion this frame: the meshes of one model share their
// arrays (GpuVerts.reuseKey), so consecutive draws can reuse it when nothing
// differs and the arrays weren't rewritten since (gpu_dynamic_changed).
static struct
{
    GpuVerts v;
    Vert* out;
} s_lastDyn;

static bool same_dynamic(const GpuVerts& a, const GpuVerts& b)
{
    return a.reuseKey && a.reuseKey == b.reuseKey && a.pos == b.pos && a.posStride == b.posStride &&
           a.posSize == b.posSize && a.col == b.col && (a.col ? a.colStride == b.colStride
                                                              : memcmp(&a.constCol, &b.constCol, sizeof(RwRGBA)) == 0) &&
           a.uv == b.uv && a.uvStride == b.uvStride && a.count == b.count;
}

void gpu_dynamic_changed()
{
    s_lastDyn.out = NULL;
}

// The state last given to citro3d: each setter marks its part of the context
// dirty and the next draw sends it again, so unchanged state isn't set again
// (forgotten at the start of each frame).
static struct
{
    bool valid;
    u32 depth, blend, alpha, cull, env, envAdd, envMat, fog;
    C3D_Tex* tex;
    u32 texParam;
    float su, sv;
    const void* buf;
} s_applied;

static void forget_applied_state()
{
    s_applied.valid = false;
    s_applied.tex = NULL;
    s_applied.buf = NULL;
}

static void apply_texture(const GpuState& s, struct C3dRaster* tex);


// PC_PROF=1: per-second frame profile (frame time, draws, vertices, uploads).
static struct { int frames, draws, verts, uploads; u64 uploadTicks, beginTicks, endTicks, start; } s_prof;
static bool prof_on() { static int on = -1; if (on < 0) on = getenv("PC_PROF") != NULL; return on; }
static bool s_mvpDirty = true;

// ---------------------------------------------------------------- setup
bool gpu_open()
{
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE * 8);
    s_top = C3D_RenderTargetCreate(SCREEN_H, SCREEN_W, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    C3D_RenderTargetSetOutput(s_top, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);
    const char* aspect = getenv("PC_ASPECT43");
    s_gameW = aspect && atoi(aspect) ? GAME_W : SCREEN_W;
    const char* hudTop = getenv("PC_HUD_TOP");
    if (!hudTop || !atoi(hudTop))
    {
        s_bottom = C3D_RenderTargetCreate(SCREEN_H, BOTTOM_W, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
        if (s_bottom)
        {
            n3ds_console_off();
            C3D_RenderTargetSetOutput(s_bottom, GFX_BOTTOM, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);
        }
    }
    s_dvlb = DVLB_ParseFile((u32*)scene_shbin, scene_shbin_size);
    shaderProgramInit(&s_program);
    shaderProgramSetVsh(&s_program, &s_dvlb->DVLE[0]);
    C3D_BindProgram(&s_program);
    s_uMvp = shaderInstanceGetUniformLocation(s_program.vertexShader, "mvp");
    s_uUvScale = shaderInstanceGetUniformLocation(s_program.vertexShader, "uvScale");
    C3D_AttrInfo* attr = C3D_GetAttrInfo();
    AttrInfo_Init(attr);
    AttrInfo_AddLoader(attr, 0, GPU_FLOAT, 3);         // v0 position
    AttrInfo_AddLoader(attr, 1, GPU_UNSIGNED_BYTE, 4); // v1 colour
    AttrInfo_AddLoader(attr, 2, GPU_FLOAT, 2);         // v2 texture coordinates
    s_attrPlain = *attr;
    // The skinning program: the same uniforms first, then the bones.
    s_skinDvlb = DVLB_ParseFile((u32*)skin_shbin, skin_shbin_size);
    shaderProgramInit(&s_skinProgram);
    shaderProgramSetVsh(&s_skinProgram, &s_skinDvlb->DVLE[0]);
    if (shaderInstanceGetUniformLocation(s_skinProgram.vertexShader, "mvp") == s_uMvp &&
        shaderInstanceGetUniformLocation(s_skinProgram.vertexShader, "uvScale") == s_uUvScale &&
        !(getenv("PC_DBG") && strstr(getenv("PC_DBG"), "cpuskin")))
    {
        s_uBones = shaderInstanceGetUniformLocation(s_skinProgram.vertexShader, "bones");
    }
    AttrInfo_Init(&s_attrSkin);
    AttrInfo_AddLoader(&s_attrSkin, 0, GPU_FLOAT, 3);         // v0 position
    AttrInfo_AddLoader(&s_attrSkin, 1, GPU_UNSIGNED_BYTE, 4); // v1 colour
    AttrInfo_AddLoader(&s_attrSkin, 2, GPU_FLOAT, 2);         // v2 texture coordinates
    AttrInfo_AddLoader(&s_attrSkin, 3, GPU_UNSIGNED_BYTE, 4); // v3 bone indices
    AttrInfo_AddLoader(&s_attrSkin, 4, GPU_UNSIGNED_BYTE, 4); // v4 bone weights
    pc_log("[c3d] GPU skinning %s", s_uBones >= 0 ? "on" : "off");
    for (int i = 0; i < 2; i++)
    {
        s_arena[i] = (u8*)linearAlloc(ARENA_SIZE);
    }
    // Depth = -z/w: 0 at the near plane, 1 at the far plane (see load_mvp).
    C3D_DepthMap(true, -1.0f, 0.0f);
    pc_log("[c3d] ready: arenas %s, linear free %lu KB", s_arena[1] ? "ok" : "FAILED",
           (unsigned long)(linearSpaceFree() / 1024));
    return s_arena[0] && s_arena[1];
}

void gpu_close()
{
    C3D_Fini();
}

bool gpu_ready() { return s_top != NULL; }

bool gpu_second_screen() { return s_bottom != NULL; }

// Where screen draws (no render target) go.
static C3D_RenderTarget* screen_target() { return s_screen && s_bottom ? s_bottom : s_top; }

static void begin_frame()
{
    if (s_inFrame)
    {
        return;
    }
    u64 t0 = svcGetSystemTick();
    // No vblank sync here: the game paces itself on the VI retrace count.
    C3D_FrameBegin(0);
    s_prof.beginTicks += svcGetSystemTick() - t0;
    s_inFrame = true;
    s_arenaIdx ^= 1;
    s_arenaUsed = 0;
    s_arenaFull = false;
    s_lastDyn.out = NULL;
    forget_applied_state();
    s_applied.fog = 2; // unknown
    s_target = NULL;
    s_screen = 0;
    if (s_bottom)
    {
        // A blank bottom screen in frames that draw no HUD (menus, movies).
        C3D_RenderTargetClear(s_bottom, C3D_CLEAR_ALL, 0x000000FF, 0);
        C3D_FrameDrawOn(s_bottom);
    }
    C3D_FrameDrawOn(s_top);
}

static void* arena_alloc(size_t n)
{
    n = (n + 15) & ~(size_t)15;
    if (s_arenaUsed + n > ARENA_SIZE)
    {
        if (!s_arenaFull)
        {
            pc_log("[c3d] vertex arena full: draws dropped this frame");
        }
        s_arenaFull = true;
        return NULL;
    }
    void* p = s_arena[s_arenaIdx] + s_arenaUsed;
    s_arenaUsed += n;
    return p;
}

// ---------------------------------------------------------------- textures
static C3dRaster* c3d_raster(PcRaster* p)
{
    if (!p->gpu)
    {
        p->gpu = calloc(1, sizeof(C3dRaster));
    }
    return (C3dRaster*)p->gpu;
}

static int pow2_at_least(int n)
{
    int p = 8;
    while (p < n && p < 1024)
    {
        p <<= 1;
    }
    return p;
}

// Index of texel (x, y) inside its 8x8 tile (Morton order).
static inline u32 morton(u32 x, u32 y)
{
    return (x & 1) | ((y & 1) << 1) | ((x & 2) << 1) | ((y & 2) << 2) | ((x & 4) << 2) | ((y & 4) << 3);
}

// Converts row-major RGBA into the PICA200's tiled RGBA8 layout. The GPU
// samples t = 0 from the last row in memory and the game's texture
// coordinates have t = 0 at the image's first (top) row, so rows are stored
// bottom to top.
static void tile_rgba(const u8* src, int w, int h, u32* dst, int tw, int th)
{
    memset(dst, 0, (size_t)tw * th * 4);
    for (int y = 0; y < h && y < th; y++)
    {
        const u8* s = src + (size_t)y * w * 4;
        int ty = th - 1 - y;
        u32* row = dst + (size_t)(ty / 8) * (tw / 8) * 64;
        for (int x = 0; x < w && x < tw; x++, s += 4)
        {
            row[(x / 8) * 64 + morton(x & 7, ty & 7)] = (u32)s[0] << 24 | (u32)s[1] << 16 | (u32)s[2] << 8 | s[3];
        }
    }
}

void gpu_prepare_texture(PcRaster* p)
{
    if (p->isTarget || !p->rgba)
    {
        return;
    }
    C3dRaster* c = c3d_raster(p);
    if (c->hasTex && !p->dirty)
    {
        return;
    }
    int w = p->r.width, h = p->r.height;
    int tw = pow2_at_least(w), th = pow2_at_least(h);
    if (!c->hasTex || c->tex.width != tw || c->tex.height != th)
    {
        if (c->hasTex)
        {
            C3D_TexDelete(&c->tex);
            s_applied.tex = NULL; // a new texture may reuse its address
        }
        c->hasTex = C3D_TexInit(&c->tex, tw, th, GPU_RGBA8);
        if (!c->hasTex)
        {
            pc_log("[c3d] texture %dx%d: out of linear memory (%lu KB free)", tw, th,
                   (unsigned long)(linearSpaceFree() / 1024));
            p->dirty = 0;
            return;
        }
    }
    u64 t0 = svcGetSystemTick();
    tile_rgba(p->rgba, w, h, (u32*)c->tex.data, tw, th);
    C3D_TexFlush(&c->tex);
    s_prof.uploads++;
    s_prof.uploadTicks += svcGetSystemTick() - t0;
    c->su = (float)w / tw;
    c->sv = (float)h / th;
    p->dirty = 0;
    pc_raster_scan_alpha(p);
}

void gpu_raster_free(PcRaster* r)
{
    C3dRaster* c = (C3dRaster*)r->gpu;
    if (c)
    {
        if (c->target)
        {
            C3D_RenderTargetDelete(c->target);
        }
        if (c->hasTex)
        {
            C3D_TexDelete(&c->tex);
            s_applied.tex = NULL; // a new texture may reuse its address
        }
        free(c);
    }
    r->gpu = NULL;
    r->isTarget = 0;
}

// ---------------------------------------------------------------- targets
static void create_target(PcRaster* p)
{
    C3dRaster* c = c3d_raster(p);
    if (c->target)
    {
        return;
    }
    int w = p->r.width, h = p->r.height;
    int tw = pow2_at_least(w), th = pow2_at_least(h);
    if (c->hasTex)
    {
        C3D_TexDelete(&c->tex);
        s_applied.tex = NULL; // a new texture may reuse its address
    }
    c->hasTex = C3D_TexInitVRAM(&c->tex, tw, th, GPU_RGBA8) || C3D_TexInit(&c->tex, tw, th, GPU_RGBA8);
    if (!c->hasTex)
    {
        pc_log("[c3d] render target %dx%d: no memory", w, h);
        return;
    }
    c->target = C3D_RenderTargetCreateFromTex(&c->tex, GPU_TEXFACE_2D, 0, GPU_RB_DEPTH24_STENCIL8);
    c->su = (float)w / tw;
    c->sv = (float)h / th;
    p->isTarget = c->target != NULL;
    pc_log("[c3d] render target %dx%d (texture %dx%d)", w, h, tw, th);
}

static void set_viewport()
{
    if (s_target)
    {
        C3dRaster* c = (C3dRaster*)s_target->gpu;
        // Texture rows run bottom to top: the image's top rows are at the top of
        // the padded texture.
        C3D_SetViewport(0, c->tex.height - s_target->r.height, s_target->r.width, s_target->r.height);
    }
    else if (screen_target() == s_bottom)
    {
        // The whole bottom screen (rotated: 240 wide, 320 tall).
        C3D_SetViewport(0, 0, SCREEN_H, BOTTOM_W);
    }
    else
    {
        // The rotated framebuffer is 240 wide and 400 tall: the game area is all
        // of it (5:3, the 3D view widened), or 320 in the middle (4:3).
        C3D_SetViewport(0, (SCREEN_W - game_w()) / 2, SCREEN_H, game_w());
    }
}

static void bind_target(PcRaster* p)
{
    begin_frame();
    if (p)
    {
        create_target(p);
        if (!p->isTarget)
        {
            p = NULL;
        }
    }
    s_target = p;
    C3D_FrameDrawOn(p ? ((C3dRaster*)p->gpu)->target : screen_target());
    set_viewport();
    s_mvpDirty = true;
}

void gpu_set_target(PcRaster* target)
{
    bind_target(target);
}

PcRaster* gpu_target() { return s_target; }

void gpu_select_screen(int screen)
{
    begin_frame();
    s_screen = screen;
    if (!s_target)
    {
        bind_target(NULL);
    }
}

void gpu_set_projection(const float* m)
{
    memcpy(s_proj, m, sizeof(s_proj));
    s_mvpDirty = true;
}

void gpu_set_modelview(const float* m)
{
    memcpy(s_mv, m, sizeof(s_mv));
    s_mvpDirty = true;
}

// o = a * b (column-major 4x4).
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

// projection * modelview, then: OpenGL clip depth [-w, w] to the PICA200's
// [0, -w] (near to far), and for the screen a quarter turn for the sideways
// framebuffer.
static void load_mvp()
{
    float m[16];
    mul16(s_proj, s_mv, m);
    C3D_Mtx out;
    for (int r = 0; r < 4; r++)
    {
        float x = m[r], y = m[4 + r], z = m[8 + r], w = m[12 + r];
        out.r[r].x = x, out.r[r].y = y, out.r[r].z = z, out.r[r].w = w;
    }
    // z' = -(z + w) / 2
    for (int c = 0; c < 4; c++)
    {
        out.r[2].c[c] = -0.5f * (out.r[2].c[c] + out.r[3].c[c]);
    }
    if (!s_target)
    {
        // Screen x runs along the framebuffer's y axis, screen y against its x.
        C3D_FVec rx = out.r[0], ry = out.r[1];
        out.r[0] = ry;
        out.r[1] = FVec4_Negate(rx);
    }
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, s_uMvp, &out);
    // Perspective: W-buffer, depth = -z' / far, linear in eye distance from 0 at
    // the near plane to 1 at the far plane (so fog can be indexed linearly).
    // Orthographic (2D): Z-buffer, depth = -z'/w as before.
    if (s_proj[11] != 0.0f && s_proj[10] != 1.0f)
    {
        float far = -s_proj[14] / (s_proj[10] - 1.0f);
        C3D_DepthMap(false, -1.0f / far, 0.0f);
    }
    else
    {
        C3D_DepthMap(true, -1.0f, 0.0f);
    }
    s_mvpDirty = false;
}

void gpu_clear(PcRaster* target, const RwRGBA* col, bool depth)
{
    begin_frame();
    C3D_RenderTarget* t = screen_target();
    if (target)
    {
        create_target(target);
        if (target->isTarget)
        {
            t = ((C3dRaster*)target->gpu)->target;
        }
    }
    C3D_ClearBits bits = (C3D_ClearBits)((col ? C3D_CLEAR_COLOR : 0) | (depth ? C3D_CLEAR_DEPTH : 0));
    u32 c = col ? ((u32)col->red << 24 | (u32)col->green << 16 | (u32)col->blue << 8 | col->alpha) : 0;
    C3D_RenderTargetClear(t, bits, c, 0x00FFFFFF);
}

// ---------------------------------------------------------------- drawing
static GPU_BLENDFACTOR blend_factor(int b)
{
    switch (b)
    {
    case 1: return GPU_ZERO;
    case 2: return GPU_ONE;
    case 3: return GPU_SRC_COLOR;
    case 4: return GPU_ONE_MINUS_SRC_COLOR;
    case 5: return GPU_SRC_ALPHA;
    case 6: return GPU_ONE_MINUS_SRC_ALPHA;
    case 7: return GPU_DST_ALPHA;
    case 8: return GPU_ONE_MINUS_DST_ALPHA;
    case 9: return GPU_DST_COLOR;
    case 10: return GPU_ONE_MINUS_DST_COLOR;
    case 11: return GPU_SRC_ALPHA_SATURATE;
    default: return GPU_ONE;
    }
}

static GPU_TESTFUNC test_func(int f)
{
    switch (f)
    {
    case 1: return GPU_NEVER;
    case 2: return GPU_LESS;
    case 3: return GPU_EQUAL;
    case 4: return GPU_LEQUAL;
    case 5: return GPU_GREATER;
    case 6: return GPU_NOTEQUAL;
    case 7: return GPU_GEQUAL;
    default: return GPU_ALWAYS;
    }
}

static GPU_TEXTURE_WRAP_PARAM wrap(int a)
{
    switch (a)
    {
    case 2: return GPU_MIRRORED_REPEAT;
    case 3: case 4: return GPU_CLAMP_TO_EDGE;
    default: return GPU_REPEAT;
    }
}

// Fog: OpenGL-style linear fog by eye distance, which the PICA200 does with a
// lookup table indexed by the fragment's depth (0 near to 1 far here, see
// load_mvp). Each entry converts its depth back to eye distance through the
// current perspective projection. 2D (orthographic) draws get no fog.
static void apply_fog(const GpuState& s)
{
    static C3D_FogLut lut;
    static float key[4] = { -1, -1, -1, -1 };
    bool perspective = s_proj[11] != 0.0f;
    if (!s.fog || !perspective || s.fogEnd <= s.fogStart)
    {
        if (s_applied.fog != 0)
        {
            s_applied.fog = 0;
            C3D_FogGasMode(GPU_NO_FOG, GPU_PLAIN_DENSITY, false);
        }
        return;
    }
    bool changed = s_applied.fog != 1;
    s_applied.fog = 1;
    float a = s_proj[10], b = s_proj[14];
    if (key[0] != s.fogStart || key[1] != s.fogEnd || key[2] != a || key[3] != b)
    {
        changed = true;
        key[0] = s.fogStart, key[1] = s.fogEnd, key[2] = a, key[3] = b;
        // Perspective draws use a W-buffer: depth is linear from the near plane
        // (0) to the far plane (1), see load_mvp.
        float n = -b / (a + 1.0f), f = -b / (a - 1.0f);
        float data[256];
        for (int i = 0; i <= 128; i++)
        {
            float z = n + (i / 128.0f) * (f - n); // eye distance
            float vis = (s.fogEnd - z) / (s.fogEnd - s.fogStart);
            vis = vis < 0.0f ? 0.0f : vis > 1.0f ? 1.0f : vis;
            if (i < 128)
            {
                data[i] = vis;
            }
            if (i > 0)
            {
                data[i + 127] = vis - data[i - 1];
            }
        }
        FogLut_FromArray(&lut, data);
    }
    static u32 colour;
    u32 c = (u32)s.fogColor.red | (u32)s.fogColor.green << 8 | (u32)s.fogColor.blue << 16;
    if (changed || c != colour)
    {
        colour = c;
        C3D_FogGasMode(GPU_FOG, GPU_PLAIN_DENSITY, false);
        C3D_FogColor(c);
        C3D_FogLutBind(&lut);
    }
}

static void apply_state(const GpuState& s, C3dRaster* tex)
{
    bool all = !s_applied.valid;
    s_applied.valid = true;
    // Depth: as in OpenGL, no depth test means no depth writes either.
    u32 writes = (s.colorWrite ? GPU_WRITE_COLOR & ~GPU_WRITE_ALPHA : 0) | (s.alphaWrite ? GPU_WRITE_ALPHA : 0);
    u32 depth = s.zTest ? (writes | (s.zWrite ? GPU_WRITE_DEPTH : 0)) | 0x100 : writes;
    if (all || depth != s_applied.depth)
    {
        s_applied.depth = depth;
        if (s.zTest)
        {
            C3D_DepthTest(true, GPU_LEQUAL, (GPU_WRITEMASK)(writes | (s.zWrite ? GPU_WRITE_DEPTH : 0)));
        }
        else
        {
            C3D_DepthTest(true, GPU_ALWAYS, (GPU_WRITEMASK)writes);
        }
    }
    GPU_BLENDFACTOR src = GPU_ONE, dst = GPU_ZERO;
    if (s.blend)
    {
        src = blend_factor(s.srcBlend);
        dst = blend_factor(s.dstBlend);
        if (s.srcBlend == 4 && s.dstBlend == 9)
        {
            // INVSRCCOLOR/DESTCOLOR (the drop shadows): on GameCube this darkens
            // the frame by the source alpha.
            src = GPU_ZERO;
            dst = GPU_ONE_MINUS_SRC_ALPHA;
        }
    }
    u32 blend = (u32)src | (u32)dst << 8;
    if (all || blend != s_applied.blend)
    {
        s_applied.blend = blend;
        C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, src, dst, src, dst);
    }
    u32 alpha = (u32)test_func(s.alphaFunc) | (u32)s.alphaRef << 8;
    if (all || alpha != s_applied.alpha)
    {
        s_applied.alpha = alpha;
        C3D_AlphaTest(true, test_func(s.alphaFunc), s.alphaRef);
    }
    // RenderWare culls back faces (mode 2) with clockwise fronts on screen.
    GPU_CULLMODE cull = GPU_CULL_NONE;
    if (s.cull == PC_CULL_BACK || s.cull == PC_CULL_FRONT)
    {
        bool front = s.cull == PC_CULL_FRONT;
        if (s_target)
        {
            front = !front; // render targets are drawn upside down
        }
        cull = front ? GPU_CULL_FRONT_CCW : GPU_CULL_BACK_CCW;
    }
    if (all || (u32)cull != s_applied.cull)
    {
        s_applied.cull = cull;
        C3D_CullFace(cull);
    }
    // Colour combiners: clamp(vertex colour + add colour) * material colour * texture.
    // Each stage is set only when it changes (citro3d resends the stages fetched).
    const RwRGBA& a = s.addColor;
    const RwRGBA& m = s.matColor;
    bool hasAdd = a.red || a.green || a.blue;
    bool hasMat = m.red != 255 || m.green != 255 || m.blue != 255 || m.alpha != 255;
    // The colours are the keys: no add colour is black, no material colour white.
    u32 envAdd = (u32)a.red | (u32)a.green << 8 | (u32)a.blue << 16;
    u32 envMat = (u32)m.red | (u32)m.green << 8 | (u32)m.blue << 16 | (u32)m.alpha << 24;
    u32 envTex = tex ? 1 : 0;
    if (all || envAdd != s_applied.envAdd)
    {
        s_applied.envAdd = envAdd;
        C3D_TexEnv* env = C3D_GetTexEnv(0);
        C3D_TexEnvInit(env);
        if (hasAdd)
        {
            C3D_TexEnvSrc(env, C3D_RGB, GPU_PRIMARY_COLOR, GPU_CONSTANT, GPU_PRIMARY_COLOR);
            C3D_TexEnvFunc(env, C3D_RGB, GPU_ADD);
            C3D_TexEnvSrc(env, C3D_Alpha, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
            C3D_TexEnvFunc(env, C3D_Alpha, GPU_REPLACE);
            C3D_TexEnvColor(env, (u32)a.red | (u32)a.green << 8 | (u32)a.blue << 16);
        }
        else
        {
            C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
            C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
        }
    }
    if (all || envMat != s_applied.envMat)
    {
        s_applied.envMat = envMat;
        C3D_TexEnv* env1 = C3D_GetTexEnv(1);
        C3D_TexEnvInit(env1);
        if (hasMat)
        {
            C3D_TexEnvSrc(env1, C3D_Both, GPU_PREVIOUS, GPU_CONSTANT, GPU_PREVIOUS);
            C3D_TexEnvFunc(env1, C3D_Both, GPU_MODULATE);
            C3D_TexEnvColor(env1, envMat);
        }
        else
        {
            C3D_TexEnvSrc(env1, C3D_Both, GPU_PREVIOUS, GPU_PREVIOUS, GPU_PREVIOUS);
            C3D_TexEnvFunc(env1, C3D_Both, GPU_REPLACE);
        }
    }
    if (all || envTex != s_applied.env)
    {
        s_applied.env = envTex;
        C3D_TexEnv* env2 = C3D_GetTexEnv(2);
        C3D_TexEnvInit(env2);
        if (tex)
        {
            C3D_TexEnvSrc(env2, C3D_Both, GPU_PREVIOUS, GPU_TEXTURE0, GPU_PREVIOUS);
            C3D_TexEnvFunc(env2, C3D_Both, GPU_MODULATE);
        }
        else
        {
            C3D_TexEnvSrc(env2, C3D_Both, GPU_PREVIOUS, GPU_PREVIOUS, GPU_PREVIOUS);
            C3D_TexEnvFunc(env2, C3D_Both, GPU_REPLACE);
        }
    }
    apply_texture(s, tex);
    apply_fog(s);
}

// The texture unit: bound, filtered and wrapped as the draw wants, and its UV
// scale, when any of it differs from the last draw's.
static void apply_texture(const GpuState& s, C3dRaster* tex)
{
    if (!tex)
    {
        return;
    }
    GPU_TEXTURE_FILTER_PARAM f = (s.filter == 1 || s.filter == 3 || s.filter == 4) ? GPU_NEAREST : GPU_LINEAR;
    C3D_TexSetFilter(&tex->tex, f, f);
    C3D_TexSetWrap(&tex->tex, wrap(s.addrU), wrap(s.addrV));
    if (s_applied.tex != &tex->tex || s_applied.texParam != tex->tex.param)
    {
        s_applied.tex = &tex->tex;
        s_applied.texParam = tex->tex.param;
        C3D_TexBind(0, &tex->tex);
    }
    if (s_applied.su != tex->su || s_applied.sv != tex->sv)
    {
        s_applied.su = tex->su;
        s_applied.sv = tex->sv;
        C3D_FVUnifSet(GPU_VERTEX_SHADER, s_uUvScale, tex->su, tex->sv, 1.0f, 1.0f);
    }
}

// Switches between the plain and the skinning vertex shader (with their
// attribute layouts; the vertex buffer has to be set again after).
static void use_program(int which)
{
    if (s_curProgram == which)
    {
        return;
    }
    s_curProgram = which;
    C3D_BindProgram(which ? &s_skinProgram : &s_program);
    *C3D_GetAttrInfo() = which ? s_attrSkin : s_attrPlain;
    s_applied.buf = NULL;
}

bool gpu_skinning() { return s_uBones >= 0; }

// Splits a skinned mesh into batches of at most SKIN_MAX_BONES bones.
static void build_skin_mesh(SkinMesh& m, const GpuVerts& v, const GpuSkin& sk, GpuPrim prim, const RwUInt16* idx,
                            int nidx)
{
    for (size_t b = 0; b < m.batches.size(); b++)
    {
        linearFree(m.batches[b].verts);
        linearFree(m.batches[b].idx);
    }
    m.batches.clear();
    m.col = v.col;
    // Triangles, with the winding a strip or fan gives them.
    std::vector<u16> tris;
    if (prim == PRIM_TRISTRIP)
    {
        for (int i = 0; i + 2 < nidx; i++)
        {
            u16 a = idx[i], b = idx[i + 1], c = idx[i + 2];
            if (a == b || b == c || a == c)
            {
                continue;
            }
            if (i & 1)
            {
                u16 t = a;
                a = b;
                b = t;
            }
            tris.push_back(a), tris.push_back(b), tris.push_back(c);
        }
    }
    else if (prim == PRIM_TRIFAN)
    {
        for (int i = 1; i + 1 < nidx; i++)
        {
            tris.push_back(idx[0]), tris.push_back(idx[i]), tris.push_back(idx[i + 1]);
        }
    }
    else
    {
        tris.assign(idx, idx + nidx);
    }
    std::vector<int> local(v.count, -1);  // vertex -> batch vertex
    std::vector<int> slot(sk.numBones, -1); // bone -> batch bone slot
    size_t t = 0;
    while (t < tris.size())
    {
        SkinBatch batch;
        batch.numBones = 0;
        std::vector<int> verts; // the batch's vertices (global indices)
        std::vector<u16> bidx;
        for (; t + 2 < tris.size(); t += 3)
        {
            // The bones this triangle adds.
            int add[12], nadd = 0;
            for (int k = 0; k < 3; k++)
            {
                int vi = tris[t + k];
                for (int j = 0; j < 4; j++)
                {
                    int bone = sk.indices[vi * 4 + j];
                    if (sk.weights[vi * 4 + j] == 0.0f || bone >= sk.numBones || slot[bone] >= 0)
                    {
                        continue;
                    }
                    bool dup = false;
                    for (int q = 0; q < nadd; q++)
                    {
                        dup |= add[q] == bone;
                    }
                    if (!dup)
                    {
                        add[nadd++] = bone;
                    }
                }
            }
            if (batch.numBones + nadd > SKIN_MAX_BONES)
            {
                break;
            }
            for (int q = 0; q < nadd; q++)
            {
                slot[add[q]] = batch.numBones;
                batch.bones[batch.numBones++] = add[q];
            }
            for (int k = 0; k < 3; k++)
            {
                int vi = tris[t + k];
                if (local[vi] < 0)
                {
                    local[vi] = (int)verts.size();
                    verts.push_back(vi);
                }
                bidx.push_back((u16)local[vi]);
            }
        }
        if (bidx.empty())
        {
            break; // can't happen: a triangle uses at most 12 bones
        }
        batch.numIdx = (int)bidx.size();
        batch.verts = (SkinVert*)linearAlloc(sizeof(SkinVert) * verts.size());
        batch.idx = (u16*)linearAlloc(sizeof(u16) * bidx.size());
        if (!batch.verts || !batch.idx)
        {
            linearFree(batch.verts);
            linearFree(batch.idx);
            break;
        }
        memcpy(batch.idx, bidx.data(), sizeof(u16) * bidx.size());
        for (size_t i = 0; i < verts.size(); i++)
        {
            int vi = verts[i];
            SkinVert& o = batch.verts[i];
            const float* p = (const float*)((const u8*)v.pos + (size_t)vi * v.posStride);
            o.x = p[0], o.y = p[1], o.z = v.posSize > 2 ? p[2] : 0.0f;
            const RwRGBA* c = v.col ? (const RwRGBA*)((const u8*)v.col + (size_t)vi * v.colStride) : NULL;
            o.col[0] = c ? c->red : 255, o.col[1] = c ? c->green : 255;
            o.col[2] = c ? c->blue : 255, o.col[3] = c ? c->alpha : 255;
            const float* uv = v.uv ? (const float*)((const u8*)v.uv + (size_t)vi * v.uvStride) : NULL;
            o.u = uv ? uv[0] : 0.0f, o.v = uv ? uv[1] : 0.0f;
            for (int j = 0; j < 4; j++)
            {
                int bone = sk.indices[vi * 4 + j];
                float w = sk.weights[vi * 4 + j];
                if (w == 0.0f || bone >= sk.numBones)
                {
                    o.bone[j] = 0, o.weight[j] = 0;
                    continue;
                }
                int q = (int)(w * 255.0f + 0.5f);
                o.bone[j] = (u8)slot[bone];
                o.weight[j] = (u8)(q < 0 ? 0 : q > 255 ? 255 : q);
            }
        }
        GSPGPU_FlushDataCache(batch.verts, sizeof(SkinVert) * verts.size());
        GSPGPU_FlushDataCache(batch.idx, sizeof(u16) * bidx.size());
        s_staticBytes += sizeof(SkinVert) * verts.size() + sizeof(u16) * bidx.size();
        m.batches.push_back(batch);
        for (size_t i = 0; i < verts.size(); i++)
        {
            local[verts[i]] = -1;
        }
        for (int q = 0; q < batch.numBones; q++)
        {
            slot[batch.bones[q]] = -1;
        }
    }
}

void gpu_draw_skinned(const GpuState& s, const GpuVerts& v, const GpuSkin& sk, GpuPrim prim, const RwUInt16* idx,
                      int nidx)
{
    if (s_uBones < 0 || !v.cacheKey || !idx || nidx < 3 || v.count <= 0 || prim > PRIM_TRIFAN)
    {
        return;
    }
    begin_frame();
    StaticBuf& sb = s_static[v.cacheKey];
    SkinMesh& m = sb.skinned[idx];
    if (m.batches.empty() || m.col != v.col)
    {
        build_skin_mesh(m, v, sk, prim, idx, nidx);
    }
    C3dRaster* tex = NULL;
    if (s.tex && s.tex->gpu && ((C3dRaster*)s.tex->gpu)->hasTex && s.tex != s_target)
    {
        tex = (C3dRaster*)s.tex->gpu;
    }
    apply_state(s, tex);
    use_program(1);
    if (s_mvpDirty)
    {
        load_mvp();
    }
    for (size_t bi = 0; bi < m.batches.size(); bi++)
    {
        const SkinBatch& b = m.batches[bi];
        s_prof.draws++;
        C3D_FVec* u = C3D_FVUnifWritePtr(GPU_VERTEX_SHADER, s_uBones, b.numBones * 3);
        for (int j = 0; j < b.numBones; j++)
        {
            const RwMatrix& M = sk.bones[b.bones[j]];
            u[j * 3] = FVec4_New(M.right.x, M.up.x, M.at.x, M.pos.x);
            u[j * 3 + 1] = FVec4_New(M.right.y, M.up.y, M.at.y, M.pos.y);
            u[j * 3 + 2] = FVec4_New(M.right.z, M.up.z, M.at.z, M.pos.z);
        }
        if (s_applied.buf != b.verts)
        {
            s_applied.buf = b.verts;
            C3D_BufInfo* bufs = C3D_GetBufInfo();
            BufInfo_Init(bufs);
            BufInfo_Add(bufs, b.verts, sizeof(SkinVert), 5, 0x43210);
        }
        C3D_DrawElements(GPU_TRIANGLES, b.numIdx, C3D_UNSIGNED_SHORT, b.idx);
    }
}

static void convert_verts(const GpuVerts& v, Vert* out);
static void draw_converted(const GpuState& s, Vert* out, GpuPrim prim, GPU_Primitive_t p, u16* ix, int nidx, int count);

void gpu_draw(const GpuState& s, const GpuVerts& v, GpuPrim prim, const RwUInt16* idx, int nidx)
{
    GPU_Primitive_t p;
    switch (prim)
    {
    case PRIM_TRIANGLES: p = GPU_TRIANGLES; break;
    case PRIM_TRISTRIP: p = GPU_TRIANGLE_STRIP; break;
    case PRIM_TRIFAN: p = GPU_TRIANGLE_FAN; break;
    default: return; // no line or point primitives on the PICA200
    }
    if (v.count <= 0 || (idx && nidx <= 0))
    {
        return;
    }
    begin_frame();
    s_prof.draws++;
    Vert* out = NULL;
    u16* ix = NULL;
    if (v.cacheKey)
    {
        StaticBuf& sb = s_static[v.cacheKey];
        if (!sb.verts || sb.count != v.count)
        {
            if (sb.verts)
            {
                linearFree(sb.verts);
                s_staticBytes -= sizeof(Vert) * sb.count;
            }
            sb.verts = (Vert*)linearAlloc(sizeof(Vert) * v.count);
            sb.count = v.count;
            if (sb.verts)
            {
                s_staticBytes += sizeof(Vert) * v.count;
                convert_verts(v, sb.verts);
                GSPGPU_FlushDataCache(sb.verts, sizeof(Vert) * v.count);
            }
        }
        out = sb.verts;
        if (idx)
        {
            u16*& cached = sb.indices[idx];
            if (!cached)
            {
                cached = (u16*)linearAlloc(sizeof(u16) * nidx);
                if (cached)
                {
                    memcpy(cached, idx, sizeof(u16) * nidx);
                    GSPGPU_FlushDataCache(cached, sizeof(u16) * nidx);
                }
            }
            ix = cached;
        }
    }
    if (!out && s_lastDyn.out && same_dynamic(v, s_lastDyn.v))
    {
        out = s_lastDyn.out;
    }
    if (!out)
    {
        s_prof.verts += v.count;
        out = (Vert*)arena_alloc(sizeof(Vert) * v.count);
        if (!out)
        {
            return;
        }
        convert_verts(v, out);
        s_lastDyn.v = v;
        s_lastDyn.out = out;
    }
    if (idx && !ix)
    {
        ix = (u16*)arena_alloc(sizeof(u16) * nidx);
        if (!ix)
        {
            return;
        }
        memcpy(ix, idx, sizeof(u16) * nidx);
    }
    draw_converted(s, out, prim, p, ix, nidx, v.count);
}

// Copies a draw's vertex arrays into the GPU layout.
static void convert_verts(const GpuVerts& v, Vert* out)
{
    const u8* pos = (const u8*)v.pos;
    const u8* col = (const u8*)v.col;
    const u8* uv = (const u8*)v.uv;
    for (int i = 0; i < v.count; i++)
    {
        const float* pp = (const float*)(pos + (size_t)i * v.posStride);
        out[i].x = pp[0];
        out[i].y = pp[1];
        out[i].z = v.posSize > 2 ? pp[2] : 0.0f;
        const RwRGBA* c = col ? (const RwRGBA*)(col + (size_t)i * v.colStride) : &v.constCol;
        out[i].col[0] = c->red;
        out[i].col[1] = c->green;
        out[i].col[2] = c->blue;
        out[i].col[3] = c->alpha;
        if (uv)
        {
            const float* t = (const float*)(uv + (size_t)i * v.uvStride);
            out[i].u = t[0];
            out[i].v = t[1];
        }
        else
        {
            out[i].u = out[i].v = 0.0f;
        }
    }
}

static void draw_converted(const GpuState& s, Vert* out, GpuPrim prim, GPU_Primitive_t p, u16* ix, int nidx, int count)
{
    C3dRaster* tex = NULL;
    if (s.tex && s.tex->gpu && ((C3dRaster*)s.tex->gpu)->hasTex && s.tex != s_target)
    {
        tex = (C3dRaster*)s.tex->gpu;
    }
    apply_state(s, tex);
    use_program(0);
    if (s_mvpDirty)
    {
        load_mvp();
    }
    if (s_applied.buf != out)
    {
        s_applied.buf = out;
        C3D_BufInfo* bufs = C3D_GetBufInfo();
        BufInfo_Init(bufs);
        BufInfo_Add(bufs, out, sizeof(Vert), 3, 0x210);
    }
    if (ix)
    {
        C3D_DrawElements(p, nidx, C3D_UNSIGNED_SHORT, ix);
    }
    else
    {
        C3D_DrawArrays(p, 0, count);
    }
}

float gpu_widen(bool toScreen)
{
    return toScreen && s_screen == 0 ? game_w() / (float)GAME_W : 1.0f;
}

void gpu_set_wide(bool on)
{
    s_wide = on;
}

int gpu_overlay_begin(int* width, int* height)
{
    // The overlay uses the game area (its pixels at half size on screen).
    s_screen = 0;
    gpu_set_target(NULL);
    *width = game_w() * 2;
    *height = 480;
    return 2;
}

// ---------------------------------------------------------------- present
void gpu_present()
{
    begin_frame();
    // The GPU reads the arena from memory, not the CPU's data cache.
    GSPGPU_FlushDataCache(s_arena[s_arenaIdx], s_arenaUsed);
    u64 t0 = svcGetSystemTick();
    C3D_FrameEnd(0);
    s_prof.endTicks += svcGetSystemTick() - t0;
    pc_stall_note(PC_STALL_PRESENT, svcGetSystemTick() - t0);
    s_inFrame = false;
    if (prof_on())
    {
        u64 now = svcGetSystemTick();
        if (!s_prof.start)
        {
            s_prof.start = now;
        }
        s_prof.frames++;
        double secs = (double)(now - s_prof.start) / SYSCLOCK_ARM11;
        if (secs >= 1.0)
        {
            double k = 1000.0 / SYSCLOCK_ARM11 / s_prof.frames;
            pc_log("[prof] gpu: %d draws, %d verts copied, %.1f uploads (%.1f ms), frame begin %.1f ms, end %.1f ms; "
                   "static %u KB, linear free %lu KB",
                   s_prof.draws / s_prof.frames, s_prof.verts / s_prof.frames, (double)s_prof.uploads / s_prof.frames,
                   s_prof.uploadTicks * k, s_prof.beginTicks * k, s_prof.endTicks * k, (unsigned)(s_staticBytes / 1024),
                   (unsigned long)(linearSpaceFree() / 1024));
            memset(&s_prof, 0, sizeof(s_prof));
            s_prof.start = now;
        }
    }
}

void gpu_forget(const void* key)
{
    std::unordered_map<const void*, StaticBuf>::iterator it = s_static.find(key);
    if (it == s_static.end())
    {
        return;
    }
    // The GPU may still be drawing the last frame that used it.
    if (s_inFrame)
    {
        C3D_FrameEnd(0);
        s_inFrame = false;
    }
    C3D_FrameSync();
    if (it->second.verts)
    {
        linearFree(it->second.verts);
    }
    s_staticBytes -= sizeof(Vert) * it->second.count;
    for (std::unordered_map<const RwUInt16*, SkinMesh>::iterator i = it->second.skinned.begin();
         i != it->second.skinned.end(); ++i)
    {
        for (size_t b = 0; b < i->second.batches.size(); b++)
        {
            linearFree(i->second.batches[b].verts);
            linearFree(i->second.batches[b].idx);
        }
    }
    for (std::unordered_map<const RwUInt16*, u16*>::iterator i = it->second.indices.begin(); i != it->second.indices.end(); ++i)
    {
        linearFree(i->second);
    }
    s_static.erase(it);
}

// Draws a movie raster (4:3, w x h) over the game area on black, and presents.
// v0/v1: the texture's t coordinates of the picture's top and bottom rows
// (after the raster's UV scale).
static void draw_movie(PcRaster* movie, int w, int h, float u1, float v0, float v1)
{
    RwRGBA black = { 0, 0, 0, 255 };
    gpu_clear(NULL, &black, true);
    gpu_set_target(NULL);
    // 4:3 in the middle of a wider game area.
    float k = gpu_widen(true);
    float p[16] = { 0 };
    p[0] = 2.0f / (w * k);
    p[5] = -2.0f / h;
    p[10] = -1.0f;
    p[12] = -1.0f / k;
    p[13] = 1.0f;
    p[15] = 1.0f;
    static const float identity[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
    gpu_set_projection(p);
    gpu_set_modelview(identity);
    GpuState s;
    memset(&s, 0, sizeof(s));
    s.alphaFunc = PC_ALPHA_ALWAYS;
    s.cull = PC_CULL_NONE;
    s.colorWrite = s.alphaWrite = true;
    s.tex = movie;
    s.filter = 2;          // linear
    s.addrU = s.addrV = 3; // clamp
    s.matColor = RwRGBA{ 255, 255, 255, 255 };
    const float pos[8] = { 0, 0, (float)w, 0, 0, (float)h, (float)w, (float)h };
    const float uv[8] = { 0, v0, u1, v0, 0, v1, u1, v1 };
    GpuVerts v;
    memset(&v, 0, sizeof(v));
    v.pos = pos;
    v.posStride = 2 * sizeof(float);
    v.posSize = 2;
    v.constCol = RwRGBA{ 255, 255, 255, 255 };
    v.uv = uv;
    v.uvStride = 2 * sizeof(float);
    v.count = 4;
    gpu_draw(s, v, PRIM_TRISTRIP, NULL, 0);
    gpu_present();
}

// Shows one movie frame (RGBA, 4:3) over the game area on black, and presents.
void gpu_fmv_frame(const unsigned char* rgba, int w, int h)
{
    static PcRaster movie;
    movie.r.width = w;
    movie.r.height = h;
    movie.rgba = (RwUInt8*)rgba;
    movie.dirty = 1;
    gpu_prepare_texture(&movie);
    draw_movie(&movie, w, h, 1.0f, 0.0f, 1.0f);
}

// ---------------------------------------------------------------- movies (YUV)
// The 3DS movie player hands over YCbCr 4:2:0 pictures (BT.601 studio range).
// The Y2R hardware converts them straight into the tiled texture by DMA; the
// CPU path below is the fallback when Y2R isn't available.
static PcRaster s_movie;
static C3dRaster s_movieTex;
static int s_y2r = -1; // -1 untried, 0 unavailable, 1 ready
static Handle s_y2rDone;

static bool movie_texture(int w, int h)
{
    int tw = pow2_at_least(w), th = pow2_at_least(h);
    if (s_movieTex.hasTex && (s_movieTex.tex.width != tw || s_movieTex.tex.height != th))
    {
        C3D_TexDelete(&s_movieTex.tex);
        s_applied.tex = NULL; // a new texture may reuse its address
        s_movieTex.hasTex = false;
    }
    if (!s_movieTex.hasTex)
    {
        s_movieTex.hasTex = C3D_TexInit(&s_movieTex.tex, tw, th, GPU_RGBA8);
        if (!s_movieTex.hasTex)
        {
            return false;
        }
        memset(s_movieTex.tex.data, 0, s_movieTex.tex.size);
        C3D_TexFlush(&s_movieTex.tex);
        s_movie.gpu = &s_movieTex;
    }
    s_movie.r.width = w;
    s_movie.r.height = h;
    return true;
}

// Hardware conversion; false if Y2R can't be used.
static bool convert_y2r(const u8* py, int ys, const u8* pu, const u8* pv, int cs, int w, int h)
{
    if (s_y2r < 0)
    {
        // Debug: PC_FMV_CPU=1 converts on the CPU.
        s_y2r = !getenv("PC_FMV_CPU") && R_SUCCEEDED(y2rInit()) ? 1 : 0;
        if (s_y2r)
        {
            Y2RU_SetTransferEndInterrupt(true);
            Y2RU_GetTransferEndEvent(&s_y2rDone);
        }
        pc_log("[fmv] Y2R %s", s_y2r ? "ready" : "unavailable: converting on the CPU");
    }
    if (!s_y2r || (w & 7) || (h & 7))
    {
        return false;
    }
    Y2RU_ConversionParams params;
    memset(&params, 0, sizeof(params));
    params.input_format = INPUT_YUV420_INDIV_8;
    params.output_format = OUTPUT_RGB_32;
    params.rotation = ROTATION_NONE;
    params.block_alignment = BLOCK_8_BY_8;
    params.input_line_width = (s16)w;
    params.input_lines = (s16)h;
    params.standard_coefficient = COEFFICIENT_ITU_R_BT_601_SCALING;
    params.alpha = 0xFF;
    Y2RU_SetConversionParams(&params);
    // The DMA reads memory, not the CPU's data cache.
    svcFlushProcessDataCache(CUR_PROCESS_HANDLE, (u32)py, (u32)(ys * h));
    svcFlushProcessDataCache(CUR_PROCESS_HANDLE, (u32)pu, (u32)(cs * h / 2));
    svcFlushProcessDataCache(CUR_PROCESS_HANDLE, (u32)pv, (u32)(cs * h / 2));
    Y2RU_SetSendingY(py, (u32)(w * h), (s16)w, (s16)(ys - w));
    Y2RU_SetSendingU(pu, (u32)(w * h / 4), (s16)(w / 2), (s16)(cs - w / 2));
    Y2RU_SetSendingV(pv, (u32)(w * h / 4), (s16)(w / 2), (s16)(cs - w / 2));
    // 8 output lines (one row of tiles) per transfer, then skip the texture's
    // padding to the next row of tiles.
    int tw = s_movieTex.tex.width;
    Y2RU_SetReceiving(s_movieTex.tex.data, (u32)(w * h * 4), (s16)(w * 8 * 4), (s16)((tw - w) * 8 * 4));
    Y2RU_StartConversion();
    if (svcWaitSynchronization(s_y2rDone, 100000000LL) != 0) // 100 ms
    {
        Y2RU_StopConversion();
        pc_log("[fmv] Y2R timed out: converting on the CPU from now on");
        s_y2r = 0;
        return false;
    }
    return true;
}

// CPU conversion into the tiled texture, two rows and two columns at a time
// sharing their chroma (rows stored bottom to top, see tile_rgba).
static void convert_cpu(const u8* py, int ys, const u8* pu, const u8* pv, int cs, int w, int h)
{
    static int yt[256], rv[256], gu[256], gv[256], bu[256];
    static u8 clip[1024];
    static bool tables;
    if (!tables)
    {
        tables = true;
        for (int i = 0; i < 256; i++)
        {
            yt[i] = (i - 16) * 298 + 128;
            rv[i] = 409 * (i - 128);
            gu[i] = -100 * (i - 128);
            gv[i] = -208 * (i - 128);
            bu[i] = 516 * (i - 128);
        }
        for (int i = 0; i < 1024; i++)
        {
            int v = i - 384;
            clip[i] = (u8)(v < 0 ? 0 : v > 255 ? 255 : v);
        }
    }
    int tw = s_movieTex.tex.width, th = s_movieTex.tex.height;
    u32* dst = (u32*)s_movieTex.tex.data;
    const u8* cl = clip + 384;
    for (int y = 0; y < h; y += 2)
    {
        const u8* y0 = py + y * ys;
        const u8* y1 = y0 + ys;
        const u8* u = pu + (y >> 1) * cs;
        const u8* v = pv + (y >> 1) * cs;
        int t0 = th - 1 - y, t1 = th - 2 - y;
        u32* row0 = dst + (size_t)(t0 / 8) * (tw / 8) * 64;
        u32* row1 = dst + (size_t)(t1 / 8) * (tw / 8) * 64;
        for (int x = 0; x < w; x += 2)
        {
            int cu = u[x >> 1], cv = v[x >> 1];
            int r = rv[cv], g = gu[cu] + gv[cv], b = bu[cu];
            for (int k = 0; k < 4; k++)
            {
                int xx = x + (k & 1);
                int c = yt[(k < 2 ? y0 : y1)[xx]];
                u32 px = (u32)cl[(c + r) >> 8] << 24 | (u32)cl[(c + g) >> 8] << 16 | (u32)cl[(c + b) >> 8] << 8 | 0xFF;
                int ty = k < 2 ? t0 : t1;
                u32* row = k < 2 ? row0 : row1;
                row[(xx / 8) * 64 + morton(xx & 7, ty & 7)] = px;
            }
        }
    }
    C3D_TexFlush(&s_movieTex.tex);
}

extern "C" void n3ds_fmv_frame_yuv(const u8* py, int ys, const u8* pu, const u8* pv, int cs, int w, int h)
{
    if (!movie_texture(w, h))
    {
        return;
    }
    static u64 convTicks;
    static int frames;
    u64 t0 = svcGetSystemTick();
    float tw = (float)s_movieTex.tex.width, th = (float)s_movieTex.tex.height;
    s_movieTex.su = s_movieTex.sv = 1.0f;
    if (convert_y2r(py, ys, pu, pv, cs, w, h))
    {
        // Y2R writes the top row first: t runs from 1 at the top downwards.
        convTicks += svcGetSystemTick() - t0;
        draw_movie(&s_movie, w, h, w / tw, 1.0f, 1.0f - h / th);
    }
    else
    {
        convert_cpu(py, ys, pu, pv, cs, w, h);
        convTicks += svcGetSystemTick() - t0;
        draw_movie(&s_movie, w, h, w / tw, 0.0f, h / th);
    }
    if (++frames == 300)
    {
        pc_log("[fmv] YUV conversion %.1f ms per frame", convTicks * 1000.0 / SYSCLOCK_ARM11 / frames);
        convTicks = 0;
        frames = 0;
    }
}
