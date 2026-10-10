/*
 * Palette quantisation: image colours are accumulated into a 5 level 4D
 * (RGBA) octree whose leaves hold the weighted colour moments, the tree is
 * split into the requested number of boxes by repeatedly cutting the box of
 * highest variance (Wu's method), and the leaves are tagged with the index
 * of the box that contains them for matching images against the palette.
 */

#include "rwsdk/rwcore.h"

#define MAXDEPTH 5
#define MAXCOLOR 256

typedef struct rwPalQuantRGBABox {
    /* 0x00 */ RwRGBA col0;
    /* 0x04 */ RwRGBA col1;
} rwPalQuantRGBABox;

typedef struct RwRGBAReal {
    RwReal red;
    RwReal green;
    RwReal blue;
    RwReal alpha;
} RwRGBAReal;

typedef struct rwPalQuantLeafNode {
    /* 0x00 */ RwReal weight;
    /* 0x04 */ RwRGBAReal ac;
    /* 0x14 */ RwReal var;
    /* 0x18 */ RwUInt8 palIndex;
} rwPalQuantLeafNode;

typedef union rwPalQuantOctNode rwPalQuantOctNode;

typedef struct rwPalQuantBranchNode {
    /* 0x00 */ rwPalQuantOctNode* dir[16];
} rwPalQuantBranchNode;

union rwPalQuantOctNode {
    rwPalQuantBranchNode Branch;
    rwPalQuantLeafNode Leaf;
};

typedef struct RwPalQuant {
    /* 0x00 */ rwPalQuantRGBABox* Mcube;
    /* 0x04 */ RwReal* Mvv;
    /* 0x08 */ rwPalQuantOctNode* root;
    /* 0x0C */ RwFreeList* cubefreelist;
} RwPalQuant;

#define RwCalloc(_n, _s) ((RWSRCGLOBAL(memoryFuncs).rwcalloc)((_n), (_s)))

static RwUInt32 splice[MAXCOLOR];

#define GETCELLINDEX(_c)                                                                                               \
    ((splice[(_c)->red >> 3] << 3) | (splice[(_c)->green >> 3] << 2) | (splice[(_c)->blue >> 3] << 1) |             \
     splice[(_c)->alpha >> 3])

static void InitLeaf(rwPalQuantLeafNode* Leaf)
{
    Leaf->palIndex = 0xFF;
    Leaf->weight = 0.0f;
    Leaf->ac.red = 0.0f;
    Leaf->ac.green = 0.0f;
    Leaf->ac.blue = 0.0f;
    Leaf->ac.alpha = 0.0f;
    Leaf->var = 0.0f;
}

static void InitBranch(rwPalQuantBranchNode* Branch)
{
    RwInt32 i;

    for (i = 0; i < 16; i++) {
        Branch->dir[i] = NULL;
    }
}

static rwPalQuantOctNode* CreateCube(RwFreeList* fl)
{
    return (rwPalQuantOctNode*)RwFreeListAlloc(fl);
}

static rwPalQuantOctNode* AllocateToLeaf(RwPalQuant* pq, rwPalQuantOctNode* root, RwUInt32 cellIndex, RwInt32 depth)
{
    if (depth == 0) {
        return root;
    } else {
        RwInt32 dir = cellIndex & 15;

        if (!root->Branch.dir[dir]) {
            rwPalQuantOctNode* cube = CreateCube(pq->cubefreelist);

            root->Branch.dir[dir] = cube;
            if (depth == 1) {
                InitLeaf(&cube->Leaf);
            } else {
                InitBranch(&cube->Branch);
            }
        }

        return AllocateToLeaf(pq, root->Branch.dir[dir], cellIndex >> 4, depth - 1);
    }
}

