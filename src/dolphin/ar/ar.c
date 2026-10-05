#include "dolphin/os/OS.h"
#include "dolphin/ar/ar.h"
#include "dolphin/ar/arq.h"
#include "dolphin/dsp.h"

// This older SDK library has no version string (and does not register one).

static void (*__AR_Callback)();
static u32 __AR_Size;
static u32 __AR_InternalSize;
static u32 __AR_ExpansionSize;
static u32 __AR_StackPointer;
static u32 __AR_FreeBlocks;
static u32*  __AR_BlockLength;
static BOOL __AR_init_flag;

// prototypes
static void __ARHandler(__OSInterrupt exception, OSContext* context);
static void __ARWaitForDMA(void);
static void __ARWriteDMA(u32 mmem_addr, u32 aram_addr, u32 length);
static void __ARReadDMA(u32 mmem_addr, u32 aram_addr, u32 length);
static void __ARChecksize(void);
static void __ARClearArea(u32 start_addr, u32 length);

ARCallback ARRegisterDMACallback(ARCallback callback) {
    ARCallback old_callback;
    BOOL old;

    old_callback = __AR_Callback;
    old = OSDisableInterrupts();
    __AR_Callback = callback;
    OSRestoreInterrupts(old);
    return old_callback;
}

void ARStartDMA(u32 type, u32 mainmem_addr, u32 aram_addr, u32 length) {
    BOOL old;

    old = OSDisableInterrupts();
    ASSERTMSGLINE(376, !(__DSPRegs[5] & 0x200), "ARAM DMA already in progress\n");
    ASSERTMSGLINE(377, !(mainmem_addr & 0x1F), "AR: Main memory address is not a multiple of 32 bytes!\n");
    ASSERTMSGLINE(378, !(length & 0x1F), "AR: DMA transfer length is not a multiple of 32 bytes!\n");
    __DSPRegs[16] = (__DSPRegs[16] & 0xFFFFFC00 | (mainmem_addr >> 0x10)); 
    __DSPRegs[17] = (__DSPRegs[17] & 0xFFFF001F | ((u16)mainmem_addr));
    __DSPRegs[18] = (__DSPRegs[18] & 0xFFFFFC00 | (aram_addr >> 0x10));
    __DSPRegs[19] = (__DSPRegs[19] & 0xFFFF001F | ((u16)aram_addr));
    __DSPRegs[20] = __DSPRegs[20] & ~0x8000 | ((type << 0xF) & ~0x7FFF);
    __DSPRegs[20] = (__DSPRegs[20] & 0xFFFFFC00) | (length >> 0x10);
    __DSPRegs[21] = (__DSPRegs[21] & 0xFFFF001F) | (length & 0x0000FFFF);
    OSRestoreInterrupts(old);
}

u32 ARAlloc(u32 length) {
    u32 tmp;
    BOOL old;

    old = OSDisableInterrupts();
    ASSERTMSGLINE(430, !(length & 0x1F), "ARAlloc(): length is not multiple of 32bytes!");
    ASSERTMSGLINE(434, length <= (__AR_Size - __AR_StackPointer), "ARAlloc(): Out of ARAM!");
    ASSERTMSGLINE(435, __AR_FreeBlocks, "ARAlloc(): No more free blocks!");

    tmp = __AR_StackPointer;
    __AR_StackPointer += length;
    *__AR_BlockLength = length;
    __AR_BlockLength += 1;
    __AR_FreeBlocks -= 1;
    OSRestoreInterrupts(old);
    return tmp;
}

u32 ARFree(u32* length) {
    BOOL old;

    old = OSDisableInterrupts();
    __AR_BlockLength--;
    if (length) {
        *length = *__AR_BlockLength;
    }
    __AR_StackPointer -= *__AR_BlockLength;
    __AR_FreeBlocks++;
    OSRestoreInterrupts(old);
    return __AR_StackPointer;
}

u32 ARInit(u32* stack_index_addr, u32 num_entries) {
    BOOL old;
    u16 refresh;

    if (__AR_init_flag == TRUE) {
        return 0x4000;
    }


    old = OSDisableInterrupts();
    __AR_Callback = NULL;
    __OSSetInterruptHandler(6, __ARHandler);
    __OSUnmaskInterrupts(0x02000000);
    __AR_StackPointer = 0x4000;
    __AR_FreeBlocks = num_entries;
    __AR_BlockLength = stack_index_addr;
    refresh = __DSPRegs[13] & 0xFF;

    ASSERTMSGLINE(590, (refresh <= 196.0f), "ARInit(): ILLEGAL SDRAM REFRESH VALUE\n");
    __DSPRegs[13] = (u16)((__DSPRegs[13] & ~0xFF) | (refresh & 0xFF));

    __ARChecksize();
    __AR_init_flag = TRUE;
    OSRestoreInterrupts(old);
    return __AR_StackPointer;
}

