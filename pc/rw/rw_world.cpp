// RenderWare worlds (BSP of atomic sectors), lights, and collision queries.
#include "rw_internal.h"
#include "pc_platform.h"

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <unordered_map>
#include <vector>

// World format flags (RpWorld.flags), same bits as geometry.
static std::vector<RpWorldSector*> s_tmpSectors;

static void collect_sectors(RpSector* s, std::vector<RpWorldSector*>& out)
{
    if (!s)
    {
        return;
    }
    if (s->type < 0)
    {
        out.push_back((RpWorldSector*)s);
        return;
    }
    RpPlaneSector* p = (RpPlaneSector*)s;
    collect_sectors(p->leftSubTree, out);
    collect_sectors(p->rightSubTree, out);
}

// Sectors of a world, cached per world (filled on load).
struct PcWorld
{
    RpWorld w;
    RwUInt8 ext[0x40];
    std::vector<RpWorldSector*>* sectors;
};
static PcWorld* pc_world(RpWorld* w) { return (PcWorld*)w; }

static void sector_plugin(RwStream* s, RwUInt32 id, RwUInt32 len, RwUInt32 ver, void* obj)
{
    RpWorldSector* sec = (RpWorldSector*)obj;
    RpWorld* w = (RpWorld*)sec->repEntry; // stashed temporarily during load
    if (id == rwID_BINMESHPLUGIN)
    {
        sec->mesh = pc_read_binmesh(s, len, &w->matList);
    }
}

static RpSector* read_sector(RwStream* s, RpWorld* w, RwUInt32 format, int numTexSets)
{
    RwChunkHeaderInfo h;
    if (!RwStreamReadChunkHeaderInfo(s, &h))
    {
        return NULL;
    }
    RwUInt32 slen, ver;
    if (!RwStreamFindChunk(s, rwID_STRUCT, &slen, &ver))
    {
        return NULL;
    }
    std::vector<RwUInt8> st(slen);
    RwStreamRead(s, st.data(), slen);
    const RwUInt8* d = st.data();
    if (h.type == rwID_PLANESECT)
    {
        RpPlaneSector* p = (RpPlaneSector*)rwCalloc(1, sizeof(RpPlaneSector));
        p->type = (RwInt32)rd32(d);
        p->value = rdf(d + 4);
        p->leftValue = rdf(d + 16);
        p->rightValue = rdf(d + 20);
        p->leftSubTree = read_sector(s, w, format, numTexSets);
        p->rightSubTree = read_sector(s, w, format, numTexSets);
        return (RpSector*)p;
    }
    if (h.type != rwID_ATOMICSECT)
    {
        RwStreamSkip(s, h.length - slen - 12);
        return NULL;
    }
    RpWorldSector* sec = (RpWorldSector*)rwCalloc(1, sizeof(RpWorldSector) + 0x20);
    sec->type = rpWORLDSECTOR_TYPE;
    sec->matListWindowBase = (RwUInt16)rd32(d);
    RwInt32 np = (RwInt32)rd32(d + 4), nv = (RwInt32)rd32(d + 8);
    sec->numPolygons = (RwUInt16)np;
    sec->numVertices = (RwUInt16)nv;
    sec->boundingBox.inf.x = rdf(d + 12);
    sec->boundingBox.inf.y = rdf(d + 16);
    sec->boundingBox.inf.z = rdf(d + 20);
    sec->boundingBox.sup.x = rdf(d + 24);
    sec->boundingBox.sup.y = rdf(d + 28);
    sec->boundingBox.sup.z = rdf(d + 32);
    sec->tightBoundingBox = sec->boundingBox;
    rwLinkListInitialize(&sec->collAtomicsInWorldSector);
    rwLinkListInitialize(&sec->noCollAtomicsInWorldSector);
    rwLinkListInitialize(&sec->lightsInWorldSector);
    size_t o = 44;
    if (nv > 0)
    {
        sec->vertices = (RwV3d*)rwMalloc((size_t)nv * sizeof(RwV3d));
        for (RwInt32 i = 0; i < nv * 3; i++)
        {
            ((float*)sec->vertices)[i] = rdf(d + o + i * 4);
        }
        o += (size_t)nv * 12;
        if (format & rpGEOMETRYNORMALS)
        {
            sec->normals = (RpVertexNormal*)rwMalloc((size_t)nv * 4);
            memcpy(sec->normals, d + o, (size_t)nv * 4);
            o += (size_t)nv * 4;
        }
        if (format & rpGEOMETRYPRELIT)
        {
            sec->preLitLum = (RwRGBA*)rwMalloc((size_t)nv * 4);
            memcpy(sec->preLitLum, d + o, (size_t)nv * 4);
            o += (size_t)nv * 4;
        }
        for (int t = 0; t < numTexSets; t++)
        {
            sec->texCoords[t] = (RwTexCoords*)rwMalloc((size_t)nv * sizeof(RwTexCoords));
            for (RwInt32 i = 0; i < nv * 2; i++)
            {
                ((float*)sec->texCoords[t])[i] = rdf(d + o + i * 4);
            }
            o += (size_t)nv * 8;
        }
    }
    if (np > 0)
    {
        sec->polygons = (RpPolygon*)rwMalloc((size_t)np * sizeof(RpPolygon));
        for (RwInt32 i = 0; i < np; i++)
        {
            const RwUInt8* q = d + o + i * 8;
            if (ver >= 0x34000)
            {
                RwUInt32 a = rd32(q), b = rd32(q + 4);
                sec->polygons[i].vertIndex[0] = (RwUInt16)(a >> 16);
                sec->polygons[i].vertIndex[1] = (RwUInt16)a;
                sec->polygons[i].matIndex = (RwUInt16)(b >> 16);
                sec->polygons[i].vertIndex[2] = (RwUInt16)b;
            }
            else
            {
                sec->polygons[i].matIndex = rd16(q);
                sec->polygons[i].vertIndex[0] = rd16(q + 2);
                sec->polygons[i].vertIndex[1] = rd16(q + 4);
                sec->polygons[i].vertIndex[2] = rd16(q + 6);
            }
        }
    }
    sec->repEntry = (RwResEntry*)w;
    pc_read_extension(s, sector_plugin, sec);
    sec->repEntry = NULL;
    return (RpSector*)sec;
}

