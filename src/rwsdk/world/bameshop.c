/*
 * Mesh optimisation: build meshes are turned into mesh headers either as
 * per-material triangle lists (sorted for vertex cache reuse) or as triangle
 * strips grown greedily from the least connected triangles and then joined
 * with degenerate triangles.
 */

#include "rwsdk/rwcore.h"

#define rwPLUGIN_ID 2 /* world */

#define rpMESHHEADERTRISTRIP 0x0001

typedef RwUInt16 RxVertexIndex;

typedef struct RpMaterial {
    /* 0x00 */ RwTexture* texture;
    /* 0x04 */ RwRGBA color;
    /* 0x08 */ void* pipeline;
} RpMaterial;

typedef struct RpMeshHeader {
    RwUInt32 flags;
    RwUInt16 numMeshes;
    RwUInt16 serialNum;
    RwUInt32 totalIndicesInMesh;
    RwUInt32 firstMeshOffset;
} RpMeshHeader;

typedef struct RpMesh {
    RxVertexIndex* indices;
    RwUInt32 numIndices;
    RpMaterial* material;
} RpMesh;

typedef struct RpBuildMeshTriangle {
    RwUInt16 vertIndex[3];
    RpMaterial* material;
} RpBuildMeshTriangle;

typedef struct RpBuildMesh {
    RwUInt32 triangleBufferSize;
    RwUInt32 numTriangles;
    RpBuildMeshTriangle* meshTriangles;
} RpBuildMesh;

typedef RpMesh* (*RpMeshCallBack)(RpMesh* mesh, RpMeshHeader* meshHeader, void* pData);
typedef RpMeshHeader* (*RpTriStripMeshCallBack)(RpBuildMesh* buildMesh, void* data);

typedef struct rpMeshGlobals {
    RwInt16 nextSerialNum;
    RwInt16 pad;
    RwFreeList* stripFreeList;
} rpMeshGlobals;

typedef struct RpTriStripEdge RpTriStripEdge;
typedef struct RpTriStripTriangle RpTriStripTriangle;

struct RpTriStripTriangle {
    /* 0x00 */ RwUInt32 index;
    /* 0x04 */ RpTriStripEdge* edges[3];
    /* 0x10 */ RpTriStripTriangle* next;
    /* 0x14 */ RpTriStripTriangle* prev;
    /* 0x18 */ RwBool used;
    /* 0x1C */ RwBool testUsed;
    /* 0x20 */ RwUInt8 numNeighbours;
};

struct RpTriStripEdge {
    /* 0x00 */ RwUInt16 vert[2];
    /* 0x04 */ RpTriStripTriangle* tris[2];
    /* 0x0C */ RpTriStripEdge* next;
};

typedef struct RpTriStrip RpTriStrip;

struct RpTriStrip {
    /* 0x00 */ RxVertexIndex* indices;
    /* 0x04 */ RwUInt32 numIndices;
    /* 0x08 */ RwUInt32 maxIndices;
    /* 0x0C */ RpTriStrip* next;
};

typedef struct RpTriStripFreeLists {
    RwFreeList* triangles;
    RwFreeList* edges;
} RpTriStripFreeLists;

typedef struct rpTriStripGlobals {
    /* 0x00 */ RpMeshHeader emptyMeshHeader;
    /* 0x10 */ RpTriStripMeshCallBack callback;
    /* 0x14 */ void* data;
} rpTriStripGlobals;

#define rpTRISTRIPTESTPASSES 4

extern RwModuleInfo meshModule;

#define RWMESHGLOBAL(var) (RWPLUGINOFFSET(rpMeshGlobals, RwEngineInstance, meshModule.globalsOffset)->var)

#define RwFreeListFree(_f, _p) ((RWSRCGLOBAL(memoryFree))((_f), (_p)))

void* memcpy(void* dst, const void* src, unsigned long n);
void qsort(void* base, unsigned long num, unsigned long size, int (*compare)(const void*, const void*));
RwFreeList* RwFreeListCreate(RwInt32 entrySize, RwInt32 entriesPerBlock, RwInt32 alignment);
RwBool RwFreeListDestroy(RwFreeList* freelist);
RpMeshHeader* _rpMeshHeaderCreate(RwUInt32 size);
RpMeshHeader* _rpMeshHeaderForAllMeshes(RpMeshHeader* meshHeader, RpMeshCallBack fpCallBack, void* pData);
RwBool _rpBuildMeshDestroy(RpBuildMesh* mesh);

RpMeshHeader* TriStripMeshGenerate(RpBuildMesh* buildMesh, RwInt32 testPasses, RwBool joinOdd);
RpMeshHeader* RpBuildMeshGenerateDefaultTriStrip(RpBuildMesh* buildMesh, void* data);

static const RwUInt32 SortAlphaKey[1] = { 0x10 };

static rpTriStripGlobals TriStripGlobals = { { 0, 0, 0, 0, 0 }, RpBuildMeshGenerateDefaultTriStrip, NULL };

/* A triangle is free when it exists and is not used (in the test or the real pass). */
#define TRIFREE(tri, field) ((tri) != NULL && !(tri)->field)
#define EDGEFREECOUNT(edge, field) (TRIFREE((edge)->tris[0], field) + TRIFREE((edge)->tris[1], field))

