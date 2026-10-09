// OpenGL (fixed-function, 2.1) graphics backend for the PC port (see gpu.h).
#include "gpu.h"
#include "pc_platform.h"

#include <SDL.h>
#include <GL/gl.h>
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

// GL objects behind a raster.
struct GlRaster
{
    GLuint tex, fbo, depthRb;
};

static GlRaster* gl_raster(PcRaster* p)
{
    if (!p->gpu)
    {
        p->gpu = calloc(1, sizeof(GlRaster));
    }
    return (GlRaster*)p->gpu;
}

// The render target being drawn to (NULL: the window).
static PcRaster* s_target;
static float s_proj[16];

SDL_Window* pc_window() { return s_window; }

bool gpu_open()
{
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
        return false;
    }
    s_gl = SDL_GL_CreateContext(s_window);
    SDL_GL_SetSwapInterval(0);
    pc_log("[gl] %s / %s", (const char*)glGetString(GL_RENDERER), (const char*)glGetString(GL_VERSION));
    load_fbo_functions();
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_LIGHTING);
    return true;
}

void gpu_close()
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

bool gpu_ready() { return s_window != NULL; }

// ---------------------------------------------------------------- textures
void gpu_raster_free(PcRaster* r)
{
    GlRaster* g = (GlRaster*)r->gpu;
    if (g && s_gl)
    {
        if (g->tex)
        {
            glDeleteTextures(1, &g->tex);
        }
        if (g->fbo)
        {
            p_glDeleteFramebuffers(1, &g->fbo);
            p_glDeleteRenderbuffers(1, &g->depthRb);
        }
    }
    free(g);
    r->gpu = NULL;
    r->isTarget = 0;
}

void gpu_prepare_texture(PcRaster* p)
{
    if (p->isTarget || !p->rgba)
    {
        return;
    }
    GlRaster* g = gl_raster(p);
    if (g->tex && !p->dirty)
    {
        return;
    }
    if (!g->tex)
    {
        glGenTextures(1, &g->tex);
    }
    glBindTexture(GL_TEXTURE_2D, g->tex);
    glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, p->r.width, p->r.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, p->rgba);
    p->dirty = 0;
    pc_raster_scan_alpha(p);
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

static void bind_texture(PcRaster* p, int filter, int addrU, int addrV)
{
    GlRaster* g = p ? (GlRaster*)p->gpu : NULL;
    if (!g || !g->tex)
    {
        glDisable(GL_TEXTURE_2D);
        return;
    }
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, g->tex);
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
    if (g->fbo)
    {
        min = mag; // render targets have no mipmaps
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, gl_addr(addrU));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, gl_addr(addrV));
}

// ---------------------------------------------------------------- targets
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

static void load_projection()
{
    float p[16];
    memcpy(p, s_proj, sizeof(p));
    if (s_target)
    {
        // Texture row 0 is the image's top row: render targets are drawn upside down.
        p[1] = -p[1], p[5] = -p[5], p[9] = -p[9], p[13] = -p[13];
    }
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(p);
}

// Creates the FBO around a camera texture raster's GL texture.
static void create_target(PcRaster* p)
{
    GlRaster* g = gl_raster(p);
    if (g->fbo || !p_glGenFramebuffers)
    {
        return;
    }
    RwRaster* fb = &p->r;
    if (!g->tex)
    {
        glGenTextures(1, &g->tex);
    }
    glBindTexture(GL_TEXTURE_2D, g->tex);
    glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fb->width, fb->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    p_glGenFramebuffers(1, &g->fbo);
    p_glGenRenderbuffers(1, &g->depthRb);
    p_glBindRenderbuffer(PC_GL_RENDERBUFFER, g->depthRb);
    p_glRenderbufferStorage(PC_GL_RENDERBUFFER, PC_GL_DEPTH_COMPONENT24, fb->width, fb->height);
    p_glBindFramebuffer(PC_GL_FRAMEBUFFER, g->fbo);
    p_glFramebufferTexture2D(PC_GL_FRAMEBUFFER, PC_GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g->tex, 0);
    p_glFramebufferRenderbuffer(PC_GL_FRAMEBUFFER, PC_GL_DEPTH_ATTACHMENT, PC_GL_RENDERBUFFER, g->depthRb);
    if (p_glCheckFramebufferStatus(PC_GL_FRAMEBUFFER) != PC_GL_FRAMEBUFFER_COMPLETE)
    {
        pc_log("[gl] render target %dx%d incomplete", fb->width, fb->height);
    }
    p_glBindFramebuffer(PC_GL_FRAMEBUFFER, s_target ? ((GlRaster*)s_target->gpu)->fbo : 0);
    p->dirty = 0;
    p->isTarget = 1;
}

