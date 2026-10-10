/*
 * GameCube device layer synchronisation and math: draw sync token tests for
 * the display queue, and the paired single matrix multiply and point/vector
 * transforms installed into the core matrix and vector modules.
 */

#include "rwsdk/rwcore.h"

RwUInt16 GXReadDrawSync(void);

volatile RwUInt16 lbl_8025E800 = 1;
volatile RwUInt16 lbl_8025F890;

RwBool _rwDlTokenDone(RwUInt16 token)
{
    RwBool done;
    RwUInt16 current;

    current = GXReadDrawSync();
    lbl_8025F890 = current;
    if (lbl_8025E800 >= current) {
        done = FALSE;
        if (token <= lbl_8025F890 || token > lbl_8025E800) {
            done = TRUE;
        }
    } else {
        done = FALSE;
        if (token > lbl_8025E800 && token <= lbl_8025F890) {
            done = TRUE;
        }
    }
    return done;
}

asm void _rwDlMatrixMultiply(register RwMatrix* dst, register const RwMatrix* matA, register const RwMatrix* matB)
{
    nofralloc
    stwu r1, -0x58(r1)
    stfd f14, 0x8(r1)
    stfd f15, 0x10(r1)
    stfd f16, 0x18(r1)
    stfd f17, 0x20(r1)
    stfd f18, 0x28(r1)
    stfd f19, 0x30(r1)
    stfd f20, 0x38(r1)
    stfd f21, 0x40(r1)
    stfd f22, 0x48(r1)
    stfd f23, 0x50(r1)
    psq_l f0, 0x0(r4), 0, 0
    psq_l f1, 0x8(r4), 1, 0
    psq_l f2, 0x10(r4), 0, 0
    psq_l f3, 0x18(r4), 1, 0
    psq_l f4, 0x20(r4), 0, 0
    psq_l f5, 0x28(r4), 1, 0
    psq_l f6, 0x30(r4), 0, 0
    psq_l f7, 0x38(r4), 1, 0
    psq_l f8, 0x0(r5), 0, 0
    psq_l f9, 0x8(r5), 1, 0
    psq_l f10, 0x10(r5), 0, 0
    psq_l f11, 0x18(r5), 1, 0
    psq_l f12, 0x20(r5), 0, 0
    psq_l f13, 0x28(r5), 1, 0
    psq_l f14, 0x30(r5), 0, 0
    psq_l f15, 0x38(r5), 1, 0
    ps_muls0 f16, f8, f0
    ps_muls0 f18, f8, f2
    ps_muls0 f20, f8, f4
    ps_madds0 f22, f8, f6, f14
    ps_madds1 f16, f10, f0, f16
    ps_madds1 f18, f10, f2, f18
    ps_madds1 f20, f10, f4, f20
    ps_madds1 f22, f10, f6, f22
    ps_madds0 f16, f12, f1, f16
    ps_madds0 f18, f12, f3, f18
    ps_madds0 f20, f12, f5, f20
    ps_madds0 f22, f12, f7, f22
    ps_muls0 f17, f9, f0
    ps_muls0 f19, f9, f2
    ps_muls0 f21, f9, f4
    ps_madds0 f23, f9, f6, f15
    ps_madds1 f17, f11, f0, f17
    ps_madds1 f19, f11, f2, f19
    ps_madds1 f21, f11, f4, f21
    ps_madds1 f23, f11, f6, f23
    ps_madds0 f17, f13, f1, f17
    ps_madds0 f19, f13, f3, f19
    ps_madds0 f21, f13, f5, f21
    ps_madds0 f23, f13, f7, f23
    psq_st f16, 0x0(r3), 0, 0
    psq_st f17, 0x8(r3), 1, 0
    psq_st f18, 0x10(r3), 0, 0
    psq_st f19, 0x18(r3), 1, 0
    psq_st f20, 0x20(r3), 0, 0
    psq_st f21, 0x28(r3), 1, 0
    psq_st f22, 0x30(r3), 0, 0
    psq_st f23, 0x38(r3), 1, 0
    lfd f14, 0x8(r1)
    lfd f15, 0x10(r1)
    lfd f16, 0x18(r1)
    lfd f17, 0x20(r1)
    lfd f18, 0x28(r1)
    lfd f19, 0x30(r1)
    lfd f20, 0x38(r1)
    lfd f21, 0x40(r1)
    lfd f22, 0x48(r1)
    lfd f23, 0x50(r1)
    addi r1, r1, 0x58
    blr
}

