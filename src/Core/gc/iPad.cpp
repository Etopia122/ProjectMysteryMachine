#include "zGlobals.h"

class PADStatus
{
public:
    unsigned short button;      // 0x0
    signed char stickX;         // 0x2
    signed char stickY;         // 0x3
    signed char substickX;      // 0x4
    signed char substickY;      // 0x5
    unsigned char triggerLeft;  // 0x6
    unsigned char triggerRight; // 0x7
    unsigned char analogA;      // 0x8
    unsigned char analogB;      // 0x9
    signed char err;            // 0xA
};

class _tagTRCPadInfo
{
public:
    signed int pinfo; // 0x0
    signed int id;    // 0x4
    signed int state; // 0x8
};

extern "C" {
int PADInit(void);
unsigned int PADRead(PADStatus* status);
void PADClamp(PADStatus* status);
int PADReset(unsigned int mask);
void PADControlMotor(signed int chan, unsigned int command);
}

extern _tagTRCPadInfo gTrcPad[4];

void xTRCPad(signed int pad_id, signed int state);
// TODO: unidentified xTRC function (true while a TRC message blocks input)
extern "C" unsigned int fn_800443F8();

PADStatus sPadData[4] = {};

static inline signed int iPadConvStick(float value)
{
    if (value > 40.0f)
    {
        value = 40.0f;
    }
    else if (value < -40.0f)
    {
        value = -40.0f;
    }

    float convertedValue = 3.2f * value;
    if (convertedValue > 127.0f)
    {
        convertedValue = 127.0f;
    }
    else if (convertedValue < -127.0f)
    {
        convertedValue = -127.0f;
    }
    return convertedValue;
}

static inline unsigned int iPadConvFromGCN(unsigned int a, unsigned int b, unsigned int c)
{
    return (a & b) ? c : 0;
}

// Functions are in reverse DOL order (-inline deferred emits last to first).

signed int iPadInit()
{
    PADInit();
    return 1;
}

_tagxPad* iPadEnable(_tagxPad* pad, signed short port)
{
    pad->port = port;
    pad->slot = 0;
    gTrcPad[pad->port].state = 2;
    pad->flags |= 3;
    pad->flags |= 4;
    return pad;
}

// TODO (97.3%): after storing each x axis the original schedules the next
// load of sPadData and the int-to-float constant in a different order.
signed int iPadUpdate(_tagxPad* pad, unsigned int* on)
{
    unsigned int temp_on;

    if (fn_800443F8())
    {
        return 0;
    }

    if (pad->port == 0 && pad->slot == 0)
    {
        PADRead(sPadData);
        PADClamp(sPadData);
    }

    switch (sPadData[pad->port].err)
    {
    case -1:
        xTRCPad(pad->port, 1);
        PADReset(0x80000000 >> pad->port);
        goto defaultError;
    case -2:
    case -3:
        return 0;
    default:
    defaultError:
        return 0;
    case 0:
        temp_on = iPadConvFromGCN(sPadData[pad->port].button, 1, 0x80);
        temp_on |= iPadConvFromGCN(sPadData[pad->port].button, 2, 0x20);
        temp_on |= iPadConvFromGCN(sPadData[pad->port].button, 4, 0x40);
        temp_on |= iPadConvFromGCN(sPadData[pad->port].button, 8, 0x10);

        if (sPadData[pad->port].button & 0x10)
        {
            temp_on |= iPadConvFromGCN(sPadData[pad->port].button, 0x20, 0x2000);
            temp_on |= iPadConvFromGCN(sPadData[pad->port].button, 0x40, 0x200);
            if (sPadData[pad->port].triggerLeft >= 0x18)
            {
                temp_on |= 0x200;
            }
            if (sPadData[pad->port].triggerRight >= 0x18)
            {
                temp_on |= 0x2000;
            }
            temp_on |= 0x100000;
        }
        else
        {
            temp_on |= iPadConvFromGCN(sPadData[pad->port].button, 0x20, 0x1000);
            temp_on |= iPadConvFromGCN(sPadData[pad->port].button, 0x40, 0x100);
            if (sPadData[pad->port].triggerLeft >= 0x18)
            {
                temp_on |= 0x100;
            }
            if (sPadData[pad->port].triggerRight >= 0x18)
            {
                temp_on |= 0x1000;
            }
        }

        temp_on |= iPadConvFromGCN(sPadData[pad->port].button, 0x100, 0x10000);
        temp_on |= iPadConvFromGCN(sPadData[pad->port].button, 0x200, 0x80000);
        temp_on |= iPadConvFromGCN(sPadData[pad->port].button, 0x800, 0x40000);
        temp_on |= iPadConvFromGCN(sPadData[pad->port].button, 0x400, 0x20000);
        temp_on |= iPadConvFromGCN(sPadData[pad->port].button, 0x1000, 0x1);
        *on = temp_on;

        pad->analog1.x = iPadConvStick(sPadData[pad->port].stickX);
        pad->analog1.y = -iPadConvStick(sPadData[pad->port].stickY);
        pad->analog2.x = iPadConvStick(sPadData[pad->port].substickX);
        pad->analog2.y = -iPadConvStick(sPadData[pad->port].substickY);
        if (gTrcPad[pad->port].state != 2)
        {
            xTRCPad(pad->port, 2);
        }
        return 1;
    }
}

void iPadRumbleFx(_tagxPad* p, _tagxRumble* r, float time_passed)
{
}

void iPadStopRumble(_tagxPad* pad)
{
    PADControlMotor(pad->port, 0);
}

void iPadStartRumble(_tagxPad* pad, _tagxRumble* rumble)
{
    PADControlMotor(pad->port, 1);
}

void iPadKill()
{
}
