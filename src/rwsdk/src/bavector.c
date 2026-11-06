/*
 * Vectors: point/vector transformation (replaceable per platform) and
 * table-driven square roots.
 */

#include "rwsdk/rwcore.h"
#include "math.h"

#define rwPLUGIN_ID 1 /* core */

#define E_RW_ZEROLENGTH 0x19

typedef RwV3d* (*rwVectorMultFn)(RwV3d* pointsOut, const RwV3d* pointsIn, RwInt32 numPoints,
                                  const RwMatrix* matrix);

typedef struct rwVectorGlobals {
    RwUInt32* SqrtTable;
    RwUInt32* InvSqrtTable;
    rwVectorMultFn multPoint;
    rwVectorMultFn multVector;
} rwVectorGlobals;

/* The integer views are volatile so the float is always reread from memory. */
typedef union RwSplitBits {
    RwReal nReal;
    volatile RwInt32 nInt;
    volatile RwUInt32 nUInt;
} RwSplitBits;

RwModuleInfo vectorModule;

#define RWVECTORGLOBAL(var) (RWPLUGINOFFSET(rwVectorGlobals, RwEngineInstance, vectorModule.globalsOffset)->var)

static RwV3d* VectorMultPoint(RwV3d* pointsOut, const RwV3d* pointsIn, RwInt32 numPoints, const RwMatrix* matrix) {
    RwV3d* result = pointsOut;

    while (--numPoints >= 0) {
        RwReal x = pointsIn->x;
        RwReal y = pointsIn->y;
        RwReal z = pointsIn->z;

        pointsIn++;
        {
            RwV3d c;

            c.x = x * matrix->right.x + y * matrix->up.x + z * matrix->at.x;
            c.y = x * matrix->right.y + y * matrix->up.y + z * matrix->at.y;
            c.z = x * matrix->right.z + y * matrix->up.z + z * matrix->at.z;
            pointsOut->x = c.x + matrix->pos.x;
            pointsOut->y = c.y + matrix->pos.y;
            pointsOut->z = c.z + matrix->pos.z;
        }
        pointsOut++;
    }

    return result;
}

static RwV3d* VectorMultVector(RwV3d* vectorsOut, const RwV3d* vectorsIn, RwInt32 numPoints,
                               const RwMatrix* matrix) {
    RwV3d* result = vectorsOut;

    while (--numPoints >= 0) {
        RwReal x = vectorsIn->x;
        RwReal y = vectorsIn->y;
        RwReal z = vectorsIn->z;

        vectorsIn++;
        {
            RwV3d c;

            c.x = x * matrix->right.x + y * matrix->up.x;
            c.y = x * matrix->right.y + y * matrix->up.y;
            c.z = x * matrix->right.z + y * matrix->up.z;
            vectorsOut->x = c.x + z * matrix->at.x;
            vectorsOut->y = c.y + z * matrix->at.y;
            vectorsOut->z = c.z + z * matrix->at.z;
        }
        vectorsOut++;
    }

    return result;
}

RwBool _rwVectorSetMultFn(rwVectorMultFn multPoint, rwVectorMultFn multVector) {
    if (!multPoint) {
        multPoint = VectorMultPoint;
    }
    RWVECTORGLOBAL(multPoint) = multPoint;

    if (!multVector) {
        multVector = VectorMultVector;
    }
    RWVECTORGLOBAL(multVector) = multVector;

    return TRUE;
}

RwReal _rwSqrt(const RwReal num);
RwReal _rwInvSqrt(const RwReal num);

void _rwV3dNormalize(RwV3d* out, const RwV3d* in) {
    RwReal recip = _rwInvSqrt(in->x * in->x + in->y * in->y + in->z * in->z);

    out->x = in->x * recip;
    out->y = in->y * recip;
    out->z = in->z * recip;
}

RwReal RwV3dNormalize(RwV3d* out, const RwV3d* in) {
    RwReal length2 = in->x * in->x + in->y * in->y + in->z * in->z;
    RwReal length = _rwSqrt(length2);
    RwReal recip = _rwInvSqrt(length2);

    out->x = in->x * recip;
    out->y = in->y * recip;
    out->z = in->z * recip;

    if (length <= (RwReal)0.0) {
        RWERROR((E_RW_ZEROLENGTH));
    }

    return length;
}

RwReal RwV3dLength(const RwV3d* in) {
    return _rwSqrt(in->x * in->x + in->y * in->y + in->z * in->z);
}

RwReal _rwSqrt(const RwReal num) {
    RwSplitBits u;

    u.nReal = num;
    if (u.nUInt) {
        u.nUInt = u.nUInt + 0x800;
        u.nUInt = ((u.nUInt >> 1) & 0x3FC00000) + RWVECTORGLOBAL(SqrtTable)[(u.nUInt >> 12) & 0xFFF];
    }

    return u.nReal;
}