void _rwPalQuantAddImage(RwPalQuant* pq, RwImage* img, RwReal weight)
{
    RwInt32 stride = img->stride;
    RwUInt8* pixels = img->cpPixels;
    RwRGBA* palette = img->palette;
    RwInt32 height = img->height;

    switch (img->depth) {
    case 4:
    case 8:
        while (height--) {
            RwInt32 width = img->width;
            RwUInt8* src = pixels;

            while (width--) {
                RwRGBA* color = &palette[*src];
                rwPalQuantLeafNode* leaf = &AllocateToLeaf(pq, pq->root, GETCELLINDEX(color), MAXDEPTH)->Leaf;
                RwRGBAReal rgba;

                leaf->weight += weight;
                rgba.red = (1.0f / 255.0f) * color->red * weight;
                rgba.green = (1.0f / 255.0f) * color->green * weight;
                rgba.blue = (1.0f / 255.0f) * color->blue * weight;
                rgba.alpha = (1.0f / 255.0f) * color->alpha * weight;
                leaf->ac.red += rgba.red;
                leaf->ac.green += rgba.green;
                leaf->ac.blue += rgba.blue;
                leaf->ac.alpha += rgba.alpha;
                leaf->var += weight * (rgba.red * rgba.red + rgba.green * rgba.green + rgba.blue * rgba.blue +
                                       rgba.alpha * rgba.alpha);
                src++;
            }
            pixels += stride;
        }
        break;
    case 32:
        while (height--) {
            RwInt32 width = img->width;
            RwUInt8* src = pixels;

            while (width--) {
                RwRGBA* color = (RwRGBA*)src;
                rwPalQuantLeafNode* leaf = &AllocateToLeaf(pq, pq->root, GETCELLINDEX(color), MAXDEPTH)->Leaf;
                RwRGBAReal rgba;

                leaf->weight += weight;
                rgba.red = (1.0f / 255.0f) * color->red * weight;
                rgba.green = (1.0f / 255.0f) * color->green * weight;
                rgba.blue = (1.0f / 255.0f) * color->blue * weight;
                rgba.alpha = (1.0f / 255.0f) * color->alpha * weight;
                leaf->ac.red += rgba.red;
                leaf->ac.green += rgba.green;
                leaf->ac.blue += rgba.blue;
                leaf->ac.alpha += rgba.alpha;
                leaf->var += weight * (rgba.red * rgba.red + rgba.green * rgba.green + rgba.blue * rgba.blue +
                                       rgba.alpha * rgba.alpha);
                src += 4;
            }
            pixels += stride;
        }
        break;
    }
}

static void assignindex(rwPalQuantOctNode* root, RwRGBA* origin, RwInt32 depth, rwPalQuantRGBABox* box,
                        RwInt32 index)
{
    if (root) {
        RwInt32 size = 1 << depth;
        RwInt32 dr = origin->red - box->col1.red;
        RwInt32 dg = origin->green - box->col1.green;
        RwInt32 db = origin->blue - box->col1.blue;
        RwInt32 da = origin->alpha - box->col1.alpha;

        if (dr >= 0 || dg >= 0 || db >= 0 || da >= 0) {
            return;
        }
        {
            RwInt32 er = box->col0.red - origin->red;
            RwInt32 eg = box->col0.green - origin->green;
            RwInt32 eb = box->col0.blue - origin->blue;
            RwInt32 ea = box->col0.alpha - origin->alpha;

            if (er >= size || eg >= size || eb >= size || ea >= size) {
                return;
            }
            {
                if (dr <= -size && dg <= -size && db <= -size && da <= -size && er <= 0 && eg <= 0 && eb <= 0 &&
                    ea <= 0 && depth == 0) {
                    root->Leaf.palIndex = (RwUInt8)index;
                } else {
                    RwInt32 i;

                    depth--;
                    for (i = 0; i < 16; i++) {
                        RwRGBA suborigin;

                        suborigin.red = origin->red + (((i >> 3) & 1) << depth);
                        suborigin.green = origin->green + (((i >> 2) & 1) << depth);
                        suborigin.blue = origin->blue + (((i >> 1) & 1) << depth);
                        suborigin.alpha = origin->alpha + ((i & 1) << depth);
                        assignindex(root->Branch.dir[i], &suborigin, depth, box, index);
                    }
                }
            }
        }
    }
}

