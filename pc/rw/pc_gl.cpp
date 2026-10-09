// OpenGL (fixed-function) renderer behind the PC RenderWare implementation.
#include "rw_internal.h"
#include "pc_platform.h"

#include <SDL.h>
#include <GL/gl.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
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

// Framebuffer objects (GL 3.0 / ARB_framebuffer_object), loaded at startup.
typedef void(APIENTRY* PFNGenFB)(GLsizei, GLuint*);
typedef void(APIENTRY* PFNBindFB)(GLenum, GLuint);
typedef void(APIENTRY* PFNFBTex2D)(GLenum, GLenum, GLenum, GLuint, GLint);
typedef void(APIENTRY* PFNRBStorage)(GLenum, GLenum, GLsizei, GLsizei);
typedef void(APIENTRY* PFNFBRB)(GLenum, GLenum, GLenum, GLuint);
typedef GLenum(APIENTRY* PFNCheckFB)(GLenum);
static PFNGenFB p_glGenFramebuffers, p_glGenRenderbuffers, p_glDeleteFramebuffers, p_glDeleteRenderbuffers;
static PFNBindFB p_glBindFramebuffer, p_glBindRenderbuffer;
static PFNFBTex2D p_glFramebufferTexture2D;
static PFNRBStorage p_glRenderbufferStorage;
static PFNFBRB p_glFramebufferRenderbuffer;
static PFNCheckFB p_glCheckFramebufferStatus;
enum { PC_GL_FRAMEBUFFER = 0x8D40, PC_GL_RENDERBUFFER = 0x8D41, PC_GL_COLOR_ATTACHMENT0 = 0x8CE0,
       PC_GL_DEPTH_ATTACHMENT = 0x8D00, PC_GL_DEPTH_COMPONENT24 = 0x81A6, PC_GL_FRAMEBUFFER_COMPLETE = 0x8CD5 };

static void load_fbo_functions()
{
    p_glGenFramebuffers = (PFNGenFB)SDL_GL_GetProcAddress("glGenFramebuffers");
    p_glGenRenderbuffers = (PFNGenFB)SDL_GL_GetProcAddress("glGenRenderbuffers");
    p_glDeleteFramebuffers = (PFNGenFB)SDL_GL_GetProcAddress("glDeleteFramebuffers");
    p_glDeleteRenderbuffers = (PFNGenFB)SDL_GL_GetProcAddress("glDeleteRenderbuffers");
    p_glBindFramebuffer = (PFNBindFB)SDL_GL_GetProcAddress("glBindFramebuffer");
    p_glBindRenderbuffer = (PFNBindFB)SDL_GL_GetProcAddress("glBindRenderbuffer");
    p_glFramebufferTexture2D = (PFNFBTex2D)SDL_GL_GetProcAddress("glFramebufferTexture2D");
    p_glRenderbufferStorage = (PFNRBStorage)SDL_GL_GetProcAddress("glRenderbufferStorage");
    p_glFramebufferRenderbuffer = (PFNFBRB)SDL_GL_GetProcAddress("glFramebufferRenderbuffer");
    p_glCheckFramebufferStatus = (PFNCheckFB)SDL_GL_GetProcAddress("glCheckFramebufferStatus");
}