u32 ARGetBaseAddress(void) {
    return 0x4000;
}

u32 ARGetSize(void) {
    return __AR_Size;
}

static void __ARHandler(__OSInterrupt exception, OSContext* context) {
    OSContext exceptionContext;
    u16 tmp;

    tmp = __DSPRegs[5];
    tmp = (tmp & ~0x88) | 0x20;
    __DSPRegs[5] = (tmp);
    OSClearContext(&exceptionContext);
    OSSetCurrentContext(&exceptionContext);
    if (__AR_Callback) {
        __AR_Callback();
    }
    OSClearContext(&exceptionContext);
    OSSetCurrentContext(context);
}

static inline void __ARClearInterrupt(void) {
    u16 tmp;

    tmp = __DSPRegs[5];
    tmp = (tmp & ~0x88) | 0x20;
    __DSPRegs[5] = (tmp);
}

static inline void __ARWaitForDMA(void) {
    while (__DSPRegs[5] & 0x200);
}

static inline void __ARWriteDMA(u32 mmem_addr, u32 aram_addr, u32 length) {
	// Main mem address
	__DSPRegs[DSP_ARAM_DMA_MM_HI] = (u16)((__DSPRegs[DSP_ARAM_DMA_MM_HI] & ~0x03ff) | (u16)(mmem_addr >> 16));
	__DSPRegs[DSP_ARAM_DMA_MM_LO] = (u16)((__DSPRegs[DSP_ARAM_DMA_MM_LO] & ~0xffe0) | (u16)(mmem_addr & 0xffff));

	// ARAM address
	__DSPRegs[DSP_ARAM_DMA_ARAM_HI] = (u16)((__DSPRegs[DSP_ARAM_DMA_ARAM_HI] & ~0x03ff) | (u16)(aram_addr >> 16));
	__DSPRegs[DSP_ARAM_DMA_ARAM_LO] = (u16)((__DSPRegs[DSP_ARAM_DMA_ARAM_LO] & ~0xffe0) | (u16)(aram_addr & 0xffff));

	// DMA buffer size
	__DSPRegs[DSP_ARAM_DMA_SIZE_HI] = (u16)(__DSPRegs[DSP_ARAM_DMA_SIZE_HI] & ~0x8000);

	__DSPRegs[DSP_ARAM_DMA_SIZE_HI] = (u16)((__DSPRegs[DSP_ARAM_DMA_SIZE_HI] & ~0x03ff) | (u16)(length >> 16));
	__DSPRegs[DSP_ARAM_DMA_SIZE_LO] = (u16)((__DSPRegs[DSP_ARAM_DMA_SIZE_LO] & ~0xffe0) | (u16)(length & 0xffff));

	__ARWaitForDMA();
    __ARClearInterrupt();
}

static inline void __ARReadDMA(u32 mmem_addr, u32 aram_addr, u32 length) {
	// Main mem address
	__DSPRegs[DSP_ARAM_DMA_MM_HI] = (u16)((__DSPRegs[DSP_ARAM_DMA_MM_HI] & ~0x03ff) | (u16)(mmem_addr >> 16));
	__DSPRegs[DSP_ARAM_DMA_MM_LO] = (u16)((__DSPRegs[DSP_ARAM_DMA_MM_LO] & ~0xffe0) | (u16)(mmem_addr & 0xffff));

	// ARAM address
	__DSPRegs[DSP_ARAM_DMA_ARAM_HI] = (u16)((__DSPRegs[DSP_ARAM_DMA_ARAM_HI] & ~0x03ff) | (u16)(aram_addr >> 16));
	__DSPRegs[DSP_ARAM_DMA_ARAM_LO] = (u16)((__DSPRegs[DSP_ARAM_DMA_ARAM_LO] & ~0xffe0) | (u16)(aram_addr & 0xffff));

	// DMA buffer size
	__DSPRegs[DSP_ARAM_DMA_SIZE_HI] = (u16)(__DSPRegs[DSP_ARAM_DMA_SIZE_HI] | 0x8000);

	__DSPRegs[DSP_ARAM_DMA_SIZE_HI] = (u16)((__DSPRegs[DSP_ARAM_DMA_SIZE_HI] & ~0x03ff) | (u16)(length >> 16));
	__DSPRegs[DSP_ARAM_DMA_SIZE_LO] = (u16)((__DSPRegs[DSP_ARAM_DMA_SIZE_LO] & ~0xffe0) | (u16)(length & 0xffff));

	__ARWaitForDMA();
    __ARClearInterrupt();
}

