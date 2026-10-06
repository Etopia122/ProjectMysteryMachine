/*
 * Debug module: in release builds it only counts its instances.
 */

#include "rwsdk/rwcore.h"

RwModuleInfo debugModule;

void* _rwDebugOpen(void* instance, RwInt32 offset, RwInt32 size) {
    debugModule.numInstances++;
    return instance;
}

void* _rwDebugClose(void* instance, RwInt32 offset, RwInt32 size) {
    debugModule.numInstances--;
    return instance;
}
