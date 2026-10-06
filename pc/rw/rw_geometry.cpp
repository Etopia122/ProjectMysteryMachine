// RenderWare materials, geometry, atomics, clumps and their stream readers
// (generic RW 3.x data as stored on the GameCube disc), plus the HAnim, Skin,
// MatFX and UserData plugins the game uses.
#include "rw_internal.h"
#include "pc_platform.h"

#include <math.h>
#include <string.h>
#include <vector>

extern "C" RpHAnimHierarchy* RpHAnimHierarchyCreate(RwInt32, RwUInt32*, RwInt32*, RwInt32, RwInt32);

// ---------------------------------------------------------------- stream helpers
static bool read_struct(RwStream* s, std::vector<RwUInt8>& buf, RwUInt32* ver)
{
    RwUInt32 len;
    if (!RwStreamFindChunk(s, rwID_STRUCT, &len, ver))
    {
        return false;
    }
    buf.resize(len);
    return RwStreamRead(s, buf.data(), len) == len;
}

static bool read_string(RwStream* s, char* out, RwUInt32 max)
{
    RwUInt32 len, ver;
    if (!RwStreamFindChunk(s, rwID_STRING, &len, &ver))
    {
        return false;
    }
    std::vector<char> tmp(len + 1, 0);
    RwStreamRead(s, tmp.data(), len);
    strncpy(out, tmp.data(), max - 1);
    out[max - 1] = 0;
    return true;
}

void pc_read_extension(RwStream* s, void (*plugin)(RwStream*, RwUInt32, RwUInt32, RwUInt32, void*), void* obj)
{
    RwUInt32 len, ver;
    if (!RwStreamFindChunk(s, rwID_EXTENSION, &len, &ver))
    {
        return;
    }
    while (len >= 12)
    {
        RwChunkHeaderInfo h;
        if (!RwStreamReadChunkHeaderInfo(s, &h))
        {
            return;
        }
        len -= 12;
        RwInt32 before = s->position;
        if (plugin)
        {
            plugin(s, h.type, h.length, h.version, obj);
        }
        RwInt32 used = s->position - before;
        if ((RwUInt32)used < h.length)
        {
            RwStreamSkip(s, h.length - used);
        }
        len -= h.length < len ? h.length : len;
    }
}

extern "C" {

// ---------------------------------------------------------------- materials
RpMaterial* RpMaterialCreate()
{
    PcMaterial* p = (PcMaterial*)rwCalloc(1, sizeof(PcMaterial));
    RpMaterial* m = &p->m;
    m->color.red = m->color.green = m->color.blue = m->color.alpha = 255;
    m->surfaceProps.ambient = m->surfaceProps.specular = m->surfaceProps.diffuse = 1.0f;
    m->refCount = 1;
    p->fxCoef = 1.0f;
    return m;
}

RwInt32 RwTextureDestroy(RwTexture* t);

RwInt32 RpMaterialDestroy(RpMaterial* m)
{
    if (--m->refCount > 0)
    {
        return 1;
    }
    if (m->texture)
    {
        RwTextureDestroy(m->texture);
    }
    if (pc_material(m)->fxTexture)
    {
        RwTextureDestroy(pc_material(m)->fxTexture);
    }
    rwFree(m);
    return 1;
}

RpMaterial* RpMaterialSetTexture(RpMaterial* m, RwTexture* t)
{
    if (t)
    {
        t->refCount++;
    }
    if (m->texture)
    {
        RwTextureDestroy(m->texture);
    }
    m->texture = t;
    return m;
}

}

static RwTexture* read_texture_chunk(RwStream* s)
{
    RwUInt32 len, ver;
    if (!RwStreamFindChunk(s, rwID_TEXTURE, &len, &ver))
    {
        return NULL;
    }
    std::vector<RwUInt8> st;
    if (!read_struct(s, st, &ver))
    {
        return NULL;
    }
    RwUInt32 fa = st.size() >= 4 ? rd32(st.data()) : 0;
    char name[32], mask[32];
    read_string(s, name, sizeof(name));
    read_string(s, mask, sizeof(mask));
    pc_read_extension(s, NULL, NULL);
    RwTexture* t = RwTextureRead(name, mask);
    if (t)
    {
        t->filterAddressing = fa & 0xFFFF;
    }
    return t;
}

static void material_plugin(RwStream* s, RwUInt32 id, RwUInt32 len, RwUInt32 ver, void* obj)
{
    PcMaterial* m = (PcMaterial*)obj;
    if (id != rwID_MATERIALEFFECTSPLUGIN)
    {
        return;
    }
    RwUInt8 b[4];
    RwStreamRead(s, b, 4);
    m->matfxType = (RwInt32)rd32(b);
    for (int i = 0; i < 2; i++)
    {
        if (RwStreamRead(s, b, 4) != 4)
        {
            return;
        }
        RwUInt32 type = rd32(b);
        if (type == 2) // environment map: coef, useFrameBufferAlpha, hasTexture, [texture]
        {
            RwUInt8 e[12];
            RwStreamRead(s, e, 12);
            m->fxCoef = rdf(e);
            m->fxUseFBAlpha = (RwInt32)rd32(e + 4);
            if (rd32(e + 8))
            {
                m->fxTexture = read_texture_chunk(s);
            }
        }
        else if (type == 1) // bump map: coef, hasBumpedTexture, [tex], hasTexture, [tex]
        {
            RwUInt8 e[8];
            RwStreamRead(s, e, 8);
            m->fxCoef = rdf(e);
            if (rd32(e + 4))
            {
                RwTexture* t = read_texture_chunk(s);
                if (t)
                {
                    RwTextureDestroy(t);
                }
            }
            RwStreamRead(s, e, 4);
            if (rd32(e))
            {
                m->fxTexture = read_texture_chunk(s);
            }
        }
        else if (type == 4) // dual: srcBlend, destBlend, hasTexture, [tex]
        {
            RwUInt8 e[12];
            RwStreamRead(s, e, 12);
            if (rd32(e + 8))
            {
                m->fxTexture = read_texture_chunk(s);
            }
        }
    }
}