RwReal _rwInvSqrt(const RwReal num) {
    RwSplitBits u;

    u.nReal = num;
    if (u.nUInt) {
        u.nUInt = u.nUInt + 0x800;
        u.nUInt = ((~u.nUInt >> 1) & 0x3FC00000) + RWVECTORGLOBAL(InvSqrtTable)[(u.nUInt >> 12) & 0xFFF];
    }

    return u.nReal;
}

RwV3d* RwV3dTransformPoints(RwV3d* pointsOut, const RwV3d* pointsIn, RwInt32 numPoints, const RwMatrix* matrix) {
    return RWVECTORGLOBAL(multPoint)(pointsOut, pointsIn, numPoints, matrix);
}

RwV3d* RwV3dTransformVectors(RwV3d* vectorsOut, const RwV3d* vectorsIn, RwInt32 numPoints,
                             const RwMatrix* matrix) {
    return RWVECTORGLOBAL(multVector)(vectorsOut, vectorsIn, numPoints, matrix);
}

void* _rwVectorClose(void* instance, RwInt32 offset, RwInt32 size) {
    if (RWVECTORGLOBAL(InvSqrtTable)) {
        RwFree(RWVECTORGLOBAL(InvSqrtTable));
        RWVECTORGLOBAL(InvSqrtTable) = NULL;
    }

    if (RWVECTORGLOBAL(SqrtTable)) {
        RwFree(RWVECTORGLOBAL(SqrtTable));
        RWVECTORGLOBAL(SqrtTable) = NULL;
    }

    vectorModule.numInstances--;

    return instance;
}

static inline RwBool SqrtTableCreate(void) {
    RwUInt32* table = (RwUInt32*)RwMalloc(0x1000 * sizeof(RwUInt32));
    RwUInt32* entry;
    RwSplitBits num;
    RwInt32 i;

    if (!table) {
        RWERROR((E_RW_NOMEM, 0x1000 * sizeof(RwUInt32)));
        return FALSE;
    }

    num.nReal = 1.0f;

    entry = table + 0x800;
    for (i = 0; i < 0x800; i++) {
        RwSplitBits root;
        RwSplitBits value;

        root.nReal = (RwReal)sqrt(num.nReal);
        value.nUInt = root.nUInt - 0x1FC00000;
        *entry++ = value.nUInt;
        num.nUInt = num.nUInt + 0x1000;
    }

    entry = table;
    for (i = 0; i < 0x800; i++) {
        RwSplitBits root;
        RwSplitBits value;

        root.nReal = (RwReal)sqrt(num.nReal);
        value.nUInt = root.nUInt - 0x20000000;
        *entry++ = value.nUInt;
        num.nUInt = num.nUInt + 0x1000;
    }

    RWVECTORGLOBAL(SqrtTable) = table;
    return TRUE;
}

static inline RwBool InvSqrtTableCreate(void) {
    RwUInt32* table = (RwUInt32*)RwMalloc(0x1000 * sizeof(RwUInt32));
    RwUInt32* entry;
    RwSplitBits num;
    RwInt32 i;

    if (!table) {
        RWERROR((E_RW_NOMEM, 0x1000 * sizeof(RwUInt32)));
        return FALSE;
    }

    num.nReal = 1.0f;

    entry = table + 0x800;
    for (i = 0; i < 0x800; i++) {
        RwSplitBits root;
        RwSplitBits value;

        root.nReal = (RwReal)(1.0 / sqrt(num.nReal));
        value.nUInt = root.nUInt - 0x20000000;
        *entry++ = value.nUInt;
        num.nUInt = num.nUInt + 0x1000;
    }

    entry = table;
    for (i = 0; i < 0x800; i++) {
        RwSplitBits root;
        RwSplitBits value;

        root.nReal = (RwReal)(1.0 / sqrt(num.nReal));
        value.nUInt = root.nUInt - 0x1FC00000;
        *entry++ = value.nUInt;
        num.nUInt = num.nUInt + 0x1000;
    }

    RWVECTORGLOBAL(InvSqrtTable) = table;
    return TRUE;
}

void* _rwVectorOpen(void* instance, RwInt32 offset, RwInt32 size) {
    vectorModule.globalsOffset = offset;

    RWVECTORGLOBAL(multPoint) = VectorMultPoint;
    RWVECTORGLOBAL(multVector) = VectorMultVector;

    if (!SqrtTableCreate()) {
        return NULL;
    }

    if (!InvSqrtTableCreate()) {
        return NULL;
    }

    vectorModule.numInstances++;

    return instance;
}