#define TRIBINREMOVE(bins, tri)                                     \
    do {                                                            \
        if ((bins)[(tri)->numNeighbours] == (tri)) {                \
            (bins)[(tri)->numNeighbours] = (bins)[(tri)->numNeighbours]->next; \
            if ((bins)[(tri)->numNeighbours]) {                     \
                (bins)[(tri)->numNeighbours]->prev = NULL;          \
            }                                                       \
        } else {                                                    \
            if ((tri)->next) {                                      \
                (tri)->next->prev = (tri)->prev;                    \
            }                                                       \
            if ((tri)->prev) {                                      \
                (tri)->prev->next = (tri)->next;                    \
            }                                                       \
        }                                                           \
    } while (0)

#define TRIBINADD(bins, tri)                                        \
    do {                                                            \
        (tri)->next = (bins)[(tri)->numNeighbours];                 \
        if ((tri)->next) {                                          \
            (tri)->next->prev = (tri);                              \
        }                                                           \
        (bins)[(tri)->numNeighbours] = (tri);                       \
        (tri)->prev = NULL;                                         \
    } while (0)

int SortPolygons(const void* pA, const void* pB)
{
    RpMaterial* matA = (*(const RpBuildMeshTriangle* const*)pA)->material;
    RpMaterial* matB = (*(const RpBuildMeshTriangle* const*)pB)->material;
    RwRaster* rasterA = NULL;
    RwRaster* rasterB = NULL;
    RwTexture* textureA = NULL;
    RwTexture* textureB = NULL;
    void* pipelineA = NULL;
    void* pipelineB = NULL;
    RwUInt32 keyA = 0;
    RwUInt32 keyB = 0;
    RwInt32 format;

    if (matA == matB) {
        return 0;
    }

    if (matA) {
        if (matA->texture) {
            rasterA = matA->texture->raster;
            textureA = matA->texture;
            format = RwRasterGetFormat(rasterA) & 0xF00;
            if (format == 0x100 || format == 0x300 || format == 0x500) {
                keyA = SortAlphaKey[0];
            }
        }
        if (matA->color.alpha != 0xFF) {
            keyA |= SortAlphaKey[0];
        }
        pipelineA = matA->pipeline;
    }

    if (matB) {
        if (matB->texture) {
            rasterB = matB->texture->raster;
            textureB = matB->texture;
            format = RwRasterGetFormat(rasterB) & 0xF00;
            if (format == 0x100 || format == 0x300 || format == 0x500) {
                keyB = SortAlphaKey[0];
            }
        }
        if (matB->color.alpha != 0xFF) {
            keyB |= SortAlphaKey[0];
        }
        pipelineB = matB->pipeline;
    }

    keyA |= rasterA > rasterB ? 8 : 0;
    keyB |= rasterB > rasterA ? 8 : 0;
    keyA |= pipelineA > pipelineB ? 4 : 0;
    keyB |= pipelineB > pipelineA ? 4 : 0;
    keyA |= textureA > textureB ? 2 : 0;
    keyB |= textureB > textureA ? 2 : 0;
    keyA |= matA > matB ? 1 : 0;
    keyB |= matB > matA ? 1 : 0;

    return keyA - keyB;
}

RpMesh* SortPolygonsInTriListMesh(RpMesh* mesh, RpMeshHeader* meshHeader, void* data)
{
    RwUInt32 usedSize;
    RwUInt32* usedVerts;
    RwUInt32 numTris;
    RxVertexIndex* indices;
    RwUInt32 maxIndex;
    RxVertexIndex* srcIndices;
    RwUInt32 i;

    indices = mesh->indices;
    numTris = mesh->numIndices / 3;
    maxIndex = 0;

    for (i = 0; i < mesh->numIndices; i++) {
        maxIndex |= indices[i];
    }

    usedSize = (maxIndex + 31) >> 3;
    usedVerts = (RwUInt32*)RwMalloc(usedSize);
    if (usedVerts) {
        RwUInt32 indexSize = mesh->numIndices * sizeof(RxVertexIndex);

        srcIndices = (RxVertexIndex*)RwMalloc(indexSize);
        if (srcIndices) {
            RxVertexIndex* tri;
            RwUInt32 numSorted;

            memcpy(srcIndices, indices, indexSize);

            numSorted = 0;
            while (numSorted < numTris) {
                RwUInt8* clear = (RwUInt8*)usedVerts;
                RwUInt32 j = usedSize;

                while (j--) {
                    *clear++ = 0;
                }

                tri = srcIndices;
                for (j = 0; j < numTris; j++, tri += 3) {
                    if ((tri[0] & tri[1] & tri[2]) != 0xFFFF && !(usedVerts[tri[0] >> 5] & (1 << (tri[0] & 31))) &&
                        !(usedVerts[tri[1] >> 5] & (1 << (tri[1] & 31))) && !(usedVerts[tri[2] >> 5] & (1 << (tri[2] & 31)))) {
                        usedVerts[tri[0] >> 5] |= 1 << (tri[0] & 31);
                        numSorted++;
                        usedVerts[tri[1] >> 5] |= 1 << (tri[1] & 31);
                        usedVerts[tri[2] >> 5] |= 1 << (tri[2] & 31);

                        indices[0] = tri[0];
                        indices[1] = tri[1];
                        indices[2] = tri[2];
                        indices += 3;

                        tri[0] = tri[1] = tri[2] = 0xFFFF;
                    }
                }
            }

            RwFree(srcIndices);
        }
        RwFree(usedVerts);
        return mesh;
    }

    return mesh;
}

