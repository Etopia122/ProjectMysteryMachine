/*
 * Matrices: creation, concatenation, rotation, inversion and the
 * type flags that let identity and orthonormal matrices take shortcuts.
 */

#include "rwsdk/rwcore.h"
#include "math.h"

#define rwPLUGIN_ID 1 /* core */

#define E_RW_BADPARAM 0x80000003

#define rwMATRIXTYPENORMAL 0x00000001
#define rwMATRIXTYPEMASK 0x00000003

typedef RwMatrix* (*rwMatrixMultFn)(RwMatrix* dst, const RwMatrix* matA, const RwMatrix* matB);

typedef struct RwMatrixTolerance {
    RwReal Normal;
    RwReal Orthogonal;
    RwReal Identity;
} RwMatrixTolerance;

typedef struct rwMatrixGlobals {
    RwFreeList* matrixFreeList;
    RwUInt32 optimizations;
    rwMatrixMultFn multiply;
    RwMatrixTolerance tolerance;
} rwMatrixGlobals;

/* The integer view is volatile so the float is always reread from memory. */
typedef union RwSplitBits {
    RwReal nReal;
    volatile RwInt32 nInt;
} RwSplitBits;

RwModuleInfo matrixModule;

#define RWMATRIXGLOBAL(var) (RWPLUGINOFFSET(rwMatrixGlobals, RwEngineInstance, matrixModule.globalsOffset)->var)

#define RwRealAbs(a) (((a) >= (RwReal)0.0) ? (a) : (-(a)))

#define RwV3dDotProduct(a, b) ((a)->x * (b)->x + (a)->y * (b)->y + (a)->z * (b)->z)

#define RwV3dCrossProduct(o, a, b)                    \
    do {                                              \
        (o)->x = (a)->y * (b)->z - (a)->z * (b)->y;   \
        (o)->y = (a)->z * (b)->x - (a)->x * (b)->z;   \
        (o)->z = (a)->x * (b)->y - (a)->y * (b)->x;   \
    } while (0)

#define RwV3dScale(o, a, s)       \
    do {                          \
        (o)->x = (a)->x * (s);    \
        (o)->y = (a)->y * (s);    \
        (o)->z = (a)->z * (s);    \
    } while (0)

#define RwMatrixSetIdentity(m)                                                      \
    do {                                                                            \
        (m)->right.x = (m)->up.y = (m)->at.z = (RwReal)1.0;                         \
        (m)->right.y = (m)->right.z = (m)->up.x = (RwReal)0.0;                      \
        (m)->up.z = (m)->at.x = (m)->at.y = (RwReal)0.0;                            \
        (m)->pos.x = (m)->pos.y = (m)->pos.z = (RwReal)0.0;                         \
        (m)->flags |= (rwMATRIXINTERNALIDENTITY | rwMATRIXTYPEORTHONORMAL);         \
    } while (0)

RwReal _rwInvSqrt(const RwReal num);
RwBool RwFreeListDestroy(RwFreeList* freelist);

static RwMatrix* MatrixMultiply(RwMatrix* dst, const RwMatrix* matA, const RwMatrix* matB) {
    dst->right.x = matA->right.x * matB->right.x + matA->right.y * matB->up.x + matA->right.z * matB->at.x;
    dst->right.y = matA->right.x * matB->right.y + matA->right.y * matB->up.y + matA->right.z * matB->at.y;
    dst->right.z = matA->right.x * matB->right.z + matA->right.y * matB->up.z + matA->right.z * matB->at.z;

    dst->up.x = matA->up.x * matB->right.x + matA->up.y * matB->up.x + matA->up.z * matB->at.x;
    dst->up.y = matA->up.x * matB->right.y + matA->up.y * matB->up.y + matA->up.z * matB->at.y;
    dst->up.z = matA->up.x * matB->right.z + matA->up.y * matB->up.z + matA->up.z * matB->at.z;

    dst->at.x = matA->at.x * matB->right.x + matA->at.y * matB->up.x + matA->at.z * matB->at.x;
    dst->at.y = matA->at.x * matB->right.y + matA->at.y * matB->up.y + matA->at.z * matB->at.y;
    dst->at.z = matA->at.x * matB->right.z + matA->at.y * matB->up.z + matA->at.z * matB->at.z;

    dst->pos.x = matA->pos.x * matB->right.x + matA->pos.y * matB->up.x + matA->pos.z * matB->at.x + matB->pos.x;
    dst->pos.y = matA->pos.x * matB->right.y + matA->pos.y * matB->up.y + matA->pos.z * matB->at.y + matB->pos.y;
    dst->pos.z = matA->pos.x * matB->right.z + matA->pos.y * matB->up.z + matA->pos.z * matB->at.z + matB->pos.z;

    return dst;
}