// Every live world, for the debug probes.
static std::vector<RpWorld*> s_worlds;

extern "C" {

RpWorld* RpWorldCreate(RwBBox* bbox)
{
    PcWorld* p = (PcWorld*)rwCalloc(1, sizeof(PcWorld));
    RpWorld* w = &p->w;
    s_worlds.push_back(w);
    w->object.type = 7;
    rwLinkListInitialize(&w->clumpList);
    rwLinkListInitialize(&w->lightList);
    rwLinkListInitialize(&w->directionalLightList);
    if (bbox)
    {
        w->boundingBox = *bbox;
    }
    w->surfaceProps.ambient = w->surfaceProps.specular = w->surfaceProps.diffuse = 1.0f;
    p->sectors = new std::vector<RpWorldSector*>();
    if (bbox)
    {
        // An empty world still has one (empty) sector.
        RpWorldSector* sec = (RpWorldSector*)rwCalloc(1, sizeof(RpWorldSector) + 0x20);
        sec->type = rpWORLDSECTOR_TYPE;
        sec->boundingBox = *bbox;
        sec->tightBoundingBox = *bbox;
        rwLinkListInitialize(&sec->collAtomicsInWorldSector);
        rwLinkListInitialize(&sec->noCollAtomicsInWorldSector);
        rwLinkListInitialize(&sec->lightsInWorldSector);
        w->rootSector = (RpSector*)sec;
        p->sectors->push_back(sec);
    }
    return w;
}

RpWorld* RpWorldStreamRead(RwStream* s)
{
    RwUInt32 len, ver;
    if (!RwStreamFindChunk(s, rwID_STRUCT, &len, &ver))
    {
        return NULL;
    }
    std::vector<RwUInt8> st(len);
    RwStreamRead(s, st.data(), len);
    const RwUInt8* d = st.data();
    PcWorld* p = (PcWorld*)RpWorldCreate(NULL);
    RpWorld* w = &p->w;
    w->worldOrigin.x = -rdf(d + 4);
    w->worldOrigin.y = -rdf(d + 8);
    w->worldOrigin.z = -rdf(d + 12);
    size_t o = 16;
    if (ver < 0x34000)
    {
        w->surfaceProps.ambient = rdf(d + o);
        w->surfaceProps.specular = rdf(d + o + 4);
        w->surfaceProps.diffuse = rdf(d + o + 8);
        o += 12;
    }
    // numTriangles, numVertices, numPlaneSectors, numWorldSectors, colSectorSize, format
    RwUInt32 format = rd32(d + o + 20);
    w->flags = format;
    int ntc = (format >> 16) & 0xFF;
    if (!ntc)
    {
        ntc = (format & rpGEOMETRYTEXTURED2) ? 2 : (format & rpGEOMETRYTEXTURED) ? 1 : 0;
    }
    w->numTexCoordSets = ntc;
    pc_read_material_list(s, &w->matList);
    w->rootSector = read_sector(s, w, format, ntc);
    collect_sectors(w->rootSector, *p->sectors);
    // World bounding box from its sectors.
    bool first = true;
    for (size_t i = 0; i < p->sectors->size(); i++)
    {
        RwBBox& b = (*p->sectors)[i]->boundingBox;
        if (first)
        {
            w->boundingBox = b;
            first = false;
            continue;
        }
        RwBBox& wb = w->boundingBox;
        if (b.inf.x < wb.inf.x) wb.inf.x = b.inf.x; if (b.inf.y < wb.inf.y) wb.inf.y = b.inf.y;
        if (b.inf.z < wb.inf.z) wb.inf.z = b.inf.z; if (b.sup.x > wb.sup.x) wb.sup.x = b.sup.x;
        if (b.sup.y > wb.sup.y) wb.sup.y = b.sup.y; if (b.sup.z > wb.sup.z) wb.sup.z = b.sup.z;
    }
    pc_read_extension(s, NULL, NULL);
    return w;
}

RwInt32 RpMaterialDestroy(RpMaterial* m);

// Collision data per sector, built on first use (see sector_coll).
struct SectorColl;
static std::unordered_map<const RpWorldSector*, SectorColl*> s_sectorColl;
static void sector_coll_free(const RpWorldSector* s);

static void free_sector(RpSector* s)
{
    if (!s)
    {
        return;
    }
    if (s->type < 0)
    {
        RpWorldSector* w = (RpWorldSector*)s;
        pc_render_forget(w);
        sector_coll_free(w);
        rwFree(w->polygons);
        rwFree(w->vertices);
        rwFree(w->normals);
        rwFree(w->preLitLum);
        for (int i = 0; i < 8; i++)
        {
            rwFree(w->texCoords[i]);
        }
        rwFree(w->mesh);
    }
    else
    {
        free_sector(((RpPlaneSector*)s)->leftSubTree);
        free_sector(((RpPlaneSector*)s)->rightSubTree);
    }
    rwFree(s);
}

RpWorld* RpWorldDestroy(RpWorld* w)
{
    s_worlds.erase(std::remove(s_worlds.begin(), s_worlds.end(), w), s_worlds.end());
    // Detach the lights still in the world so destroying them later doesn't
    // unlink through freed memory.
    RwLinkList* lists[2] = { &w->directionalLightList, &w->lightList };
    for (int i = 0; i < 2; i++)
    {
        RwLLLink* end = &lists[i]->link;
        for (RwLLLink* l = end->next; l && l != end;)
        {
            RwLLLink* next = l->next;
            l->next = l->prev = NULL;
            l = next;
        }
        rwLinkListInitialize(lists[i]);
    }
    free_sector(w->rootSector);
    for (RwInt32 i = 0; i < w->matList.numMaterials; i++)
    {
        if (w->matList.materials[i])
        {
            RpMaterialDestroy(w->matList.materials[i]);
        }
    }
    rwFree(w->matList.materials);
    delete pc_world(w)->sectors;
    if (rwGlobals()->curWorld == w)
    {
        rwGlobals()->curWorld = NULL;
    }
    rwFree(w);
    return w;
}

RpWorld* RpWorldForAllWorldSectors(RpWorld* w, RpWorldSector* (*cb)(RpWorldSector*, void*), void* data)
{
    std::vector<RpWorldSector*>& v = *pc_world(w)->sectors;
    for (size_t i = 0; i < v.size(); i++)
    {
        if (!cb(v[i], data))
        {
            break;
        }
    }
    return w;
}

RpWorldSector* RpMatFXWorldSectorEnableEffects(RpWorldSector* s) { return s; }

RpWorld* RpWorldAddCamera(RpWorld* w, RwCamera* c)
{
    rwGlobals()->curWorld = w;
    return w;
}

RpWorld* RpWorldRemoveCamera(RpWorld* w, RwCamera* c) { return w; }

// ---------------------------------------------------------------- lights
static RwObjectHasFrame* light_sync(RwObjectHasFrame* o) { return o; }

RpLight* RpLightCreate(RwInt32 type)
{
    RpLight* l = (RpLight*)rwCalloc(1, sizeof(RpLight) + 0x20);
    l->object.object.type = rpLIGHT;
    l->object.object.subType = (RwUInt8)type;
    l->object.object.flags = rpLIGHTLIGHTATOMICS | rpLIGHTLIGHTWORLD;
    l->object.sync = light_sync;
    l->color.red = l->color.green = l->color.blue = l->color.alpha = 1.0f;
    l->radius = 1.0f;
    l->minusCosAngle = 1.0f;
    rwLinkListInitialize(&l->WorldSectorsInLight);
    l->inWorld.next = l->inWorld.prev = NULL;
    return l;
}

// The game links and unlinks lights by hand (zLightAddLocalEnv and
// zLightRemoveLocalEnv) without clearing their links, so only unlink a light
// whose neighbours still point back at it.
static void light_unlink(RpLight* l)
{
    RwLLLink* k = &l->inWorld;
    if (k->next && k->prev && k->next->prev == k && k->prev->next == k)
    {
        rwLinkListRemoveLLLink(k);
    }
    k->next = k->prev = NULL;
}

RwInt32 RpLightDestroy(RpLight* l)
{
    light_unlink(l);
    _rwObjectHasFrameSetFrame(l, NULL);
    rwFree(l);
    return 1;
}

RpLight* RpLightSetColor(RpLight* l, RwRGBAReal* c)
{
    l->color = *c;
    return l;
}

RpLight* RpLightSetRadius(RpLight* l, float r)
{
    l->radius = r;
    return l;
}

RpLight* RpLightSetConeAngle(RpLight* l, float angle)
{
    l->minusCosAngle = -cosf(angle);
    return l;
}

RpWorld* RpWorldAddLight(RpWorld* w, RpLight* l)
{
    light_unlink(l);
    RwLinkList* list = (l->object.object.subType < 0x80) ? &w->directionalLightList : &w->lightList;
    rwLinkListAddLLLink(list, &l->inWorld);
    return w;
}

RpWorld* RpWorldRemoveLight(RpWorld* w, RpLight* l)
{
    light_unlink(l);
    return w;
}

RpWorld* RpWorldSetSurfaceProperties(RpWorld* w, const RwSurfaceProperties* p)
{
    w->surfaceProps = *p;
    return w;
}

}

