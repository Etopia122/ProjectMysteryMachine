/*
 * Error reporting: the engine keeps the first error raised since the last
 * query.
 */

#include "rwsdk/rwcore.h"

/* Variable arguments (the MSL definitions, without its headers). */
typedef struct __va_list_struct {
    char gpr;
    char fpr;
    char reserved[2];
    char* input_arg_area;
    char* reg_save_area;
} __va_list[1];

#define va_list __va_list
#define va_start(list, fmt) __builtin_va_info(&list)
#define va_end(list) ((void)0)

#define E_RW_NOERROR 0x80000000

typedef struct rwErrorGlobals {
    RwError errorCode;
} rwErrorGlobals;

RwModuleInfo errorModule;

#define ERRORGLOBAL(var) (RWPLUGINOFFSET(rwErrorGlobals, RwEngineInstance, errorModule.globalsOffset)->var)

void* _rwErrorOpen(void* instance, RwInt32 offset, RwInt32 size) {
    errorModule.globalsOffset = offset;
    errorModule.numInstances++;

    ERRORGLOBAL(errorCode).pluginID = 0;
    ERRORGLOBAL(errorCode).errorCode = E_RW_NOERROR;

    return instance;
}

void* _rwErrorClose(void* instance, RwInt32 offset, RwInt32 size) {
    errorModule.numInstances--;
    return instance;
}

RwError* RwErrorSet(RwError* code) {
    RwError* error = &ERRORGLOBAL(errorCode);

    if (error->pluginID == 0 && error->errorCode == E_RW_NOERROR) {
        if (code->errorCode & 0x80000000) {
            error->pluginID = 0;
        } else {
            error->pluginID = code->pluginID;
        }
        ERRORGLOBAL(errorCode).errorCode = code->errorCode;
    }

    return code;
}

RwInt32 _rwerror(RwInt32 code, ...) {
    va_list ap;

    va_start(ap, code);
    va_end(ap);

    return code;
}