static RwMatrix* MatrixOrthoNormalize(RwMatrix* dst, const RwMatrix* src) {
    RwV3d right = src->right;
    RwV3d up = src->up;
    RwV3d at = src->at;
    RwV3d pos = src->pos;
    RwReal rightRecip;
    RwReal upRecip;
    RwReal atRecip;
    RwV3d* vpA;
    RwV3d* vpB;
    RwV3d* vpC;
    RwReal recip;

    rightRecip = _rwInvSqrt(RwV3dDotProduct(&right, &right));
    RwV3dScale(&right, &right, rightRecip);

    upRecip = _rwInvSqrt(RwV3dDotProduct(&up, &up));
    RwV3dScale(&up, &up, upRecip);

    atRecip = _rwInvSqrt(RwV3dDotProduct(&at, &at));
    RwV3dScale(&at, &at, atRecip);

    if (rightRecip > (RwReal)0.0) {
        if (upRecip > (RwReal)0.0) {
            if (atRecip > (RwReal)0.0) {
                RwReal upAt = RwRealAbs(RwV3dDotProduct(&up, &at));
                RwReal atRight = RwRealAbs(RwV3dDotProduct(&at, &right));
                RwReal rightUp = RwRealAbs(RwV3dDotProduct(&right, &up));

                if (upAt < atRight) {
                    if (upAt < rightUp) {
                        vpA = &up;
                        vpB = &at;
                        vpC = &right;
                    } else {
                        vpA = &right;
                        vpB = &up;
                        vpC = &at;
                    }
                } else {
                    if (atRight < rightUp) {
                        vpA = &at;
                        vpB = &right;
                        vpC = &up;
                    } else {
                        vpA = &right;
                        vpB = &up;
                        vpC = &at;
                    }
                }
            } else {
                vpA = &right;
                vpB = &up;
                vpC = &at;
            }
        } else {
            vpA = &at;
            vpB = &right;
            vpC = &up;
        }
    } else {
        vpA = &up;
        vpB = &at;
        vpC = &right;
    }

    RwV3dCrossProduct(vpC, vpA, vpB);
    recip = _rwInvSqrt(RwV3dDotProduct(vpC, vpC));
    RwV3dScale(vpC, vpC, recip);

    RwV3dCrossProduct(vpB, vpC, vpA);
    recip = _rwInvSqrt(RwV3dDotProduct(vpB, vpB));
    RwV3dScale(vpB, vpB, recip);

    dst->right = right;
    dst->up = up;
    dst->at = at;
    dst->pos = pos;

    dst->flags = (dst->flags | rwMATRIXTYPEORTHONORMAL) & ~rwMATRIXINTERNALIDENTITY;

    return dst;
}

RwBool _rwMatrixSetMultFn(rwMatrixMultFn multMat) {
    if (!multMat) {
        multMat = MatrixMultiply;
    }
    RWMATRIXGLOBAL(multiply) = multMat;

    return TRUE;
}

RwReal _rwMatrixDeterminant(const RwMatrix* matrix) {
    RwV3d cross;

    RwV3dCrossProduct(&cross, &matrix->up, &matrix->at);

    return RwV3dDotProduct(&cross, &matrix->right);
}