void pc_render_world(RpWorld* w, std::vector<RpWorldSector*>& sectors);

extern "C" {

RpWorld* RpWorldRender(RpWorld* w)
{
    pc_render_world(w, *pc_world(w)->sectors);
    return w;
}

}

// ---------------------------------------------------------------- collision
static bool line_tri(const RwV3d& o, const RwV3d& dir, const RwV3d& a, const RwV3d& b, const RwV3d& c, float* t)
{
    RwV3d e1 = { b.x - a.x, b.y - a.y, b.z - a.z }, e2 = { c.x - a.x, c.y - a.y, c.z - a.z };
    RwV3d p = { dir.y * e2.z - dir.z * e2.y, dir.z * e2.x - dir.x * e2.z, dir.x * e2.y - dir.y * e2.x };
    float det = e1.x * p.x + e1.y * p.y + e1.z * p.z;
    if (fabsf(det) < 1e-12f)
    {
        return false;
    }
    float inv = 1.0f / det;
    RwV3d s = { o.x - a.x, o.y - a.y, o.z - a.z };
    float u = (s.x * p.x + s.y * p.y + s.z * p.z) * inv;
    if (u < 0 || u > 1)
    {
        return false;
    }
    RwV3d q = { s.y * e1.z - s.z * e1.y, s.z * e1.x - s.x * e1.z, s.x * e1.y - s.y * e1.x };
    float v = (dir.x * q.x + dir.y * q.y + dir.z * q.z) * inv;
    if (v < 0 || u + v > 1)
    {
        return false;
    }
    *t = (e2.x * q.x + e2.y * q.y + e2.z * q.z) * inv;
    return *t >= 0 && *t <= 1;
}

