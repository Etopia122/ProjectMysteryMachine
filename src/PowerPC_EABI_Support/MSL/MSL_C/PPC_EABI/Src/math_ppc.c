#include "math_ppc.h"
#include "fdlibm.h"

#if defined(SCOOBY_MSL)
float atanf(float x) {
    return atan(x);
}

float cosf(float x) {
    return cos(x);
}

float sinf(float x) {
    return sin(x);
}

float powf(float x, float y) {
    return pow(x, y);
}
#else
__declspec(weak) float acosf(float x) {
    return acos(x);
}

__declspec(weak) float cosf(float x) {
    return cos(x);
}

__declspec(weak) float sinf(float x) {
    return sin(x);
}

__declspec(weak) float tanf(float x) {
    return tan(x);
}
#endif