static inline RpTriStripEdge* TriStripEdgeAdd(RwFreeList* edgeFreeList, RpTriStripEdge** edgeList,
                                             RpTriStripTriangle** entry, RwUInt16 vA, RwUInt16 vB)
{
    RpTriStripEdge* edge = *edgeList;

    while (edge) {
        if (vA == edge->vert[1] && vB == edge->vert[0] && edge->tris[1] == NULL) {
            edge->tris[0]->numNeighbours++;
            (*entry)->numNeighbours++;
            edge->tris[1] = *entry;
            return edge;
        }
        edge = edge->next;
    }

    edge = (RpTriStripEdge*)RwFreeListAlloc(edgeFreeList);
    edge->vert[0] = vA;
    edge->vert[1] = vB;
    edge->tris[0] = *entry;
    edge->tris[1] = NULL;
    edge->next = *edgeList;
    *edgeList = edge;
    return edge;
}

RpTriStripTriangle** TriStripBinEntryArrayCreate(RwUInt32 numTris, RpTriStripFreeLists* freeLists,
                                                 RpTriStripEdge** edgeList, RpBuildMeshTriangle* tris)
{
    RpTriStripTriangle** triArray;
    RpTriStripTriangle** entry;
    RpBuildMeshTriangle* tri;
    RwUInt32 i;

    triArray = (RpTriStripTriangle**)RwMalloc(numTris * sizeof(RpTriStripTriangle*));
    freeLists->triangles = RwFreeListCreate(sizeof(RpTriStripTriangle), numTris, 0);
    freeLists->edges = RwFreeListCreate(sizeof(RpTriStripEdge), numTris / 2 + 1, 0);

    entry = triArray;
    tri = tris;
    for (i = 0; i < numTris; i++) {
        *entry = (RpTriStripTriangle*)RwFreeListAlloc(freeLists->triangles);
        (*entry)->numNeighbours = 0;
        (*entry)->index = i;
        (*entry)->prev = NULL;
        (*entry)->next = NULL;
        (*entry)->used = FALSE;

        (*entry)->edges[0] = TriStripEdgeAdd(freeLists->edges, edgeList, entry, tri->vertIndex[0], tri->vertIndex[1]);
        (*entry)->edges[1] = TriStripEdgeAdd(freeLists->edges, edgeList, entry, tri->vertIndex[1], tri->vertIndex[2]);
        (*entry)->edges[2] = TriStripEdgeAdd(freeLists->edges, edgeList, entry, tri->vertIndex[2], tri->vertIndex[0]);

        entry++;
        tri++;
    }

    return triArray;
}

void TriStripMarkTriUsed(RpTriStripTriangle* tri, RpTriStripTriangle** bins, RwInt32 pass)
{
    RwUInt8 i;

    if (pass < rpTRISTRIPTESTPASSES) {
        tri->testUsed = TRUE;
        return;
    }

    tri->used = TRUE;
    TRIBINREMOVE(bins, tri);

    for (i = 0; i < 3; i++) {
        RpTriStripEdge* edge = tri->edges[i];
        RpTriStripTriangle* neighbour = NULL;

        if (edge->tris[0] && edge->tris[0] != tri && !edge->tris[0]->used) {
            neighbour = edge->tris[0];
        } else if (edge->tris[1] && !edge->tris[1]->used) {
            neighbour = edge->tris[1];
        }

        if (neighbour) {
            TRIBINREMOVE(bins, neighbour);
            neighbour->numNeighbours--;
            TRIBINADD(bins, neighbour);
        }
    }
}

static inline RpTriStripEdge* TriStripTriFindEdge(RpTriStripTriangle* tri, RwUInt16 a, RwUInt16 b)
{
    RpTriStripEdge* edge;

    edge = tri->edges[0];
    if ((edge->vert[0] == a && edge->vert[1] == b) || (edge->vert[0] == b && edge->vert[1] == a)) {
        return edge;
    }
    edge = tri->edges[1];
    if ((edge->vert[0] == a && edge->vert[1] == b) || (edge->vert[0] == b && edge->vert[1] == a)) {
        return edge;
    }
    edge = tri->edges[2];
    if ((edge->vert[0] == a && edge->vert[1] == b) || (edge->vert[0] == b && edge->vert[1] == a)) {
        return edge;
    }
    return NULL;
}

static inline RwBool TriStripEdgeNoTestFree(RpTriStripEdge* edge)
{
    return edge == NULL || EDGEFREECOUNT(edge, testUsed) == 0;
}

static inline RwBool TriStripEdgeNoFree(RpTriStripEdge* edge)
{
    return edge == NULL || EDGEFREECOUNT(edge, used) == 0;
}

static inline RwBool TriStripEdgeTestFree(RpTriStripEdge* edge)
{
    return edge != NULL && EDGEFREECOUNT(edge, testUsed) != 0;
}

static inline RwBool TriStripEdgeFree(RpTriStripEdge* edge)
{
    return edge != NULL && EDGEFREECOUNT(edge, used) != 0;
}

static inline RwBool TriStripEdgeIsTestEnd(RpTriStripEdge* edge)
{
    RwBool result = TRUE;

    if (!TriStripEdgeNoTestFree(edge)) {
        result = FALSE;
    }
    return result;
}

