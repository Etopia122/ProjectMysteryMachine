// RenderWare cameras: view matrix, frustum, begin/end update, clear, present.
#include "rw_internal.h"

#include <math.h>
#include <string.h>

static void normalize_plane(RwPlane* p)
{
    float l = sqrtf(p->normal.x * p->normal.x + p->normal.y * p->normal.y + p->normal.z * p->normal.z);
    if (l > 0)
    {
        p->normal.x /= l;
        p->normal.y /= l;
        p->normal.z /= l;
        p->distance /= l;
    }
}

// Rebuild world-space frustum planes (normals point outwards) and corners.
static void camera_update_frustum(RwCamera* c)
{
    RwFrame* f = (RwFrame*)c->object.object.parent;
    if (!f)
    {
        return;
    }
    const RwMatrix* m = RwFrameGetLTM(f);
    // As wide as the renderer draws it (a widescreen game area shows more).
    float vx = c->viewWindow.x * pc_render_camera_widen(c), vy = c->viewWindow.y;
    float ox = c->viewOffset.x, oy = c->viewOffset.y;
    float zs[2] = { c->nearPlane, c->farPlane };
    // Corner order: near (tl, tr, br, bl), far (tl, tr, br, bl), in camera space.
    for (int k = 0; k < 2; k++)
    {
        float z = zs[k];
        float sx = c->projectionType == rwPARALLEL ? 1.0f : z;
        float xs[4] = { ox + vx, ox - vx, ox - vx, ox + vx };
        float ys[4] = { oy + vy, oy + vy, oy - vy, oy - vy };
        for (int i = 0; i < 4; i++)
        {
            float x = xs[i] * sx, y = ys[i] * sx;
            RwV3d* o = &c->frustumCorners[k * 4 + i];
            o->x = m->right.x * x + m->up.x * y + m->at.x * z + m->pos.x;
            o->y = m->right.y * x + m->up.y * y + m->at.y * z + m->pos.y;
            o->z = m->right.z * x + m->up.z * y + m->at.z * z + m->pos.z;
        }
    }
    RwV3d* v = c->frustumCorners;
    // plane through three corners, normal = (b-a)x(c-a)
    auto plane = [](RwPlane* p, const RwV3d& a, const RwV3d& b, const RwV3d& cc) {
        RwV3d u = { b.x - a.x, b.y - a.y, b.z - a.z };
        RwV3d w = { cc.x - a.x, cc.y - a.y, cc.z - a.z };
        p->normal.x = u.y * w.z - u.z * w.y;
        p->normal.y = u.z * w.x - u.x * w.z;
        p->normal.z = u.x * w.y - u.y * w.x;
        p->distance = p->normal.x * a.x + p->normal.y * a.y + p->normal.z * a.z;
        normalize_plane(p);
    };
    RwPlane pl[6];
    plane(&pl[0], v[0], v[3], v[1]); // near
    plane(&pl[1], v[4], v[5], v[7]); // far
    plane(&pl[2], v[0], v[1], v[4]); // top
    plane(&pl[3], v[3], v[7], v[2]); // bottom
    plane(&pl[4], v[0], v[4], v[3]); // left (+x side)
    plane(&pl[5], v[1], v[2], v[5]); // right
    // Make sure every normal points away from the frustum centre.
    RwV3d ctr = { 0, 0, 0 };
    for (int i = 0; i < 8; i++)
    {
        ctr.x += v[i].x / 8;
        ctr.y += v[i].y / 8;
        ctr.z += v[i].z / 8;
    }
    for (int i = 0; i < 6; i++)
    {
        float d = pl[i].normal.x * ctr.x + pl[i].normal.y * ctr.y + pl[i].normal.z * ctr.z - pl[i].distance;
        if (d > 0)
        {
            pl[i].normal.x = -pl[i].normal.x;
            pl[i].normal.y = -pl[i].normal.y;
            pl[i].normal.z = -pl[i].normal.z;
            pl[i].distance = -pl[i].distance;
        }
        c->frustumPlanes[i].plane = pl[i];
        c->frustumPlanes[i].closestX = pl[i].normal.x < 0;
        c->frustumPlanes[i].closestY = pl[i].normal.y < 0;
        c->frustumPlanes[i].closestZ = pl[i].normal.z < 0;
    }
    RwBBox& bb = c->frustumBoundBox;
    bb.sup = bb.inf = v[0];
    for (int i = 1; i < 8; i++)
    {
        if (v[i].x > bb.sup.x) bb.sup.x = v[i].x;
        if (v[i].y > bb.sup.y) bb.sup.y = v[i].y;
        if (v[i].z > bb.sup.z) bb.sup.z = v[i].z;
        if (v[i].x < bb.inf.x) bb.inf.x = v[i].x;
        if (v[i].y < bb.inf.y) bb.inf.y = v[i].y;
        if (v[i].z < bb.inf.z) bb.inf.z = v[i].z;
    }
}

