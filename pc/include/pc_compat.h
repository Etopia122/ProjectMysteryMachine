#ifndef PC_COMPAT_H
#define PC_COMPAT_H

// Force-included into every game source when building for PC (-include pc_compat.h).
// Maps CodeWarrior/GameCube compiler intrinsics onto portable equivalents.

#ifdef __cplusplus
extern "C" {
#endif

// MWCC PowerPC intrinsics used by the game code.
static inline double __fabs(double x) { return __builtin_fabs(x); }
static inline float __fabsf(float x) { return __builtin_fabsf(x); }
// frsqrte is only an estimate on hardware; an exact value is fine on PC.
static inline double __frsqrte(double x) { return 1.0 / __builtin_sqrt(x); }

#ifdef __cplusplus
}
#endif

// MSL's stdio objects (__files) don't exist on PC; use the host's streams.
#ifdef PC_PORT
#include <stdio.h>
#define PC_STDERR_OR(x) stderr
#else
#define PC_STDERR_OR(x) (x)
#endif

#include "pc_names.h"

#endif // PC_COMPAT_H
