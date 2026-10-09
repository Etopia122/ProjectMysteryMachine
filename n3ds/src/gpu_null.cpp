// Graphics backend that draws nothing: lets the game run on the 3DS before the
// citro3d backend exists (bring-up only).
#include "gpu.h"

#include <stdlib.h>
#include <string.h>

static PcRaster* s_target;

bool gpu_open() { return true; }
void gpu_close() {}
bool gpu_ready() { return true; }

void gpu_set_target(PcRaster* target)
{
    if (target)
    {
        target->isTarget = 1;
    }
    s_target = target;
}

PcRaster* gpu_target() { return s_target; }
void gpu_set_projection(const float* m) {}
void gpu_set_modelview(const float* m) {}
void gpu_clear(PcRaster* target, const RwRGBA* colour, bool depth) {}

void gpu_prepare_texture(PcRaster* r)
{
    if (r->dirty && r->rgba)
    {
        pc_raster_scan_alpha(r);
        r->dirty = 0;
    }
}

void gpu_raster_free(PcRaster* r) { r->isTarget = 0; }
void gpu_draw(const GpuState& s, const GpuVerts& v, GpuPrim prim, const RwUInt16* idx, int nidx) {}

int gpu_overlay_begin(int* width, int* height)
{
    *width = 400;
    *height = 240;
    return 1;
}

// The game paces itself on the VI retrace count (vi_time.cpp).
void gpu_present() {}

void gpu_fmv_frame(const unsigned char* rgba, int w, int h) {}