static void addvolume(rwPalQuantOctNode* root, RwRGBA* origin, RwInt32 depth, rwPalQuantRGBABox* box,
                      rwPalQuantLeafNode* vol)
{
    if (root) {
        RwInt32 size = 1 << depth;
        RwInt32 dr = origin->red - box->col1.red;
        RwInt32 dg = origin->green - box->col1.green;
        RwInt32 db = origin->blue - box->col1.blue;
        RwInt32 da = origin->alpha - box->col1.alpha;

        if (dr >= 0 || dg >= 0 || db >= 0 || da >= 0) {
            return;
        }
        {
            RwInt32 er = box->col0.red - origin->red;
            RwInt32 eg = box->col0.green - origin->green;
            RwInt32 eb = box->col0.blue - origin->blue;
            RwInt32 ea = box->col0.alpha - origin->alpha;

            if (er >= size || eg >= size || eb >= size || ea >= size) {
                return;
            }
            {
                if (dr <= -size && dg <= -size && db <= -size && da <= -size && er <= 0 && eg <= 0 && eb <= 0 &&
                    ea <= 0 && depth == 0) {
                    vol->weight += root->Leaf.weight;
                    vol->ac.red += root->Leaf.ac.red;
                    vol->ac.green += root->Leaf.ac.green;
                    vol->ac.blue += root->Leaf.ac.blue;
                    vol->ac.alpha += root->Leaf.ac.alpha;
                    vol->var += root->Leaf.var;
                } else {
                    RwInt32 i;

                    depth--;
                    for (i = 0; i < 16; i++) {
                        RwRGBA suborigin;

                        suborigin.red = origin->red + (((i >> 3) & 1) << depth);
                        suborigin.green = origin->green + (((i >> 2) & 1) << depth);
                        suborigin.blue = origin->blue + (((i >> 1) & 1) << depth);
                        suborigin.alpha = origin->alpha + ((i & 1) << depth);
                        addvolume(root->Branch.dir[i], &suborigin, depth, box, vol);
                    }
                }
            }
        }
    }
}

#define MAXIMIZEDIRECTION(_field)                                                                                      \
    for (i = box->col0._field; i < box->col1._field; i++) {                                                            \
        RwRGBA origin;                                                                                                 \
                                                                                                                       \
        subbox.col1._field = i;                                                                                        \
        origin.red = 0;                                                                                                \
        origin.green = 0;                                                                                              \
        origin.blue = 0;                                                                                               \
        origin.alpha = 0;                                                                                              \
        InitLeaf(&half);                                                                                               \
        addvolume(root, &origin, MAXDEPTH, &subbox, &half);                                                            \
                                                                                                                       \
        rest.weight = whole->weight - half.weight;                                                                     \
        rest.ac.red = whole->ac.red - half.ac.red;                                                                     \
        rest.ac.green = whole->ac.green - half.ac.green;                                                               \
        rest.ac.blue = whole->ac.blue - half.ac.blue;                                                                  \
        rest.ac.alpha = whole->ac.alpha - half.ac.alpha;                                                               \
                                                                                                                       \
        if (half.weight > 0.0f && rest.weight > 0.0f) {                                                                \
            RwReal temp = (half.ac.red * half.ac.red + half.ac.green * half.ac.green + half.ac.blue * half.ac.blue +   \
                           half.ac.alpha * half.ac.alpha) / half.weight +                                              \
                          (rest.ac.red * rest.ac.red + rest.ac.green * rest.ac.green + rest.ac.blue * rest.ac.blue +   \
                           rest.ac.alpha * rest.ac.alpha) / rest.weight;                                               \
                                                                                                                       \
            if (temp > max) {                                                                                          \
                max = temp;                                                                                            \
                *cut = i;                                                                                              \
            } else if (temp < prev) {                                                                                  \
                break;                                                                                                 \
            }                                                                                                          \
            prev = temp;                                                                                               \
        }                                                                                                              \
    }

static RwReal nMaximize(rwPalQuantOctNode* root, rwPalQuantRGBABox* box, RwInt32 dir, RwInt32* cut,
                        rwPalQuantLeafNode* whole)
{
    RwReal max;
    RwReal prev;
    rwPalQuantRGBABox subbox;
    RwInt32 i;
    rwPalQuantLeafNode half;
    rwPalQuantLeafNode rest;

    *cut = -1;
    max = 0.0f;
    prev = max;
    subbox = *box;

    switch (dir) {
    case 1:
        MAXIMIZEDIRECTION(red);
        break;
    case 2:
        MAXIMIZEDIRECTION(green);
        break;
    case 3:
        MAXIMIZEDIRECTION(blue);
        break;
    case 4:
        MAXIMIZEDIRECTION(alpha);
        break;
    }

    return max;
}