static inline RwBool TriStripEdgeIsRealEnd(RpTriStripEdge* edge)
{
    RwBool result = TRUE;

    if (!TriStripEdgeNoFree(edge)) {
        result = FALSE;
    }
    return result;
}

static inline RwBool TriStripEdgeIsEnd(RpTriStripEdge* edge, RwInt32 pass)
{
    if (pass < rpTRISTRIPTESTPASSES) {
        return TriStripEdgeIsTestEnd(edge);
    }
    return TriStripEdgeIsRealEnd(edge);
}

static inline RwBool TriStripEdgeIsOpen(RpTriStripEdge* edge, RwInt32 pass)
{
    RwBool result;

    if (pass < rpTRISTRIPTESTPASSES) {
        result = FALSE;
        if (TriStripEdgeTestFree(edge)) {
            result = TRUE;
        }
    } else {
        result = FALSE;
        if (TriStripEdgeFree(edge)) {
            result = TRUE;
        }
    }
    return result;
}

RwUInt32 TriStripFollow(RpTriStrip* strip, RpTriStripEdge* edge, RpTriStripTriangle** bins, RpBuildMeshTriangle* tris,
                        RwInt32 pass)
{
    RwUInt16 vB;
    RpTriStripEdge* lastEdge;
    RwUInt16 newVert;
    RpTriStripEdge* nextEdge;
    RpTriStripTriangle* tri;
    RwUInt16* vi;
    RwUInt16 vA;
    RwBool isEnd;
    RwUInt32 numAdded;
    RpTriStripEdge* otherEdge;

    numAdded = 0;
    otherEdge = NULL;
    while (edge) {
        tri = NULL;
        newVert = 0;

        if (strip->numIndices & 1) {
            vA = strip->indices[strip->numIndices - 1];
            vB = strip->indices[strip->numIndices - 2];
        } else {
            vA = strip->indices[strip->numIndices - 2];
            vB = strip->indices[strip->numIndices - 1];
        }

        if (pass < rpTRISTRIPTESTPASSES) {
            if (TRIFREE(edge->tris[0], testUsed)) {
                tri = edge->tris[0];
            } else if (TRIFREE(edge->tris[1], testUsed)) {
                tri = edge->tris[1];
            }
        } else {
            if (TRIFREE(edge->tris[0], used)) {
                tri = edge->tris[0];
            } else if (TRIFREE(edge->tris[1], used)) {
                tri = edge->tris[1];
            }
        }

        if (tri == NULL) {
            return numAdded;
        }

        numAdded++;
        TriStripMarkTriUsed(tri, bins, pass);

        vi = tris[tri->index].vertIndex;
        if (vi[0] != vA && vi[0] != vB) {
            newVert = vi[0];
        }
        if (vi[1] != vA && vi[1] != vB) {
            newVert = vi[1];
        }
        if (vi[2] != vA && vi[2] != vB) {
            newVert = vi[2];
        }

        lastEdge = edge;
        nextEdge = TriStripTriFindEdge(tri, strip->indices[strip->numIndices - 1], newVert);
        edge = nextEdge;

        if (pass < rpTRISTRIPTESTPASSES) {
            isEnd = TriStripEdgeIsTestEnd(nextEdge);
        } else {
            isEnd = TriStripEdgeIsRealEnd(nextEdge);
        }
        if (isEnd) {
            if (tri->edges[0] != lastEdge && tri->edges[0] != nextEdge) {
                otherEdge = tri->edges[0];
            } else if (tri->edges[1] != lastEdge && tri->edges[1] != nextEdge) {
                otherEdge = tri->edges[1];
            } else if (tri->edges[2] != lastEdge && tri->edges[2] != nextEdge) {
                otherEdge = tri->edges[2];
            }

            if (TriStripEdgeIsOpen(otherEdge, pass)) {
                if (strip->numIndices & 1) {
                    strip->indices[strip->numIndices] = newVert;
                    edge = NULL;
                    strip->numIndices++;
                } else {
                    strip->indices[strip->numIndices] = strip->indices[strip->numIndices - 2];
                    edge = otherEdge;
                    strip->numIndices++;
                    strip->indices[strip->numIndices] = newVert;
                    strip->numIndices++;
                }
            } else {
                strip->indices[strip->numIndices] = newVert;
                edge = NULL;
                strip->numIndices++;
            }
        } else {
            strip->indices[strip->numIndices] = newVert;
            strip->numIndices++;
        }
    }

    return numAdded;
}

static inline RwBool TriStripEdgeHasFree(RpTriStripEdge* edge, RwInt32 pass)
{
    RwInt32 count;

    if (pass < rpTRISTRIPTESTPASSES) {
        count = EDGEFREECOUNT(edge, testUsed);
    } else {
        count = EDGEFREECOUNT(edge, used);
    }
    return count > 0;
}

