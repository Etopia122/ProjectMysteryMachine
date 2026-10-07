#ifndef PC_NAMES_H
#define PC_NAMES_H

// Real names for library functions the decomp still calls by address
// (fn_XXXXXXXX). On GameCube they resolve to assembly; on PC they must bind to
// the replacement libraries, so map them to their RenderWare/SDK names here.
// Force-included after pc_compat.h. Identified from arguments and call sites.

#define fn_8013D854 RpHAnimPluginAttach
#define fn_8016274C RwMatrixInvert
#define fn_80162B60 RwMatrixScale
#define fn_80162DA0 RwMatrixTranslate
#define fn_8016356C RwFreeListDestroy
#define fn_80163610 RwFreeListCreate
#define fn_80164200 RwResourcesGetArenaSize
#define fn_8018B9A0 RxRenderStateVectorSetDefaultRenderStateVector
#define fn_80151CA4 RpWorldSetSurfaceProperties
#define fn_80135484 CARDOpen
#define fn_8013558C CARDClose

#endif // PC_NAMES_H