RpMaterial* pc_read_material(RwStream* s)
{
    RwUInt32 len, ver;
    if (!RwStreamFindChunk(s, rwID_MATERIAL, &len, &ver))
    {
        return NULL;
    }
    std::vector<RwUInt8> st;
    if (!read_struct(s, st, &ver))
    {
        return NULL;
    }
    RpMaterial* m = RpMaterialCreate();
    memcpy(&m->color, &st[4], 4);
    RwInt32 textured = (RwInt32)rd32(&st[12]);
    if (st.size() >= 28)
    {
        m->surfaceProps.ambient = rdf(&st[16]);
        m->surfaceProps.specular = rdf(&st[20]);
        m->surfaceProps.diffuse = rdf(&st[24]);
    }
    if (textured)
    {
        m->texture = read_texture_chunk(s);
    }
    pc_read_extension(s, material_plugin, m);
    return m;
}

RwInt32 pc_read_material_list(RwStream* s, RpMaterialList* list)
{
    RwUInt32 len, ver;
    if (!RwStreamFindChunk(s, rwID_MATLIST, &len, &ver))
    {
        return 0;
    }
    std::vector<RwUInt8> st;
    if (!read_struct(s, st, &ver))
    {
        return 0;
    }
    RwInt32 n = (RwInt32)rd32(st.data());
    list->numMaterials = n;
    list->space = n;
    list->materials = (RpMaterial**)rwCalloc(n ? n : 1, sizeof(RpMaterial*));
    for (RwInt32 i = 0; i < n; i++)
    {
        RwInt32 idx = (RwInt32)rd32(&st[4 + i * 4]);
        if (idx < 0)
        {
            list->materials[i] = pc_read_material(s);
        }
        else
        {
            list->materials[i] = list->materials[idx];
            list->materials[i]->refCount++;
        }
    }
    return 1;
}

RpMeshHeader* pc_read_binmesh(RwStream* s, RwUInt32 len, RpMaterialList* mats)
{
    RwUInt8 h[12];
    RwStreamRead(s, h, 12);
    RwUInt32 flags = rd32(h), numMeshes = rd32(h + 4), total = rd32(h + 8);
    size_t size = sizeof(RpMeshHeader) + numMeshes * sizeof(RpMesh) + total * sizeof(RwUInt16);
    RpMeshHeader* mh = (RpMeshHeader*)rwCalloc(1, size);
    mh->flags = flags;
    mh->numMeshes = (RwUInt16)numMeshes;
    mh->totalIndicesInMesh = total;
    mh->firstMeshOffset = 0;
    RpMesh* mesh = (RpMesh*)(mh + 1);
    RwUInt16* idx = (RwUInt16*)(mesh + numMeshes);
    for (RwUInt32 i = 0; i < numMeshes; i++)
    {
        RwUInt8 m[8];
        RwStreamRead(s, m, 8);
        mesh[i].numIndices = rd32(m);
        RwUInt32 mat = rd32(m + 4);
        mesh[i].material = mats && mat < (RwUInt32)mats->numMaterials ? mats->materials[mat] : NULL;
        mesh[i].indices = idx;
        std::vector<RwUInt8> raw(mesh[i].numIndices * 4);
        RwStreamRead(s, raw.data(), (RwUInt32)raw.size());
        for (RwUInt32 k = 0; k < mesh[i].numIndices; k++)
        {
            idx[k] = (RwUInt16)rd32(&raw[k * 4]);
        }
        idx += mesh[i].numIndices;
    }
    return mh;
}