RwBool TriStripStripTris(RpBuildMeshTriangle* tris, RwUInt32 numTris, RpTriStrip** stripList, RwInt32 testPasses)
{
    RpTriStripEdge* edgeList = NULL;
    RpTriStripFreeLists freeLists;
    RpTriStripTriangle* bins[4];
    RpTriStripTriangle** triArray;
    RpTriStrip* forward;
    RpTriStrip* backward;
    RpTriStrip* strip;
    RpTriStripEdge* forwardEdge = NULL;
    RpTriStripEdge* backwardEdge = NULL;
    RwUInt32 numDone = 0;
    RwUInt32 i;

    bins[0] = NULL;
    bins[1] = NULL;
    bins[2] = NULL;
    bins[3] = NULL;
    freeLists.triangles = NULL;
    freeLists.edges = NULL;

    triArray = TriStripBinEntryArrayCreate(numTris, &freeLists, &edgeList, tris);

    for (i = 0; i < numTris; i++) {
        TRIBINADD(bins, triArray[i]);
    }

    forward = (RpTriStrip*)RwFreeListAlloc(RWMESHGLOBAL(stripFreeList));
    forward->maxIndices = numTris * 2 + 2;
    forward->numIndices = 0;
    forward->indices = (RxVertexIndex*)RwMalloc(forward->maxIndices * sizeof(RwUInt32));

    backward = (RpTriStrip*)RwFreeListAlloc(RWMESHGLOBAL(stripFreeList));
    backward->maxIndices = numTris * 2 + 2;
    backward->numIndices = 0;
    backward->indices = (RxVertexIndex*)RwMalloc(backward->maxIndices * sizeof(RwUInt32));

    while (numDone < numTris) {
        RwUInt32 best = 0;
        RwInt32 pass = testPasses ? 0 : 3;
        RwUInt32 bin;
        RwInt32 start;
        RwInt32 bestStart;
        RwInt32 rot0;
        RwInt32 rot1;
        RwInt32 rot2;
        RwUInt32 numDoneStart;

        if (bins[0]) {
            RwUInt32 index = bins[0]->index;

            strip = (RpTriStrip*)RwFreeListAlloc(RWMESHGLOBAL(stripFreeList));
            strip->next = *stripList;
            *stripList = strip;
            strip->maxIndices = 3;
            strip->numIndices = 3;
            strip->indices = (RxVertexIndex*)RwMalloc(3 * sizeof(RwUInt32));
            strip->indices[0] = tris[index].vertIndex[0];
            strip->indices[1] = tris[index].vertIndex[1];
            strip->indices[2] = tris[index].vertIndex[2];

            bins[0]->used = TRUE;
            bins[0]->testUsed = TRUE;
            bins[0] = bins[0]->next;
            if (bins[0]) {
                bins[0]->prev = NULL;
            }
            numDone++;
            continue;
        }

        for (bin = 1; bins[bin] == NULL; bin++) {
        }

        {
            RpTriStripTriangle* tri = bins[bin];
            RpTriStripEdge* e0 = tri->edges[0];
            RpTriStripEdge* e1 = tri->edges[1];
            RpTriStripEdge* e2 = tri->edges[2];

            if (EDGEFREECOUNT(e2, used) >= 2 && EDGEFREECOUNT(e1, used) >= 2) {
                start = 1;
            } else if (EDGEFREECOUNT(e0, used) >= 2 && EDGEFREECOUNT(e2, used) >= 2) {
                start = 2;
            } else if (EDGEFREECOUNT(e1, used) >= 2 && EDGEFREECOUNT(e0, used) >= 2) {
                start = 0;
            } else if (EDGEFREECOUNT(e0, used) > EDGEFREECOUNT(e1, used)) {
                if (EDGEFREECOUNT(e0, used) > EDGEFREECOUNT(e2, used)) {
                    start = 2;
                } else {
                    start = 1;
                }
            } else {
                if (EDGEFREECOUNT(e1, used) > EDGEFREECOUNT(e2, used)) {
                    start = 0;
                } else {
                    start = 1;
                }
            }
        }

        numDoneStart = numDone;
        bestStart = start;
        rot0 = start % 3;
        rot1 = (start + 1) % 3;
        rot2 = (start + 2) % 3;

        do {
            RwUInt32 count;
            RpTriStripTriangle* tri;

            for (i = 0; i < numTris; i++) {
                triArray[i]->testUsed = triArray[i]->used;
            }

            switch (pass++) {
            case 0:
                start = rot0;
                break;
            case 1:
                start = rot1;
                break;
            case 2:
                start = rot2;
                break;
            default:
                start = bestStart;
                break;
            }

            switch (start) {
            case 0:
                forwardEdge = bins[bin]->edges[1];
                backwardEdge = bins[bin]->edges[0];
                break;
            case 1:
                forwardEdge = bins[bin]->edges[2];
                backwardEdge = bins[bin]->edges[1];
                break;
            case 2:
                forwardEdge = bins[bin]->edges[0];
                backwardEdge = bins[bin]->edges[2];
                break;
            }

            tri = bins[bin];
            forward->indices[0] = tris[tri->index].vertIndex[start % 3];
            forward->indices[1] = tris[tri->index].vertIndex[(start + 1) % 3];
            forward->indices[2] = tris[tri->index].vertIndex[(start + 2) % 3];
            forward->numIndices = 3;
            TriStripMarkTriUsed(bins[bin], bins, pass);

            count = numDoneStart + 1;
            count += TriStripFollow(forward, forwardEdge, bins, tris, pass);
            numDone = count;

            if (TriStripEdgeHasFree(backwardEdge, pass)) {
                backward->indices[0] = forward->indices[1];
                backward->indices[1] = forward->indices[0];
                backward->numIndices = 2;
                numDone += TriStripFollow(backward, backwardEdge, bins, tris, pass);

                if (backward->numIndices & 1) {
                    backward->indices[backward->numIndices] = backward->indices[backward->numIndices - 2];
                    backward->numIndices++;
                }

                if (numDone > best) {
                    best = numDone;
                    bestStart = start;
                }

                if (pass >= rpTRISTRIPTESTPASSES) {
                    strip = (RpTriStrip*)RwFreeListAlloc(RWMESHGLOBAL(stripFreeList));
                    strip->next = *stripList;
                    *stripList = strip;
                    strip->maxIndices = forward->numIndices + (backward->numIndices - 2);
                    strip->numIndices = 0;
                    strip->indices = (RxVertexIndex*)RwMalloc(strip->maxIndices * sizeof(RwUInt32));

                    while (backward->numIndices > 2) {
                        strip->indices[strip->numIndices] = backward->indices[backward->numIndices - 1];
                        strip->numIndices++;
                        backward->numIndices--;
                    }

                    memcpy(strip->indices + strip->numIndices, forward->indices,
                           forward->numIndices * sizeof(RwUInt32));
                    strip->numIndices = strip->maxIndices;
                    break;
                }
            } else {
                if (numDone > best) {
                    best = numDone;
                    bestStart = start;
                }

                if (pass >= rpTRISTRIPTESTPASSES) {
                    strip = (RpTriStrip*)RwFreeListAlloc(RWMESHGLOBAL(stripFreeList));
                    strip->next = *stripList;
                    *stripList = strip;
                    strip->maxIndices = forward->numIndices;
                    strip->numIndices = forward->numIndices;
                    strip->indices = (RxVertexIndex*)RwMalloc(forward->numIndices * sizeof(RwUInt32));
                    memcpy(strip->indices, forward->indices, forward->numIndices * sizeof(RwUInt32));
                    break;
                }
            }
        } while (pass < rpTRISTRIPTESTPASSES + 1);
    }

    RwFree(backward->indices);
    backward->indices = NULL;
    RwFreeListFree(RWMESHGLOBAL(stripFreeList), backward);

    RwFree(forward->indices);
    forward->indices = NULL;
    RwFreeListFree(RWMESHGLOBAL(stripFreeList), forward);

    {
        RpTriStripEdge* edge = edgeList;

        while (edge) {
            RpTriStripEdge* next = edge->next;

            RwFreeListFree(freeLists.edges, edge);
            edge = next;
        }
    }
    RwFreeListDestroy(freeLists.edges);
    freeLists.edges = NULL;

    for (i = 0; i < numTris; i++) {
        RwFreeListFree(freeLists.triangles, triArray[i]);
        triArray[i] = NULL;
    }
    RwFreeListDestroy(freeLists.triangles);
    freeLists.triangles = NULL;

    RwFree(triArray);
    return TRUE;
}

