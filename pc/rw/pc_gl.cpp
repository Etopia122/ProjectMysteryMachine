// OpenGL (fixed-function) renderer behind the PC RenderWare implementation.
#include "rw_internal.h"
#include "pc_platform.h"

#include <SDL.h>
#include <GL/gl.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifndef GL_MIRRORED_REPEAT
#define GL_MIRRORED_REPEAT 0x8370
#endif
#ifndef GL_GENERATE_MIPMAP
#define GL_GENERATE_MIPMAP 0x8191
#endif

static SDL_Window* s_window;
static SDL_GLContext s_gl;
static int s_winW = 1280, s_winH = 960;

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

// Debug statistics (PC_STATS=1): draws per frame, printed every 120 frames.
static struct { int cams, atomics, atomicsDrawn, sectors, im2d, im3d, clears; } s_st;
static bool stats_on() { static int on = -1; if (on < 0) on = getenv("PC_STATS") != NULL; return on; }

SDL_Window* pc_window() { return s_window; }

void pc_rw_device_open(void* displayID)
{
    if (s_window)
    {
        return;
    }
    SDL_InitSubSystem(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    s_window = SDL_CreateWindow("Scooby-Doo! Night of 100 Frights", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                s_winW, s_winH, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!s_window)
    {
        pc_log("[gl] window creation failed: %s", SDL_GetError());
        return;
    }
    s_gl = SDL_GL_CreateContext(s_window);
    SDL_GL_SetSwapInterval(0);
    pc_log("[gl] %s / %s", (const char*)glGetString(GL_RENDERER), (const char*)glGetString(GL_VERSION));
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
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
    if (s_gl)
    {
        SDL_GL_DeleteContext(s_gl);
    }
    if (s_window)
    {
        SDL_DestroyWindow(s_window);
    }
    s_gl = NULL;
    s_window = NULL;
}

// ---------------------------------------------------------------- textures
void pc_gl_raster_free(PcRaster* r)
{
    if (r->glTex && s_gl)
    {
        glDeleteTextures(1, (GLuint*)&r->glTex);
    }
    r->glTex = 0;
}

static void upload(PcRaster* p)
{
    if (!p->rgba)
    {
        return;
    }
    if (!p->glTex)
    {
        glGenTextures(1, (GLuint*)&p->glTex);
    }
    glBindTexture(GL_TEXTURE_2D, p->glTex);
    glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, p->r.width, p->r.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, p->rgba);
    p->dirty = 0;
}

static GLint gl_addr(int a)
{
    switch (a)
    {
    case 2: return GL_MIRRORED_REPEAT;
    case 3: case 4: return GL_CLAMP_TO_EDGE;
    default: return GL_REPEAT;
    }
}

static void bind_texture(RwRaster* r, int filter, int addrU, int addrV)
{
    if (!r)
    {
        glDisable(GL_TEXTURE_2D);
        return;
    }
    PcRaster* p = pc_raster(r);
    if (!p->glTex || p->dirty)
    {
        upload(p);
    }
    if (!p->glTex)
    {
        glDisable(GL_TEXTURE_2D);
        return;
    }
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, p->glTex);
    GLint mag = filter == 1 || filter == 3 || filter == 4 ? GL_NEAREST : GL_LINEAR;
    GLint min;
    switch (filter)
    {
    case 1: min = GL_NEAREST; break;
    case 3: min = GL_NEAREST_MIPMAP_NEAREST; break;
    case 4: min = GL_NEAREST_MIPMAP_LINEAR; break;
    case 5: min = GL_LINEAR_MIPMAP_NEAREST; break;
    case 6: min = GL_LINEAR_MIPMAP_LINEAR; break;
    default: min = GL_LINEAR; break;
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, gl_addr(addrU));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, gl_addr(addrV));
}

void pc_gl_bind_raster(RwRaster* r) { bind_texture(r, s_rs.filter, s_rs.addrU, s_rs.addrV); }

