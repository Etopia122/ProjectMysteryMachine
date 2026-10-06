#include "TRK_MINNOW_DOLPHIN/Os/dolphin/dolphin_trk_glue.h"
#include "dolphin/base/PPCArch.h"
#include "trk.h"

// Older MetroTRK glue: one comm table (GDEV "DB" or AMC "EXI2" functions), a
// polled read buffer and a byte-at-a-time write buffer flushed in 0x800 blocks.

typedef int (*DBCommOldFunc)();
typedef int (*DBCommOldInitFunc)(void*, __OSInterruptHandler);
typedef int (*DBCommOldReadFunc)(u8*, int);
typedef int (*DBCommOldWriteFunc)(const u8*, int);

typedef struct DBCommTableOld {
    DBCommOldInitFunc initialize_func;
    DBCommOldFunc init_interrupts_func;
    DBCommOldFunc peek_func;
    DBCommOldReadFunc read_func;
    DBCommOldWriteFunc write_func;
    DBCommOldFunc open_func;
    DBCommOldFunc close_func;
} DBCommTableOld;

typedef struct TRKUARTState {
    s32 writeCount;
    s32 readPos;
    s32 readLength;
    s32 _0C;
    u8 readBuffer[0x110C];
} TRKUARTState;

extern int Hu_IsStub(void);
extern int AMC_IsStub(void);
extern int DBInitComm(void*, __OSInterruptHandler);
extern int DBInitInterrupts(void);
extern int DBQueryData(void);
extern int DBRead(u8*, int);
extern int DBWrite(const u8*, int);
extern int DBOpen(void);
extern int DBClose(void);
extern int EXI2_Init(void*, __OSInterruptHandler);
extern int EXI2_EnableInterrupts(void);
extern int EXI2_Poll(void);
extern int EXI2_ReadN(u8*, int);
extern int EXI2_WriteN(const u8*, int);
extern int EXI2_Reserve(void);
extern int EXI2_Unreserve(void);

void TRKInterruptHandler();

DBCommTableOld gDBCommTable = {};

static u8 gTRKWriteBuffer[0x110C];
static TRKUARTState gTRKUARTState;

void TRKEXICallBack(s16 param_0, OSContext* ctx) {
    OSEnableScheduler();
    TRKLoadContext(ctx, 0x500);
}

int InitMetroTRKCommTable(int hwId) {
    int result;

    if (hwId == HARDWARE_GDEV) {
        OSReport("MetroTRK : Set to GDEV hardware\n");
        result = Hu_IsStub();
        gDBCommTable.initialize_func = DBInitComm;
        gDBCommTable.init_interrupts_func = DBInitInterrupts;
        gDBCommTable.peek_func = DBQueryData;
        gDBCommTable.read_func = DBRead;
        gDBCommTable.write_func = DBWrite;
        gDBCommTable.open_func = DBOpen;
        gDBCommTable.close_func = DBClose;
    } else {
        OSReport("MetroTRK : Set to AMC DDH hardware\n");
        result = AMC_IsStub();
        gDBCommTable.initialize_func = EXI2_Init;
        gDBCommTable.init_interrupts_func = EXI2_EnableInterrupts;
        gDBCommTable.peek_func = EXI2_Poll;
        gDBCommTable.read_func = EXI2_ReadN;
        gDBCommTable.write_func = EXI2_WriteN;
        gDBCommTable.open_func = EXI2_Reserve;
        gDBCommTable.close_func = EXI2_Unreserve;
    }

    return result;
}

DSError TRKInitializeIntDrivenUART(u32 param_0, u32 param_1, u32 param_2, void* param_3) {
    gDBCommTable.initialize_func(param_3, TRKEXICallBack);
    return DS_NoError;
}

void EnableEXI2Interrupts(void) {
    gDBCommTable.init_interrupts_func();
}

static inline int TRKWriteUARTN_Old(const void* bytes, u32 length) {
    int writeErr = gDBCommTable.write_func(bytes, length);
    return ((-writeErr | writeErr) >> 31);
}

int WriteUARTFlush(void) {
    int err = 0;

    for (; gTRKUARTState.writeCount < 0x800; gTRKUARTState.writeCount++) {
        gTRKWriteBuffer[gTRKUARTState.writeCount] = 0;
    }

    if (gTRKUARTState.writeCount != 0) {
        err = TRKWriteUARTN_Old(gTRKWriteBuffer, gTRKUARTState.writeCount);
        gTRKUARTState.writeCount = 0;
    }

    return err;
}

int WriteUART1(s8 c) {
    gTRKWriteBuffer[gTRKUARTState.writeCount++] = c;
    return 0;
}

static inline int TRKReadUARTN_Old(void* bytes, u32 length) {
    int readErr = gDBCommTable.read_func(bytes, length);
    return ((-readErr | readErr) >> 31);
}

int TRKReadUARTPoll(char* c) {
    TRKUARTState* state = &gTRKUARTState;
    int err = 4;

    if (state->readPos >= state->readLength) {
        state->readPos = 0;
        state->readLength = gDBCommTable.peek_func();
        if (state->readLength > 0) {
            if (state->readLength > 0x110A) {
                state->readLength = 0x110A;
            }

            err = TRKReadUARTN_Old(state->readBuffer, state->readLength);
            if (err != 0) {
                state->readLength = 0;
            }
        }
    }

    if (state->readPos < state->readLength) {
        *c = state->readBuffer[state->readPos++];
        err = 0;
    }

    return err;
}

void ReserveEXI2Port(void) {
    gDBCommTable.open_func();
}

void UnreserveEXI2Port(void) {
    gDBCommTable.close_func();
}

void TRK_board_display(char* str) {
    OSReport(str);
}

void TRKUARTInterruptHandler() {}

asm void TRKLoadContext(OSContext* ctx, u32) {
    // clang-format off
	nofralloc
	lwz r0, OSContext.gpr[0](r3)
	lwz r1, OSContext.gpr[1](r3)
	lwz r2, OSContext.gpr[2](r3)
	lhz r5, OSContext.state(r3)
	rlwinm. r6, r5, 0, 0x1e, 0x1e
	beq lbl_80371C1C
	rlwinm r5, r5, 0, 0x1f, 0x1d
	sth r5, OSContext.state(r3)
	lmw r5, OSContext.gpr[5](r3)
	b lbl_80371C20
lbl_80371C1C:
	lmw r13, OSContext.gpr[13](r3)
lbl_80371C20:
	mr r31, r3
	mr r3, r4
	lwz r4, OSContext.cr(r31)
	mtcrf 0xff, r4
	lwz r4, OSContext.lr(r31)
	mtlr r4
	lwz r4, OSContext.ctr(r31)
	mtctr r4
	lwz r4, OSContext.xer(r31)
	mtxer r4
	mfmsr r4
	rlwinm r4, r4, 0, 0x11, 0xf //Turn off external exceptions
	rlwinm r4, r4, 0, 0x1f, 0x1d //Turn off recoverable exception flag
	mtmsr r4
	mtsprg 1, r2
	lwz r4, OSContext.gpr[3](r31)
	mtsprg 2, r4
	lwz r4, OSContext.gpr[4](r31)
	mtsprg 3, r4
	lwz r2, OSContext.srr0(r31)
	lwz r4, OSContext.srr1(r31)
	lwz r31, OSContext.gpr[31](r31)
	b TRKInterruptHandler
    // clang-format on
}