static inline RpTriStrip* TriStripFree(RpTriStrip* strip)
{
    RpTriStrip* next = strip->next;

    RwFreeListFree(RWMESHGLOBAL(stripFreeList), strip);
    return next;
}

RwBool TriStripJoin(RpTriStrip** stripList, RwBool joinOdd)
{
    RpTriStrip* joined;
    RpTriStrip* strip;
    RpTriStrip* remaining;
    RwUInt32 i;

    if (*stripList == NULL) {
        return FALSE;
    }

    joined = (RpTriStrip*)RwFreeListAlloc(RWMESHGLOBAL(stripFreeList));
    joined->numIndices = 0;
    joined->maxIndices = 0;
    for (strip = *stripList; strip; strip = strip->next) {
        joined->maxIndices += strip->numIndices + 6;
    }
    joined->indices = (RxVertexIndex*)RwMalloc(joined->maxIndices * sizeof(RwUInt32));

    strip = *stripList;
    for (i = 0; i < strip->numIndices; i++) {
        joined->indices[joined->numIndices] = strip->indices[i];
        joined->numIndices++;
    }
    RwFree(strip->indices);
    strip->indices = NULL;
    remaining = TriStripFree(strip);

    while (remaining) {
        RxVertexIndex* end;

        for (strip = remaining; strip; strip = strip->next) {
            end = &joined->indices[joined->numIndices];
            if (strip->indices[0] == end[-1]) {
                if ((joined->numIndices & 1) && joinOdd) {
                    end[0] = strip->indices[0];
                    joined->numIndices++;
                }
                break;
            }
        }

        if (strip == NULL) {
            for (strip = remaining; strip; strip = strip->next) {
                end = &joined->indices[joined->numIndices];
                if (strip->indices[0] == end[-2]) {
                    if (!(joined->numIndices & 1) && joinOdd) {
                        end[0] = strip->indices[0];
                        joined->indices[joined->numIndices + 1] = strip->indices[0];
                        joined->numIndices += 2;
                    } else {
                        end[0] = strip->indices[0];
                        joined->numIndices++;
                    }
                    break;
                }
            }
        }

        if (strip == NULL) {
            strip = remaining;
            if (!(joined->numIndices & 1) || !joinOdd) {
                end = &joined->indices[joined->numIndices];
                end[0] = end[-1];
                joined->indices[joined->numIndices + 1] = remaining->indices[0];
                joined->numIndices += 2;
            } else {
                end = &joined->indices[joined->numIndices];
                end[0] = end[-1];
                joined->indices[joined->numIndices + 1] = remaining->indices[0];
                joined->indices[joined->numIndices + 2] = remaining->indices[0];
                joined->numIndices += 3;
            }
        }

        for (i = 0; i < strip->numIndices; i++) {
            joined->indices[joined->numIndices] = strip->indices[i];
            joined->numIndices++;
        }
        RwFree(strip->indices);
        strip->indices = NULL;

        if (remaining == strip) {
            remaining = remaining->next;
            RwFreeListFree(RWMESHGLOBAL(stripFreeList), strip);
        } else {
            RpTriStrip* prev = remaining;

            while (prev->next != strip) {
                prev = prev->next;
            }
            prev->next = strip->next;
            RwFreeListFree(RWMESHGLOBAL(stripFreeList), strip);
        }
    }

    *stripList = joined;
    joined->next = NULL;
    return TRUE;
}