static RwCamera* camera_begin(RwCamera* c)
{
    _rwFrameSyncDirty();
    rwGlobals()->curCamera = c;
    RwFrame* f = (RwFrame*)c->object.object.parent;
    if (f)
    {
        RwMatrixInvert(&c->viewMatrix, RwFrameGetLTM(f));
    }
    camera_update_frustum(c);
    rwGlobals()->renderFrame++;
    c->renderFrame = rwGlobals()->renderFrame;
    pc_render_begin_camera(c);
    return c;
}

static RwCamera* camera_end(RwCamera* c)
{
    pc_render_end_camera(c);
    rwGlobals()->curCamera = NULL;
    return c;
}

extern "C" {

RwCamera* RwCameraCreate()
{
    RwCamera* c = (RwCamera*)rwCalloc(1, sizeof(RwCamera) + 0x40);
    c->object.object.type = rwCAMERA;
    c->projectionType = rwPERSPECTIVE;
    c->beginUpdate = camera_begin;
    c->endUpdate = camera_end;
    c->viewWindow.x = c->viewWindow.y = 1.0f;
    c->recipViewWindow.x = c->recipViewWindow.y = 1.0f;
    c->nearPlane = 0.05f;
    c->farPlane = 10.0f;
    c->fogPlane = 5.0f;
    c->object.lFrame.next = c->object.lFrame.prev = &c->object.lFrame;
    return c;
}

RwInt32 RwCameraDestroy(RwCamera* c)
{
    _rwObjectHasFrameSetFrame(c, NULL);
    if (rwGlobals()->curCamera == c)
    {
        rwGlobals()->curCamera = NULL;
    }
    rwFree(c);
    return 1;
}

RwCamera* RwCameraBeginUpdate(RwCamera* c) { return c->beginUpdate(c); }
RwCamera* RwCameraEndUpdate(RwCamera* c) { return c->endUpdate(c); }

RwCamera* RwCameraSetNearClipPlane(RwCamera* c, float z)
{
    c->nearPlane = z;
    return c;
}

RwCamera* RwCameraSetFarClipPlane(RwCamera* c, float z)
{
    c->farPlane = z;
    return c;
}

RwCamera* RwCameraSetFogDistance(RwCamera* c, float z)
{
    c->fogPlane = z;
    return c;
}

RwCamera* RwCameraSetProjection(RwCamera* c, RwInt32 projection)
{
    c->projectionType = projection;
    return c;
}

RwCamera* RwCameraSetViewWindow(RwCamera* c, const RwV2d* w)
{
    c->viewWindow = *w;
    c->recipViewWindow.x = w->x != 0 ? 1.0f / w->x : 0;
    c->recipViewWindow.y = w->y != 0 ? 1.0f / w->y : 0;
    return c;
}

RwCamera* RwCameraSetViewOffset(RwCamera* c, const RwV2d* o)
{
    c->viewOffset = *o;
    return c;
}

RwCamera* RwCameraClear(RwCamera* c, RwRGBA* colour, RwInt32 mode)
{
    pc_render_clear(c, colour, mode);
    return c;
}

RwCamera* RwCameraShowRaster(RwCamera* c, void* dev, RwUInt32 flags)
{
    pc_render_present();
    return c;
}

// rwSPHEREOUTSIDE = 0, rwSPHEREBOUNDARY = 1, rwSPHEREINSIDE = 2
RwInt32 RwCameraFrustumTestSphere(const RwCamera* c, const RwSphere* s)
{
    RwInt32 result = 2;
    for (int i = 0; i < 6; i++)
    {
        const RwPlane& p = c->frustumPlanes[i].plane;
        float d = p.normal.x * s->center.x + p.normal.y * s->center.y + p.normal.z * s->center.z - p.distance;
        if (d > s->radius)
        {
            return 0;
        }
        if (d > -s->radius)
        {
            result = 1;
        }
    }
    return result;
}

// GameCube RenderWare: copies the embedded framebuffer into a raster (the
// shadow pass saves and restores the screen with it). Cameras render into
// their own targets on PC, so there is nothing to copy.
void fn_80178A3C(RwRaster* raster) {}

}