asm void _rwDlV3dTransformPoints(register RwV3d* pointsOut, register const RwV3d* pointsIn, register RwInt32 numPoints,
                                 register const RwMatrix* matrix)
{
    nofralloc
    cmpwi r5, 0x0
    mtctr r5
    addi r7, r3, -0x4
    addi r4, r4, -0x4
    blelr
    psq_l f0, 0x0(r6), 0, 0
    psq_l f1, 0x8(r6), 1, 0
    psq_l f2, 0x10(r6), 0, 0
    psq_l f3, 0x18(r6), 1, 0
    psq_l f4, 0x20(r6), 0, 0
    psq_l f5, 0x28(r6), 1, 0
    psq_l f6, 0x30(r6), 0, 0
    psq_l f7, 0x38(r6), 1, 0
    psq_lu f8, 0x4(r4), 1, 0
    psq_lu f9, 0x4(r4), 1, 0
    psq_lu f10, 0x4(r4), 1, 0
L_8017DDA0:
    ps_madds0 f11, f0, f8, f6
    ps_madds0 f12, f1, f8, f7
    psq_lu f8, 0x4(r4), 1, 0
    ps_madds0 f11, f2, f9, f11
    ps_madds0 f12, f3, f9, f12
    psq_lu f9, 0x4(r4), 1, 0
    ps_madds0 f11, f4, f10, f11
    ps_madds0 f12, f5, f10, f12
    psq_lu f10, 0x4(r4), 1, 0
    ps_merge11 f13, f11, f11
    psq_stu f11, 0x4(r7), 1, 0
    psq_stu f13, 0x4(r7), 1, 0
    psq_stu f12, 0x4(r7), 1, 0
    bdnz L_8017DDA0
    blr
}

asm void _rwDlV3dTransformVectors(register RwV3d* vectorsOut, register const RwV3d* vectorsIn,
                                  register RwInt32 numPoints, register const RwMatrix* matrix)
{
    nofralloc
    cmpwi r5, 0x0
    mtctr r5
    addi r7, r3, -0x4
    addi r4, r4, -0x4
    blelr
    psq_l f0, 0x0(r6), 0, 0
    psq_l f1, 0x8(r6), 1, 0
    psq_l f2, 0x10(r6), 0, 0
    psq_l f3, 0x18(r6), 1, 0
    psq_l f4, 0x20(r6), 0, 0
    psq_l f5, 0x28(r6), 1, 0
    psq_l f6, 0x30(r6), 0, 0
    psq_l f7, 0x38(r6), 1, 0
    psq_lu f8, 0x4(r4), 1, 0
    psq_lu f9, 0x4(r4), 1, 0
    psq_lu f10, 0x4(r4), 1, 0
L_8017DE1C:
    ps_muls0 f11, f0, f8
    ps_muls0 f12, f1, f8
    psq_lu f8, 0x4(r4), 1, 0
    ps_madds0 f11, f2, f9, f11
    ps_madds0 f12, f3, f9, f12
    psq_lu f9, 0x4(r4), 1, 0
    ps_madds0 f11, f4, f10, f11
    ps_madds0 f12, f5, f10, f12
    psq_lu f10, 0x4(r4), 1, 0
    ps_merge11 f13, f11, f11
    psq_stu f11, 0x4(r7), 1, 0
    psq_stu f13, 0x4(r7), 1, 0
    psq_stu f12, 0x4(r7), 1, 0
    bdnz L_8017DE1C
    blr
}