static float dot(const RwV3d& a, const RwV3d& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static RwV3d sub(const RwV3d& a, const RwV3d& b) { RwV3d r = { a.x - b.x, a.y - b.y, a.z - b.z }; return r; }

// Squared distance from p to triangle abc.
static float point_tri_dist2(const RwV3d& p, const RwV3d& a, const RwV3d& b, const RwV3d& c)
{
    RwV3d ab = sub(b, a), ac = sub(c, a), ap = sub(p, a);
    float d1 = dot(ab, ap), d2 = dot(ac, ap);
    RwV3d cl;
    if (d1 <= 0 && d2 <= 0) cl = a;
    else
    {
        RwV3d bp = sub(p, b);
        float d3 = dot(ab, bp), d4 = dot(ac, bp);
        if (d3 >= 0 && d4 <= d3) cl = b;
        else
        {
            float vc = d1 * d4 - d3 * d2;
            if (vc <= 0 && d1 >= 0 && d3 <= 0) { float v = d1 / (d1 - d3); cl.x = a.x + ab.x * v; cl.y = a.y + ab.y * v; cl.z = a.z + ab.z * v; }
            else
            {
                RwV3d cp = sub(p, c);
                float d5 = dot(ab, cp), d6 = dot(ac, cp);
                if (d6 >= 0 && d5 <= d6) cl = c;
                else
                {
                    float vb = d5 * d2 - d1 * d6;
                    if (vb <= 0 && d2 >= 0 && d6 <= 0) { float w = d2 / (d2 - d6); cl.x = a.x + ac.x * w; cl.y = a.y + ac.y * w; cl.z = a.z + ac.z * w; }
                    else
                    {
                        float va = d3 * d6 - d5 * d4;
                        if (va <= 0 && (d4 - d3) >= 0 && (d5 - d6) >= 0)
                        {
                            float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
                            cl.x = b.x + (c.x - b.x) * w; cl.y = b.y + (c.y - b.y) * w; cl.z = b.z + (c.z - b.z) * w;
                        }
                        else
                        {
                            float den = 1.0f / (va + vb + vc);
                            float v = vb * den, w = vc * den;
                            cl.x = a.x + ab.x * v + ac.x * w; cl.y = a.y + ab.y * v + ac.y * w; cl.z = a.z + ab.z * v + ac.z * w;
                        }
                    }
                }
            }
        }
    }
    RwV3d d = sub(p, cl);
    return dot(d, d);
}

static void make_coll_tri(RpCollisionTriangle* t, RwV3d* a, RwV3d* b, RwV3d* c, RwInt32 index)
{
    RwV3d e1 = sub(*b, *a), e2 = sub(*c, *a);
    RwV3d n = { e1.y * e2.z - e1.z * e2.y, e1.z * e2.x - e1.x * e2.z, e1.x * e2.y - e1.y * e2.x };
    float l = sqrtf(dot(n, n));
    if (l > 0)
    {
        n.x /= l; n.y /= l; n.z /= l;
    }
    t->normal = n;
    t->point = *a;
    t->index = index;
    t->vertices[0] = a;
    t->vertices[1] = b;
    t->vertices[2] = c;
}

static bool box_overlap(const RwBBox& b, const RwV3d& mn, const RwV3d& mx)
{
    return !(mx.x < b.inf.x || mn.x > b.sup.x || mx.y < b.inf.y || mn.y > b.sup.y || mx.z < b.inf.z || mn.z > b.sup.z);
}

// A sector's collision data, built on first use: each polygon's bounding box
// (inf xyz, sup xyz) and a bounding volume tree over them, so a query visits
// only the polygons near it instead of every box in the sector.
struct BvhNode
{
    float box[6];
    int first, count; // leaf: polygons order[first .. first + count)
    int skip;         // inner node: index of the second child (the first follows it)
};

struct SectorColl
{
    std::vector<float> boxes;
    std::vector<int> order;
    std::vector<BvhNode> nodes;
};

static bool boxes_overlap(const float* a, const RwV3d& mn, const RwV3d& mx)
{
    return !(mx.x < a[0] || mn.x > a[3] || mx.y < a[1] || mn.y > a[4] || mx.z < a[2] || mn.z > a[5]);
}

static void bvh_build(SectorColl* c, int first, int count)
{
    int self = (int)c->nodes.size();
    c->nodes.push_back(BvhNode());
    float box[6] = { FLT_MAX, FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX };
    float cmin[3] = { FLT_MAX, FLT_MAX, FLT_MAX }, cmax[3] = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
    for (int i = first; i < first + count; i++)
    {
        const float* b = &c->boxes[(size_t)c->order[i] * 6];
        for (int k = 0; k < 3; k++)
        {
            box[k] = fminf(box[k], b[k]);
            box[k + 3] = fmaxf(box[k + 3], b[k + 3]);
            float centre = b[k] + b[k + 3];
            cmin[k] = fminf(cmin[k], centre);
            cmax[k] = fmaxf(cmax[k], centre);
        }
    }
    memcpy(c->nodes[self].box, box, sizeof(box));
    if (count <= 6)
    {
        c->nodes[self].first = first;
        c->nodes[self].count = count;
        c->nodes[self].skip = 0;
        return;
    }
    // Split at the median along the axis where the polygon centres spread most.
    int axis = 0;
    for (int k = 1; k < 3; k++)
    {
        if (cmax[k] - cmin[k] > cmax[axis] - cmin[axis])
        {
            axis = k;
        }
    }
    const float* boxes = c->boxes.data();
    int half = count / 2;
    std::nth_element(c->order.begin() + first, c->order.begin() + first + half, c->order.begin() + first + count,
                     [boxes, axis](int a, int b) {
                         return boxes[(size_t)a * 6 + axis] + boxes[(size_t)a * 6 + axis + 3] <
                                boxes[(size_t)b * 6 + axis] + boxes[(size_t)b * 6 + axis + 3];
                     });
    c->nodes[self].first = 0;
    c->nodes[self].count = 0;
    bvh_build(c, first, half);
    c->nodes[self].skip = (int)c->nodes.size();
    bvh_build(c, first + half, count - half);
}

// Builds the boxes and tree of n triangles; vi(t, k) is triangle t's vertex k.
template <typename VertIndex>
static SectorColl* coll_build(int n, const RwV3d* verts, VertIndex vi)
{
    SectorColl* c = new SectorColl;
    c->boxes.resize((size_t)n * 6);
    c->order.resize(n);
    for (int p = 0; p < n; p++)
    {
        const RwV3d& a = verts[vi(p, 0)];
        const RwV3d& c1 = verts[vi(p, 1)];
        const RwV3d& c2 = verts[vi(p, 2)];
        float* o = &c->boxes[(size_t)p * 6];
        o[0] = fminf(a.x, fminf(c1.x, c2.x));
        o[1] = fminf(a.y, fminf(c1.y, c2.y));
        o[2] = fminf(a.z, fminf(c1.z, c2.z));
        o[3] = fmaxf(a.x, fmaxf(c1.x, c2.x));
        o[4] = fmaxf(a.y, fmaxf(c1.y, c2.y));
        o[5] = fmaxf(a.z, fmaxf(c1.z, c2.z));
        c->order[p] = p;
    }
    if (n > 0)
    {
        c->nodes.reserve((size_t)n / 2 + 1);
        bvh_build(c, 0, n);
    }
    return c;
}

static SectorColl* sector_coll(const RpWorldSector* s)
{
    SectorColl*& c = s_sectorColl[s];
    if (!c)
    {
        const RpPolygon* polys = s->polygons;
        c = coll_build(s->numPolygons, s->vertices, [polys](int t, int k) { return polys[t].vertIndex[k]; });
    }
    return c;
}

static void sector_coll_free(const RpWorldSector* s)
{
    std::unordered_map<const RpWorldSector*, SectorColl*>::iterator it = s_sectorColl.find(s);
    if (it != s_sectorColl.end())
    {
        delete it->second;
        s_sectorColl.erase(it);
    }
}

// The polygons of a sector whose boxes overlap mn-mx, in index order (the
// order the callbacks saw before the tree: some stop at the first hit).
static void coll_candidates(SectorColl* c, const RwV3d& mn, const RwV3d& mx, std::vector<int>& out)
{
    out.clear();
    const BvhNode* nodes = c->nodes.data();
    int stack[64], sp = 0;
    if (!c->nodes.empty())
    {
        stack[sp++] = 0;
    }
    while (sp)
    {
        int i = stack[--sp];
        const BvhNode& node = nodes[i];
        if (!boxes_overlap(node.box, mn, mx))
        {
            continue;
        }
        if (node.count)
        {
            for (int k = node.first; k < node.first + node.count; k++)
            {
                int p = c->order[k];
                if (boxes_overlap(&c->boxes[(size_t)p * 6], mn, mx))
                {
                    out.push_back(p);
                }
            }
        }
        else
        {
            // The tree halves each level: 64 entries cover far more polygons than a sector has.
            stack[sp++] = node.skip;
            stack[sp++] = i + 1;
        }
    }
    if (out.size() > 1)
    {
        std::sort(out.begin(), out.end());
    }
}

static void sector_candidates(const RpWorldSector* s, const RwV3d& mn, const RwV3d& mx, std::vector<int>& out)
{
    coll_candidates(sector_coll(s), mn, mx, out);
}

// The same for geometries (object space), built on first use. Geometry the
// game locks to rewrite keeps testing every triangle.
static std::unordered_map<const RpGeometry*, SectorColl*> s_geomColl;
static std::unordered_map<const RpGeometry*, bool> s_geomDynamic;

void pc_collision_forget(const void* geometry, int dynamic)
{
    const RpGeometry* g = (const RpGeometry*)geometry;
    std::unordered_map<const RpGeometry*, SectorColl*>::iterator it = s_geomColl.find(g);
    if (it != s_geomColl.end())
    {
        delete it->second;
        s_geomColl.erase(it);
    }
    if (dynamic)
    {
        s_geomDynamic[g] = true;
    }
    else
    {
        s_geomDynamic.erase(g);
    }
}

static SectorColl* geometry_coll(const RpGeometry* g)
{
    if (s_geomDynamic.count(g))
    {
        return NULL;
    }
    SectorColl*& c = s_geomColl[g];
    if (!c)
    {
        const RpTriangle* tris = g->triangles;
        c = coll_build(g->numTriangles, g->morphTarget->verts, [tris](int t, int k) { return tris[t].vertIndex[k]; });
    }
    return c;
}

// Bounds of an intersection primitive.
static void isx_bounds(const RpIntersection* isx, RwV3d* mn, RwV3d* mx)
{
    if (isx->type == rpINTERSECTLINE)
    {
        const RwLine& l = isx->t.line;
        mn->x = fminf(l.start.x, l.end.x); mn->y = fminf(l.start.y, l.end.y); mn->z = fminf(l.start.z, l.end.z);
        mx->x = fmaxf(l.start.x, l.end.x); mx->y = fmaxf(l.start.y, l.end.y); mx->z = fmaxf(l.start.z, l.end.z);
    }
    else if (isx->type == rpINTERSECTSPHERE)
    {
        const RwSphere& s = isx->t.sphere;
        mn->x = s.center.x - s.radius; mn->y = s.center.y - s.radius; mn->z = s.center.z - s.radius;
        mx->x = s.center.x + s.radius; mx->y = s.center.y + s.radius; mx->z = s.center.z + s.radius;
    }
    else if (isx->type == rpINTERSECTBOX)
    {
        *mn = isx->t.box.inf;
        *mx = isx->t.box.sup;
    }
    else
    {
        *mn = *mx = isx->t.point;
    }
}

// Tests one triangle; returns true and the reported distance on a hit.
static bool test_tri(const RpIntersection* isx, const RwV3d& a, const RwV3d& b, const RwV3d& c, float* dist)
{
    if (isx->type == rpINTERSECTLINE)
    {
        RwV3d dir = sub(isx->t.line.end, isx->t.line.start);
        return line_tri(isx->t.line.start, dir, a, b, c, dist);
    }
    if (isx->type == rpINTERSECTSPHERE)
    {
        float d2 = point_tri_dist2(isx->t.sphere.center, a, b, c);
        if (d2 > isx->t.sphere.radius * isx->t.sphere.radius)
        {
            return false;
        }
        *dist = sqrtf(d2);
        return true;
    }
    if (isx->type == rpINTERSECTPOINT)
    {
        float d2 = point_tri_dist2(isx->t.point, a, b, c);
        *dist = sqrtf(d2);
        return d2 < 1e-6f;
    }
    if (isx->type == rpINTERSECTBOX)
    {
        RwV3d mn = { fminf(a.x, fminf(b.x, c.x)), fminf(a.y, fminf(b.y, c.y)), fminf(a.z, fminf(b.z, c.z)) };
        RwV3d mx = { fmaxf(a.x, fmaxf(b.x, c.x)), fmaxf(a.y, fmaxf(b.y, c.y)), fmaxf(a.z, fmaxf(b.z, c.z)) };
        *dist = 0;
        return box_overlap(isx->t.box, mn, mx);
    }
    return false;
}

extern "C" {

typedef RpCollisionTriangle* (*WorldCollCB)(RpIntersection*, RpWorldSector*, RpCollisionTriangle*, float, void*);

// PC_PROF=1: collision work per second.
static struct { int queries[5], tris, hits, atomicQueries, atomicTris; unsigned long long start; } s_cprof;
static void cprof_tick()
{
    static int on = -1;
    if (on < 0)
    {
        on = getenv("PC_PROF") != NULL;
    }
    if (!on)
    {
        return;
    }
    unsigned long long now = pc_perf_counter();
    if (!s_cprof.start)
    {
        s_cprof.start = now;
    }
    if (pc_perf_seconds(now - s_cprof.start) >= 1.0)
    {
        pc_log("[prof] collision/s: world queries line %d sphere %d box %d point %d, %d tris tested, %d hits; "
               "atomic queries %d, %d tris", s_cprof.queries[rpINTERSECTLINE], s_cprof.queries[rpINTERSECTSPHERE],
               s_cprof.queries[rpINTERSECTBOX], s_cprof.queries[rpINTERSECTPOINT], s_cprof.tris, s_cprof.hits,
               s_cprof.atomicQueries, s_cprof.atomicTris);
        memset(&s_cprof, 0, sizeof(s_cprof));
        s_cprof.start = now;
    }
}
typedef RpCollisionTriangle* (*AtomicCollCB)(RpIntersection*, RpCollisionTriangle*, float, void*);

RpWorld* RpCollisionWorldForAllIntersections(RpWorld* w, RpIntersection* isx, WorldCollCB cb, void* data)
{
    if (!w)
    {
        return w;
    }
    RwV3d mn, mx;
    isx_bounds(isx, &mn, &mx);
    cprof_tick();
    if (isx->type >= 0 && isx->type < 5)
    {
        s_cprof.queries[isx->type]++;
    }
    // Candidate lists, one per nesting level (callbacks may run queries of their own).
    static std::vector<int> s_cand[8];
    static int s_depth;
    std::vector<int> local;
    std::vector<int>& cand = s_depth < 8 ? s_cand[s_depth] : local;
    s_depth++;
    std::vector<RpWorldSector*>& v = *pc_world(w)->sectors;
    for (size_t i = 0; i < v.size(); i++)
    {
        RpWorldSector* s = v[i];
        if (!s->polygons || !box_overlap(s->boundingBox, mn, mx))
        {
            continue;
        }
        sector_candidates(s, mn, mx, cand);
        for (size_t ci = 0; ci < cand.size(); ci++)
        {
            RwInt32 p = cand[ci];
            s_cprof.tris++;
            RpPolygon& poly = s->polygons[p];
            RwV3d* a = &s->vertices[poly.vertIndex[0]];
            RwV3d* b = &s->vertices[poly.vertIndex[1]];
            RwV3d* c = &s->vertices[poly.vertIndex[2]];
            float d;
            if (test_tri(isx, *a, *b, *c, &d))
            {
                s_cprof.hits++;
                RpCollisionTriangle tri;
                make_coll_tri(&tri, a, b, c, p);
                if (!cb(isx, s, &tri, d, data))
                {
                    s_depth--;
                    return w;
                }
            }
        }
    }
    s_depth--;
    return w;
}

RpAtomic* RpAtomicForAllIntersections(RpAtomic* a, RpIntersection* isx, AtomicCollCB cb, void* data)
{
    RpGeometry* g = a->geometry;
    RwFrame* f = (RwFrame*)a->object.object.parent;
    if (!g || !g->triangles || !g->morphTarget || !g->morphTarget->verts || !f)
    {
        return a;
    }
    // Move the query into the atomic's object space.
    RwMatrix inv;
    RwMatrixInvert(&inv, RwFrameGetLTM(f));
    RpIntersection local = *isx;
    if (isx->type == rpINTERSECTLINE)
    {
        RwV3dTransformPoints(&local.t.line.start, &isx->t.line.start, 1, &inv);
        RwV3dTransformPoints(&local.t.line.end, &isx->t.line.end, 1, &inv);
    }
    else if (isx->type == rpINTERSECTSPHERE)
    {
        RwV3dTransformPoints(&local.t.sphere.center, &isx->t.sphere.center, 1, &inv);
        float sx = sqrtf(dot(inv.right, inv.right));
        local.t.sphere.radius = isx->t.sphere.radius * sx;
    }
    else if (isx->type == rpINTERSECTPOINT)
    {
        RwV3dTransformPoints(&local.t.point, &isx->t.point, 1, &inv);
    }
    RwV3d* verts = g->morphTarget->verts;
    s_cprof.atomicQueries++;
    // The triangles near the query, in index order (all of them for geometry
    // the game rewrites), one list per nesting level.
    static std::vector<int> s_cand[8];
    static int s_depth;
    std::vector<int> local2;
    std::vector<int>& cand = s_depth < 8 ? s_cand[s_depth] : local2;
    SectorColl* coll = geometry_coll(g);
    if (coll)
    {
        RwV3d mn, mx;
        isx_bounds(&local, &mn, &mx);
        coll_candidates(coll, mn, mx, cand);
    }
    int count = coll ? (int)cand.size() : g->numTriangles;
    s_cprof.atomicTris += count;
    s_depth++;
    for (RwInt32 ci = 0; ci < count; ci++)
    {
        RwInt32 i = coll ? cand[ci] : ci;
        RpTriangle& t = g->triangles[i];
        RwV3d* va = &verts[t.vertIndex[0]];
        RwV3d* vb = &verts[t.vertIndex[1]];
        RwV3d* vc = &verts[t.vertIndex[2]];
        float d;
        if (test_tri(&local, *va, *vb, *vc, &d))
        {
            RpCollisionTriangle tri;
            make_coll_tri(&tri, va, vb, vc, i);
            if (!cb(isx, &tri, d, data))
            {
                break;
            }
        }
    }
    s_depth--;
    return a;
}

}

// ---------------------------------------------------------------- debugging
extern "C" {

RpWorld* pc_current_world() { return (RpWorld*)rwGlobals()->curWorld; }

// Bounds of the world polygons using material 'mat'; returns their count.
int pc_world_material_bounds(RpWorld* w, int mat, float* out6)
{
    int n = 0;
    if (!w)
    {
        return 0;
    }
    std::vector<RpWorldSector*>& v = *pc_world(w)->sectors;
    for (size_t i = 0; i < v.size(); i++)
    {
        RpWorldSector* s = v[i];
        for (RwInt32 p = 0; p < s->numPolygons && s->polygons; p++)
        {
            if (s->polygons[p].matIndex != mat)
            {
                continue;
            }
            if (n % 25 == 0)
            {
                const RwV3d& q0 = s->vertices[s->polygons[p].vertIndex[0]];
                pc_log("[surf]   sample poly at %.1f %.1f %.1f", q0.x, q0.y, q0.z);
            }
            for (int k = 0; k < 3; k++)
            {
                const RwV3d& q = s->vertices[s->polygons[p].vertIndex[k]];
                const float c[3] = { q.x, q.y, q.z };
                for (int a = 0; a < 3; a++)
                {
                    if (!n || c[a] < out6[a]) out6[a] = c[a];
                    if (!n || c[a] > out6[3 + a]) out6[3 + a] = c[a];
                }
            }
            n++;
        }
    }
    return n;
}

}

extern "C" int pc_world_max_window_base(RpWorld* w)
{
    int m = 0;
    std::vector<RpWorldSector*>& v = *pc_world(w)->sectors;
    for (size_t i = 0; i < v.size(); i++)
    {
        m = v[i]->matListWindowBase > m ? v[i]->matListWindowBase : m;
    }
    return m;
}

// Debug: lists what a vertical ray at (x, z) hits in the current world.
static RpCollisionTriangle* probe_cb(RpIntersection* isx, RpWorldSector* s, RpCollisionTriangle* t, float d, void*)
{
    const RwLine& l = isx->t.line;
    pc_log("[probe]   hit y %.2f mat %d normal %.2f %.2f %.2f", l.start.y + (l.end.y - l.start.y) * d,
           s ? s->polygons[t->index].matIndex : -1, t->normal.x, t->normal.y, t->normal.z);
    return t;
}

extern "C" void pc_debug_probe(float x, float z)
{
    RpWorld* w = (RpWorld*)rwGlobals()->curWorld;
    if (!w)
    {
        return;
    }
    RpIntersection isx;
    memset(&isx, 0, sizeof(isx));
    isx.type = rpINTERSECTLINE;
    isx.t.line.start.x = x, isx.t.line.start.y = 300, isx.t.line.start.z = z;
    isx.t.line.end.x = x, isx.t.line.end.y = -300, isx.t.line.end.z = z;
    pc_log("[probe] (%.2f, %.2f)", x, z);
    for (size_t i = 0; i < s_worlds.size(); i++)
    {
        if (s_worlds.size() > 1)
        {
            pc_log("[probe]  world %u%s (%d sectors)", (unsigned)i, s_worlds[i] == w ? " (current)" : "",
                   (int)pc_world(s_worlds[i])->sectors->size());
        }
        RpCollisionWorldForAllIntersections(s_worlds[i], &isx, probe_cb, NULL);
    }
}

// Debug: triangles of every world within radius r of (x, y, z).
static RpCollisionTriangle* sphere_cb(RpIntersection* isx, RpWorldSector* s, RpCollisionTriangle* t, float d, void*)
{
    const RwV3d* v = t->vertices[0];
    pc_log("[probe]   tri mat %d normal %.2f %.2f %.2f dist %.2f v0 %.2f %.2f %.2f", s ? s->polygons[t->index].matIndex : -1,
           t->normal.x, t->normal.y, t->normal.z, d, v->x, v->y, v->z);
    return t;
}

extern "C" void pc_debug_sphere(float x, float y, float z, float r)
{
    RpIntersection isx;
    memset(&isx, 0, sizeof(isx));
    isx.type = rpINTERSECTSPHERE;
    isx.t.sphere.center.x = x, isx.t.sphere.center.y = y, isx.t.sphere.center.z = z;
    isx.t.sphere.radius = r;
    pc_log("[probe] sphere (%.2f, %.2f, %.2f) r %.2f", x, y, z, r);
    for (size_t i = 0; i < s_worlds.size(); i++)
    {
        pc_log("[probe]  world %u (%d sectors)", (unsigned)i, (int)pc_world(s_worlds[i])->sectors->size());
        RpCollisionWorldForAllIntersections(s_worlds[i], &isx, sphere_cb, NULL);
    }
}

extern "C" void pc_debug_sectors()
{
    RpWorld* w = (RpWorld*)rwGlobals()->curWorld;
    if (!w)
    {
        return;
    }
    std::vector<RpWorldSector*>& v = *pc_world(w)->sectors;
    for (size_t i = 0; i < v.size(); i++)
    {
        RpWorldSector* s = v[i];
        // Cross-check the polygon array against the BinMesh triangles.
        std::vector<unsigned long long> tris;
        if (s->mesh)
        {
            RpMesh* m = (RpMesh*)(s->mesh + 1);
            bool strip = (s->mesh->flags & 1) != 0;
            for (int k = 0; k < s->mesh->numMeshes; k++)
            {
                for (RwUInt32 j = 0; j + 2 < m[k].numIndices; j += strip ? 1 : 3)
                {
                    unsigned a = m[k].indices[j], b = m[k].indices[j + 1], c = m[k].indices[j + 2];
                    if (a == b || b == c || a == c) continue;
                    unsigned v[3] = { a, b, c };
                    std::sort(v, v + 3);
                    tris.push_back((unsigned long long)v[0] << 32 | v[1] << 16 | v[2]);
                }
            }
        }
        std::sort(tris.begin(), tris.end());
        int found = 0;
        for (RwInt32 p = 0; p < s->numPolygons && s->polygons; p++)
        {
            unsigned v[3] = { s->polygons[p].vertIndex[0], s->polygons[p].vertIndex[1], s->polygons[p].vertIndex[2] };
            std::sort(v, v + 3);
            found += std::binary_search(tris.begin(), tris.end(), (unsigned long long)v[0] << 32 | v[1] << 16 | v[2]);
        }
        pc_log("[sector] %u: %d of %d polygons match mesh triangles (%u mesh tris)", (unsigned)i, found, s->numPolygons,
               (unsigned)tris.size());
        pc_log("[sector] %u: polys %d (array %p) verts %d bbox (%.1f %.1f %.1f)-(%.1f %.1f %.1f)", (unsigned)i,
               s->numPolygons, s->polygons, s->numVertices, s->boundingBox.inf.x, s->boundingBox.inf.y,
               s->boundingBox.inf.z, s->boundingBox.sup.x, s->boundingBox.sup.y, s->boundingBox.sup.z);
    }
}
