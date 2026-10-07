/*
 * Engine: start-up and shut-down, the core module registrations and the
 * requests passed down to the device.
 */

#include "rwsdk/rwcore.h"

#define rwPLUGIN_ID 1 /* core */

#define E_RW_BADENGINESTATE 0x80000001
#define E_RW_NULLP 0x80000016
#define E_RW_DEVICEERROR 0x18

#define rwENGINEINITNOFREELISTS 0x1

typedef enum RwEngineStatus {
    rwENGINESTATUSIDLE,
    rwENGINESTATUSINITED,
    rwENGINESTATUSOPENED,
    rwENGINESTATUSSTARTED,
} RwEngineStatus;

typedef enum RwDeviceSystemRequest {
    rwDEVICESYSTEMOPEN = 0x00,
    rwDEVICESYSTEMCLOSE = 0x01,
    rwDEVICESYSTEMSTART = 0x02,
    rwDEVICESYSTEMSTOP = 0x03,
    rwDEVICESYSTEMREGISTER = 0x04,
    rwDEVICESYSTEMGETMODEINFO = 0x06,
    rwDEVICESYSTEMGETMODE = 0x0A,
    rwDEVICESYSTEMSTANDARDS = 0x0B,
    rwDEVICESYSTEMFINALIZESTART = 0x11,
    rwDEVICESYSTEMINITIATESTOP = 0x12,
} RwDeviceSystemRequest;

typedef RwBool (*RwSystemFunc)(RwInt32 request, void* pOut, void* pInOut, RwInt32 nIn);

typedef struct RwVideoMode RwVideoMode;

typedef struct RwEngineOpenParams {
    void* displayID;
} RwEngineOpenParams;

void* memcpy(void* dst, const void* src, unsigned long n);

RwBool _rwStringOpen(void);
RwBool _rwStringClose(void);
RwBool _rwMemoryOpen(const RwMemoryFunctions* memFuncs);
void _rwMemoryClose(void);
RwBool _rwFileSystemOpen(void);
RwBool _rwFileSystemClose(void);
RwBool _rwPluginRegistryOpen(void);
RwBool _rwPluginRegistryClose(void);
RwInt32 _rwPluginRegistryGetPluginOffset(const RwPluginRegistry* reg, RwUInt32 pluginID);
RwFreeList* _rwFreeListFreeReal(RwFreeList* freelist, void* pData);
RwDevice* _rwDeviceGetHandle(void);
RwBool _rwPipeAttach(void);
RwBool RwImageSetGamma(RwReal gammaValue);

void* _rwErrorOpen(void* instance, RwInt32 offset, RwInt32 size);
void* _rwErrorClose(void* instance, RwInt32 offset, RwInt32 size);
void* _rwVectorOpen(void* instance, RwInt32 offset, RwInt32 size);
void* _rwVectorClose(void* instance, RwInt32 offset, RwInt32 size);
void* _rwDebugOpen(void* instance, RwInt32 offset, RwInt32 size);
void* _rwDebugClose(void* instance, RwInt32 offset, RwInt32 size);
void* _rwMatrixOpen(void* instance, RwInt32 offset, RwInt32 size);
void* _rwMatrixClose(void* instance, RwInt32 offset, RwInt32 size);
void* _rwFrameOpen(void* instance, RwInt32 offset, RwInt32 size);
void* _rwFrameClose(void* instance, RwInt32 offset, RwInt32 size);
void* _rwStreamModuleOpen(void* instance, RwInt32 offset, RwInt32 size);
void* _rwStreamModuleClose(void* instance, RwInt32 offset, RwInt32 size);
void* _rwCameraOpen(void* instance, RwInt32 offset, RwInt32 size);
void* _rwCameraClose(void* instance, RwInt32 offset, RwInt32 size);
void* _rwImageOpen(void* instance, RwInt32 offset, RwInt32 size);
void* _rwImageClose(void* instance, RwInt32 offset, RwInt32 size);
void* _rwRasterOpen(void* instance, RwInt32 offset, RwInt32 size);
void* _rwRasterClose(void* instance, RwInt32 offset, RwInt32 size);
void* _rwTextureOpen(void* instance, RwInt32 offset, RwInt32 size);
void* _rwTextureClose(void* instance, RwInt32 offset, RwInt32 size);
void* _rwRenderPipelineOpen(void* instance, RwInt32 offset, RwInt32 size);
void* _rwRenderPipelineClose(void* instance, RwInt32 offset, RwInt32 size);
void* _rwIm3DOpen(void* instance, RwInt32 offset, RwInt32 size);
void* _rwIm3DClose(void* instance, RwInt32 offset, RwInt32 size);
void* _rwResourcesOpen(void* instance, RwInt32 offset, RwInt32 size);
void* _rwResourcesClose(void* instance, RwInt32 offset, RwInt32 size);

RwPluginRegistry engineTKList = { sizeof(RwGlobals), sizeof(RwGlobals), 0, 0, NULL, NULL };

static RwGlobals staticGlobals;

void* RwEngineInstance;
static RwUInt32 engineInstances;

