/*
 * Lights: creation, colour and cone angle, and reading the light chunk.
 */

#include "rwsdk/rwcore.h"
#include "math.h"

#define rwPLUGIN_ID 2 /* world */

#define rpLIGHT 3
#define rpLIGHTLIGHTATOMICS 0x01
#define rpLIGHTLIGHTWORLD 0x02
#define rpLIGHTPRIVATENOCHROMA 0x01

typedef struct RwRGBAReal {
    RwReal red;
    RwReal green;
    RwReal blue;
    RwReal alpha;
} RwRGBAReal;

typedef struct RpLight {
    RwObjectHasFrame object;
    RwReal radius;
    RwRGBAReal color;
    RwReal minusCosAngle;
    RwLinkList WorldSectorsInLight;
    RwLLLink inWorld;
    RwUInt16 lightFrame;
    RwUInt16 pad;
} RpLight;

typedef struct _rpLight {
    RwReal radius;
    RwReal red;
    RwReal green;
    RwReal blue;
    RwReal minusCosAngle;
    RwUInt32 typeAndFlags;
} _rpLight;

typedef union RwSplitBits {
    RwReal nReal;
    RwInt32 nInt;
    RwUInt32 nUInt;
} RwSplitBits;

RwPluginRegistry lightTKList = { sizeof(RpLight), sizeof(RpLight), 0, 0, NULL, NULL };

RwModuleInfo lightModule;

#define RWLIGHTGLOBAL(var) (RWPLUGINOFFSET(RwFreeList*, RwEngineInstance, lightModule.globalsOffset)[0])

#define rwLLLinkInitialize(linkvar) ((linkvar)->prev = NULL, (linkvar)->next = NULL)

void* memset(void* dst, int c, unsigned long n);
RwUInt32 RwStreamRead(RwStream* stream, void* buffer, RwUInt32 length);
void* RwMemNative32(void* mem, RwUInt32 size);
void* RwMemFloat32ToReal(void* mem, RwUInt32 size);
RwInt32 _rwPluginRegistryAddPluginStream(RwPluginRegistry* reg, RwUInt32 pluginID,
                                         RwPluginDataChunkReadCallBack readCB,
                                         RwPluginDataChunkWriteCallBack writeCB,
                                         RwPluginDataChunkGetSizeCallBack getSizeCB);
const RwPluginRegistry* _rwPluginRegistryReadDataChunks(const RwPluginRegistry* reg, RwStream* stream,
                                                        void* object);
RwFrame* RwFrameUpdateObjects(RwFrame* frame);
void _rwObjectHasFrameReleaseFrame(void* object);
RwReal _rwSqrt(const RwReal num);
RwBool RpLightDestroy(RpLight* light);
RpLight* RpLightCreate(RwInt32 type);

static void LightTidyDestroyLight(void* light, void* data) {
    RpLightDestroy((RpLight*)light);
}

static RwObjectHasFrame* LightSync(RwObjectHasFrame* object) {
    return object;
}

RpLight* RpLightSetRadius(RpLight* light, RwReal radius) {
    RwFrame* frame = (RwFrame*)rwObjectGetParent(light);

    light->radius = radius;
    if (frame) {
        RwFrameUpdateObjects(frame);
    }

    return light;
}

RpLight* RpLightSetColor(RpLight* light, const RwRGBAReal* color) {
    light->color = *color;

    if (light->color.red == light->color.green && light->color.red == light->color.blue) {
        rwObjectSetPrivateFlags(light, rpLIGHTPRIVATENOCHROMA);
    } else {
        rwObjectSetPrivateFlags(light, 0);
    }

    return light;
}

RwReal RpLightGetConeAngle(const RpLight* light) {
    RwReal x = -light->minusCosAngle;
    RwSplitBits bits;
    RwInt32 hx;
    RwInt32 ix;

    bits.nReal = x;
    hx = bits.nInt;
    ix = hx & 0x7FFFFFFF;

    if (ix >= 0x3F800000) {
        if (hx > 0) {
            return (RwReal)0.0;
        }

        return (RwReal)3.14159274;
    }

    if (ix < 0x3F000000) {
        RwReal z;
        RwReal p;
        RwReal q;

        if (ix <= 0x23000000) {
            return (RwReal)1.57079637;
        }

        z = x * x;
        p = z * (1.6666667163e-01f +
                 z * (-3.2556581497e-01f +
                      z * (2.0121252537e-01f +
                           z * (-4.0055535734e-02f + z * (7.9153501429e-04f + z * 3.4793309169e-05f)))));
        q = 1.0f + z * (-2.4033949375e+00f + z * (2.0209457874e+00f + z * (-6.8828397989e-01f + z * 7.7038154006e-02f)));

        return 1.5707962513e+00f - (x - (7.5497894159e-08f - x * (p / q)));
    } else if (hx < 0) {
        RwReal z = (1.0f + x) * 0.5f;
        RwReal s = _rwSqrt(z);
        RwReal p = z * (1.6666667163e-01f +
                        z * (-3.2556581497e-01f +
                             z * (2.0121252537e-01f +
                                  z * (-4.0055535734e-02f + z * (7.9153501429e-04f + z * 3.4793309169e-05f)))));
        RwReal q =
            1.0f + z * (-2.4033949375e+00f + z * (2.0209457874e+00f + z * (-6.8828397989e-01f + z * 7.7038154006e-02f)));
        RwReal r = p / q;
        RwReal w = r * s - 7.5497894159e-08f;

        return 3.1415925026e+00f - 2.0f * (s + w);
    } else {
        RwReal z = (1.0f - x) * 0.5f;
        RwReal s = _rwSqrt(z);
        RwSplitBits sBits;
        RwSplitBits df;
        RwReal c;
        RwReal p;
        RwReal q;

        sBits.nReal = s;
        df.nUInt = sBits.nUInt & 0xFFFFF000;
        c = (z - df.nReal * df.nReal) / (s + df.nReal);
        p = z * (1.6666667163e-01f +
                 z * (-3.2556581497e-01f +
                      z * (2.0121252537e-01f +
                           z * (-4.0055535734e-02f + z * (7.9153501429e-04f + z * 3.4793309169e-05f)))));
        q = 1.0f + z * (-2.4033949375e+00f + z * (2.0209457874e+00f + z * (-6.8828397989e-01f + z * 7.7038154006e-02f)));

        {
            RwReal r = p / q;

            return 2.0f * (df.nReal + (r * s + c));
        }
    }
}

