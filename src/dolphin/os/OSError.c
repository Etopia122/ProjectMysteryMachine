#include "dolphin/os/OSError.h"
#include "dolphin/base/PPCArch.h"
#include "dolphin/dsp.h"
#include "dolphin/dvd/dvdlow.h"
#include "dolphin/os/OS.h"

OSThread* __OSCurrentThread AT_ADDRESS(OS_BASE_CACHED | 0x00E4);
OSThreadQueue __OSActiveThreadQueue AT_ADDRESS(OS_BASE_CACHED | 0x00DC);
volatile OSContext* __OSFPUContext AT_ADDRESS(OS_BASE_CACHED | 0x00D8);

extern volatile u32 __OSLastInterruptSrr0;
extern volatile s16 __OSLastInterrupt;
extern volatile OSTime __OSLastInterruptTime;

// This SDK has no floating-point exception slot: 16 handlers.
OSErrorHandler __OSErrorTable[16];

__declspec(weak) void OSPanic(const char* file, s32 line, const char* msg, ...) {
    va_list marker;
    u32 i;
    u32* p;

    OSDisableInterrupts();
    va_start(marker, msg);
    vprintf(msg, marker);
    va_end(marker);
    OSReport(" in \"%s\" on line %d.\n", file, line);

    // actually useful for debugging- stack crawl
    OSReport("\nAddress:      Back Chain    LR Save\n");
    for (i = 0, p = (u32*)OSGetStackPointer();                 // get current stack pointer
         p && (u32)p != 0xffffffff && i++ < 16; p = (u32*)*p)  // get caller stack pointer
    {
        OSReport("0x%08x:   0x%08x    0x%08x\n", p, p[0], p[1]);
    }

    PPCHalt();
}

// Older SDK: no floating-point exception enabling.
OSErrorHandler OSSetErrorHandler(OSError error, OSErrorHandler handler) {
    OSErrorHandler oldHandler;

    oldHandler = __OSErrorTable[error];
    __OSErrorTable[error] = (OSErrorHandler)handler;
    return oldHandler;
}

void __OSUnhandledException(__OSException exception, OSContext* context, u32 dsisr, u32 dar) {
    if (!(context->srr1 & 2)) {
        OSReport("Non-recoverable Exception %d", exception);

    } else {
        if (__OSErrorTable[exception]) {
            OSDisableScheduler();
            __OSErrorTable[exception](exception, context, dsisr, dar);
            OSEnableScheduler();
            __OSReschedule();
            OSLoadContext(context);
        }

        if (exception == OS_ERROR_DECREMENTER) {
            OSLoadContext(context);
        }

        OSReport("Unhandled Exception %d", exception);
    }

    OSReport("\n");
    OSDumpContext(context);
    OSReport("\nDSISR = 0x%08x                   DAR  = 0x%08x\n", dsisr, dar);
    OSReport("TB = 0x%016llx\n", OSGetTime());

    switch (exception) {
    case EXCEPTION_DSI:
        OSReport("\nInstruction at 0x%x (read from SRR0) attempted to access "
                 "invalid address 0x%x (read from DAR)\n",
                 context->srr0, dar);
        break;
    case EXCEPTION_ISI:
        OSReport("\nAttempted to fetch instruction from invalid address 0x%x "
                 "(read from SRR0)\n",
                 context->srr0);
        break;
    case EXCEPTION_ALIGNMENT:
        OSReport("\nInstruction at 0x%x (read from SRR0) attempted to access "
                 "unaligned address 0x%x (read from DAR)\n",
                 context->srr0, dar);
        break;
    case EXCEPTION_PROGRAM:
        OSReport("\nProgram exception : Possible illegal instruction/operation "
                 "at or around 0x%x (read from SRR0)\n",
                 context->srr0, dar);
        break;
    case EXCEPTION_MEMORY_PROTECTION:
        OSReport("\n");
        OSReport("AI DMA Address =   0x%04x%04x\n", __DSPRegs[DSP_DMA_START_HI],
                 __DSPRegs[DSP_DMA_START_LO]);
        OSReport("ARAM DMA Address = 0x%04x%04x\n", __DSPRegs[DSP_ARAM_DMA_MM_HI],
                 __DSPRegs[DSP_ARAM_DMA_MM_LO]);
        OSReport("DI DMA Address =   0x%08x\n", __DIRegs[5]);
        break;
    }

    OSReport("\nLast interrupt (%d): SRR0 = 0x%08x  TB = 0x%016llx\n", __OSLastInterrupt,
             __OSLastInterruptSrr0, __OSLastInterruptTime);

    PPCHalt();
}