// Older SDK size probe: dummy data goes to every mirror candidate first, then
// the test pattern to offset 0 is read back through each candidate.
static void __ARChecksize(void) {
    u8 test_data_pad[0x20 + 31];
    u8 dummy_data_pad[0x20 + 31];
    u8 buffer_pad[0x20 + 31];
    u32* test_data;
    u32* dummy_data;
    u32* buffer;
    u16 ARAM_mode = 0;
    u32 ARAM_size = 0;
    u32 i;

    while (!(__DSPRegs[11] & 1));

    ARAM_mode = 3;
    ARAM_size = __AR_InternalSize = 0x1000000;
    __DSPRegs[9] = (u16)((__DSPRegs[9] & ~(0x00000007 | 0x00000038)) | 0x20 | 2 | 1);

    test_data = (u32*)(OSRoundUp32B((u32)(test_data_pad)));
    dummy_data = (u32*)(OSRoundUp32B((u32)(dummy_data_pad)));
    buffer = (u32*)(OSRoundUp32B((u32)(buffer_pad)));

    for (i = 0; i < 8; i++) {
        *(test_data + i) = 0xDEADBEEF;
        *(dummy_data + i) = 0xBAD0BAD0;
    }

    DCFlushRange((void*)test_data, 0x20);
    DCFlushRange((void*)dummy_data, 0x20);

    __AR_ExpansionSize = 0;

    __ARWriteDMA((u32)dummy_data, ARAM_size + 0x0000000, 0x20);
    __ARWriteDMA((u32)dummy_data, ARAM_size + 0x0200000, 0x20);
    __ARWriteDMA((u32)dummy_data, ARAM_size + 0x1000000, 0x20);
    __ARWriteDMA((u32)dummy_data, ARAM_size + 0x0000200, 0x20);
    __ARWriteDMA((u32)dummy_data, ARAM_size + 0x0400000, 0x20);

    memset((void*)buffer, 0, 0x20);
    DCFlushRange((void*)buffer, 0x20);

    __ARWriteDMA((u32)test_data, ARAM_size + 0x0000000, 0x20);
    DCInvalidateRange((void*)buffer, 0x20);
    __ARReadDMA((u32)buffer, ARAM_size + 0x0000000, 0x20);
    PPCSync();

    if (buffer[0] == test_data[0]) {
        memset((void*)buffer, 0, 0x20);
        DCFlushRange((void*)buffer, 0x20);
        __ARReadDMA((u32)buffer, ARAM_size + 0x0200000, 0x20);
        PPCSync();

        if (buffer[0] == test_data[0]) {
            __AR_ExpansionSize = 0x0200000;
            ARAM_mode |= 0 << 1;
            ARAM_size += 0x0200000;
        } else {
            memset((void*)buffer, 0, 0x20);
            DCFlushRange((void*)buffer, 0x20);
            __ARReadDMA((u32)buffer, ARAM_size + 0x1000000, 0x20);
            PPCSync();

            if (buffer[0] == test_data[0]) {
                __AR_ExpansionSize = 0x0400000;
                ARAM_mode |= 4 << 1;
                ARAM_size += 0x0400000;
            } else {
                memset((void*)buffer, 0, 0x20);
                DCFlushRange((void*)buffer, 0x20);
                __ARReadDMA((u32)buffer, ARAM_size + 0x0000200, 0x20);
                PPCSync();

                if (buffer[0] == test_data[0]) {
                    __AR_ExpansionSize = 0x0800000;
                    ARAM_mode |= 8 << 1;
                    ARAM_size += 0x0800000;
                } else {
                    memset((void*)buffer, 0, 0x20);
                    DCFlushRange((void*)buffer, 0x20);
                    __ARReadDMA((u32)buffer, ARAM_size + 0x0400000, 0x20);
                    PPCSync();

                    if (buffer[0] == test_data[0]) {
                        __AR_ExpansionSize = 0x1000000;
                        ARAM_mode |= 12 << 1;
                        ARAM_size += 0x1000000;
                    } else {
                        __AR_ExpansionSize = 0x2000000;
                        ARAM_mode |= 16 << 1;
                        ARAM_size += 0x2000000;
                    }
                }
            }
        }

        __DSPRegs[9] = (u16)((__DSPRegs[9] & ~(0x07 | 0x38)) | ARAM_mode);
    }

    *(u32*)OSPhysicalToUncached(0x00D0) = ARAM_size;
    __AR_Size = ARAM_size;
}