static void* MallocWrapper(RwFreeList* freelist) {
    return RwMalloc(freelist->entrySize);
}

static RwFreeList* FreeWrapper(RwFreeList* freelist, void* pData) {
    RwFree(pData);

    return freelist;
}

RwUInt32 _rwGetNumEngineInstances(void) {
    return engineInstances;
}

RwInt32 RwEngineGetVersion(void) {
    return rwLIBRARYCURRENTVERSION;
}

RwInt32 RwEngineRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor initCB,
                               RwPluginObjectDestructor termCB) {
    return _rwPluginRegistryAddPlugin(&engineTKList, size, pluginID, initCB, termCB, NULL);
}

RwInt32 RwEngineGetPluginOffset(RwUInt32 pluginID) {
    return _rwPluginRegistryGetPluginOffset(&engineTKList, pluginID);
}

static inline RwBool _rwDeviceSystemRequest(RwDevice* device, RwInt32 requestID, void* pOut, void* pInOut, RwInt32 numIn) {
    RwBool status = ((RwSystemFunc)device->fpSystem)(requestID, pOut, pInOut, numIn);

    /* These two are optional for a device. */
    if (!status && (requestID == rwDEVICESYSTEMFINALIZESTART || requestID == rwDEVICESYSTEMINITIATESTOP)) {
        status = TRUE;
    }

    if (!status) {
        RWERROR((E_RW_DEVICEERROR, requestID));
    }

    return status;
}

RwVideoMode* RwEngineGetVideoModeInfo(RwVideoMode* modeinfo, RwInt32 modeIndex) {
    if (!_rwDeviceSystemRequest(&RWSRCGLOBAL(dOpenDevice), rwDEVICESYSTEMGETMODEINFO, modeinfo, NULL, modeIndex)) {
        modeinfo = NULL;
    }

    return modeinfo;
}

RwInt32 RwEngineGetCurrentVideoMode(void) {
    RwInt32 currentMode;
    RwBool status = _rwDeviceSystemRequest(&RWSRCGLOBAL(dOpenDevice), rwDEVICESYSTEMGETMODE, &currentMode, NULL, 0);

    return status ? currentMode : -1;
}

RwBool RwEngineStop(void) {
    RwDevice* device = &RWSRCGLOBAL(dOpenDevice);
    RwBool status;

    _rwDeviceSystemRequest(device, rwDEVICESYSTEMINITIATESTOP, NULL, NULL, 0);

    _rwPluginRegistryDeInitObject(&engineTKList, RwEngineInstance);

    status = _rwDeviceSystemRequest(device, rwDEVICESYSTEMSTOP, NULL, NULL, 0);
    if (status) {
        RWSRCGLOBAL(engineStatus) = rwENGINESTATUSOPENED;
    }

    return status;
}

RwBool RwEngineStart(void) {
    RwDevice* device = &RWSRCGLOBAL(dOpenDevice);

    if (_rwDeviceSystemRequest(device, rwDEVICESYSTEMSTART, NULL, NULL, 0)) {
        if (_rwPluginRegistryInitObject(&engineTKList, RwEngineInstance)) {
            RwImageSetGamma(RWSRCGLOBAL(dOpenDevice).gammaCorrection);

            _rwDeviceSystemRequest(device, rwDEVICESYSTEMFINALIZESTART, NULL, NULL, 0);

            RWSRCGLOBAL(engineStatus) = rwENGINESTATUSSTARTED;
            return TRUE;
        }

        _rwDeviceSystemRequest(device, rwDEVICESYSTEMSTOP, NULL, NULL, 0);
    }

    return FALSE;
}

RwBool RwEngineClose(void) {
    RwBool status = _rwDeviceSystemRequest(&RWSRCGLOBAL(dOpenDevice), rwDEVICESYSTEMCLOSE, NULL, NULL, 0);

    if (status) {
        void* instance = RwEngineInstance;

        RwEngineInstance = &staticGlobals;
        memcpy(RwEngineInstance, instance, sizeof(RwGlobals));
        RwFree(instance);

        engineInstances--;
        RWSRCGLOBAL(engineStatus) = rwENGINESTATUSINITED;
    }

    return status;
}

static inline RwBool EngineOpenInstance(RwDevice* device, RwEngineOpenParams* initParams) {
    void* instance;

    instance = RwEngineInstance = RwMalloc(engineTKList.sizeOfStruct);
    if (instance) {
        memcpy(RwEngineInstance, &staticGlobals, sizeof(RwGlobals));

        _rwDeviceSystemRequest(device, rwDEVICESYSTEMREGISTER, &RWSRCGLOBAL(dOpenDevice), &RWSRCGLOBAL(memoryFuncs),
                               0);

        if (_rwDeviceSystemRequest(device, rwDEVICESYSTEMOPEN, NULL, initParams, 0)) {
            _rwDeviceSystemRequest(device, rwDEVICESYSTEMSTANDARDS, RWSRCGLOBAL(stdFunc), NULL, 29);

            engineInstances++;
            return TRUE;
        }

        RwEngineInstance = &staticGlobals;
        memcpy(&staticGlobals, instance, sizeof(RwGlobals));
        RwFree(instance);

        return FALSE;
    }

    RWERROR((E_RW_NOMEM, engineTKList.sizeOfStruct));
    return FALSE;
}

