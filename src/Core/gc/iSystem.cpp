// TODO: partial. Only the GameCube TRC code at the end of the unit (reset
// button, disc errors; it shares iSystem's .data, so it is not its own unit)
// is decompiled so far. The RenderWare file/memory callbacks, iSystemInit/Exit
// and the date helpers above it are still to do.

class RwResEntry
{
public:
    unsigned char pad0[0x18];
};

class xTRCState
{
public:
    signed int state; // 0x0
    signed int mode;  // 0x4
};

extern "C" {
signed int DVDGetDriveStatus();
signed int DVDCheckDisk();
void PADRecalibrate(unsigned int mask);
void GXDrawDone();
void VISetBlack(signed int black);
void VIFlush();
void VIWaitForRetrace();
signed int OSGetResetButtonState();
void OSResetSystem(signed int reset, unsigned int resetCode, signed int forceMenu);
RwResEntry* RwResourcesAllocateResEntry(void* owner, RwResEntry** ownerRef, signed int size,
                                        void* destroyNotify);
void RwResourcesFreeResEntry(RwResEntry* entry);
void __start();

extern unsigned int _stack_addr[];
extern unsigned int _stack_end[];

// TODO: unidentified (xTRC: reports the disc state, returns its result)
signed int fn_8004442C(signed int state);
// TODO: unidentified (iSnd: stops all sound before a reset)
void fn_800D84A0();
// TODO: unidentified (stops the movie player before a reset)
void fn_800FDA54();
// TODO: unidentified (size of the RenderWare resource arena)
signed int fn_80164200();

// TODO: unidentified iSystem/xTRC globals
extern signed int lbl_8025E96C;       // last reset-button state
extern signed int lbl_8025E970;       // reset requested
extern void (*lbl_8025E974)(signed int);
extern signed int lbl_8025E978;
extern unsigned char lbl_801FE3D4[0x14];
extern xTRCState lbl_8025EBD8;
}

class iTRCDisk
{
public:
    static signed int CheckDVDAndResetState();
};

extern "C" {
void fn_80016698(signed int toMenu);
void fn_80016730();
void fn_80016768();
unsigned int fn_80016900();
signed int fn_80016918();
}

// Functions are in reverse DOL order (-inline deferred emits last to first).

extern "C" signed int fn_80016918()
{
    return lbl_8025EBD8.state;
}

extern "C" unsigned int fn_80016900()
{
    return (unsigned char)((&lbl_8025EBD8)->mode == 6);
}

extern "C" unsigned char* fn_800168F4()
{
    return lbl_801FE3D4;
}

// Entry point: fills the stack with a marker, then starts the runtime.
extern "C" void fn_80016898()
{
    for (unsigned int* p = _stack_end; p < _stack_addr; p++)
    {
        *p = 0xDEADBEEF;
    }
    __start();
}

// Fills the free RenderWare resource arena with a marker.
extern "C" void fn_800167F0()
{
    signed int size = fn_80164200();
    RwResEntry* entry = RwResourcesAllocateResEntry(0, 0, size, 0);

    while (!entry && size > 0)
    {
        size -= 0x400;
        entry = RwResourcesAllocateResEntry(0, 0, size, 0);
    }

    if (entry)
    {
        unsigned int* p = (unsigned int*)(entry + 1);
        for (unsigned int i = 0; i < (unsigned int)size / 4; i++)
        {
            p[i] = 0xDEADBEEF;
        }
        RwResourcesFreeResEntry(entry);
    }
}

extern "C" void fn_800167A8(signed int arg)
{
    lbl_8025E978 = arg;
    fn_80016768();
    if (lbl_8025E974)
    {
        lbl_8025E974(arg);
    }
}

extern "C" void fn_80016768()
{
    signed int pressed = OSGetResetButtonState();
    if (lbl_8025E96C == 1 && pressed == 0)
    {
        lbl_8025E970 = 1;
    }
    lbl_8025E96C = pressed;
}

extern "C" void fn_80016730()
{
    if (lbl_8025E970)
    {
        lbl_8025E970 = 0;
        fn_80016698(0);
    }
}

extern "C" void fn_80016698(signed int toMenu)
{
    fn_800D84A0();
    PADRecalibrate(0xF0000000);
    fn_800FDA54();
    GXDrawDone();
    VISetBlack(1);
    VIFlush();
    VIWaitForRetrace();

    signed int state = fn_80016918();
    if (toMenu || state == 10 || state == 7 || !fn_80016900())
    {
        OSResetSystem(1, 0, 1);
    }
    else
    {
        OSResetSystem(0, 0, 0);
    }
}

signed int iTRCDisk::CheckDVDAndResetState()
{
    signed int state;

    fn_80016768();
    fn_80016730();

    switch (DVDGetDriveStatus())
    {
    case 4:
        state = 9;
        break;
    case 6:
        state = 10;
        break;
    case 5:
        state = 7;
        break;
    case 11:
        state = 11;
        break;
    case -1:
        state = 12;
        break;
    case 0:
        state = 8;
        break;
    default:
        if (DVDCheckDisk())
        {
            state = 8;
        }
        else
        {
            state = 5;
        }
        break;
    }

    return fn_8004442C(state);
}