// ---------------------------------------------------------------- render state
static GLenum gl_blend(int b)
{
    switch (b)
    {
    case 1: return GL_ZERO;
    case 2: return GL_ONE;
    case 3: return GL_SRC_COLOR;
    case 4: return GL_ONE_MINUS_SRC_COLOR;
    case 5: return GL_SRC_ALPHA;
    case 6: return GL_ONE_MINUS_SRC_ALPHA;
    case 7: return GL_DST_ALPHA;
    case 8: return GL_ONE_MINUS_DST_ALPHA;
    case 9: return GL_DST_COLOR;
    case 10: return GL_ONE_MINUS_DST_COLOR;
    case 11: return GL_SRC_ALPHA_SATURATE;
    default: return GL_ONE;
    }
}

static GLenum gl_alpha_func(int f)
{
    switch (f)
    {
    case 1: return GL_NEVER;
    case 2: return GL_LESS;
    case 3: return GL_EQUAL;
    case 4: return GL_LEQUAL;
    case 5: return GL_GREATER;
    case 6: return GL_NOTEQUAL;
    case 7: return GL_GEQUAL;
    default: return GL_ALWAYS;
    }
}

// Applies blending/depth/fog/cull/alpha-test. 'alpha' forces blending on.
static void apply_states(bool alpha)
{
    if (s_rs.zTest) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    glDepthMask(s_rs.zWrite ? GL_TRUE : GL_FALSE);
    if (alpha || s_rs.vertexAlpha)
    {
        glEnable(GL_BLEND);
        glBlendFunc(gl_blend(s_rs.srcBlend), gl_blend(s_rs.dstBlend));
    }
    else
    {
        glDisable(GL_BLEND);
    }
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(gl_alpha_func(s_rs.alphaFunc), s_rs.alphaRef / 255.0f);
    if (s_rs.cullMode == 2 || s_rs.cullMode == 3)
    {
        glEnable(GL_CULL_FACE);
        // The view matrix flips X, which flips winding: RW's back faces are GL front faces.
        glCullFace(s_rs.cullMode == 2 ? GL_FRONT : GL_BACK);
    }
    else
    {
        glDisable(GL_CULL_FACE);
    }
    static const char* dbg = getenv("PC_DBG");
    if (dbg && strstr(dbg, "notex"))
    {
        glDisable(GL_TEXTURE_2D);
    }
    if (dbg && strstr(dbg, "noat"))
    {
        glDisable(GL_ALPHA_TEST);
    }
    if (dbg && strstr(dbg, "nocull"))
    {
        glDisable(GL_CULL_FACE);
    }
    if (dbg && strstr(dbg, "nodepth"))
    {
        glDisable(GL_DEPTH_TEST);
    }
    if (dbg && strstr(dbg, "noblend"))
    {
        glDisable(GL_BLEND);
    }
    if (s_rs.fogEnable && !(dbg && strstr(dbg, "nofog")))
    {
        RwCamera* cam = rwGlobals()->curCamera;
        glEnable(GL_FOG);
        GLfloat c[4] = { s_rs.fogColor.red / 255.0f, s_rs.fogColor.green / 255.0f, s_rs.fogColor.blue / 255.0f,
                         s_rs.fogColor.alpha / 255.0f };
        glFogfv(GL_FOG_COLOR, c);
        glFogi(GL_FOG_MODE, GL_LINEAR);
        glFogf(GL_FOG_START, cam ? cam->fogPlane : 0.0f);
        glFogf(GL_FOG_END, cam ? cam->farPlane : 1.0f);
    }
    else
    {
        glDisable(GL_FOG);
    }
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

// ---------------------------------------------------------------- cameras
static float s_view[16];
static RwCamera* s_cam;

// The game renders at 640x480; fit that into the window keeping 4:3.
static void set_viewport()
{
    SDL_GL_GetDrawableSize(s_window, &s_winW, &s_winH);
    int w = s_winW, h = s_winH;
    if (w * 3 > h * 4)
    {
        w = h * 4 / 3;
    }
    else
    {
        h = w * 3 / 4;
    }
    glViewport((s_winW - w) / 2, (s_winH - h) / 2, w, h);
}

static void load_projection(RwCamera* c)
{
    float vx = c->viewWindow.x, vy = c->viewWindow.y, n = c->nearPlane, f = c->farPlane;
    float ox = c->viewOffset.x, oy = c->viewOffset.y;
    float p[16] = { 0 };
    if (c->projectionType == rwPARALLEL)
    {
        p[0] = 1.0f / vx;
        p[5] = 1.0f / vy;
        p[10] = 2.0f / (f - n);
        p[12] = -ox / vx;
        p[13] = -oy / vy;
        p[14] = -(f + n) / (f - n);
        p[15] = 1.0f;
    }
    else
    {
        // Camera space looks down +Z (x already flipped to point right).
        p[0] = 1.0f / vx;
        p[5] = 1.0f / vy;
        p[8] = -ox / vx;
        p[9] = -oy / vy;
        p[10] = (f + n) / (f - n);
        p[11] = 1.0f;
        p[14] = -2.0f * f * n / (f - n);
    }
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(p);
}

static void matrix16(const RwMatrix* m, float* o)
{
    o[0] = m->right.x; o[1] = m->right.y; o[2] = m->right.z; o[3] = 0;
    o[4] = m->up.x; o[5] = m->up.y; o[6] = m->up.z; o[7] = 0;
    o[8] = m->at.x; o[9] = m->at.y; o[10] = m->at.z; o[11] = 0;
    o[12] = m->pos.x; o[13] = m->pos.y; o[14] = m->pos.z; o[15] = 1;
}

void pc_gl_begin_camera(RwCamera* c)
{
    if (!s_window)
    {
        return;
    }
    s_cam = c;
    s_st.cams++;
    set_viewport();
    load_projection(c);
    // View = inverse(camera LTM) with X flipped (RW's +X points left on screen).
    RwMatrix v = c->viewMatrix;
    v.right.x = -v.right.x;
    v.up.x = -v.up.x;
    v.at.x = -v.at.x;
    v.pos.x = -v.pos.x;
    matrix16(&v, s_view);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(s_view);
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

void pc_gl_end_camera(RwCamera* c) { s_cam = NULL; }

void pc_gl_clear(RwCamera* c, const RwRGBA* col, RwInt32 mode)
{
    if (!s_window)
    {
        return;
    }
    s_st.clears++;
    set_viewport();
    GLbitfield bits = 0;
    if ((mode & rwCAMERACLEARIMAGE) && col)
    {
        glClearColor(col->red / 255.0f, col->green / 255.0f, col->blue / 255.0f, col->alpha / 255.0f);
        bits |= GL_COLOR_BUFFER_BIT;
    }
    if (mode & rwCAMERACLEARZ)
    {
        glDepthMask(GL_TRUE);
        bits |= GL_DEPTH_BUFFER_BIT;
    }
    if (bits)
    {
        // Clear the whole window (letterbox bars included) for image clears.
        if (bits & GL_COLOR_BUFFER_BIT)
        {
            glViewport(0, 0, s_winW, s_winH);
        }
        glClear(bits);
        set_viewport();
    }
}

void pc_events_pump();

// Debug: PC_SHOT=<frame> saves build/pc/shot.bmp of that frame (and every 600 frames after).
// PC_SHOT=<frame>,<step> saves build/pc/shot_<frame>.bmp every <step> frames instead.
static void maybe_screenshot()
{
    static int frame;
    static int at = -2, step = 600;
    static bool numbered;
    if (at == -2)
    {
        const char* e = getenv("PC_SHOT");
        at = e ? atoi(e) : -1;
        const char* c = e ? strchr(e, ',') : NULL;
        if (c && atoi(c + 1) > 0)
        {
            step = atoi(c + 1);
            numbered = true;
        }
    }
    frame++;
    if (at < 0 || frame < at || (frame - at) % step != 0)
    {
        return;
    }
    std::vector<unsigned char> px((size_t)s_winW * s_winH * 4);
    glReadPixels(0, 0, s_winW, s_winH, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(0, s_winW, s_winH, 32, SDL_PIXELFORMAT_ABGR8888);
    for (int y = 0; y < s_winH; y++)
    {
        memcpy((unsigned char*)surf->pixels + y * surf->pitch, &px[(size_t)(s_winH - 1 - y) * s_winW * 4], (size_t)s_winW * 4);
    }
    char name[64];
    if (numbered)
    {
        snprintf(name, sizeof(name), "build/pc/shot_%d.bmp", frame);
    }
    else
    {
        snprintf(name, sizeof(name), "build/pc/shot.bmp");
    }
    SDL_SaveBMP(surf, name);
    SDL_FreeSurface(surf);
    pc_log("[gl] screenshot at frame %d", frame);
}

void pc_gl_present()
{
    if (!s_window)
    {
        return;
    }
    maybe_screenshot();
    if (stats_on())
    {
        static int frame;
        if (++frame % 120 == 0)
        {
            pc_log("[stats] frame %d: cams %d atomics %d/%d sectors %d im2d %d im3d %d clears %d", frame, s_st.cams,
                   s_st.atomicsDrawn, s_st.atomics, s_st.sectors, s_st.im2d, s_st.im3d, s_st.clears);
        }
        memset(&s_st, 0, sizeof(s_st));
    }
    SDL_GL_SwapWindow(s_window);
    pc_events_pump();
}

// ---------------------------------------------------------------- lighting
// Lighting is done on the CPU with RenderWare's rules: per vertex,
//   colour = prelit + materialColour * (ambient * ka + sum(lights) * kd)
//   alpha  = prelit.alpha * materialAlpha
// (fixed-function GL lighting can't add the prelit colour as well).
static std::vector<float> s_light; // per-vertex accumulated direct light (rgb), or empty when unlit
static float s_amb[3];              // accumulated ambient light

static void light_vertices(RpGeometry* g, const RwMatrix* ltm, const RwV3d* pos, const RwV3d* nrm)
{
    RpWorld* w = (RpWorld*)rwGlobals()->curWorld;
    int nv = g->numVertices;
    s_light.assign(nv * 3, 0.0f);
    s_amb[0] = s_amb[1] = s_amb[2] = 0;
    if (!w)
    {
        return;
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
static std::vector<float> s_pos, s_nrm;
static std::vector<RwRGBA> s_col;

static void draw_meshes(RpMeshHeader* mh, RpMaterialList* mats, const RwRGBA* prelit, bool modulate, int nv,
                        const RpTriangle* tris, int ntris)
{
    if (mh && mh->numMeshes)
    {
        RpMesh* mesh = (RpMesh*)(mh + 1);
        GLenum prim = (mh->flags & rpMESHHEADERTRISTRIP) ? GL_TRIANGLE_STRIP : GL_TRIANGLES;
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
            if (prelit || lit)
            {
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
                        r = (s_amb[0] * ka + s_light[k * 3] * kd) * mc.red;
                        gr = (s_amb[1] * ka + s_light[k * 3 + 1] * kd) * mc.green;
                        b = (s_amb[2] * ka + s_light[k * 3 + 2] * kd) * mc.blue;
                        if (prelit)
                        {
                            r += prelit[k].red, gr += prelit[k].green, b += prelit[k].blue;
                        }
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
                glEnableClientState(GL_COLOR_ARRAY);
                glColorPointer(4, GL_UNSIGNED_BYTE, 0, s_col.data());
            }
            else
            {
                glDisableClientState(GL_COLOR_ARRAY);
                glColor4ub(mc.red, mc.green, mc.blue, mc.alpha);
            }
            RwTexture* t = m ? m->texture : NULL;
            if (t && t->raster)
            {
                bind_texture(t->raster, rwTexFilter(t), rwTexAddrU(t), rwTexAddrV(t));
            }
            else
            {
                glDisable(GL_TEXTURE_2D);
            }
            apply_states(alpha || (t && t->raster));
            static const char* dbg = getenv("PC_DBG");
            if (dbg && strstr(dbg, "white"))
            {
                glDisableClientState(GL_COLOR_ARRAY);
                glColor4ub(255, 255, 255, 255);
            }
            glDrawElements(prim, (GLsizei)mesh[i].numIndices, GL_UNSIGNED_SHORT, mesh[i].indices);
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
        glDisable(GL_TEXTURE_2D);
        glColor4ub(255, 255, 255, 255);
        apply_states(false);
        glDrawElements(GL_TRIANGLES, (GLsizei)idx.size(), GL_UNSIGNED_SHORT, idx.data());
    }
}

static void skin_vertices(RpGeometry* g, PcSkin* sk, RpHAnimHierarchy* h, RwV3d* outPos, RwV3d* outNrm)
{
    int nb = sk->numBones;
    std::vector<RwMatrix> m(nb);
    for (int b = 0; b < nb && b < h->numNodes; b++)
    {
        RwMatrixMultiply(&m[b], &sk->invMats[b], &h->pMatrixArray[b]);
    }
    const RwV3d* v = g->morphTarget->verts;
    const RwV3d* n = g->morphTarget->normals;
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

void pc_gl_render_atomic(RpAtomic* a)
{
    RpGeometry* g = a->geometry;
    RwFrame* f = (RwFrame*)a->object.object.parent;
    s_st.atomics++;
    if (!s_window || !g || !f || !g->morphTarget || !g->morphTarget->verts || g->numVertices <= 0)
    {
        return;
    }
    static const char* dbg = getenv("PC_DBG");
    if (dbg && strstr(dbg, "noatomic"))
    {
        return;
    }
    s_st.atomicsDrawn++;
    if (stats_on() && dbg && strstr(dbg, "atomlog"))
    {
        static int n;
        if (n++ < 40)
        {
            pc_log("[atomic] %p verts %d flags %x pos %.1f %.1f %.1f prelit %p mesh %d mat0 %02x%02x%02x%02x", a,
                   g->numVertices, g->flags, f->ltm.pos.x, f->ltm.pos.y, f->ltm.pos.z, g->preLitLum,
                   g->mesh ? g->mesh->numMeshes : -1, g->matList.numMaterials ? g->matList.materials[0]->color.red : 0,
                   g->matList.numMaterials ? g->matList.materials[0]->color.green : 0,
                   g->matList.numMaterials ? g->matList.materials[0]->color.blue : 0,
                   g->matList.numMaterials ? g->matList.materials[0]->color.alpha : 0);
        }
    }
    float model[16];
    matrix16(&f->ltm, model);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(s_view);
    glMultMatrixf(model);

    const RwV3d* pos = g->morphTarget->verts;
    const RwV3d* nrm = g->morphTarget->normals;
    PcSkin* sk = pc_geometry(g)->skin;
    RpHAnimHierarchy* h = pc_atomic(a)->skinHierarchy;
    std::vector<RwV3d> sp, sn;
    if (sk && h && h->pMatrixArray)
    {
        sp.resize(g->numVertices);
        if (nrm)
        {
            sn.resize(g->numVertices);
        }
        skin_vertices(g, sk, h, sp.data(), nrm ? sn.data() : NULL);
        pos = sp.data();
        nrm = nrm ? sn.data() : NULL;
    }
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, pos);
    if (nrm)
    {
        glEnableClientState(GL_NORMAL_ARRAY);
        glNormalPointer(GL_FLOAT, 0, nrm);
    }
    else
    {
        glDisableClientState(GL_NORMAL_ARRAY);
    }
    if (g->texCoords[0])
    {
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, 0, g->texCoords[0]);
    }
    else
    {
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }
    if (nrm && (g->flags & rpGEOMETRYLIGHT))
    {
        light_vertices(g, &f->ltm, pos, nrm);
    }
    else
    {
        s_light.clear();
    }
    draw_meshes(g->mesh, &g->matList, g->preLitLum, (g->flags & rpGEOMETRYMODULATEMATERIALCOLOR) != 0,
                g->numVertices, g->triangles, g->numTriangles);
    s_light.clear();
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(s_view);
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

void pc_gl_render_world(RpWorld* w, std::vector<RpWorldSector*>& sectors)
{
    if (!s_window)
    {
        return;
    }
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(s_view);
    glDisable(GL_LIGHTING);
    glDisableClientState(GL_NORMAL_ARRAY);
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
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, 0, s->vertices);
        if (s->texCoords[0])
        {
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            glTexCoordPointer(2, GL_FLOAT, 0, s->texCoords[0]);
        }
        else
        {
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        }
        draw_meshes(s->mesh, &w->matList, s->preLitLum, (w->flags & rpGEOMETRYMODULATEMATERIALCOLOR) != 0,
                    s->numVertices, NULL, 0);
    }
    glDisableClientState(GL_COLOR_ARRAY);
}

// ---------------------------------------------------------------- immediate mode
static GLenum gl_prim(int t)
{
    switch (t)
    {
    case rwPRIMTYPELINELIST: return GL_LINES;
    case rwPRIMTYPEPOLYLINE: return GL_LINE_STRIP;
    case rwPRIMTYPETRISTRIP: return GL_TRIANGLE_STRIP;
    case rwPRIMTYPETRIFAN: return GL_TRIANGLE_FAN;
    case rwPRIMTYPEPOINTLIST: return GL_POINTS;
    default: return GL_TRIANGLES;
    }
}

static void begin_2d()
{
    set_viewport();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, FB_W, FB_H, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_LIGHTING);
    pc_gl_bind_raster(s_rs.raster);
}

static void end_2d()
{
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

static void im2d_arrays(RwIm2DVertex* v)
{
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, sizeof(RwIm2DVertex), &v->x);
    glEnableClientState(GL_COLOR_ARRAY);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(RwIm2DVertex), &v->emissiveColor);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(2, GL_FLOAT, sizeof(RwIm2DVertex), &v->u);
    glDisableClientState(GL_NORMAL_ARRAY);
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
    if (!s_window || n <= 0)
    {
        return 1;
    }
    static int logged;
    if (getenv("PC_LOG_IM2D") && logged++ < 20)
    {
        pc_log("[im2d] prim %d n %d raster %p v0 (%.1f %.1f %.3f) col %02x%02x%02x%02x uv (%.3f %.3f) v1 (%.1f %.1f)",
               primType, n, s_rs.raster, v[0].x, v[0].y, v[0].z, v[0].emissiveColor.red, v[0].emissiveColor.green,
               v[0].emissiveColor.blue, v[0].emissiveColor.alpha, v[0].u, v[0].v, n > 1 ? v[1].x : 0, n > 1 ? v[1].y : 0);
    }
    s_st.im2d++;
    begin_2d();
    apply_states(true);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE); // screen-space quads; the Y-down projection flips their winding
    im2d_arrays(v);
    glDrawArrays(gl_prim(primType), 0, n);
    glDisableClientState(GL_COLOR_ARRAY);
    end_2d();
    return 1;
}