// ---------------------------------------------------------------- geometry
extern "C" {

RpGeometry* RpGeometryCreate(RwInt32 numVerts, RwInt32 numTris, RwUInt32 format)
{
    PcGeometry* p = (PcGeometry*)rwCalloc(1, sizeof(PcGeometry));
    RpGeometry* g = &p->g;
    g->object.type = 8;
    g->flags = format & 0x00FF00FF;
    g->refCount = 1;
    g->numVertices = numVerts;
    g->numTriangles = numTris;
    g->numMorphTargets = 1;
    int ntc = (format >> 16) & 0xFF;
    if (!ntc)
    {
        ntc = (format & rpGEOMETRYTEXTURED2) ? 2 : (format & rpGEOMETRYTEXTURED) ? 1 : 0;
    }
    g->numTexCoordSets = ntc;
    if (numVerts > 0)
    {
        if (format & rpGEOMETRYPRELIT)
        {
            g->preLitLum = (RwRGBA*)rwCalloc(numVerts, sizeof(RwRGBA));
        }
        for (int i = 0; i < ntc; i++)
        {
            g->texCoords[i] = (RwTexCoords*)rwCalloc(numVerts, sizeof(RwTexCoords));
        }
    }
    if (numTris > 0)
    {
        g->triangles = (RpTriangle*)rwCalloc(numTris, sizeof(RpTriangle));
    }
    g->morphTarget = (RpMorphTarget*)rwCalloc(1, sizeof(RpMorphTarget));
    g->morphTarget->parentGeom = g;
    if (numVerts > 0)
    {
        g->morphTarget->verts = (RwV3d*)rwCalloc(numVerts, sizeof(RwV3d));
        if (format & rpGEOMETRYNORMALS)
        {
            g->morphTarget->normals = (RwV3d*)rwCalloc(numVerts, sizeof(RwV3d));
        }
    }
    return g;
}

RwInt32 RpGeometryDestroy(RpGeometry* g)
{
    if (--g->refCount > 0)
    {
        return 1;
    }
    PcGeometry* p = pc_geometry(g);
    for (RwInt32 i = 0; i < g->matList.numMaterials; i++)
    {
        if (g->matList.materials[i])
        {
            RpMaterialDestroy(g->matList.materials[i]);
        }
    }
    rwFree(g->matList.materials);
    rwFree(g->triangles);
    rwFree(g->preLitLum);
    for (int i = 0; i < 8; i++)
    {
        rwFree(g->texCoords[i]);
    }
    for (RwInt32 i = 0; i < g->numMorphTargets && g->morphTarget; i++)
    {
        rwFree(g->morphTarget[i].verts);
        rwFree(g->morphTarget[i].normals);
    }
    rwFree(g->morphTarget);
    rwFree(g->mesh);
    if (p->skin)
    {
        rwFree(p->skin->invMats);
        rwFree(p->skin->indices);
        rwFree(p->skin->weights);
        rwFree(p->skin);
    }
    for (RwInt32 i = 0; i < p->numUserData; i++)
    {
        rwFree(p->userData[i].name);
        rwFree(p->userData[i].data);
    }
    rwFree(p->userData);
    rwFree(p);
    return 1;
}

}

static void geometry_plugin(RwStream* s, RwUInt32 id, RwUInt32 len, RwUInt32 ver, void* obj)
{
    PcGeometry* p = (PcGeometry*)obj;
    RpGeometry* g = &p->g;
    if (id == rwID_BINMESHPLUGIN)
    {
        g->mesh = pc_read_binmesh(s, len, &g->matList);
    }
    else if (id == rwID_SKINPLUGIN)
    {
        std::vector<RwUInt8> d(len);
        RwStreamRead(s, d.data(), len);
        PcSkin* sk = (PcSkin*)rwCalloc(1, sizeof(PcSkin));
        sk->numBones = d[0];
        RwInt32 numUsed = d[1];
        sk->maxWeights = d[2];
        size_t o = 4;
        bool newFormat = ver >= 0x34003;
        if (newFormat)
        {
            o += numUsed;
        }
        else if (len == 4 + (size_t)g->numVertices * 20 + (size_t)sk->numBones * 64)
        {
            // pre-3.4.0.3 without per-bone markers
        }
        RwInt32 nv = g->numVertices;
        sk->indices = (RwUInt32*)rwMalloc((size_t)nv * 4);
        memcpy(sk->indices, &d[o], (size_t)nv * 4);
        o += (size_t)nv * 4;
        sk->weights = (float*)rwMalloc((size_t)nv * 16);
        for (RwInt32 i = 0; i < nv * 4; i++)
        {
            sk->weights[i] = rdf(&d[o + i * 4]);
        }
        o += (size_t)nv * 16;
        sk->invMats = (RwMatrix*)rwCalloc(sk->numBones, sizeof(RwMatrix));
        // Old files store a 0xDEADDEAD marker before each matrix.
        bool markers = !newFormat && len >= o + (size_t)sk->numBones * 68;
        for (RwInt32 b = 0; b < sk->numBones; b++)
        {
            if (markers)
            {
                o += 4;
            }
            float m[16];
            for (int k = 0; k < 16; k++)
            {
                m[k] = rdf(&d[o + k * 4]);
            }
            o += 64;
            RwMatrix& M = sk->invMats[b];
            M.right.x = m[0]; M.right.y = m[1]; M.right.z = m[2];
            M.up.x = m[4]; M.up.y = m[5]; M.up.z = m[6];
            M.at.x = m[8]; M.at.y = m[9]; M.at.z = m[10];
            M.pos.x = m[12]; M.pos.y = m[13]; M.pos.z = m[14];
            M.flags = 0;
        }
        p->skin = sk;
    }
    else if (id == rwID_USERDATAPLUGIN)
    {
        std::vector<RwUInt8> d(len);
        RwStreamRead(s, d.data(), len);
        size_t o = 0;
        RwInt32 n = (RwInt32)rd32(&d[o]);
        o += 4;
        p->userData = (RpUserDataArray*)rwCalloc(n ? n : 1, sizeof(RpUserDataArray));
        p->numUserData = n;
        for (RwInt32 i = 0; i < n && o < len; i++)
        {
            RpUserDataArray& a = p->userData[i];
            RwInt32 nl = (RwInt32)rd32(&d[o]);
            o += 4;
            a.name = (char*)rwCalloc(nl + 1, 1);
            memcpy(a.name, &d[o], (size_t)nl);
            o += (size_t)nl;
            a.format = (RwInt32)rd32(&d[o]);
            a.numElements = (RwInt32)rd32(&d[o + 4]);
            o += 8;
            if (a.format == rpSTRINGUSERDATA)
            {
                char** strs = (char**)rwCalloc(a.numElements ? a.numElements : 1, sizeof(char*));
                for (RwInt32 k = 0; k < a.numElements; k++)
                {
                    RwInt32 sl = (RwInt32)rd32(&d[o]);
                    o += 4;
                    strs[k] = (char*)rwCalloc(sl + 1, 1);
                    memcpy(strs[k], &d[o], (size_t)sl);
                    o += (size_t)sl;
                }
                a.data = strs;
            }
            else
            {
                a.data = rwMalloc((size_t)a.numElements * 4);
                for (RwInt32 k = 0; k < a.numElements; k++)
                {
                    ((RwUInt32*)a.data)[k] = rd32(&d[o + k * 4]);
                }
                o += (size_t)a.numElements * 4;
            }
        }
    }
}