static RwBool nCut(rwPalQuantOctNode* root, rwPalQuantRGBABox* set1, rwPalQuantRGBABox* set2)
{
    RwInt32 cutr;
    RwInt32 cutg;
    RwInt32 cutb;
    RwInt32 cuta;
    RwReal maxr;
    RwReal maxg;
    RwReal maxb;
    RwReal maxa;
    rwPalQuantLeafNode whole;
    RwRGBA origin;

    origin.red = 0;
    origin.green = 0;
    origin.blue = 0;
    origin.alpha = 0;
    InitLeaf(&whole);
    addvolume(root, &origin, MAXDEPTH, set1, &whole);

    maxr = nMaximize(root, set1, 1, &cutr, &whole);
    maxg = nMaximize(root, set1, 2, &cutg, &whole);
    maxb = nMaximize(root, set1, 3, &cutb, &whole);
    maxa = nMaximize(root, set1, 4, &cuta, &whole);

    if (maxr <= 0.0f && maxg <= 0.0f && maxb <= 0.0f && maxa <= 0.0f) {
        return FALSE;
    }

    *set2 = *set1;

    if (maxr >= maxg) {
        if (maxr >= maxb) {
            if (maxr >= maxa) {
                set1->col1.red = set2->col0.red = cutr;
            } else {
                set1->col1.alpha = set2->col0.alpha = cuta;
            }
        } else if (maxb >= maxa) {
            set1->col1.blue = set2->col0.blue = cutb;
        } else {
            set1->col1.alpha = set2->col0.alpha = cuta;
        }
    } else if (maxg >= maxb) {
        if (maxg >= maxa) {
            set1->col1.green = set2->col0.green = cutg;
        } else {
            set1->col1.alpha = set2->col0.alpha = cuta;
        }
    } else if (maxb >= maxa) {
        set1->col1.blue = set2->col0.blue = cutb;
    } else {
        set1->col1.alpha = set2->col0.alpha = cuta;
    }

    return TRUE;
}

static RwInt32 CountLeafs(rwPalQuantOctNode* root, RwInt32 depth)
{
    RwInt32 i;
    RwInt32 n = 0;

    if (root) {
        if (depth > 0) {
            for (i = 0; i < 16; i++) {
                n += CountLeafs(root->Branch.dir[i], depth - 1);
            }
        } else {
            n = 1;
        }
    }

    return n;
}

static RwInt32 ExtractNodes(rwPalQuantOctNode* root, RwRGBA* palette, RwInt32 index, RwInt32 depth)
{
    if (root) {
        if (depth > 0) {
            RwInt32 i;

            for (i = 0; i < 16; i++) {
                index = ExtractNodes(root->Branch.dir[i], palette, index, depth - 1);
            }
        } else {
            RwReal weight = root->Leaf.weight;
            RwReal recip = 0.0f;

            if (weight > 0.0f) {
                recip = 255.9999f / weight;
            }
            palette[index].red = (RwUInt8)(RwInt32)(root->Leaf.ac.red * recip);
            palette[index].green = (RwUInt8)(RwInt32)(root->Leaf.ac.green * recip);
            palette[index].blue = (RwUInt8)(RwInt32)(root->Leaf.ac.blue * recip);
            palette[index].alpha = (RwUInt8)(RwInt32)(root->Leaf.ac.alpha * recip);
            root->Leaf.palIndex = (RwUInt8)index;
            index++;
        }
    }

    return index;
}

static inline RwReal BoxVariance(rwPalQuantOctNode* root, rwPalQuantRGBABox* box)
{
    rwPalQuantLeafNode vol;
    RwRGBA origin;

    origin.red = 0;
    origin.green = 0;
    origin.blue = 0;
    origin.alpha = 0;
    InitLeaf(&vol);
    addvolume(root, &origin, MAXDEPTH, box, &vol);
    return vol.var - (vol.ac.red * vol.ac.red + vol.ac.green * vol.ac.green + vol.ac.blue * vol.ac.blue +
                      vol.ac.alpha * vol.ac.alpha) /
                         vol.weight;
}

