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
    // Non-NULL: the arrays keep their contents for every draw with this key
    // until gpu_dynamic_changed() (the meshes of one model), so a draw with the
    // same key and arrays may reuse the previous draw's converted vertices.
    const void* reuseKey;
};

bool gpu_open();
void gpu_close();
bool gpu_ready();

// Where draws go: a camera texture raster, or NULL for the screen (the game's
// 640x480 area). Creating a raster's render target sets its isTarget flag.
void gpu_set_target(PcRaster* target);
PcRaster* gpu_target();

// A second screen (the 3DS bottom screen) for the HUD: while it is selected,
// screen draws (no target) go to it, with the same 640x480 space. Each frame
// starts on the main screen.
bool gpu_second_screen();
void gpu_select_screen(int screen); // 0 main, 1 second

// How much wider than 4:3 draws are on the screen now selected (1: 4:3),
// toScreen false meaning a render target: the 3D view widens by it; 2D, made
// for 4:3, stays 4:3 in the middle.
float gpu_widen(bool toScreen);
// Whether the screen's game area may be wider than 4:3 now (in play; the
// title and menus are made for 4:3).
void gpu_set_wide(bool on);

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

// Arrays passed with a reuseKey were rewritten: no reuse past this point.
void gpu_dynamic_changed();

// Skinning on the GPU (gpu_skinning() says whether the backend has it): the
// bind-pose arrays stay cached under v.cacheKey like static geometry, with each
// vertex's four bone indices and weights; bones are the pose's skin matrices
// (bind pose to object space), one per bone. v.col NULL means white.
struct GpuSkin
{
    const RwUInt8* indices; // 4 per vertex
    const float* weights;   // 4 per vertex
    const RwMatrix* bones;
    int numBones;
};
bool gpu_skinning();
void gpu_draw_skinned(const GpuState& s, const GpuVerts& v, const GpuSkin& skin, GpuPrim prim, const RwUInt16* idx,
                      int nidx);

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