RwInt32 RwIm2DRenderIndexedPrimitive(RwInt32 primType, void* vertices, RwInt32 n, RwUInt16* idx, RwInt32 ni)
{
    if (!s_window || n <= 0)
    {
        return 1;
    }
    begin_2d();
    apply_states(true);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    im2d_arrays((RwIm2DVertex*)vertices);
    glDrawElements(gl_prim(primType), ni, GL_UNSIGNED_SHORT, idx);
    glDisableClientState(GL_COLOR_ARRAY);
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

static void im3d_setup()
{
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(s_view);
    if (s_im3dHasLtm)
    {
        float m[16];
        matrix16(&s_im3dLtm, m);
        glMultMatrixf(m);
    }
    glDisable(GL_LIGHTING);
    pc_gl_bind_raster(s_rs.raster);
    apply_states(true);
    RwIm3DVertex* v = s_im3dVerts;
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(RwIm3DVertex), &v->objVertex);
    glEnableClientState(GL_COLOR_ARRAY);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(RwIm3DVertex), &v->color);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(2, GL_FLOAT, sizeof(RwIm3DVertex), &v->u);
    glDisableClientState(GL_NORMAL_ARRAY);
}

RwInt32 RwIm3DRenderPrimitive(RwInt32 primType)
{
    if (!s_window || !s_im3dVerts)
    {
        return 1;
    }
    s_st.im3d++;
    im3d_setup();
    glDrawArrays(gl_prim(primType), 0, (GLsizei)s_im3dCount);
    glDisableClientState(GL_COLOR_ARRAY);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(s_view);
    return 1;
}

RwInt32 RwIm3DRenderIndexedPrimitive(RwInt32 primType, RwUInt16* idx, RwInt32 n)
{
    if (!s_window || !s_im3dVerts)
    {
        return 1;
    }
    im3d_setup();
    glDrawElements(gl_prim(primType), n, GL_UNSIGNED_SHORT, idx);
    glDisableClientState(GL_COLOR_ARRAY);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(s_view);
    return 1;
}

RwInt32 RwIm3DEnd()
{
    s_im3dVerts = NULL;
    return 1;
}

}