RwInt32 _rwPalQuantResolvePalette(RwRGBA* palette, RwInt32 maxcols, RwPalQuant* pq)
{
    RwInt32 numCols = maxcols;
    RwInt32 leafs = CountLeafs(pq->root, MAXDEPTH);
    RwInt32 i;

    if (maxcols >= leafs) {
        RwInt32 n;

        numCols = leafs;
        n = ExtractNodes(pq->root, palette, 0, MAXDEPTH);
        for (i = n; i < maxcols; i++) {
            palette[i].red = 0;
            palette[i].green = 0;
            palette[i].blue = 0;
            palette[i].alpha = 0;
        }
    } else {
        pq->Mcube[0].col0.red = 0;
        pq->Mcube[0].col0.green = 0;
        pq->Mcube[0].col0.blue = 0;
        pq->Mcube[0].col0.alpha = 0;
        pq->Mcube[0].col1.red = 32;
        pq->Mcube[0].col1.green = 32;
        pq->Mcube[0].col1.blue = 32;
        pq->Mcube[0].col1.alpha = 32;
        pq->Mvv[0] = BoxVariance(pq->root, &pq->Mcube[0]);

        for (i = 1; i < maxcols; i++) {
            RwReal maxv = 0.0f;
            RwInt32 next = -1;
            RwInt32 k;

            for (k = 0; k < i; k++) {
                if (pq->Mvv[k] > maxv) {
                    maxv = pq->Mvv[k];
                    next = k;
                }
            }

            if (next == -1) {
                break;
            }

            if (nCut(pq->root, &pq->Mcube[next], &pq->Mcube[i])) {
                pq->Mvv[next] = BoxVariance(pq->root, &pq->Mcube[next]);
                pq->Mvv[i] = BoxVariance(pq->root, &pq->Mcube[i]);
            } else {
                pq->Mvv[next] = 0.0f;
                i--;
            }
        }

        for (i = 0; i < maxcols; i++) {
            if (i < maxcols) {
                RwRGBA origin;
                rwPalQuantLeafNode vol;
                RwReal recip;

                origin.red = 0;
                origin.green = 0;
                origin.blue = 0;
                origin.alpha = 0;
                assignindex(pq->root, &origin, MAXDEPTH, &pq->Mcube[i], i);

                {
                    RwRGBA origin2;

                    origin2.red = 0;
                    origin2.green = 0;
                    origin2.blue = 0;
                    origin2.alpha = 0;
                    InitLeaf(&vol);
                    addvolume(pq->root, &origin2, MAXDEPTH, &pq->Mcube[i], &vol);
                }

                recip = 0.0f;
                if (vol.weight > 0.0f) {
                    recip = 255.9999f / vol.weight;
                }
                palette[i].red = (RwUInt8)(vol.ac.red * recip);
                palette[i].green = (RwUInt8)(vol.ac.green * recip);
                palette[i].blue = (RwUInt8)(vol.ac.blue * recip);
                palette[i].alpha = (RwUInt8)(vol.ac.alpha * recip);
            } else {
                palette[i].red = 0;
                palette[i].green = 0;
                palette[i].blue = 0;
                palette[i].alpha = 0;
            }
        }
    }

    return numCols;
}

static RwUInt8 GetIndex(rwPalQuantOctNode* root, RwUInt32 cellIndex, RwInt32 depth)
{
    if (depth == 0) {
        return root->Leaf.palIndex;
    }
    return GetIndex(root->Branch.dir[cellIndex & 15], cellIndex >> 4, depth - 1);
}