RwReal _rwMatrixOrthogonalError(const RwMatrix* matrix) {
    RwReal upAt = RwV3dDotProduct(&matrix->up, &matrix->at);
    RwReal atRight = RwV3dDotProduct(&matrix->at, &matrix->right);
    RwReal rightUp = RwV3dDotProduct(&matrix->right, &matrix->up);

    return upAt * upAt + atRight * atRight + rightUp * rightUp;
}

RwReal _rwMatrixNormalError(const RwMatrix* matrix) {
    RwReal rightErr = RwV3dDotProduct(&matrix->right, &matrix->right) - (RwReal)1.0;
    RwReal upErr = RwV3dDotProduct(&matrix->up, &matrix->up) - (RwReal)1.0;
    RwReal atErr = RwV3dDotProduct(&matrix->at, &matrix->at) - (RwReal)1.0;

    return rightErr * rightErr + upErr * upErr + atErr * atErr;
}

void* _rwMatrixClose(void* instance, RwInt32 offset, RwInt32 size) {
    if (RWMATRIXGLOBAL(matrixFreeList)) {
        RwFreeListDestroy(RWMATRIXGLOBAL(matrixFreeList));
        RWMATRIXGLOBAL(matrixFreeList) = NULL;
    }

    matrixModule.numInstances--;

    return instance;
}

RwBool RwEngineSetMatrixTolerances(const RwMatrixTolerance* tolerance);

static const RwMatrixTolerance defaultTolerance = {0.01f, 0.01f, 0.01f};

void* _rwMatrixOpen(void* instance, RwInt32 offset, RwInt32 size) {
    matrixModule.globalsOffset = offset;

    RWMATRIXGLOBAL(matrixFreeList) = RwFreeListCreate(sizeof(RwMatrix), 50, 4);
    if (!RWMATRIXGLOBAL(matrixFreeList)) {
        instance = NULL;
    } else {
        RwMatrixTolerance tolerance;

        RWMATRIXGLOBAL(optimizations) = rwMATRIXINTERNALIDENTITY;
        tolerance = defaultTolerance;
        RWMATRIXGLOBAL(multiply) = MatrixMultiply;
        RwEngineSetMatrixTolerances(&tolerance);

        matrixModule.numInstances++;
    }

    return instance;
}

RwBool RwEngineSetMatrixTolerances(const RwMatrixTolerance* tolerance) {
    RWMATRIXGLOBAL(tolerance) = *tolerance;

    return TRUE;
}

static inline RwReal MatrixIdentityError(const RwMatrix* matrix) {
    RwV3d dr, du, da;

    dr.x = matrix->right.x - (RwReal)1.0;
    dr.y = matrix->right.y;
    dr.z = matrix->right.z;
    du.x = matrix->up.x;
    du.y = matrix->up.y - (RwReal)1.0;
    du.z = matrix->up.z;
    da.x = matrix->at.x;
    da.y = matrix->at.y;
    da.z = matrix->at.z - (RwReal)1.0;
    return RwV3dDotProduct(&dr, &dr) + RwV3dDotProduct(&du, &du) + RwV3dDotProduct(&da, &da) +
           RwV3dDotProduct(&matrix->pos, &matrix->pos);
}

void RwMatrixOptimize(RwMatrix* matrix, const RwMatrixTolerance* tolerance) {
    RwBool normal;
    RwBool orthogonal;
    RwBool orthonormal;
    RwBool identity;
    RwUInt32 flags;

    if (!tolerance) {
        tolerance = &RWMATRIXGLOBAL(tolerance);
    }

    normal = (tolerance->Normal >= _rwMatrixNormalError(matrix));
    orthogonal = (tolerance->Orthogonal >= _rwMatrixOrthogonalError(matrix));
    identity = FALSE;
    orthonormal = normal && orthogonal;

    if (orthonormal) {
        /* The identity test reuses the orthogonality tolerance. */
        if (tolerance->Orthogonal >= MatrixIdentityError(matrix)) {
            identity = TRUE;
        }
    }

    flags = matrix->flags;
    flags = normal ? (flags | 1) : (flags & ~1);
    flags = orthogonal ? (flags | 2) : (flags & ~2);
    flags = identity ? (flags | rwMATRIXINTERNALIDENTITY) : (flags & ~rwMATRIXINTERNALIDENTITY);
    matrix->flags = flags;
}