static void calc_bounding_sphere(RpMorphTarget* mt, RwInt32 nv)
{
    if (nv <= 0 || !mt->verts)
    {
        return;
    }
    RwV3d mn = mt->verts[0], mx = mt->verts[0];
    for (RwInt32 i = 1; i < nv; i++)
    {
        const RwV3d& v = mt->verts[i];
        if (v.x < mn.x) mn.x = v.x; if (v.y < mn.y) mn.y = v.y; if (v.z < mn.z) mn.z = v.z;
        if (v.x > mx.x) mx.x = v.x; if (v.y > mx.y) mx.y = v.y; if (v.z > mx.z) mx.z = v.z;
    }
    RwV3d c = { (mn.x + mx.x) / 2, (mn.y + mx.y) / 2, (mn.z + mx.z) / 2 };
    float r = 0;
    for (RwInt32 i = 0; i < nv; i++)
    {
        float dx = mt->verts[i].x - c.x, dy = mt->verts[i].y - c.y, dz = mt->verts[i].z - c.z;
        float d = dx * dx + dy * dy + dz * dz;
        if (d > r) r = d;
    }
    mt->boundingSphere.center = c;
    mt->boundingSphere.radius = sqrtf(r);
}

static RpGeometry* read_geometry(RwStream* s)
{
    RwUInt32 len, ver;
    if (!RwStreamFindChunk(s, rwID_GEOMETRY, &len, &ver))
    {
        return NULL;
    }
    std::vector<RwUInt8> st;
    if (!read_struct(s, st, &ver))
    {
        return NULL;
    }
    const RwUInt8* d = st.data();
    RwUInt32 format = rd32(d);
    RwInt32 numTris = (RwInt32)rd32(d + 4), numVerts = (RwInt32)rd32(d + 8), numMT = (RwInt32)rd32(d + 12);
    size_t o = 16;
    RpGeometry* g = RpGeometryCreate(numVerts, numTris, format & ~(RwUInt32)rpGEOMETRYNORMALS);
    if (ver < 0x34000)
    {
        g->ignoredSurfaceProps.ambient = rdf(d + o);
        g->ignoredSurfaceProps.specular = rdf(d + o + 4);
        g->ignoredSurfaceProps.diffuse = rdf(d + o + 8);
        o += 12;
    }
    if (!(format & rpGEOMETRYNATIVE))
    {
        if (format & rpGEOMETRYPRELIT)
        {
            memcpy(g->preLitLum, d + o, (size_t)numVerts * 4);
            o += (size_t)numVerts * 4;
        }
        for (int t = 0; t < g->numTexCoordSets; t++)
        {
            for (RwInt32 i = 0; i < numVerts; i++)
            {
                g->texCoords[t][i].u = rdf(d + o + i * 8);
                g->texCoords[t][i].v = rdf(d + o + i * 8 + 4);
            }
            o += (size_t)numVerts * 8;
        }
        for (RwInt32 i = 0; i < numTris; i++)
        {
            RwUInt32 a = rd32(d + o), b = rd32(d + o + 4);
            g->triangles[i].vertIndex[0] = (RwUInt16)(a >> 16);
            g->triangles[i].vertIndex[1] = (RwUInt16)a;
            g->triangles[i].matIndex = (RwInt16)(b >> 16);
            g->triangles[i].vertIndex[2] = (RwUInt16)b;
            o += 8;
        }
    }
    // Morph targets
    if (numMT > 1)
    {
        rwFree(g->morphTarget[0].verts);
        rwFree(g->morphTarget);
        g->morphTarget = (RpMorphTarget*)rwCalloc(numMT, sizeof(RpMorphTarget));
    }
    g->numMorphTargets = numMT;
    for (RwInt32 m = 0; m < numMT; m++)
    {
        RpMorphTarget* mt = &g->morphTarget[m];
        mt->parentGeom = g;
        mt->boundingSphere.center.x = rdf(d + o);
        mt->boundingSphere.center.y = rdf(d + o + 4);
        mt->boundingSphere.center.z = rdf(d + o + 8);
        mt->boundingSphere.radius = rdf(d + o + 12);
        RwUInt32 hasVerts = rd32(d + o + 16), hasNormals = rd32(d + o + 20);
        o += 24;
        if (hasVerts)
        {
            if (!mt->verts)
            {
                mt->verts = (RwV3d*)rwCalloc(numVerts, sizeof(RwV3d));
            }
            for (RwInt32 i = 0; i < numVerts * 3; i++)
            {
                ((float*)mt->verts)[i] = rdf(d + o + i * 4);
            }
            o += (size_t)numVerts * 12;
        }
        if (hasNormals)
        {
            mt->normals = (RwV3d*)rwCalloc(numVerts, sizeof(RwV3d));
            for (RwInt32 i = 0; i < numVerts * 3; i++)
            {
                ((float*)mt->normals)[i] = rdf(d + o + i * 4);
            }
            o += (size_t)numVerts * 12;
            g->flags |= rpGEOMETRYNORMALS;
        }
        if (mt->boundingSphere.radius <= 0)
        {
            calc_bounding_sphere(mt, numVerts);
        }
    }
    pc_read_material_list(s, &g->matList);
    pc_read_extension(s, geometry_plugin, g);
    return g;
}