RpLight* RpLightSetConeAngle(RpLight* light, RwReal angle) {
    if (angle < (RwReal)0.0 || angle > (RwReal)89.9999) {
        return NULL;
    }

    light->minusCosAngle = -(RwReal)cos(angle);

    return light;
}

RwInt32 RpLightRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB,
                              RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB) {
    return _rwPluginRegistryAddPlugin(&lightTKList, size, pluginID, constructCB, destructCB, copyCB);
}

RwInt32 RpLightRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB,
                                    RwPluginDataChunkWriteCallBack writeCB,
                                    RwPluginDataChunkGetSizeCallBack getSizeCB) {
    return _rwPluginRegistryAddPluginStream(&lightTKList, pluginID, readCB, writeCB, getSizeCB);
}

RpLight* RpLightStreamRead(RwStream* stream) {
    RwUInt32 size;
    RwUInt32 version;

    if (!RwStreamFindChunk(stream, rwID_STRUCT, &size, &version)) {
        return NULL;
    }

    if (version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION) {
        _rpLight lt;
        RpLight* light;

        memset(&lt, 0, sizeof(lt));
        if (RwStreamRead(stream, &lt, size) != size) {
            return NULL;
        }

        RwMemNative32(&lt, sizeof(lt));

        light = RpLightCreate((lt.typeAndFlags >> 16) & 0xFF);
        if (!light) {
            return NULL;
        }

        RwMemFloat32ToReal(&lt.radius, sizeof(lt.radius));
        RwMemFloat32ToReal(&lt.red, sizeof(lt.red));
        RwMemFloat32ToReal(&lt.green, sizeof(lt.green));
        RwMemFloat32ToReal(&lt.blue, sizeof(lt.blue));
        RwMemFloat32ToReal(&lt.minusCosAngle, sizeof(lt.minusCosAngle));

        light->radius = lt.radius;
        light->color.red = lt.red;
        light->color.green = lt.green;
        light->color.blue = lt.blue;

        if (version < 0x30300) {
            light->minusCosAngle =
                -_rwSqrt((RwReal)1.0 / ((RwReal)1.0 + lt.minusCosAngle * lt.minusCosAngle));
        } else {
            light->minusCosAngle = lt.minusCosAngle;
        }

        if (light->color.red == light->color.green && light->color.red == light->color.blue) {
            rwObjectSetPrivateFlags(light, rpLIGHTPRIVATENOCHROMA);
        } else {
            rwObjectSetPrivateFlags(light, 0);
        }

        ((RwObject*)light)->flags = (RwUInt8)lt.typeAndFlags;

        if (!_rwPluginRegistryReadDataChunks(&lightTKList, stream, light)) {
            return NULL;
        }

        return light;
    }

    RWERROR((E_RW_BADVERSION));
    return NULL;
}

RwBool RpLightDestroy(RpLight* light) {
    _rwPluginRegistryDeInitObject(&lightTKList, light);
    _rwObjectHasFrameReleaseFrame(light);
    RwFreeListFree(RWLIGHTGLOBAL(lightFreeList), light);

    return TRUE;
}

RpLight* RpLightCreate(RwInt32 type) {
    RpLight* light = (RpLight*)RwFreeListAlloc(RWLIGHTGLOBAL(lightFreeList));

    if (!light) {
        return NULL;
    }

    rwObjectInitialize(light, rpLIGHT, type);
    light->object.sync = LightSync;

    light->radius = (RwReal)0.0;
    light->minusCosAngle = (RwReal)1.0;
    light->color.red = (RwReal)1.0;
    light->color.green = (RwReal)1.0;
    light->color.blue = (RwReal)1.0;
    light->color.alpha = (RwReal)1.0;
    rwObjectSetPrivateFlags(light, rpLIGHTPRIVATENOCHROMA);

    rwLinkListInitialize(&light->WorldSectorsInLight);
    rwLLLinkInitialize(&light->inWorld);
    light->lightFrame = RWSRCGLOBAL(lightFrame) - 1;

    ((RwObject*)light)->flags = rpLIGHTLIGHTATOMICS | rpLIGHTLIGHTWORLD;

    _rwPluginRegistryInitObject(&lightTKList, light);

    return light;
}

void* _rpLightClose(void* instance, RwInt32 offset, RwInt32 size) {
    RwFreeListForAllUsed(RWLIGHTGLOBAL(lightFreeList), LightTidyDestroyLight, NULL);
    RwFreeListDestroy(RWLIGHTGLOBAL(lightFreeList));
    RWLIGHTGLOBAL(lightFreeList) = NULL;

    lightModule.numInstances--;

    return instance;
}

void* _rpLightOpen(void* instance, RwInt32 offset, RwInt32 size) {
    lightModule.globalsOffset = offset;

    RWLIGHTGLOBAL(lightFreeList) = RwFreeListCreate(lightTKList.sizeOfStruct, 20, 0);
    if (RWLIGHTGLOBAL(lightFreeList)) {
        lightModule.numInstances++;
        return instance;
    }

    return NULL;
}