RwMatrix* RwMatrixUpdate(RwMatrix* matrix) {
    matrix->flags &= ~(rwMATRIXTYPEMASK | rwMATRIXINTERNALIDENTITY);

    return matrix;
}

RwMatrix* RwMatrixMultiply(RwMatrix* matrixOut, const RwMatrix* matrixIn1, const RwMatrix* matrixIn2) {
    RwUInt32 flags1 = matrixIn1->flags;
    RwUInt32 flags2 = matrixIn2->flags;
    rwMatrixGlobals* globals = RWPLUGINOFFSET(rwMatrixGlobals, RwEngineInstance, matrixModule.globalsOffset);
    RwUInt32 identity = globals->optimizations & rwMATRIXINTERNALIDENTITY;

    if (flags1 & identity) {
        *matrixOut = *matrixIn2;
    } else if (flags2 & identity) {
        *matrixOut = *matrixIn1;
    } else {
        globals->multiply(matrixOut, matrixIn1, matrixIn2);
        matrixOut->flags = flags1 & flags2;
    }

    return matrixOut;
}

RwMatrix* RwMatrixOrthoNormalize(RwMatrix* dst, const RwMatrix* src) {
    return MatrixOrthoNormalize(dst, src);
}

RwMatrix* RwMatrixRotateOneMinusCosineSine(RwMatrix* matrix, const RwV3d* unitAxis, RwReal oneMinusCosine,
                                           RwReal sine, RwOpCombineType combineOp) {
    RwMatrix mRot;
    RwMatrix mTmp;
    RwV3d leading;
    RwV3d crossed;
    RwV3d scaled;

    mRot.flags = rwMATRIXTYPEORTHONORMAL;
    mRot.flags &= ~rwMATRIXINTERNALIDENTITY;
    mRot.pos.x = (RwReal)0.0;
    mRot.pos.y = (RwReal)0.0;
    mRot.pos.z = (RwReal)0.0;

    leading.x = (RwReal)1.0 - unitAxis->x * unitAxis->x;
    leading.y = (RwReal)1.0 - unitAxis->y * unitAxis->y;
    leading.z = (RwReal)1.0 - unitAxis->z * unitAxis->z;
    RwV3dScale(&leading, &leading, oneMinusCosine);

    crossed.x = unitAxis->y * unitAxis->z;
    crossed.y = unitAxis->z * unitAxis->x;
    crossed.z = unitAxis->x * unitAxis->y;
    RwV3dScale(&crossed, &crossed, oneMinusCosine);

    RwV3dScale(&scaled, unitAxis, sine);

    mRot.right.x = (RwReal)1.0 - leading.x;
    mRot.right.y = crossed.z + scaled.z;
    mRot.right.z = crossed.y - scaled.y;

    mRot.up.x = crossed.z - scaled.z;
    mRot.up.y = (RwReal)1.0 - leading.y;
    mRot.up.z = crossed.x + scaled.x;

    mRot.at.x = crossed.y + scaled.y;
    mRot.at.y = crossed.x - scaled.x;
    mRot.at.z = (RwReal)1.0 - leading.z;

    switch (combineOp) {
    case rwCOMBINEREPLACE:
        mRot.flags = matrix->flags;
        *matrix = mRot;
        matrix->flags &= ~rwMATRIXINTERNALIDENTITY;
        break;
    case rwCOMBINEPRECONCAT:
        RwMatrixMultiply(&mTmp, &mRot, matrix);
        mTmp.flags = matrix->flags;
        *matrix = mTmp;
        matrix->flags &= ~rwMATRIXINTERNALIDENTITY;
        break;
    case rwCOMBINEPOSTCONCAT:
        RwMatrixMultiply(&mTmp, matrix, &mRot);
        mTmp.flags = matrix->flags;
        *matrix = mTmp;
        matrix->flags &= ~rwMATRIXINTERNALIDENTITY;
        break;
    default:
        RWERROR((E_RW_BADPARAM, "Invalid combination type"));
        matrix = NULL;
        break;
    }

    return matrix;
}