// ---------------------------------------------------------------- atomics & clumps
static RwObjectHasFrame* atomic_sync(RwObjectHasFrame* o)
{
    RpAtomic* a = (RpAtomic*)o;
    RwFrame* f = (RwFrame*)o->object.parent;
    if (f && a->geometry && a->geometry->morphTarget)
    {
        a->boundingSphere = a->geometry->morphTarget[0].boundingSphere;
        RwV3dTransformPoints(&a->worldBoundingSphere.center, &a->boundingSphere.center, 1, &f->ltm);
        float sx = sqrtf(f->ltm.right.x * f->ltm.right.x + f->ltm.right.y * f->ltm.right.y +
                         f->ltm.right.z * f->ltm.right.z);
        a->worldBoundingSphere.radius = a->boundingSphere.radius * sx;
    }
    return o;
}

static RpAtomic* atomic_default_render(RpAtomic* a)
{
    pc_gl_render_atomic(a);
    return a;
}

extern "C" {

RpAtomic* RpAtomicCreate()
{
    PcAtomic* p = (PcAtomic*)rwCalloc(1, sizeof(PcAtomic));
    RpAtomic* a = &p->a;
    a->object.object.type = rpATOMIC;
    a->object.object.flags = rpATOMICCOLLISIONTEST | rpATOMICRENDER;
    a->object.sync = atomic_sync;
    a->renderCallBack = atomic_default_render;
    a->interpolator.time = 1.0f;
    a->interpolator.recipTime = 1.0f;
    rwLinkListInitialize(&a->llWorldSectorsInAtomic);
    return a;
}

RpAtomic* RpAtomicSetGeometry(RpAtomic* a, RpGeometry* g, RwUInt32 flags)
{
    if (g)
    {
        g->refCount++;
    }
    if (a->geometry)
    {
        RpGeometryDestroy(a->geometry);
    }
    a->geometry = g;
    if (g && g->morphTarget)
    {
        a->boundingSphere = g->morphTarget[0].boundingSphere;
    }
    return a;
}

RpAtomic* RpAtomicSetFrame(RpAtomic* a, RwFrame* f)
{
    _rwObjectHasFrameSetFrame(a, f);
    return a;
}

RwInt32 RpAtomicDestroy(RpAtomic* a)
{
    if (a->clump)
    {
        rwLinkListRemoveLLLink(&a->inClumpLink);
    }
    _rwObjectHasFrameSetFrame(a, NULL);
    if (a->geometry)
    {
        RpGeometryDestroy(a->geometry);
    }
    rwFree(a);
    return 1;
}

RpAtomic* RpAtomicClone(RpAtomic* a)
{
    RpAtomic* c = RpAtomicCreate();
    c->object.object.flags = a->object.object.flags;
    c->renderCallBack = a->renderCallBack;
    RpAtomicSetGeometry(c, a->geometry, 0);
    pc_atomic(c)->matfxEnabled = pc_atomic(a)->matfxEnabled;
    pc_atomic(c)->skinType = pc_atomic(a)->skinType;
    pc_atomic(c)->skinHierarchy = pc_atomic(a)->skinHierarchy;
    return c;
}

RpClump* RpClumpCreate()
{
    RpClump* c = (RpClump*)rwCalloc(1, sizeof(RpClump) + 0x20);
    c->object.type = rpCLUMP;
    rwLinkListInitialize(&c->atomicList);
    return c;
}

void RpClumpAddAtomic(RpClump* c, RpAtomic* a)
{
    rwLinkListAddLLLink(&c->atomicList, &a->inClumpLink);
    a->clump = c;
}

void RpClumpRemoveAtomic(RpClump* c, RpAtomic* a)
{
    rwLinkListRemoveLLLink(&a->inClumpLink);
    a->clump = NULL;
}

RpClump* RpClumpForAllAtomics(RpClump* c, RpAtomic* (*cb)(RpAtomic*, void*), void* data)
{
    RwLLLink* end = &c->atomicList.link;
    for (RwLLLink* l = end->next; l != end;)
    {
        RwLLLink* n = l->next;
        if (!cb(rwLLLinkGetData(l, RpAtomic, inClumpLink), data))
        {
            break;
        }
        l = n;
    }
    return c;
}

RwInt32 RwFrameDestroyHierarchy(RwFrame* f);

RwInt32 RpClumpDestroy(RpClump* c)
{
    RwLLLink* end = &c->atomicList.link;
    while (end->next != end)
    {
        RpAtomic* a = rwLLLinkGetData(end->next, RpAtomic, inClumpLink);
        rwLinkListRemoveLLLink(&a->inClumpLink);
        a->clump = NULL;
        _rwObjectHasFrameSetFrame(a, NULL);
        if (a->geometry)
        {
            RpGeometryDestroy(a->geometry);
        }
        rwFree(a);
    }
    if (c->object.parent)
    {
        RwFrameDestroyHierarchy((RwFrame*)c->object.parent);
    }
    rwFree(c);
    return 1;
}

RwFrame* RwFrameCreate();

}