void _rwPalQuantMatchImage(RwUInt8* dstpixels, RwInt32 dststride, RwInt32 dstdepth, RwBool dstPacked,
                           RwPalQuant* pq, RwImage* img)
{
    RwInt32 stride = img->stride;
    RwUInt8* pixels = img->cpPixels;

    if (dstdepth == 4 && dstPacked) {
        switch (img->depth) {
        case 4:
        case 8: {
            RwRGBA* palette = img->palette;
            RwUInt32 height = img->height;

            while (height--) {
                RwUInt32 width = img->width;
                RwUInt8* src = pixels;
                RwUInt8* dst = dstpixels;
                RwUInt32 x;

                for (x = 0; x < width; x++) {
                    RwRGBA* color = &palette[*src++];
                    RwUInt8 index = GetIndex(pq->root, GETCELLINDEX(color), MAXDEPTH);

                    if (x & 1) {
                        *dst &= 0x0F;
                        *dst |= (index & 0x0F) << 4;
                        dst++;
                    } else {
                        *dst &= 0xF0;
                        *dst |= index & 0x0F;
                    }
                }
                pixels += stride;
                dstpixels += dststride;
            }
            break;
        }
        case 32: {
            RwUInt32 height = img->height;

            while (height--) {
                RwUInt32 width = img->width;
                RwUInt8* src = pixels;
                RwUInt8* dst = dstpixels;
                RwUInt32 x;

                for (x = 0; x < width; x++) {
                    RwRGBA* color = (RwRGBA*)src;
                    RwUInt8 index;

                    src += 4;
                    index = GetIndex(pq->root, GETCELLINDEX(color), MAXDEPTH);
                    if (x & 1) {
                        *dst &= 0x0F;
                        *dst |= (index & 0x0F) << 4;
                        dst++;
                    } else {
                        *dst &= 0xF0;
                        *dst |= index & 0x0F;
                    }
                }
                pixels += stride;
                dstpixels += dststride;
            }
            break;
        }
        }
    } else {
        switch (img->depth) {
        case 4:
        case 8: {
            RwRGBA* palette = img->palette;
            RwUInt32 height = img->height;

            while (height--) {
                RwUInt32 width = img->width;
                RwUInt8* src = pixels;
                RwUInt8* dst = dstpixels;

                while (width--) {
                    RwRGBA* color = &palette[*src++];

                    *dst++ = GetIndex(pq->root, GETCELLINDEX(color), MAXDEPTH);
                }
                pixels += stride;
                dstpixels += dststride;
            }
            break;
        }
        case 32: {
            RwUInt32 height = img->height;

            while (height--) {
                RwUInt32 width = img->width;
                RwUInt8* src = pixels;
                RwUInt8* dst = dstpixels;

                while (width--) {
                    RwRGBA* color = (RwRGBA*)src;

                    src += 4;
                    *dst++ = GetIndex(pq->root, GETCELLINDEX(color), MAXDEPTH);
                }
                pixels += stride;
                dstpixels += dststride;
            }
            break;
        }
        }
    }
}

RwBool _rwPalQuantInit(RwPalQuant* pq)
{
    RwInt32 i;

    for (i = 0; i < 32; i++) {
        RwInt32 j;
        RwUInt32 mask = 0;

        for (j = 0; j < MAXDEPTH; j++) {
            mask |= (i & (1 << j)) ? (1 << ((4 - j) * 4)) : 0;
        }
        splice[i] = mask;
    }

    pq->Mcube = (rwPalQuantRGBABox*)RwCalloc(sizeof(rwPalQuantRGBABox), MAXCOLOR);
    pq->Mvv = (RwReal*)RwCalloc(sizeof(RwReal), MAXCOLOR);
    pq->cubefreelist = RwFreeListCreate(sizeof(rwPalQuantOctNode), 64, 0);
    pq->root = CreateCube(pq->cubefreelist);
    InitBranch(&pq->root->Branch);

    return TRUE;
}

static void DeleteOctTree(RwPalQuant* pq, rwPalQuantOctNode* root, RwInt32 depth)
{
    if (root) {
        if (depth > 0) {
            RwInt32 i;

            for (i = 0; i < 16; i++) {
                DeleteOctTree(pq, root->Branch.dir[i], depth - 1);
            }
        }
        RwFreeListFree(pq->cubefreelist, root);
    }
}

void _rwPalQuantTerm(RwPalQuant* pq)
{
    DeleteOctTree(pq, pq->root, MAXDEPTH);
    pq->root = NULL;
    RwFreeListDestroy(pq->cubefreelist);
    RwFree(pq->Mvv);
    RwFree(pq->Mcube);
}
