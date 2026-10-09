// citro3d graphics backend for the New 3DS port (see pc/rw/gpu.h).
//
// The game's 640x480 frame is drawn into a 320x240 4:3 area in the middle of
// the 400x240 top screen. The top screen's framebuffer is mounted sideways,
// so screen draws get a quarter-turn rotation on top of the projection.
// Vertex data is copied per draw into a per-frame arena in linear memory.
#include <3ds.h>
#include <citro3d.h>
#include <stdlib.h>
#include <string.h>
#include <unordered_map>

#include "gpu.h"
#include "pc_platform.h"
#include "scene_shbin.h"

#define DISPLAY_TRANSFER_FLAGS \
    (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
     GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
     GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

enum { SCREEN_W = 400, SCREEN_H = 240, GAME_W = 320, GAME_H = 240 };

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
static DVLB_s* s_dvlb;
static shaderProgram_s s_program;
static int s_uMvp, s_uUvScale;
static bool s_inFrame;
static PcRaster* s_target;
static bool s_overlay;

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
struct StaticBuf
{
    Vert* verts;
    int count;
    std::unordered_map<const RwUInt16*, u16*> indices;
};
static std::unordered_map<const void*, StaticBuf> s_static;
static size_t s_staticBytes;

// The last dynamic conversion this frame: the meshes of one geometry share
// their arrays, so consecutive draws can reuse it when nothing differs (the
// renderer rewrites its colour buffer per mesh, so a colour array never matches).
static struct
{
    GpuVerts v;
    Vert* out;
} s_lastDyn;

static bool same_dynamic(const GpuVerts& a, const GpuVerts& b)
{
    return a.pos == b.pos && a.posStride == b.posStride && a.posSize == b.posSize && !a.col && !b.col &&
           memcmp(&a.constCol, &b.constCol, sizeof(RwRGBA)) == 0 && a.uv == b.uv && a.uvStride == b.uvStride &&
           a.count == b.count;
}

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

static void begin_frame()
{
    if (s_inFrame)
    {
        return;
    }
    u64 t0 = svcGetSystemTick();
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    s_prof.beginTicks += svcGetSystemTick() - t0;
    s_inFrame = true;
    s_arenaIdx ^= 1;
    s_arenaUsed = 0;
    s_arenaFull = false;
    s_lastDyn.out = NULL;
    s_target = NULL;
    s_overlay = false;
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
    else if (s_overlay)
    {
        C3D_SetViewport(0, 0, SCREEN_H, SCREEN_W);
    }
    else
    {
        // The rotated framebuffer is 240 wide and 400 tall.
        C3D_SetViewport(0, (SCREEN_W - GAME_W) / 2, SCREEN_H, GAME_W);
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
    C3D_FrameDrawOn(p ? ((C3dRaster*)p->gpu)->target : s_top);
    set_viewport();
    s_mvpDirty = true;
}

void gpu_set_target(PcRaster* target)
{
    s_overlay = false;
    bind_target(target);
}

PcRaster* gpu_target() { return s_target; }

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
    s_mvpDirty = false;
}

void gpu_clear(PcRaster* target, const RwRGBA* col, bool depth)
{
    begin_frame();
    C3D_RenderTarget* t = s_top;
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

static void apply_state(const GpuState& s, C3dRaster* tex)
{
    // Depth: as in OpenGL, no depth test means no depth writes either.
    u32 writes = (s.colorWrite ? GPU_WRITE_COLOR & ~GPU_WRITE_ALPHA : 0) | (s.alphaWrite ? GPU_WRITE_ALPHA : 0);
    if (s.zTest)
    {
        C3D_DepthTest(true, GPU_LEQUAL, (GPU_WRITEMASK)(writes | (s.zWrite ? GPU_WRITE_DEPTH : 0)));
    }
    else
    {
        C3D_DepthTest(true, GPU_ALWAYS, (GPU_WRITEMASK)writes);
    }
    if (s.blend)
    {
        GPU_BLENDFACTOR src = blend_factor(s.srcBlend), dst = blend_factor(s.dstBlend);
        if (s.srcBlend == 4 && s.dstBlend == 9)
        {
            // INVSRCCOLOR/DESTCOLOR (the drop shadows): on GameCube this darkens
            // the frame by the source alpha.
            src = GPU_ZERO;
            dst = GPU_ONE_MINUS_SRC_ALPHA;
        }
        C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, src, dst, src, dst);
    }
    else
    {
        C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO);
    }
    C3D_AlphaTest(true, test_func(s.alphaFunc), s.alphaRef);
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
    C3D_CullFace(cull);
    // Colour combiners: clamp(vertex colour + add colour) * material colour * texture.
    C3D_TexEnv* env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    const RwRGBA& a = s.addColor;
    if (a.red || a.green || a.blue)
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
    C3D_TexEnv* env1 = C3D_GetTexEnv(1);
    C3D_TexEnvInit(env1);
    const RwRGBA& m = s.matColor;
    if (m.red != 255 || m.green != 255 || m.blue != 255 || m.alpha != 255)
    {
        C3D_TexEnvSrc(env1, C3D_Both, GPU_PREVIOUS, GPU_CONSTANT, GPU_PREVIOUS);
        C3D_TexEnvFunc(env1, C3D_Both, GPU_MODULATE);
        C3D_TexEnvColor(env1, (u32)m.red | (u32)m.green << 8 | (u32)m.blue << 16 | (u32)m.alpha << 24);
    }
    else
    {
        C3D_TexEnvSrc(env1, C3D_Both, GPU_PREVIOUS, GPU_PREVIOUS, GPU_PREVIOUS);
        C3D_TexEnvFunc(env1, C3D_Both, GPU_REPLACE);
    }
    C3D_TexEnv* env2 = C3D_GetTexEnv(2);
    C3D_TexEnvInit(env2);
    if (tex)
    {
        C3D_TexBind(0, &tex->tex);
        GPU_TEXTURE_FILTER_PARAM f = (s.filter == 1 || s.filter == 3 || s.filter == 4) ? GPU_NEAREST : GPU_LINEAR;
        C3D_TexSetFilter(&tex->tex, f, f);
        C3D_TexSetWrap(&tex->tex, wrap(s.addrU), wrap(s.addrV));
        C3D_TexEnvSrc(env2, C3D_Both, GPU_PREVIOUS, GPU_TEXTURE0, GPU_PREVIOUS);
        C3D_TexEnvFunc(env2, C3D_Both, GPU_MODULATE);
        C3D_FVUnifSet(GPU_VERTEX_SHADER, s_uUvScale, tex->su, tex->sv, 1.0f, 1.0f);
    }
    else
    {
        C3D_TexEnvSrc(env2, C3D_Both, GPU_PREVIOUS, GPU_PREVIOUS, GPU_PREVIOUS);
        C3D_TexEnvFunc(env2, C3D_Both, GPU_REPLACE);
    }
    // No fog yet (the PICA200 fogs by depth through a lookup table).
    C3D_FogGasMode(GPU_NO_FOG, GPU_PLAIN_DENSITY, false);
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
    if (s_mvpDirty)
    {
        load_mvp();
    }
    C3D_BufInfo* bufs = C3D_GetBufInfo();
    BufInfo_Init(bufs);
    BufInfo_Add(bufs, out, sizeof(Vert), 3, 0x210);
    if (ix)
    {
        C3D_DrawElements(p, nidx, C3D_UNSIGNED_SHORT, ix);
    }
    else
    {
        C3D_DrawArrays(p, 0, count);
    }
}

int gpu_overlay_begin(int* width, int* height)
{
    begin_frame();
    s_overlay = true;
    s_target = NULL;
    C3D_FrameDrawOn(s_top);
    set_viewport();
    s_mvpDirty = true;
    *width = SCREEN_W;
    *height = SCREEN_H;
    return 1;
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
    linearFree(it->second.verts);
    s_staticBytes -= sizeof(Vert) * it->second.count;
    for (std::unordered_map<const RwUInt16*, u16*>::iterator i = it->second.indices.begin(); i != it->second.indices.end(); ++i)
    {
        linearFree(i->second);
    }
    s_static.erase(it);
}

void gpu_fmv_frame(const unsigned char* rgba, int w, int h) {}