RwBool RwEngineOpen(RwEngineOpenParams* initParams) {
    RwBool status;

    if (!RwEngineInstance) {
        RwEngineInstance = &staticGlobals;
    }

    status = (RWSRCGLOBAL(engineStatus) == rwENGINESTATUSINITED);
    if (status) {
        status = (initParams != NULL);
        if (status) {
            RwDevice* device = _rwDeviceGetHandle();

            status = (device != NULL);
            if (status) {
                status = EngineOpenInstance(device, initParams);
                if (status) {
                    RWSRCGLOBAL(engineStatus) = rwENGINESTATUSOPENED;
                }
            }
        } else {
            RWERROR((E_RW_NULLP));
        }
    } else {
        RWERROR((E_RW_BADENGINESTATE));
    }

    return status;
}

RwBool RwEngineTerm(void) {
    RwBool status = (engineInstances == 0);

    if (status) {
        _rwPluginRegistryClose();
        _rwFileSystemClose();
        _rwMemoryClose();

        RWSRCGLOBAL(engineStatus) = rwENGINESTATUSIDLE;
    }

    return status;
}

RwBool RwEngineInit(RwMemoryFunctions* memFuncs, RwUInt32 initFlags, RwUInt32 resArenaSize) {
    RwBool result = FALSE;

    RwEngineInstance = &staticGlobals;

    if (initFlags & rwENGINEINITNOFREELISTS) {
        RWSRCGLOBAL(memoryAlloc) = MallocWrapper;
        RWSRCGLOBAL(memoryFree) = FreeWrapper;
    } else {
        RWSRCGLOBAL(memoryAlloc) = _rwFreeListAllocReal;
        RWSRCGLOBAL(memoryFree) = _rwFreeListFreeReal;
    }

    RWSRCGLOBAL(resArenaInitSize) = resArenaSize;

    if (RWSRCGLOBAL(engineStatus) == rwENGINESTATUSIDLE && (result = _rwStringOpen())) {
        if ((result = _rwMemoryOpen(memFuncs))) {
            if ((result = _rwFileSystemOpen())) {
                if ((result = _rwPluginRegistryOpen())) {
                    RwInt32 offset;
                    RwBool status;

                    offset = _rwPluginRegistryAddPlugin(&engineTKList, 0x08, 0x40F, _rwErrorOpen, _rwErrorClose, NULL);
                    offset |= _rwPluginRegistryAddPlugin(&engineTKList, 0x10, 0x401, _rwVectorOpen, _rwVectorClose, NULL);
                    offset |= _rwPluginRegistryAddPlugin(&engineTKList, 0x00, 0x40D, _rwDebugOpen, _rwDebugClose, NULL);
                    offset |= _rwPluginRegistryAddPlugin(&engineTKList, 0x18, 0x402, _rwMatrixOpen, _rwMatrixClose, NULL);
                    offset |= _rwPluginRegistryAddPlugin(&engineTKList, 0x04, 0x403, _rwFrameOpen, _rwFrameClose, NULL);
                    offset |= _rwPluginRegistryAddPlugin(&engineTKList, 0x04, 0x404, _rwStreamModuleOpen,
                                                         _rwStreamModuleClose, NULL);
                    offset |= _rwPluginRegistryAddPlugin(&engineTKList, 0x04, 0x405, _rwCameraOpen, _rwCameraClose, NULL);
                    offset |= _rwPluginRegistryAddPlugin(&engineTKList, 0x220, 0x406, _rwImageOpen, _rwImageClose, NULL);
                    offset |= _rwPluginRegistryAddPlugin(&engineTKList, 0x64, 0x407, _rwRasterOpen, _rwRasterClose, NULL);
                    offset |= _rwPluginRegistryAddPlugin(&engineTKList, 0x30, 0x408, _rwTextureOpen, _rwTextureClose, NULL);
                    offset |= _rwPluginRegistryAddPlugin(&engineTKList, 0x64, 0x409, _rwRenderPipelineOpen,
                                                         _rwRenderPipelineClose, NULL);
                    offset |= _rwPipeAttach();
                    offset |= _rwPluginRegistryAddPlugin(&engineTKList, 0x90, 0x40A, _rwIm3DOpen, _rwIm3DClose, NULL);
                    offset |= _rwPluginRegistryAddPlugin(&engineTKList, 0x28, 0x40B, _rwResourcesOpen,
                                                         _rwResourcesClose, NULL);

                    status = (offset >= 0);
                    result = status;
                    if (status) {
                        RWSRCGLOBAL(engineStatus) = rwENGINESTATUSINITED;
                        return status;
                    }

                    _rwPluginRegistryClose();
                }

                _rwFileSystemClose();
            }

            _rwMemoryClose();
        }

        _rwStringClose();
    }

    return result;
}