RwMatrix* RwMatrixRotate(RwMatrix* matrix, const RwV3d* axis, RwReal angle, RwOpCombineType combineOp) {
    RwReal sinA;
    RwReal radians = (RwReal)(3.14159265358979323846 / 180.0) * angle;
    RwV3d unitAxis;
    RwReal recip;
    RwReal cosA;

    recip = _rwInvSqrt(RwV3dDotProduct(axis, axis));
    RwV3dScale(&unitAxis, axis, recip);

    sinA = (RwReal)sin(radians);
    cosA = (RwReal)cos(radians);

    RwMatrixRotateOneMinusCosineSine(matrix, &unitAxis, (RwReal)1.0 - cosA, sinA, combineOp);

    return matrix;
}

RwMatrix* RwMatrixInvert(RwMatrix* dst, const RwMatrix* src) {
    if (src->flags & (RWMATRIXGLOBAL(optimizations) & rwMATRIXINTERNALIDENTITY)) {
        *dst = *src;
    } else if ((src->flags & rwMATRIXTYPEMASK) == rwMATRIXTYPEORTHONORMAL) {
        dst->right.x = src->right.x;
        dst->right.y = src->up.x;
        dst->right.z = src->at.x;
        dst->up.x = src->right.y;
        dst->up.y = src->up.y;
        dst->up.z = src->at.y;
        dst->at.x = src->right.z;
        dst->at.y = src->up.z;
        dst->at.z = src->at.z;

        dst->pos.x = -RwV3dDotProduct(&src->pos, &src->right);
        dst->pos.y = -RwV3dDotProduct(&src->pos, &src->up);
        dst->pos.z = -RwV3dDotProduct(&src->pos, &src->at);

        dst->flags = (dst->flags | rwMATRIXTYPEORTHONORMAL) & ~rwMATRIXINTERNALIDENTITY;
    } else {
        RwSplitBits det;
        RwReal recip;

        dst->right.x = src->up.y * src->at.z - src->up.z * src->at.y;
        dst->right.y = -(src->right.y * src->at.z - src->right.z * src->at.y);
        dst->right.z = src->right.y * src->up.z - src->right.z * src->up.y;

        det.nReal = dst->right.x * src->right.x + dst->right.y * src->up.x + dst->right.z * src->at.x;
        recip = det.nInt ? ((RwReal)1.0 / det.nReal) : (RwReal)1.0;

        dst->right.x *= recip;
        dst->right.y *= recip;
        dst->right.z *= recip;

        dst->up.x = recip * -(src->up.x * src->at.z - src->up.z * src->at.x);
        dst->up.y = recip * (src->right.x * src->at.z - src->right.z * src->at.x);
        dst->up.z = recip * -(src->right.x * src->up.z - src->right.z * src->up.x);

        dst->at.x = recip * (src->up.x * src->at.y - src->up.y * src->at.x);
        dst->at.y = recip * -(src->right.x * src->at.y - src->right.y * src->at.x);
        dst->at.z = recip * (src->right.x * src->up.y - src->right.y * src->up.x);

        dst->pos.x = -(src->pos.x * dst->right.x + src->pos.y * dst->up.x + src->pos.z * dst->at.x);
        dst->pos.y = -(src->pos.x * dst->right.y + src->pos.y * dst->up.y + src->pos.z * dst->at.y);
        dst->pos.z = -(src->pos.x * dst->right.z + src->pos.y * dst->up.z + src->pos.z * dst->at.z);

        dst->flags &= ~rwMATRIXINTERNALIDENTITY;
    }

    return dst;
}

