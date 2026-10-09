// Graphics backend for the shared RenderWare renderer (rw_render.cpp).
// gpu_gl.cpp implements it with OpenGL (PC); n3ds/src/gpu_c3d.cpp with citro3d.
//
// Conventions: matrices are column-major (OpenGL style) and describe an upright
// image (row 0 at the top for 2D, +Y up in clip space). A backend whose render
// targets are stored upside down flips them itself, cull mode included.
#ifndef PC_GPU_H
#define PC_GPU_H

#include "rw_internal.h"

// RenderWare state values used by the renderer.
enum
{
    PC_BLEND_SRCALPHA = 5,
    PC_BLEND_INVSRCALPHA = 6,
    PC_ALPHA_ALWAYS = 8,
    PC_CULL_NONE = 1,
    PC_CULL_BACK = 2,
    PC_CULL_FRONT = 3,
};

enum GpuPrim
{
    PRIM_TRIANGLES,
    PRIM_TRISTRIP,
    PRIM_TRIFAN,
    PRIM_LINES,
    PRIM_LINESTRIP,
    PRIM_POINTS,
};

// Fixed-function state of one draw, already resolved from RenderWare's rules.
struct GpuState
{
    bool zTest, zWrite;
    bool blend;
    int srcBlend, dstBlend;   // RenderWare blend values (1 zero ... 11 src alpha sat)
    int alphaFunc, alphaRef;  // RenderWare alpha test (1 never ... 8 always), ref 0-255
    int cull;                 // PC_CULL_*
    bool colorWrite, alphaWrite;
    bool fog;
    RwRGBA fogColor;
    float fogStart, fogEnd;   // camera-space depth
    PcRaster* tex;            // NULL: untextured
    int filter, addrU, addrV; // rwFILTER*, rwTEXTUREADDRESS* values
    // Vertex colours become clamp(colour + addColor) * matColor (alpha: times
    // matColor's only); addColor black and matColor white leave them as they are.
    RwRGBA addColor, matColor;
};

// Vertex arrays of one draw. Strides are in bytes.
struct GpuVerts
{
    const float* pos;
    int posStride;
    int posSize;           // 2 (screen pixels) or 3
    const RwRGBA* col;     // NULL: constCol for every vertex
    int colStride;
    RwRGBA constCol;
    const float* uv;       // NULL: no texture coordinates
    int uvStride;
    int count;
    // Non-NULL: these arrays never change for this key (a world sector or an
    // unskinned geometry), so the backend may keep a converted copy until
    // gpu_forget(key).
    const void* cacheKey;
};

bool gpu_open();
void gpu_close();
bool gpu_ready();

// Where draws go: a camera texture raster, or NULL for the screen (the game's
// 640x480 area). Creating a raster's render target sets its isTarget flag.
void gpu_set_target(PcRaster* target);
PcRaster* gpu_target();

void gpu_set_projection(const float* m);
void gpu_set_modelview(const float* m);

// Clears a target (NULL: the whole screen) without changing the current one.
// colour NULL: depth only.
void gpu_clear(PcRaster* target, const RwRGBA* colour, bool depth);

// Uploads a texture raster if its pixels changed (sets hasAlpha).
void gpu_prepare_texture(PcRaster* r);
void gpu_raster_free(PcRaster* r);

// Drops anything cached for a cacheKey (its geometry is being destroyed).
void gpu_forget(const void* key);

// idx NULL: v.count vertices in order.
void gpu_draw(const GpuState& s, const GpuVerts& v, GpuPrim prim, const RwUInt16* idx, int nidx);

// Debug overlay: switches to a pixel space over the whole screen and returns
// the size of one font pixel. Draws after it use that space until the next
// gpu_set_target.
int gpu_overlay_begin(int* width, int* height);

void gpu_present();

// Shows one movie frame (RGBA) in the 4:3 game area and presents it.
void gpu_fmv_frame(const unsigned char* rgba, int w, int h);

// Shared helper for backends: scans a raster's pixels for alpha < 255.
void pc_raster_scan_alpha(PcRaster* r);

#endif
