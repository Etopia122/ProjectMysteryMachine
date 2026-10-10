#ifndef PC_RW_INTERNAL_H
#define PC_RW_INTERNAL_H

// Declarations shared between the PC RenderWare modules.
#include "rwcore.h"

// A raster plus our private data (RGBA8 pixels, GL texture).
struct PcRaster
{
    RwRaster r;
    RwUInt8 ext[0x40]; // room for the platform extension the game may touch
    RwUInt8* rgba;
    void* gpu;    // graphics backend data (texture, render target)
    int dirty;    // pixels changed since the last upload
    int numLevels;
    int isTarget; // a render target (camera texture raster) was made for it
    int hasAlpha; // any texel with alpha < 255 (set on upload)
};
PcRaster* pc_raster(RwRaster* r);


// Private data appended to RenderWare objects (game code only sees the public part).
struct PcSkin
{
    RwInt32 numBones, maxWeights;
    RwMatrix* invMats;   // skin-to-bone matrices
    RwUInt32* indices;   // 4 bone indices per vertex (packed bytes)
    float* weights;      // 4 weights per vertex
};

struct PcGeometry
{
    RpGeometry g;
    RwUInt8 ext[0x20];
    PcSkin* skin;
    RpUserDataArray* userData;
    RwInt32 numUserData;
};

struct PcMaterial
{
    RpMaterial m;
    RwUInt8 ext[0x20];
    RwInt32 matfxType;
    RwTexture* fxTexture;
    RwFrame* fxFrame;
    float fxCoef;
    RwInt32 fxUseFBAlpha;
};

struct PcAtomic
{
    RpAtomic a;
    RwUInt8 ext[0x20];
    RpHAnimHierarchy* skinHierarchy;
    RwInt32 skinType;
    RwInt32 matfxEnabled;
};

// Stored in the spare bytes after each RwFrame (RwFrameCreate over-allocates).
struct PcFrameExt
{
    RpHAnimHierarchy* hierarchy;
    RwInt32 hanimID;
    char name[24];
};
inline PcFrameExt* pc_frame_ext(RwFrame* f) { return (PcFrameExt*)(f + 1); }
inline PcGeometry* pc_geometry(RpGeometry* g) { return (PcGeometry*)g; }
inline PcMaterial* pc_material(RpMaterial* m) { return (PcMaterial*)m; }
inline PcAtomic* pc_atomic(RpAtomic* a) { return (PcAtomic*)a; }

RpMaterial* pc_read_material(RwStream* s);
RwInt32 pc_read_material_list(RwStream* s, RpMaterialList* list);
RpMeshHeader* pc_read_binmesh(RwStream* s, RwUInt32 len, RpMaterialList* mats);
void pc_read_extension(RwStream* s, void (*plugin)(RwStream*, RwUInt32 id, RwUInt32 len, RwUInt32 ver, void* obj), void* obj);

void pc_gx_decode(int fmt, const RwUInt8* src, int w, int h, const RwUInt8* pal, int tlutFmt, RwUInt8* dst);
int pc_gx_level_size(int fmt, int w, int h);

// Renderer (pc_gl.cpp)
void pc_render_raster_free(PcRaster* r);
// A world sector or geometry is being destroyed: drop render caches for it.
void pc_render_forget(const void* key);
// A geometry's arrays change at run time (it was locked): never cache them.
void pc_render_geometry_dynamic(const void* key);
void pc_render_begin_camera(RwCamera* cam);
void pc_render_end_camera(RwCamera* cam);
void pc_render_clear(RwCamera* cam, const RwRGBA* colour, RwInt32 mode);
void pc_render_reset_gx_alpha();
void pc_render_present();
void pc_render_atomic(RpAtomic* atomic);

extern "C" {
RwUInt32 RwStreamRead(RwStream* s, void* buffer, RwUInt32 length);
RwStream* RwStreamSkip(RwStream* s, RwUInt32 offset);
RwInt32 RwStreamFindChunk(RwStream* s, RwUInt32 type, RwUInt32* lengthOut, RwUInt32* versionOut);
RwStream* RwStreamReadChunkHeaderInfo(RwStream* s, RwChunkHeaderInfo* info);
RwTexture* RwTextureRead(const char* name, const char* mask);
RwTexture* RwTexDictionaryFindNamedTexture(RwTexDictionary* d, const char* name);
RwMatrix* RwFrameGetLTM(RwFrame* f);
RwFrame* RwFrameAddChild(RwFrame* parent, RwFrame* child);
void* _rwObjectHasFrameSetFrame(void* object, RwFrame* frame);
RwInt32 _rwFrameSyncDirty();
RwMatrix* RwMatrixInvert(RwMatrix* dst, const RwMatrix* src);
RwMatrix* RwMatrixMultiply(RwMatrix* out, const RwMatrix* a, const RwMatrix* b);
RwV3d* RwV3dTransformPoints(RwV3d* out, const RwV3d* in, RwInt32 n, const RwMatrix* m);
}

// Little-endian readers for RW chunk payloads.
inline RwUInt32 rd32(const RwUInt8* p) { return (RwUInt32)(p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24); }
inline RwUInt16 rd16(const RwUInt8* p) { return (RwUInt16)(p[0] | p[1] << 8); }
inline float rdf(const RwUInt8* p) { RwUInt32 v = rd32(p); float f; __builtin_memcpy(&f, &v, 4); return f; }

#endif // PC_RW_INTERNAL_H