RwMatrix* RwMatrixScale(RwMatrix* matrix, const RwV3d* scale, RwOpCombineType combineOp) {
    switch (combineOp) {
    case rwCOMBINEREPLACE:
        RwMatrixSetIdentity(matrix);
        matrix->right.x = scale->x;
        matrix->up.y = scale->y;
        matrix->at.z = scale->z;
        break;
    case rwCOMBINEPRECONCAT:
        RwV3dScale(&matrix->right, &matrix->right, scale->x);
        RwV3dScale(&matrix->up, &matrix->up, scale->y);
        RwV3dScale(&matrix->at, &matrix->at, scale->z);
        break;
    case rwCOMBINEPOSTCONCAT:
        matrix->right.x *= scale->x;
        matrix->right.y *= scale->y;
        matrix->right.z *= scale->z;
        matrix->up.x *= scale->x;
        matrix->up.y *= scale->y;
        matrix->up.z *= scale->z;
        matrix->at.x *= scale->x;
        matrix->at.y *= scale->y;
        matrix->at.z *= scale->z;
        matrix->pos.x *= scale->x;
        matrix->pos.y *= scale->y;
        matrix->pos.z *= scale->z;
        break;
    default:
        RWERROR((E_RW_BADPARAM, "Invalid combination type"));
        matrix = NULL;
        break;
    }

    matrix->flags &= ~(rwMATRIXTYPEMASK | rwMATRIXINTERNALIDENTITY);

    return matrix;
}

RwMatrix* RwMatrixTranslate(RwMatrix* matrix, const RwV3d* translation, RwOpCombineType combineOp) {
    switch (combineOp) {
    case rwCOMBINEREPLACE:
        RwMatrixSetIdentity(matrix);
        matrix->pos.x = translation->x;
        matrix->pos.y = translation->y;
        matrix->pos.z = translation->z;
        break;
    case rwCOMBINEPRECONCAT:
        matrix->pos.x = translation->x * matrix->right.x + translation->y * matrix->up.x +
                        translation->z * matrix->at.x + matrix->pos.x;
        matrix->pos.y = translation->x * matrix->right.y + translation->y * matrix->up.y +
                        translation->z * matrix->at.y + matrix->pos.y;
        matrix->pos.z = translation->x * matrix->right.z + translation->y * matrix->up.z +
                        translation->z * matrix->at.z + matrix->pos.z;
        break;
    case rwCOMBINEPOSTCONCAT:
        matrix->pos.x += translation->x;
        matrix->pos.y += translation->y;
        matrix->pos.z += translation->z;
        break;
    default:
        RWERROR((E_RW_BADPARAM, "Invalid combination type"));
        matrix = NULL;
        break;
    }

    matrix->flags &= ~rwMATRIXINTERNALIDENTITY;

    return matrix;
}

RwMatrix* RwMatrixTransform(RwMatrix* matrix, const RwMatrix* transform, RwOpCombineType combineOp) {
    RwMatrix mTmp;

    mTmp.flags = rwMATRIXTYPENORMAL;

    switch (combineOp) {
    case rwCOMBINEREPLACE:
        *matrix = *transform;
        matrix->flags = transform->flags;
        break;
    case rwCOMBINEPRECONCAT:
        RwMatrixMultiply(&mTmp, transform, matrix);
        mTmp.flags = matrix->flags;
        *matrix = mTmp;
        matrix->flags &= ~(rwMATRIXTYPEMASK | rwMATRIXINTERNALIDENTITY);
        break;
    case rwCOMBINEPOSTCONCAT:
        RwMatrixMultiply(&mTmp, matrix, transform);
        mTmp.flags = matrix->flags;
        *matrix = mTmp;
        matrix->flags &= ~(rwMATRIXTYPEMASK | rwMATRIXINTERNALIDENTITY);
        break;
    default:
        RWERROR((E_RW_BADPARAM, "Invalid combination type"));
        matrix = NULL;
        break;
    }

    return matrix;
}

RwBool RwMatrixDestroy(RwMatrix* mpMat) {
    RwFreeListFree(RWMATRIXGLOBAL(matrixFreeList), mpMat);

    return TRUE;
}

RwMatrix* RwMatrixCreate(void) {
    RwMatrix* matrix = (RwMatrix*)RwFreeListAlloc(RWMATRIXGLOBAL(matrixFreeList));

    if (matrix) {
        matrix->flags = rwMATRIXTYPEORTHONORMAL;
        RwMatrixSetIdentity(matrix);
    }

    return matrix;
}
