#include "float.h"

unsigned long __float_nan[]     = { 0x7FFFFFFF };
unsigned long __float_huge[]    = { 0x7F800000 };
#if defined(SCOOBY_MSL)
unsigned long __double_min[]    = { 0x00100000, 0 };
#endif
#if VERSION == VERSION_DEMO || defined(SCOOBY_MSL)
unsigned long __double_max[]    = { 0x7FEFFFFF, 0xFFFFFFFF };
unsigned long __double_huge[]   = { 0x7FF00000, 0 };
#if defined(SCOOBY_MSL)
#pragma force_active on
#endif
unsigned long __extended_min[]  = { 0x00100000, 0 };
unsigned long __extended_max[]  = { 0x7FEFFFFF, 0xFFFFFFFF };
#if defined(SCOOBY_MSL)
#pragma force_active reset
#endif
#endif