static void bind_target(PcRaster* p)
{
    if (p)
    {
        create_target(p);
        if (!p->isTarget)
        {
            p = NULL;
        }
    }
    if (p == s_target)
    {
        return;
    }
    s_target = p;
    if (p_glBindFramebuffer)
    {
        p_glBindFramebuffer(PC_GL_FRAMEBUFFER, p ? ((GlRaster*)p->gpu)->fbo : 0);
    }
    set_viewport();
    load_projection();
}

void gpu_set_target(PcRaster* target)
{
    bind_target(target);
    set_viewport();
}

PcRaster* gpu_target() { return s_target; }

void gpu_set_projection(const float* m)
{
    memcpy(s_proj, m, sizeof(s_proj));
    load_projection();
}

void gpu_set_modelview(const float* m)
{
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(m);
}

void gpu_clear(PcRaster* target, const RwRGBA* col, bool depth)
{
    PcRaster* prev = s_target;
    bind_target(target);
    GLbitfield bits = 0;
    if (col)
    {
        glClearColor(col->red / 255.0f, col->green / 255.0f, col->blue / 255.0f, col->alpha / 255.0f);
        bits |= GL_COLOR_BUFFER_BIT;
    }
    if (depth)
    {
        glDepthMask(GL_TRUE);
        bits |= GL_DEPTH_BUFFER_BIT;
    }
    // Clear the whole window (letterbox bars included) for image clears.
    if ((bits & GL_COLOR_BUFFER_BIT) && !s_target)
    {
        glViewport(0, 0, s_winW, s_winH);
    }
    // The GameCube clears with an EFB copy, which ignores the write masks.
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClear(bits);
    set_viewport();
    bind_target(prev);
}

// ---------------------------------------------------------------- drawing
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

static GLenum gl_prim(GpuPrim p)
{
    switch (p)
    {
    case PRIM_TRISTRIP: return GL_TRIANGLE_STRIP;
    case PRIM_TRIFAN: return GL_TRIANGLE_FAN;
    case PRIM_LINES: return GL_LINES;
    case PRIM_LINESTRIP: return GL_LINE_STRIP;
    case PRIM_POINTS: return GL_POINTS;
    default: return GL_TRIANGLES;
    }
}

static void apply_state(const GpuState& s)
{
    bind_texture(s.tex, s.filter, s.addrU, s.addrV);
    if (s.zTest) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    glDepthMask(s.zWrite ? GL_TRUE : GL_FALSE);
    if (s.blend)
    {
        glEnable(GL_BLEND);
        if (s.srcBlend == 4 && s.dstBlend == 9)
        {
            // INVSRCCOLOR/DESTCOLOR (the drop shadows): GX has no source-colour
            // source factor or dest-colour dest factor, and on GameCube this
            // darkens the frame by the source alpha.
            glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
        }
        else
        {
            glBlendFunc(gl_blend(s.srcBlend), gl_blend(s.dstBlend));
        }
    }
    else
    {
        glDisable(GL_BLEND);
    }
    glColorMask(s.colorWrite, s.colorWrite, s.colorWrite, s.alphaWrite);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(gl_alpha_func(s.alphaFunc), s.alphaRef / 255.0f);
    if (s.cull == PC_CULL_BACK || s.cull == PC_CULL_FRONT)
    {
        glEnable(GL_CULL_FACE);
        // RW back faces (cull mode 2) are GL back faces here; render targets are
        // drawn flipped vertically, which swaps them.
        bool front = (s.cull == PC_CULL_FRONT) != (s_target != NULL);
        glCullFace(front ? GL_FRONT : GL_BACK);
    }
    else
    {
        glDisable(GL_CULL_FACE);
    }
    if (s.fog)
    {
        glEnable(GL_FOG);
        GLfloat c[4] = { s.fogColor.red / 255.0f, s.fogColor.green / 255.0f, s.fogColor.blue / 255.0f,
                         s.fogColor.alpha / 255.0f };
        glFogfv(GL_FOG_COLOR, c);
        glFogi(GL_FOG_MODE, GL_LINEAR);
        glFogf(GL_FOG_START, s.fogStart);
        glFogf(GL_FOG_END, s.fogEnd);
    }
    else
    {
        glDisable(GL_FOG);
    }
}