// The camera texture being rendered to, or NULL for the window.
static PcRaster* s_target;

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
    load_fbo_functions();
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
    if (r->fbo && s_gl)
    {
        p_glDeleteFramebuffers(1, (GLuint*)&r->fbo);
        p_glDeleteRenderbuffers(1, (GLuint*)&r->depthRb);
    }
    r->glTex = 0;
    r->fbo = r->depthRb = 0;
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
    p->hasAlpha = 0;
    for (size_t i = 3, n = (size_t)p->r.width * p->r.height * 4; i < n; i += 4)
    {
        if (p->rgba[i] != 255)
        {
            p->hasAlpha = 1;
            break;
        }
    }
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
    if (!p->fbo && (!p->glTex || p->dirty))
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
    if (p->fbo)
    {
        min = mag; // render targets have no mipmaps
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

// RenderWare blends when vertex alpha is enabled or the texture has alpha
// (call after the raster is bound, which uploads it).
static bool raster_has_alpha(RwRaster* r)
{
    if (!r)
    {
        return false;
    }
    PcRaster* p = pc_raster(r);
    return p->fbo != 0 || p->hasAlpha;
}

// Applies blending/depth/fog/cull/alpha-test. 'alpha' forces blending on.
static void apply_states(bool alpha)
{
    if (s_rs.zTest) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    glDepthMask(s_rs.zWrite ? GL_TRUE : GL_FALSE);
    if (alpha || s_rs.vertexAlpha)
    {
        glEnable(GL_BLEND);
        if (s_rs.srcBlend == 4 && s_rs.dstBlend == 9)
        {
            // INVSRCCOLOR/DESTCOLOR (the drop shadows): GX has no source-colour
            // source factor or dest-colour dest factor, and on GameCube this
            // darkens the frame by the source alpha.
            glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
        }
        else
        {
            glBlendFunc(gl_blend(s_rs.srcBlend), gl_blend(s_rs.dstBlend));
        }
    }
    else
    {
        glDisable(GL_BLEND);
    }
    glColorMask(s_colorUpdate, s_colorUpdate, s_colorUpdate, s_alphaUpdate);
    glEnable(GL_ALPHA_TEST);
    if (s_gxAlphaFunc >= 0)
    {
        glAlphaFunc(gl_alpha_func(s_gxAlphaFunc), s_gxAlphaRef / 255.0f);
    }
    else
    {
        glAlphaFunc(gl_alpha_func(s_rs.alphaFunc), s_rs.alphaRef / 255.0f);
    }
    if (s_rs.cullMode == 2 || s_rs.cullMode == 3)
    {
        glEnable(GL_CULL_FACE);
        // RW back faces (cull mode 2) are GL back faces here; render targets are
        // drawn flipped vertically, which swaps them.
        bool front = (s_rs.cullMode == 3) != (s_target != NULL);
        glCullFace(front ? GL_FRONT : GL_BACK);
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

// Debug: PC_TRACE_AT=<seconds> logs the next ~60 clears/camera begins/presents.
static bool trace_frame()
{
    static const char* at = getenv("PC_TRACE_AT");
    static int left = 200;
    return at && left > 0 && SDL_GetTicks() >= (Uint32)(atof(at) * 1000) && left--;
}

// ---------------------------------------------------------------- cameras
static void set_viewport();

// Points rendering at a camera texture raster (an FBO around its GL texture),
// or back at the window for any other raster.
static void bind_target(RwRaster* fb)
{
    PcRaster* p = NULL;
    if (fb && fb->cType == rwRASTERTYPECAMERATEXTURE && p_glGenFramebuffers && fb->width > 0 && fb->height > 0)
    {
        p = pc_raster(fb);
        if (!p->fbo)
        {
            if (!p->glTex)
            {
                glGenTextures(1, (GLuint*)&p->glTex);
            }
            glBindTexture(GL_TEXTURE_2D, p->glTex);
            glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fb->width, fb->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
            p_glGenFramebuffers(1, (GLuint*)&p->fbo);
            p_glGenRenderbuffers(1, (GLuint*)&p->depthRb);
            p_glBindRenderbuffer(PC_GL_RENDERBUFFER, p->depthRb);
            p_glRenderbufferStorage(PC_GL_RENDERBUFFER, PC_GL_DEPTH_COMPONENT24, fb->width, fb->height);
            p_glBindFramebuffer(PC_GL_FRAMEBUFFER, p->fbo);
            p_glFramebufferTexture2D(PC_GL_FRAMEBUFFER, PC_GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, p->glTex, 0);
            p_glFramebufferRenderbuffer(PC_GL_FRAMEBUFFER, PC_GL_DEPTH_ATTACHMENT, PC_GL_RENDERBUFFER, p->depthRb);
            if (p_glCheckFramebufferStatus(PC_GL_FRAMEBUFFER) != PC_GL_FRAMEBUFFER_COMPLETE)
            {
                pc_log("[gl] render target %dx%d incomplete", fb->width, fb->height);
            }
            p_glBindFramebuffer(PC_GL_FRAMEBUFFER, s_target ? s_target->fbo : 0);
            p->dirty = 0;
        }
    }
    if (p == s_target)
    {
        return;
    }
    s_target = p;
    if (p_glBindFramebuffer)
    {
        p_glBindFramebuffer(PC_GL_FRAMEBUFFER, p ? p->fbo : 0);
    }
    set_viewport();
}

static float s_view[16];
static RwCamera* s_cam;

// The game renders at 640x480; fit that into the window keeping 4:3.
static void set_viewport()
{
    if (s_target)
    {
        glViewport(0, 0, s_target->r.width, s_target->r.height);
        return;
    }
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
    if (s_target)
    {
        // Texture row 0 is the image's top row: render targets are drawn upside down.
        p[1] = -p[1], p[5] = -p[5], p[9] = -p[9], p[13] = -p[13];
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
    if (trace_frame())
    {
        pc_log("[trace] begin cam %p fb %p vw %.3f %.3f off %.3f %.3f", c, c->frameBuffer, c->viewWindow.x,
               c->viewWindow.y, c->viewOffset.x, c->viewOffset.y);
    }
    bind_target(c->frameBuffer);
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

void pc_gl_end_camera(RwCamera* c)
{
    s_cam = NULL;
    bind_target(NULL);
}

void pc_gl_clear(RwCamera* c, const RwRGBA* col, RwInt32 mode)
{
    if (!s_window)
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
                   col ? col->green : 0, col ? col->blue : 0, col ? col->alpha : 0, s_target);
        }
    }
    // RwCameraClear clears the camera's own raster, and may be called outside
    // its Begin/EndUpdate: clear that target, then restore the current one.
    PcRaster* prev = s_target;
    bind_target(c ? c->frameBuffer : NULL);
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
        if ((bits & GL_COLOR_BUFFER_BIT) && !s_target)
        {
            glViewport(0, 0, s_winW, s_winH);
        }
        // The GameCube clears with an EFB copy, which ignores the write masks.
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glClear(bits);
        set_viewport();
    }
    bind_target(prev ? &prev->r : NULL);
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

static void draw_overlay();

void pc_gl_present()
{
    if (!s_window)
    {
        return;
    }
    if (trace_frame())
    {
        pc_log("[trace] present");
    }
    draw_overlay();
    maybe_screenshot();
    if (stats_on())
    {
        static int frame;
        if (++frame % 120 == 0)
        {
            pc_log("[stats] %.1fs frame %d: cams %d atomics %d/%d sectors %d im2d %d im3d %d clears %d", SDL_GetTicks() / 1000.0, frame, s_st.cams,
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
            apply_states(alpha || (t && raster_has_alpha(t->raster)));
            // Debug: PC_HILITE_MAT=<n> paints world material n bright red.
            static const char* hilite = getenv("PC_HILITE_MAT");
            RpWorld* cw = (RpWorld*)rwGlobals()->curWorld;
            if (hilite && cw && mats == &cw->matList && m && atoi(hilite) < cw->matList.numMaterials &&
                cw->matList.materials[atoi(hilite)] == m)
            {
                glDisableClientState(GL_COLOR_ARRAY);
                glDisable(GL_TEXTURE_2D);
                glDisable(GL_FOG);
                glColor4ub(255, 0, 0, 255);
            }
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
    if (trace_frame())
    {
        pc_log("[trace] atomic verts %d pos %.1f %.1f %.1f blend %d/%d zt %d", g->numVertices, f->ltm.pos.x,
               f->ltm.pos.y, f->ltm.pos.z, s_rs.srcBlend, s_rs.dstBlend, s_rs.zTest);
    }
    if (stats_on() && dbg && strstr(dbg, "atomlog"))
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
    if (sk && h && h->pMatrixArray)
    {
        sp.resize(g->numVertices);
        if (nrm)
        {
            sn.resize(g->numVertices);
        }
        skin_vertices(g, sk, h, sp.data(), nrm ? sn.data() : NULL);
        pos = sp.data();
        // Debug: PC_GEOLOG=<verts> logs the skinned bounds of models with that many vertices.
        static int geolog = getenv("PC_GEOLOG") ? atoi(getenv("PC_GEOLOG")) : -1;
        static int geologged;
        if (g->numVertices == geolog && geologged++ % 300 == 0)
        {
            float mn[3] = { 1e9f, 1e9f, 1e9f }, mx[3] = { -1e9f, -1e9f, -1e9f };
            for (int i = 0; i < g->numVertices; i++)
            {
                const float* v = &sp[i].x;
                for (int k = 0; k < 3; k++) { mn[k] = fminf(mn[k], v[k]); mx[k] = fmaxf(mx[k], v[k]); }
            }
            const RwV3d* b0 = g->morphTarget->verts;
            pc_log("[geo] skinned bounds (%.2f %.2f %.2f)-(%.2f %.2f %.2f) bind v0 (%.2f %.2f %.2f) skinned v0 (%.2f %.2f %.2f) "
                   "bones %d nodes %d ltm pos %.1f %.1f %.1f alpha %d", mn[0], mn[1], mn[2], mx[0], mx[1], mx[2], b0->x, b0->y, b0->z,
                   sp[0].x, sp[0].y, sp[0].z, sk->numBones, h->numNodes, f->ltm.pos.x, f->ltm.pos.y, f->ltm.pos.z,
                   g->matList.numMaterials ? g->matList.materials[0]->color.alpha : -1);
        }
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
    static const char* dbg = getenv("PC_DBG");
    if (!s_window || (dbg && strstr(dbg, "nosector")))
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

static void begin_2d()
{
    set_viewport();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    if (s_target)
    {
        glOrtho(0, s_target->r.width, 0, s_target->r.height, -1, 1);
    }
    else
    {
        glOrtho(0, FB_W, FB_H, 0, -1, 1);
    }
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
    static const char* dbg = getenv("PC_DBG");
    if (dbg && strstr(dbg, "noim2d"))
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
        pc_log("[trace] im2d prim %d n %d raster %p (%.0f,%.0f)-(%.0f,%.0f) col %02x%02x%02x%02x blend %d/%d va %d",
               primType, n, s_rs.raster, v[0].x, v[0].y, v[n - 1].x, v[n - 1].y, v[0].emissiveColor.red,
               v[0].emissiveColor.green, v[0].emissiveColor.blue, v[0].emissiveColor.alpha, s_rs.srcBlend,
               s_rs.dstBlend, s_rs.vertexAlpha);
    }
    begin_2d();
    apply_states(im2d_alpha(v, n));
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
    if (trace_frame())
    {
        RwIm2DVertex* tv = (RwIm2DVertex*)vertices;
        pc_log("[trace] im2d-idx prim %d n %d raster %p (%.0f,%.0f) col %02x%02x%02x%02x blend %d/%d", primType, n,
               s_rs.raster, tv[0].x, tv[0].y, tv[0].emissiveColor.red, tv[0].emissiveColor.green,
               tv[0].emissiveColor.blue, tv[0].emissiveColor.alpha, s_rs.srcBlend, s_rs.dstBlend);
    }
    begin_2d();
    apply_states(im2d_alpha((RwIm2DVertex*)vertices, n));
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
    pc_gl_bind_raster(s_rs.raster);
    bool alpha = raster_has_alpha(s_rs.raster);
    for (RwUInt32 i = 0; i < s_im3dCount && !alpha; i++)
    {
        alpha = s_im3dVerts[i].color.alpha != 255;
    }
    apply_states(alpha);
    RwIm3DVertex* v = s_im3dVerts;
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(RwIm3DVertex), &v->objVertex);
    glEnableClientState(GL_COLOR_ARRAY);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(RwIm3DVertex), &v->color);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(2, GL_FLOAT, sizeof(RwIm3DVertex), &v->u);
    glDisableClientState(GL_NORMAL_ARRAY);
}

static bool im3d_disabled()
{
    static const char* dbg = getenv("PC_DBG");
    return dbg && strstr(dbg, "noim3d");
}

RwInt32 RwIm3DRenderPrimitive(RwInt32 primType)
{
    if (!s_window || !s_im3dVerts || im3d_disabled())
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
    if (!s_window || !s_im3dVerts || im3d_disabled())
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

// ---------------------------------------------------------------- movies
// Shows one decoded movie frame (RGBA) fitted to the 4:3 game area, and presents.
extern "C" void pc_gl_fmv_frame(const unsigned char* rgba, int w, int h, int letterbox)
{
    if (!s_window)
    {
        return;
    }
    static GLuint tex;
    static int texW, texH;
    if (!tex)
    {
        glGenTextures(1, &tex);
    }
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    if (w != texW || h != texH)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        texW = w;
        texH = h;
    }
    else
    {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    }
    // Whole window black, then the movie in the 4:3 area keeping its aspect ratio.
    glViewport(0, 0, s_winW, s_winH);
    glDisable(GL_SCISSOR_TEST);
    glClearColor(0, 0, 0, 1);
    glDepthMask(GL_TRUE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    set_viewport();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_FOG);
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    float aspect = (float)w / h, area = 4.0f / 3.0f;
    float sx = 1.0f, sy = 1.0f;
    if (aspect > area)
    {
        sy = area / aspect;
    }
    else
    {
        sx = aspect / area;
    }
    (void)letterbox;
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 1); glVertex2f(-sx, -sy);
    glTexCoord2f(1, 1); glVertex2f(sx, -sy);
    glTexCoord2f(1, 0); glVertex2f(sx, sy);
    glTexCoord2f(0, 0); glVertex2f(-sx, sy);
    glEnd();
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glBindTexture(GL_TEXTURE_2D, 0);
    maybe_screenshot();
    SDL_GL_SwapWindow(s_window);
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
};

static char s_overlay[4096];

extern "C" void pc_gl_set_overlay(const char* text)
{
    snprintf(s_overlay, sizeof(s_overlay), "%s", text ? text : "");
}

static void draw_overlay()
{
    if (!s_overlay[0])
    {
        return;
    }
    const int S = 3; // pixels per font pixel
    glViewport(0, 0, s_winW, s_winH);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, s_winW, s_winH, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_FOG);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_LIGHTING);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // Backing box, then the text.
    int lines = 1, cols = 0, col = 0;
    for (const char* p = s_overlay; *p; p++)
    {
        if (*p == '\n') { lines++; col = 0; } else if (++col > cols) cols = col;
    }
    glColor4f(0, 0, 0, 0.6f);
    glBegin(GL_QUADS);
    glVertex2i(4, 4);
    glVertex2i(12 + cols * 6 * S, 4);
    glVertex2i(12 + cols * 6 * S, 12 + lines * 9 * S);
    glVertex2i(4, 12 + lines * 9 * S);
    glEnd();
    glColor4f(1, 1, 0.3f, 1);
    glBegin(GL_QUADS);
    int x0 = 8, x = x0, y = 8;
    for (const char* p = s_overlay; *p; p++)
    {
        char c = *p >= 'a' && *p <= 'z' ? *p - 32 : *p;
        if (c == '\n')
        {
            x = x0;
            y += 9 * S;
            continue;
        }
        for (size_t g = 0; g < sizeof(s_font) / sizeof(s_font[0]); g++)
        {
            if (s_font[g].c != c)
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
                        glVertex2i(px, py);
                        glVertex2i(px + S, py);
                        glVertex2i(px + S, py + S);
                        glVertex2i(px, py + S);
                    }
                }
            }
            break;
        }
        x += 6 * S;
    }
    glEnd();
    glDisable(GL_BLEND);
}

// GX calls the game makes directly (src/Core/gc/iEnv.cpp, iSystem.cpp).
extern "C" void GXSetColorUpdate(int enable)
{
    s_colorUpdate = enable != 0;
}

extern "C" void GXSetAlphaUpdate(int enable)
{
    s_alphaUpdate = enable != 0;
}

// GX tests (alpha comp0 ref0) op (alpha comp1 ref1); fixed-function GL has one
// comparison, so keep the side that can fail. The game only uses
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
void pc_gl_reset_gx_alpha()
{
    s_gxAlphaFunc = -1;
}

// Z before or after texturing: GL discards alpha-tested fragments before the
// depth write either way.
extern "C" void GXSetZCompLoc(int beforeTex) {}
extern "C" void GXSetMisc(int token, int value) {}
extern "C" void GXAbortFrame() {}
extern "C" void GXDrawDone() {}