static void frame_plugin(RwStream* s, RwUInt32 id, RwUInt32 len, RwUInt32 ver, void* obj)
{
    RwFrame* f = (RwFrame*)obj;
    PcFrameExt* x = pc_frame_ext(f);
    if (id == rwID_HANIMPLUGIN)
    {
        std::vector<RwUInt8> d(len);
        RwStreamRead(s, d.data(), len);
        x->hanimID = (RwInt32)rd32(&d[4]);
        RwInt32 numNodes = (RwInt32)rd32(&d[8]);
        if (numNodes > 0)
        {
            RwUInt32 flags = rd32(&d[12]);
            std::vector<RwUInt32> nodeFlags(numNodes);
            std::vector<RwInt32> ids(numNodes);
            for (RwInt32 i = 0; i < numNodes; i++)
            {
                ids[i] = (RwInt32)rd32(&d[20 + i * 12]);
                nodeFlags[i] = rd32(&d[20 + i * 12 + 8]);
            }
            x->hierarchy = RpHAnimHierarchyCreate(numNodes, nodeFlags.data(), ids.data(), (RwInt32)flags, 36);
            x->hierarchy->parentFrame = f;
        }
    }
    else if (id == rwID_FRAMENAME)
    {
        RwUInt32 n = len < sizeof(x->name) - 1 ? len : sizeof(x->name) - 1;
        RwStreamRead(s, x->name, n);
        x->name[n] = 0;
    }
}

// Links HAnim node infos to the frames with matching node ids.
static void hanim_attach_frames(RpHAnimHierarchy* h, std::vector<RwFrame*>& frames)
{
    for (RwInt32 i = 0; i < h->numNodes; i++)
    {
        for (size_t k = 0; k < frames.size(); k++)
        {
            if (pc_frame_ext(frames[k])->hanimID == h->pNodeInfo[i].nodeID)
            {
                h->pNodeInfo[i].pFrame = frames[k];
                break;
            }
        }
    }
}

static bool read_frame_list(RwStream* s, std::vector<RwFrame*>& frames)
{
    RwUInt32 len, ver;
    if (!RwStreamFindChunk(s, rwID_FRAMELIST, &len, &ver))
    {
        return false;
    }
    std::vector<RwUInt8> st;
    if (!read_struct(s, st, &ver))
    {
        return false;
    }
    RwInt32 n = (RwInt32)rd32(st.data());
    frames.resize(n);
    for (RwInt32 i = 0; i < n; i++)
    {
        const RwUInt8* d = &st[4 + i * 56];
        RwFrame* f = RwFrameCreate();
        RwMatrix& m = f->modelling;
        m.right.x = rdf(d); m.right.y = rdf(d + 4); m.right.z = rdf(d + 8);
        m.up.x = rdf(d + 12); m.up.y = rdf(d + 16); m.up.z = rdf(d + 20);
        m.at.x = rdf(d + 24); m.at.y = rdf(d + 28); m.at.z = rdf(d + 32);
        m.pos.x = rdf(d + 36); m.pos.y = rdf(d + 40); m.pos.z = rdf(d + 44);
        m.flags = rd32(d + 52) & 0x3;
        f->ltm = m;
        RwInt32 parent = (RwInt32)rd32(d + 48);
        frames[i] = f;
        if (parent >= 0 && parent < i)
        {
            // Append as last child to keep the file's sibling order.
            RwFrameAddChild(frames[parent], f);
        }
    }
    for (RwInt32 i = 0; i < n; i++)
    {
        pc_read_extension(s, frame_plugin, frames[i]);
    }
    for (RwInt32 i = 0; i < n; i++)
    {
        if (pc_frame_ext(frames[i])->hierarchy)
        {
            hanim_attach_frames(pc_frame_ext(frames[i])->hierarchy, frames);
        }
    }
    return true;
}