typedef struct TriStripMeshInfo {
    RwUInt32 first;
    RwUInt32 numTris;
    RpMaterial* material;
} TriStripMeshInfo;

RpMeshHeader* TriStripMeshGenerate(RpBuildMesh* buildMesh, RwInt32 testPasses, RwBool joinOdd)
{
    RwUInt32 numInfos;
    TriStripMeshInfo* infos;
    RpMeshHeader* meshHeader;
    RpBuildMeshTriangle** triPointers;
    RwUInt32 i;
    RwUInt16 numMeshes = 0;
    RpMesh** meshes;

    triPointers = (RpBuildMeshTriangle**)RwMalloc(buildMesh->numTriangles * sizeof(RpBuildMeshTriangle*));
    if (triPointers == NULL) {
        return NULL;
    }

    for (i = 0; i < buildMesh->numTriangles; i++) {
        triPointers[i] = &buildMesh->meshTriangles[i];
    }

    qsort(triPointers, buildMesh->numTriangles, sizeof(RpBuildMeshTriangle*), SortPolygons);

    numInfos = 1;
    if (buildMesh->numTriangles >= 2) {
        RpMaterial* material = triPointers[0]->material;

        for (i = 1; i < buildMesh->numTriangles; i++) {
            if (triPointers[i]->material != material) {
                material = triPointers[i]->material;
                numInfos++;
            }
        }
    }

    meshes = (RpMesh**)RwMalloc(numInfos * sizeof(RpMesh*));
    infos = (TriStripMeshInfo*)RwMalloc(numInfos * sizeof(TriStripMeshInfo));

    infos[0].material = triPointers[0]->material;
    infos[0].numTris = 0;
    infos[0].first = 0;

    {
        RwUInt32 numFound = 1;

        if (buildMesh->numTriangles >= 2) {
            TriStripMeshInfo* info = &infos[1];

            for (i = 0; i < buildMesh->numTriangles - 1; i++) {
                if (triPointers[i]->material != triPointers[i + 1]->material) {
                    info->material = triPointers[i + 1]->material;
                    numFound++;
                    info->numTris = i + 1;
                    info->first = 0;
                    info[-1].numTris = i + 1 - info[-1].numTris;
                    info++;
                }
            }
        }

        infos[numFound - 1].numTris = buildMesh->numTriangles - infos[numFound - 1].numTris;

        RWMESHGLOBAL(stripFreeList) = RwFreeListCreate(sizeof(RpTriStrip), buildMesh->numTriangles / 10 + 5, 0);

        {
            RpTriStrip* stripList;
            RpBuildMeshTriangle** src;
            TriStripMeshInfo* info;
            RwUInt32 m;

            m = 0;
            src = triPointers;
            info = infos;
            stripList = NULL;
            for (; m < numFound; info++, m++) {
                RpBuildMeshTriangle* tris =
                    (RpBuildMeshTriangle*)RwMalloc(info->numTris * sizeof(RpBuildMeshTriangle));
                RpTriStrip* strip;

                for (i = 0; i < info->numTris; i++) {
                    tris[i] = **src++;
                }

                TriStripStripTris(tris, info->numTris, &stripList, testPasses);
                TriStripJoin(&stripList, joinOdd);

                for (strip = stripList; strip; strip = strip->next) {
                    RpMesh* mesh = (RpMesh*)RwMalloc(sizeof(RpMesh) + strip->numIndices * sizeof(RxVertexIndex));
                    RwUInt32 j;

                    mesh->material = info->material;
                    mesh->numIndices = strip->numIndices;
                    mesh->indices = (RxVertexIndex*)(mesh + 1);
                    for (j = 0; j < mesh->numIndices; j++) {
                        mesh->indices[j] = strip->indices[j];
                    }
                    meshes[numMeshes++] = mesh;
                }

                while (stripList) {
                    strip = stripList;
                    stripList = strip->next;
                    RwFree(strip->indices);
                    strip->indices = NULL;
                    RwFreeListFree(RWMESHGLOBAL(stripFreeList), strip);
                }

                RwFree(tris);
            }
        }
    }

    RwFreeListDestroy(RWMESHGLOBAL(stripFreeList));
    RWMESHGLOBAL(stripFreeList) = NULL;

    {
        RwUInt32 size = sizeof(RpMeshHeader);
        RwUInt32 totalIndices = 0;
        RpMesh* mesh;
        RxVertexIndex* indices;

        for (i = 0; i < numMeshes; i++) {
            totalIndices += meshes[i]->numIndices;
            size += sizeof(RpMesh) + meshes[i]->numIndices * sizeof(RxVertexIndex);
        }

        meshHeader = _rpMeshHeaderCreate(size);
        meshHeader->flags = rpMESHHEADERTRISTRIP;
        meshHeader->numMeshes = numMeshes;
        meshHeader->serialNum = RWMESHGLOBAL(nextSerialNum);
        meshHeader->firstMeshOffset = 0;
        meshHeader->totalIndicesInMesh = totalIndices;
        RWMESHGLOBAL(nextSerialNum)++;

        mesh = (RpMesh*)(meshHeader + 1);
        indices = (RxVertexIndex*)(mesh + numMeshes);
        for (i = 0; i < numMeshes; i++) {
            mesh->indices = indices;
            mesh->numIndices = meshes[i]->numIndices;
            mesh->material = meshes[i]->material;
            memcpy(indices, meshes[i]->indices, meshes[i]->numIndices * sizeof(RxVertexIndex));
            indices += mesh->numIndices;
            mesh++;
            RwFree(meshes[i]);
            meshes[i] = NULL;
        }
    }

    RwFree(triPointers);
    RwFree(meshes);
    RwFree(infos);

    return meshHeader;
}