void gpu_forget(const void* key) {}

// clamp(c + add) * m, as gpu.h defines the vertex colour.
static RwRGBA modulate(const RwRGBA& c, const RwRGBA& add, const RwRGBA& m)
{
    int r = c.red + add.red, g = c.green + add.green, b = c.blue + add.blue;
    RwRGBA o;
    o.red = (RwUInt8)((r > 255 ? 255 : r) * m.red / 255.0f);
    o.green = (RwUInt8)((g > 255 ? 255 : g) * m.green / 255.0f);
    o.blue = (RwUInt8)((b > 255 ? 255 : b) * m.blue / 255.0f);
    o.alpha = (RwUInt8)(c.alpha * m.alpha / 255);
    return o;
}

void gpu_draw(const GpuState& s, const GpuVerts& v, GpuPrim prim, const RwUInt16* idx, int nidx)
{
    if (!s_window)
    {
        return;
    }
    apply_state(s);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(v.posSize, GL_FLOAT, v.posStride, v.pos);
    // The material colour multiplies the vertex colours (fixed-function GL has
    // no constant colour for that with a colour array, so do it here).
    bool white = s.matColor.red == 255 && s.matColor.green == 255 && s.matColor.blue == 255 && s.matColor.alpha == 255 &&
                 !s.addColor.red && !s.addColor.green && !s.addColor.blue;
    static std::vector<RwRGBA> modulated;
    if (v.col && !white)
    {
        modulated.resize(v.count);
        for (int i = 0; i < v.count; i++)
        {
            modulated[i] = modulate(*(const RwRGBA*)((const RwUInt8*)v.col + (size_t)i * v.colStride), s.addColor,
                                    s.matColor);
        }
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(4, GL_UNSIGNED_BYTE, 0, modulated.data());
    }
    else if (v.col)
    {
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(4, GL_UNSIGNED_BYTE, v.colStride, v.col);
    }
    else
    {
        RwRGBA c = white ? v.constCol : modulate(v.constCol, s.addColor, s.matColor);
        glDisableClientState(GL_COLOR_ARRAY);
        glColor4ub(c.red, c.green, c.blue, c.alpha);
    }
    if (v.uv)
    {
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, v.uvStride, v.uv);
    }
    else
    {
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoord2f(0, 0);
    }
    glDisableClientState(GL_NORMAL_ARRAY);
    if (idx)
    {
        glDrawElements(gl_prim(prim), nidx, GL_UNSIGNED_SHORT, idx);
    }
    else
    {
        glDrawArrays(gl_prim(prim), 0, v.count);
    }
    glDisableClientState(GL_COLOR_ARRAY);
}

int gpu_overlay_begin(int* width, int* height)
{
    glViewport(0, 0, s_winW, s_winH);
    *width = s_winW;
    *height = s_winH;
    return 3;
}

// ---------------------------------------------------------------- present
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

void gpu_present()
{
    maybe_screenshot();
    SDL_GL_SwapWindow(s_window);
    set_viewport();
}

void gpu_fmv_frame(const unsigned char* rgba, int w, int h)
{
    static GLuint tex;
    static int texW, texH;
    if (!tex)
    {
        glGenTextures(1, &tex);
    }
    bind_target(NULL);
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
    glEnable(GL_TEXTURE_2D);
    glColor4ub(255, 255, 255, 255);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 1); glVertex2f(-sx, -sy);
    glTexCoord2f(1, 1); glVertex2f(sx, -sy);
    glTexCoord2f(1, 0); glVertex2f(sx, sy);
    glTexCoord2f(0, 0); glVertex2f(-sx, sy);
    glEnd();
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glBindTexture(GL_TEXTURE_2D, 0);
    load_projection();
    maybe_screenshot();
    SDL_GL_SwapWindow(s_window);
}