static void atomic_plugin(RwStream* s, RwUInt32 id, RwUInt32 len, RwUInt32 ver, void* obj)
{
    PcAtomic* a = (PcAtomic*)obj;
    if (id == rwID_MATERIALEFFECTSPLUGIN && len >= 4)
    {
        RwUInt8 b[4];
        RwStreamRead(s, b, 4);
        a->matfxEnabled = (RwInt32)rd32(b);
    }
}

extern "C" {

RpClump* RpClumpStreamRead(RwStream* s)
{
    std::vector<RwUInt8> st;
    RwUInt32 ver;
    if (!read_struct(s, st, &ver))
    {
        return NULL;
    }
    RwInt32 numAtomics = (RwInt32)rd32(st.data());
    RpClump* clump = RpClumpCreate();
    std::vector<RwFrame*> frames;
    read_frame_list(s, frames);
    if (!frames.empty())
    {
        clump->object.parent = frames[0];
    }
    std::vector<RpGeometry*> geoms;
    RwUInt32 len;
    if (RwStreamFindChunk(s, rwID_GEOMETRYLIST, &len, &ver))
    {
        std::vector<RwUInt8> gl;
        read_struct(s, gl, &ver);
        RwInt32 ng = (RwInt32)rd32(gl.data());
        for (RwInt32 i = 0; i < ng; i++)
        {
            geoms.push_back(read_geometry(s));
        }
    }
    for (RwInt32 i = 0; i < numAtomics; i++)
    {
        if (!RwStreamFindChunk(s, rwID_ATOMIC, &len, &ver))
        {
            break;
        }
        std::vector<RwUInt8> as;
        read_struct(s, as, &ver);
        RwInt32 fi = (RwInt32)rd32(as.data()), gi = (RwInt32)rd32(&as[4]);
        RwUInt32 flags = rd32(&as[8]);
        RpAtomic* a = RpAtomicCreate();
        a->object.object.flags = (RwUInt8)flags;
        if (gi >= 0 && gi < (RwInt32)geoms.size())
        {
            RpAtomicSetGeometry(a, geoms[gi], 0);
        }
        else
        {
            RpGeometry* g = read_geometry(s);
            RpAtomicSetGeometry(a, g, 0);
            if (g)
            {
                RpGeometryDestroy(g);
            }
        }
        if (fi >= 0 && fi < (RwInt32)frames.size())
        {
            RpAtomicSetFrame(a, frames[fi]);
        }
        pc_read_extension(s, atomic_plugin, a);
        RpClumpAddAtomic(clump, a);
    }
    for (size_t i = 0; i < geoms.size(); i++)
    {
        if (geoms[i])
        {
            RpGeometryDestroy(geoms[i]); // atomics hold their own references
        }
    }
    pc_read_extension(s, NULL, NULL);
    return clump;
}

RpGeometry* RpGeometryForAllMaterials(RpGeometry* g, RpMaterial* (*cb)(RpMaterial*, void*), void* data)
{
    for (RwInt32 i = 0; i < g->matList.numMaterials; i++)
    {
        if (!cb(g->matList.materials[i], data))
        {
            break;
        }
    }
    return g;
}

RpGeometry* RpGeometryLock(RpGeometry* g, RwInt32 lockMode)
{
    g->lockedSinceLastInst |= (RwUInt16)lockMode;
    return g;
}

RpGeometry* RpGeometryUnlock(RpGeometry* g) { return g; }

// ---------------------------------------------------------------- user data
RwInt32 RpGeometryGetUserDataArrayCount(const RpGeometry* g) { return pc_geometry((RpGeometry*)g)->numUserData; }

RpUserDataArray* RpGeometryGetUserDataArray(const RpGeometry* g, RwInt32 i)
{
    PcGeometry* p = pc_geometry((RpGeometry*)g);
    return i >= 0 && i < p->numUserData ? &p->userData[i] : NULL;
}

RwInt32 RpGeometryAddUserDataArray(RpGeometry* g, char* name, RwInt32 format, RwInt32 numElements)
{
    PcGeometry* p = pc_geometry(g);
    RpUserDataArray* n = (RpUserDataArray*)rwCalloc(p->numUserData + 1, sizeof(RpUserDataArray));
    if (p->userData)
    {
        memcpy(n, p->userData, sizeof(RpUserDataArray) * p->numUserData);
        rwFree(p->userData);
    }
    p->userData = n;
    RpUserDataArray& a = n[p->numUserData];
    a.name = (char*)rwCalloc(strlen(name) + 1, 1);
    strcpy(a.name, name);
    a.format = format;
    a.numElements = numElements;
    a.data = rwCalloc(numElements ? numElements : 1, 4);
    return p->numUserData++;
}

// ---------------------------------------------------------------- HAnim
RpHAnimHierarchy* RpHAnimHierarchyCreate(RwInt32 numNodes, RwUInt32* nodeFlags, RwInt32* nodeIDs, RwInt32 flags,
                                         RwInt32 maxKeyFrameSize)
{
    RpHAnimHierarchy* h = (RpHAnimHierarchy*)rwCalloc(1, sizeof(RpHAnimHierarchy));
    h->flags = flags;
    h->numNodes = numNodes;
    h->maxKeyFrameSize = maxKeyFrameSize;
    h->currentKeyFrameSize = maxKeyFrameSize;
    h->pMatrixArrayUnaligned = rwCalloc((size_t)numNodes * sizeof(RwMatrix) + 15, 1);
    h->pMatrixArray = (RwMatrix*)(((uintptr_t)h->pMatrixArrayUnaligned + 15) & ~(uintptr_t)15);
    for (RwInt32 i = 0; i < numNodes; i++)
    {
        RwMatrix& m = h->pMatrixArray[i];
        m.right.x = m.up.y = m.at.z = 1.0f;
    }
    h->pNodeInfo = (RpHAnimNodeInfo*)rwCalloc(numNodes ? numNodes : 1, sizeof(RpHAnimNodeInfo));
    for (RwInt32 i = 0; i < numNodes; i++)
    {
        h->pNodeInfo[i].nodeID = nodeIDs ? nodeIDs[i] : i;
        h->pNodeInfo[i].nodeIndex = i;
        h->pNodeInfo[i].flags = nodeFlags ? (RwInt32)nodeFlags[i] : 0;
    }
    h->parentHierarchy = h;
    return h;
}

RpHAnimHierarchy* RpHAnimFrameGetHierarchy(RwFrame* f) { return f ? pc_frame_ext(f)->hierarchy : NULL; }

RwInt32 RpHAnimFrameSetHierarchy(RwFrame* f, RpHAnimHierarchy* h)
{
    pc_frame_ext(f)->hierarchy = h;
    if (h)
    {
        h->parentFrame = f;
    }
    return 1;
}

// ---------------------------------------------------------------- skin
struct RpSkin;
RpSkin* RpSkinGeometryGetSkin(RpGeometry* g) { return g ? (RpSkin*)pc_geometry(g)->skin : NULL; }
RwUInt32 RpSkinGetNumBones(RpSkin* s) { return (RwUInt32)((PcSkin*)s)->numBones; }
const RwMatrix* RpSkinGetSkinToBoneMatrices(RpSkin* s) { return ((PcSkin*)s)->invMats; }
const RwUInt32* RpSkinGetVertexBoneIndices(RpSkin* s) { return ((PcSkin*)s)->indices; }
const float* RpSkinGetVertexBoneWeights(RpSkin* s) { return ((PcSkin*)s)->weights; }

RpAtomic* RpSkinAtomicSetHAnimHierarchy(RpAtomic* a, RpHAnimHierarchy* h)
{
    pc_atomic(a)->skinHierarchy = h;
    return a;
}

RpAtomic* RpSkinAtomicSetType(RpAtomic* a, RwInt32 type)
{
    pc_atomic(a)->skinType = type;
    return a;
}

// ---------------------------------------------------------------- MatFX
RpAtomic* RpMatFXAtomicEnableEffects(RpAtomic* a)
{
    pc_atomic(a)->matfxEnabled = 1;
    return a;
}

RwInt32 RpMatFXMaterialGetEffects(const RpMaterial* m) { return pc_material((RpMaterial*)m)->matfxType; }

RpMaterial* RpMatFXMaterialSetEffects(RpMaterial* m, RwInt32 flags)
{
    pc_material(m)->matfxType = flags;
    return m;
}

RpMaterial* RpMatFXMaterialSetEnvMapCoefficient(RpMaterial* m, float coef)
{
    pc_material(m)->fxCoef = coef;
    return m;
}

RpMaterial* RpMatFXMaterialSetBumpMapCoefficient(RpMaterial* m, float coef)
{
    pc_material(m)->fxCoef = coef;
    return m;
}

RpMaterial* RpMatFXMaterialSetupEnvMap(RpMaterial* m, RwTexture* t, RwFrame* f, RwInt32 useFBAlpha, float coef)
{
    PcMaterial* p = pc_material(m);
    if (t)
    {
        t->refCount++;
    }
    if (p->fxTexture)
    {
        RwTextureDestroy(p->fxTexture);
    }
    p->fxTexture = t;
    p->fxFrame = f;
    p->fxUseFBAlpha = useFBAlpha;
    p->fxCoef = coef;
    return m;
}

RpMaterial* RpMatFXMaterialSetupBumpMap(RpMaterial* m, RwTexture* t, RwFrame* f, float coef)
{
    pc_material(m)->fxCoef = coef;
    return m;
}

// ---------------------------------------------------------------- quaternions
void RtQuatSetupSlerpCache(RtQuat* from, RtQuat* to, RtQuatSlerpCache* c)
{
    c->raFrom = *from;
    c->raTo = *to;
    float d = from->imag.x * to->imag.x + from->imag.y * to->imag.y + from->imag.z * to->imag.z +
              from->real * to->real;
    if (d < 0)
    {
        d = -d;
        c->raTo.imag.x = -to->imag.x;
        c->raTo.imag.y = -to->imag.y;
        c->raTo.imag.z = -to->imag.z;
        c->raTo.real = -to->real;
    }
    if (d > 1.0f)
    {
        d = 1.0f;
    }
    c->omega = acosf(d);
    c->nearlyZeroOm = c->omega < 0.0001f;
}

}