RpMeshHeader* RpBuildMeshGenerateDefaultTriStrip(RpBuildMesh* buildMesh, void* data)
{
    return TriStripMeshGenerate(buildMesh, 0, TRUE);
}

RpMeshHeader* _rpTriListMeshGenerate(RpBuildMesh* buildMesh, void* data)
{
    RpBuildMeshTriangle** triPointers;
    RpMeshHeader* meshHeader;
    RwUInt32 numMeshes;
    RwUInt32 size;
    RwUInt32 i;

    triPointers = (RpBuildMeshTriangle**)RwMalloc(buildMesh->numTriangles * sizeof(RpBuildMeshTriangle*));
    if (triPointers) {
        RpBuildMeshTriangle** tri;
        RpMesh* mesh;
        RxVertexIndex* indices;

        for (i = 0; i < buildMesh->numTriangles; i++) {
            triPointers[i] = &buildMesh->meshTriangles[i];
        }

        qsort(triPointers, buildMesh->numTriangles, sizeof(RpBuildMeshTriangle*), SortPolygons);

        numMeshes = 1;
        if (buildMesh->numTriangles >= 2) {
            RpMaterial* material = triPointers[0]->material;

            for (i = 1; i < buildMesh->numTriangles; i++) {
                if (triPointers[i]->material != material) {
                    material = triPointers[i]->material;
                    numMeshes++;
                }
            }
        }

        size = sizeof(RpMeshHeader) + sizeof(RpMesh) * numMeshes + sizeof(RxVertexIndex) * 3 * buildMesh->numTriangles;
        meshHeader = _rpMeshHeaderCreate(size);
        if (meshHeader == NULL) {
            RwFree(triPointers);
            RWERROR((E_RW_NOMEM, size));
            return NULL;
        }

        meshHeader->flags = 0;
        meshHeader->numMeshes = 1;
        meshHeader->serialNum = RWMESHGLOBAL(nextSerialNum);
        meshHeader->firstMeshOffset = 0;
        meshHeader->totalIndicesInMesh = buildMesh->numTriangles * 3;
        RWMESHGLOBAL(nextSerialNum)++;

        mesh = (RpMesh*)(meshHeader + 1);
        indices = (RxVertexIndex*)(mesh + numMeshes);
        mesh->numIndices = 0;
        mesh->material = triPointers[0]->material;
        mesh->indices = indices;

        tri = triPointers;
        i = 0;
        do {
            if ((*tri)->material != mesh->material) {
                mesh[1].numIndices = 0;
                mesh[1].material = (*tri)->material;
                mesh++;
                mesh->indices = indices;
                meshHeader->numMeshes++;
            }

            indices[0] = (*tri)->vertIndex[0];
            indices[1] = (*tri)->vertIndex[1];
            indices[2] = (*tri)->vertIndex[2];
            tri++;
            indices += 3;
            mesh->numIndices += 3;
            i++;
        } while (i < buildMesh->numTriangles);

        _rpMeshHeaderForAllMeshes(meshHeader, SortPolygonsInTriListMesh, NULL);
        RwFree(triPointers);
        return meshHeader;
    }

    RWERROR((E_RW_NOMEM, buildMesh->numTriangles * sizeof(RpBuildMeshTriangle*)));
    return NULL;
}

RpMeshHeader* _rpMeshOptimise(RpBuildMesh* buildMesh, RwUInt32 flags)
{
    RpMeshHeader* meshHeader;

    if (buildMesh) {
        RpTriStripMeshCallBack callback;
        void* data;

        if (buildMesh->numTriangles == 0) {
            _rpBuildMeshDestroy(buildMesh);
            return &TriStripGlobals.emptyMeshHeader;
        }

        if (flags & rpMESHHEADERTRISTRIP) {
            callback = TriStripGlobals.callback;
            data = TriStripGlobals.data;
        } else {
            callback = _rpTriListMeshGenerate;
            data = NULL;
        }

        meshHeader = callback(buildMesh, data);
        if (meshHeader) {
            _rpBuildMeshDestroy(buildMesh);
            return meshHeader;
        }
    }
    return NULL;
}
