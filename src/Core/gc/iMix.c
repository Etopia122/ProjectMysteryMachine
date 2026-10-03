// Dolphin SDK MIX library, an older revision than the one in BFBB: no Dolby
// Pro Logic II mode, no l1/r1 rear pan values, separate left/right pan tables
// and the older AX mixer control bits.

typedef unsigned char u8;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned int u32;

extern "C" {
int OSDisableInterrupts(void);
int OSRestoreInterrupts(int level);
void AISetStreamVolLeft(u8 vol);
void AISetStreamVolRight(u8 vol);
}

typedef struct AXVPB
{
    unsigned char pad0[0x18];
    u32 index;              // 0x18
    u32 sync;               // 0x1C
    unsigned char pad20[0x144 - 0x20];
    u16 mixerCtrl;          // 0x144
    unsigned char pad146[0x14A - 0x146];
    u16 vL;                 // 0x14A
    u16 vDeltaL;            // 0x14C
    u16 vR;                 // 0x14E
    u16 vDeltaR;            // 0x150
    u16 vAuxAL;             // 0x152
    u16 vDeltaAuxAL;        // 0x154
    u16 vAuxAR;             // 0x156
    u16 vDeltaAuxAR;        // 0x158
    u16 vAuxBL;             // 0x15A
    u16 vDeltaAuxBL;        // 0x15C
    u16 vAuxBR;             // 0x15E
    u16 vDeltaAuxBR;        // 0x160
    u16 vAuxBS;             // 0x162
    u16 vDeltaAuxBS;        // 0x164
    u16 vS;                 // 0x166
    u16 vDeltaS;            // 0x168
    u16 vAuxAS;             // 0x16A
    u16 vDeltaAuxAS;        // 0x16C
    unsigned char pad16E[0x19C - 0x16E];
    u16 currentVolume;      // 0x19C
    s16 currentDelta;       // 0x19E
} AXVPB;

typedef struct MIXChannel
{
    AXVPB* axvpb; // 0x0
    u32 mode;     // 0x4
    int input;    // 0x8
    int auxA;     // 0xC
    int auxB;     // 0x10
    int pan;      // 0x14
    int span;     // 0x18
    int fader;    // 0x1C
    int l;        // 0x20
    int r;        // 0x24
    int f;        // 0x28
    int b;        // 0x2C
    u16 v;        // 0x30
    u16 v1;
    u16 vL;       // 0x34
    u16 vL1;
    u16 vR;       // 0x38
    u16 vR1;
    u16 vS;       // 0x3C
    u16 vS1;
    u16 vAL;      // 0x40
    u16 vAL1;
    u16 vAR;      // 0x44
    u16 vAR1;
    u16 vAS;      // 0x48
    u16 vAS1;
    u16 vBL;      // 0x4C
    u16 vBL1;
    u16 vBR;      // 0x50
    u16 vBR1;
    u16 vBS;      // 0x54
    u16 vBS1;
} MIXChannel;

static u16 __MIXVolumeTable[965] = {
    0x0000, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
    0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
    0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
    0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
    0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
    0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
    0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
    0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0002, 0x0002,
    0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002,
    0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002,
    0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002,
    0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002,
    0x0002, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003,
    0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003,
    0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003,
    0x0003, 0x0003, 0x0004, 0x0004, 0x0004, 0x0004, 0x0004, 0x0004,
    0x0004, 0x0004, 0x0004, 0x0004, 0x0004, 0x0004, 0x0004, 0x0004,
    0x0004, 0x0004, 0x0004, 0x0004, 0x0004, 0x0005, 0x0005, 0x0005,
    0x0005, 0x0005, 0x0005, 0x0005, 0x0005, 0x0005, 0x0005, 0x0005,
    0x0005, 0x0005, 0x0005, 0x0005, 0x0005, 0x0006, 0x0006, 0x0006,
    0x0006, 0x0006, 0x0006, 0x0006, 0x0006, 0x0006, 0x0006, 0x0006,
    0x0006, 0x0006, 0x0007, 0x0007, 0x0007, 0x0007, 0x0007, 0x0007,
    0x0007, 0x0007, 0x0007, 0x0007, 0x0007, 0x0007, 0x0008, 0x0008,
    0x0008, 0x0008, 0x0008, 0x0008, 0x0008, 0x0008, 0x0008, 0x0008,
    0x0009, 0x0009, 0x0009, 0x0009, 0x0009, 0x0009, 0x0009, 0x0009,
    0x0009, 0x000A, 0x000A, 0x000A, 0x000A, 0x000A, 0x000A, 0x000A,
    0x000A, 0x000A, 0x000B, 0x000B, 0x000B, 0x000B, 0x000B, 0x000B,
    0x000B, 0x000C, 0x000C, 0x000C, 0x000C, 0x000C, 0x000C, 0x000C,
    0x000D, 0x000D, 0x000D, 0x000D, 0x000D, 0x000D, 0x000D, 0x000E,
    0x000E, 0x000E, 0x000E, 0x000E, 0x000E, 0x000F, 0x000F, 0x000F,
    0x000F, 0x000F, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0011,
    0x0011, 0x0011, 0x0011, 0x0011, 0x0012, 0x0012, 0x0012, 0x0012,
    0x0012, 0x0013, 0x0013, 0x0013, 0x0013, 0x0013, 0x0014, 0x0014,
    0x0014, 0x0014, 0x0015, 0x0015, 0x0015, 0x0015, 0x0016, 0x0016,
    0x0016, 0x0016, 0x0017, 0x0017, 0x0017, 0x0018, 0x0018, 0x0018,
    0x0018, 0x0019, 0x0019, 0x0019, 0x001A, 0x001A, 0x001A, 0x001A,
    0x001B, 0x001B, 0x001B, 0x001C, 0x001C, 0x001C, 0x001D, 0x001D,
    0x001D, 0x001E, 0x001E, 0x001E, 0x001F, 0x001F, 0x0020, 0x0020,
    0x0020, 0x0021, 0x0021, 0x0021, 0x0022, 0x0022, 0x0023, 0x0023,
    0x0023, 0x0024, 0x0024, 0x0025, 0x0025, 0x0026, 0x0026, 0x0026,
    0x0027, 0x0027, 0x0028, 0x0028, 0x0029, 0x0029, 0x002A, 0x002A,
    0x002B, 0x002B, 0x002C, 0x002C, 0x002D, 0x002D, 0x002E, 0x002E,
    0x002F, 0x002F, 0x0030, 0x0031, 0x0031, 0x0032, 0x0032, 0x0033,
    0x0033, 0x0034, 0x0035, 0x0035, 0x0036, 0x0037, 0x0037, 0x0038,
    0x0038, 0x0039, 0x003A, 0x003A, 0x003B, 0x003C, 0x003D, 0x003D,
    0x003E, 0x003F, 0x003F, 0x0040, 0x0041, 0x0042, 0x0042, 0x0043,
    0x0044, 0x0045, 0x0046, 0x0046, 0x0047, 0x0048, 0x0049, 0x004A,
    0x004B, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F, 0x0050, 0x0051,
    0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059,
    0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F, 0x0060, 0x0061,
    0x0062, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x006A, 0x006B,
    0x006C, 0x006D, 0x006F, 0x0070, 0x0071, 0x0072, 0x0074, 0x0075,
    0x0076, 0x0078, 0x0079, 0x007B, 0x007C, 0x007E, 0x007F, 0x0080,
    0x0082, 0x0083, 0x0085, 0x0087, 0x0088, 0x008A, 0x008B, 0x008D,
    0x008F, 0x0090, 0x0092, 0x0094, 0x0095, 0x0097, 0x0099, 0x009B,
    0x009C, 0x009E, 0x00A0, 0x00A2, 0x00A4, 0x00A6, 0x00A8, 0x00AA,
    0x00AB, 0x00AD, 0x00AF, 0x00B2, 0x00B4, 0x00B6, 0x00B8, 0x00BA,
    0x00BC, 0x00BE, 0x00C0, 0x00C3, 0x00C5, 0x00C7, 0x00CA, 0x00CC,
    0x00CE, 0x00D1, 0x00D3, 0x00D6, 0x00D8, 0x00DB, 0x00DD, 0x00E0,
    0x00E2, 0x00E5, 0x00E7, 0x00EA, 0x00ED, 0x00F0, 0x00F2, 0x00F5,
    0x00F8, 0x00FB, 0x00FE, 0x0101, 0x0104, 0x0107, 0x010A, 0x010D,
    0x0110, 0x0113, 0x0116, 0x011A, 0x011D, 0x0120, 0x0124, 0x0127,
    0x012A, 0x012E, 0x0131, 0x0135, 0x0138, 0x013C, 0x0140, 0x0143,
    0x0147, 0x014B, 0x014F, 0x0153, 0x0157, 0x015B, 0x015F, 0x0163,
    0x0167, 0x016B, 0x016F, 0x0173, 0x0178, 0x017C, 0x0180, 0x0185,
    0x0189, 0x018E, 0x0193, 0x0197, 0x019C, 0x01A1, 0x01A6, 0x01AB,
    0x01AF, 0x01B4, 0x01BA, 0x01BF, 0x01C4, 0x01C9, 0x01CE, 0x01D4,
    0x01D9, 0x01DF, 0x01E4, 0x01EA, 0x01EF, 0x01F5, 0x01FB, 0x0201,
    0x0207, 0x020D, 0x0213, 0x0219, 0x021F, 0x0226, 0x022C, 0x0232,
    0x0239, 0x0240, 0x0246, 0x024D, 0x0254, 0x025B, 0x0262, 0x0269,
    0x0270, 0x0277, 0x027E, 0x0286, 0x028D, 0x0295, 0x029D, 0x02A4,
    0x02AC, 0x02B4, 0x02BC, 0x02C4, 0x02CC, 0x02D5, 0x02DD, 0x02E6,
    0x02EE, 0x02F7, 0x0300, 0x0309, 0x0312, 0x031B, 0x0324, 0x032D,
    0x0337, 0x0340, 0x034A, 0x0354, 0x035D, 0x0367, 0x0371, 0x037C,
    0x0386, 0x0390, 0x039B, 0x03A6, 0x03B1, 0x03BB, 0x03C7, 0x03D2,
    0x03DD, 0x03E9, 0x03F4, 0x0400, 0x040C, 0x0418, 0x0424, 0x0430,
    0x043D, 0x0449, 0x0456, 0x0463, 0x0470, 0x047D, 0x048A, 0x0498,
    0x04A5, 0x04B3, 0x04C1, 0x04CF, 0x04DD, 0x04EC, 0x04FA, 0x0509,
    0x0518, 0x0527, 0x0536, 0x0546, 0x0555, 0x0565, 0x0575, 0x0586,
    0x0596, 0x05A6, 0x05B7, 0x05C8, 0x05D9, 0x05EB, 0x05FC, 0x060E,
    0x0620, 0x0632, 0x0644, 0x0657, 0x066A, 0x067D, 0x0690, 0x06A4,
    0x06B7, 0x06CB, 0x06DF, 0x06F4, 0x0708, 0x071D, 0x0732, 0x0748,
    0x075D, 0x0773, 0x0789, 0x079F, 0x07B6, 0x07CD, 0x07E4, 0x07FB,
    0x0813, 0x082B, 0x0843, 0x085C, 0x0874, 0x088E, 0x08A7, 0x08C1,
    0x08DA, 0x08F5, 0x090F, 0x092A, 0x0945, 0x0961, 0x097D, 0x0999,
    0x09B5, 0x09D2, 0x09EF, 0x0A0D, 0x0A2A, 0x0A48, 0x0A67, 0x0A86,
    0x0AA5, 0x0AC5, 0x0AE5, 0x0B05, 0x0B25, 0x0B47, 0x0B68, 0x0B8A,
    0x0BAC, 0x0BCF, 0x0BF2, 0x0C15, 0x0C39, 0x0C5D, 0x0C82, 0x0CA7,
    0x0CCC, 0x0CF2, 0x0D19, 0x0D3F, 0x0D67, 0x0D8E, 0x0DB7, 0x0DDF,
    0x0E08, 0x0E32, 0x0E5C, 0x0E87, 0x0EB2, 0x0EDD, 0x0F09, 0x0F36,
    0x0F63, 0x0F91, 0x0FBF, 0x0FEE, 0x101D, 0x104D, 0x107D, 0x10AE,
    0x10DF, 0x1111, 0x1144, 0x1177, 0x11AB, 0x11DF, 0x1214, 0x124A,
    0x1280, 0x12B7, 0x12EE, 0x1326, 0x135F, 0x1399, 0x13D3, 0x140D,
    0x1449, 0x1485, 0x14C2, 0x14FF, 0x153E, 0x157D, 0x15BC, 0x15FD,
    0x163E, 0x1680, 0x16C3, 0x1706, 0x174A, 0x178F, 0x17D5, 0x181C,
    0x1863, 0x18AC, 0x18F5, 0x193F, 0x198A, 0x19D5, 0x1A22, 0x1A6F,
    0x1ABE, 0x1B0D, 0x1B5D, 0x1BAE, 0x1C00, 0x1C53, 0x1CA7, 0x1CFC,
    0x1D52, 0x1DA9, 0x1E01, 0x1E5A, 0x1EB4, 0x1F0F, 0x1F6B, 0x1FC8,
    0x2026, 0x2086, 0x20E6, 0x2148, 0x21AA, 0x220E, 0x2273, 0x22D9,
    0x2341, 0x23A9, 0x2413, 0x247E, 0x24EA, 0x2557, 0x25C6, 0x2636,
    0x26A7, 0x271A, 0x278E, 0x2803, 0x287A, 0x28F2, 0x296B, 0x29E6,
    0x2A62, 0x2AE0, 0x2B5F, 0x2BDF, 0x2C61, 0x2CE5, 0x2D6A, 0x2DF1,
    0x2E79, 0x2F03, 0x2F8E, 0x301B, 0x30AA, 0x313A, 0x31CC, 0x325F,
    0x32F5, 0x338C, 0x3425, 0x34BF, 0x355B, 0x35FA, 0x369A, 0x373C,
    0x37DF, 0x3885, 0x392C, 0x39D6, 0x3A81, 0x3B2F, 0x3BDE, 0x3C90,
    0x3D43, 0x3DF9, 0x3EB1, 0x3F6A, 0x4026, 0x40E5, 0x41A5, 0x4268,
    0x432C, 0x43F4, 0x44BD, 0x4589, 0x4657, 0x4727, 0x47FA, 0x48D0,
    0x49A8, 0x4A82, 0x4B5F, 0x4C3E, 0x4D20, 0x4E05, 0x4EEC, 0x4FD6,
    0x50C3, 0x51B2, 0x52A4, 0x5399, 0x5491, 0x558C, 0x5689, 0x578A,
    0x588D, 0x5994, 0x5A9D, 0x5BAA, 0x5CBA, 0x5DCD, 0x5EE3, 0x5FFC,
    0x6119, 0x6238, 0x635C, 0x6482, 0x65AC, 0x66D9, 0x680A, 0x693F,
    0x6A77, 0x6BB2, 0x6CF2, 0x6E35, 0x6F7B, 0x70C6, 0x7214, 0x7366,
    0x74BC, 0x7616, 0x7774, 0x78D6, 0x7A3D, 0x7BA7, 0x7D16, 0x7E88,
    0x7FFF, 0x817B, 0x82FB, 0x847F, 0x8608, 0x8795, 0x8927, 0x8ABE,
    0x8C59, 0x8DF9, 0x8F9E, 0x9148, 0x92F6, 0x94AA, 0x9663, 0x9820,
    0x99E3, 0x9BAB, 0x9D79, 0x9F4C, 0xA124, 0xA302, 0xA4E5, 0xA6CE,
    0xA8BC, 0xAAB0, 0xACAA, 0xAEAA, 0xB0B0, 0xB2BC, 0xB4CE, 0xB6E5,
    0xB904, 0xBB28, 0xBD53, 0xBF84, 0xC1BC, 0xC3FA, 0xC63F, 0xC88B,
    0xCADD, 0xCD37, 0xCF97, 0xD1FE, 0xD46D, 0xD6E3, 0xD960, 0xDBE4,
    0xDE70, 0xE103, 0xE39E, 0xE641, 0xE8EB, 0xEB9E, 0xEE58, 0xF11B,
    0xF3E6, 0xF6B9, 0xF994, 0xFC78, 0xFF64,
};

static int __MIXPanTable[128] = {
    0, 0, -1, -1, -1, -2, -2, -2, -3, -3, -4, -4,
    -4, -5, -5, -5, -6, -6, -7, -7, -7, -8, -8, -9,
    -9, -10, -10, -10, -11, -11, -12, -12, -13, -13, -14, -14,
    -14, -15, -15, -16, -16, -17, -17, -18, -18, -19, -20, -20,
    -21, -21, -22, -22, -23, -23, -24, -25, -25, -26, -26, -27,
    -28, -28, -29, -30, -30, -31, -32, -33, -33, -34, -35, -36,
    -36, -37, -38, -39, -40, -40, -41, -42, -43, -44, -45, -46,
    -47, -48, -49, -50, -51, -52, -54, -55, -56, -57, -59, -60,
    -61, -63, -64, -66, -67, -69, -71, -72, -74, -76, -78, -80,
    -83, -85, -87, -90, -93, -96, -99, -102, -106, -110, -115, -120,
    -126, -133, -140, -150, -163, -180, -210, -904,
};

static int __MIXPanTableRev[128] = {
    -904, -210, -180, -163, -150, -140, -133, -126, -120, -115, -110, -106,
    -102, -99, -96, -93, -90, -87, -85, -83, -80, -78, -76, -74,
    -72, -71, -69, -67, -66, -64, -63, -61, -60, -59, -57, -56,
    -55, -54, -52, -51, -50, -49, -48, -47, -46, -45, -44, -43,
    -42, -41, -40, -40, -39, -38, -37, -36, -36, -35, -34, -33,
    -33, -32, -31, -30, -30, -29, -28, -28, -27, -26, -26, -25,
    -25, -24, -23, -23, -22, -22, -21, -21, -20, -20, -19, -18,
    -18, -17, -17, -16, -16, -15, -15, -14, -14, -14, -13, -13,
    -12, -12, -11, -11, -10, -10, -10, -9, -9, -8, -8, -7,
    -7, -7, -6, -6, -5, -5, -5, -4, -4, -4, -3, -3,
    -2, -2, -2, -1, -1, -1, 0, 0,
};

static u8 __MIXAIVolumeTable[50] = {
    0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02,
    0x02, 0x03, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0A, 0x0B, 0x0C, 0x0E, 0x10, 0x12, 0x14, 0x16, 0x19,
    0x1C, 0x20, 0x24, 0x28, 0x2D, 0x32, 0x39, 0x40, 0x47, 0x50,
    0x5A, 0x65, 0x71, 0x7F, 0x8F, 0xA0, 0xB4, 0xCA, 0xE3, 0xFF,
};

static MIXChannel __MIXChannel[64];
static int __MIXDvdStreamAttenCurrent;
static int __MIXDvdStreamAttenUser;
static u32 __MIXSoundMode;

static inline u16 __MIXGetVolume(int db)
{
    if (db <= -904)
    {
        return 0;
    }
    if (db >= 60)
    {
        return 0xFF64;
    }
    return __MIXVolumeTable[db + 904];
}

static inline void __MIXSetPan(MIXChannel* c)
{
    c->l = __MIXPanTable[c->pan];
    c->r = __MIXPanTableRev[c->pan];
    c->f = __MIXPanTableRev[c->span];
    c->b = __MIXPanTable[c->span];
}

static inline void __MIXResetChannel(MIXChannel* c)
{
    c->mode = 0x50000000;
    c->input = 0;
    c->auxA = -960;
    c->auxB = -960;
    c->l = __MIXPanTable[64];
    c->r = __MIXPanTableRev[64];
    c->f = __MIXPanTableRev[0];
    c->b = __MIXPanTable[0];
    c->fader = 0;
    c->pan = 64;
    c->span = 127;
    c->vBS = 0;
    c->vBR = 0;
    c->vBL = 0;
    c->vAS = 0;
    c->vAR = 0;
    c->vAL = 0;
    c->vS = 0;
    c->vR = 0;
    c->vL = 0;
    c->v = 0;
}

static inline int __MIXClampPan(int pan)
{
    if (pan < 0)
    {
        return 0;
    }
    if (pan > 127)
    {
        return 127;
    }
    return pan;
}

extern "C" {

void MIXInit(void)
{
    int i;

    for (i = 0; i < 64; i++)
    {
        __MIXResetChannel(&__MIXChannel[i]);
    }
    __MIXDvdStreamAttenCurrent = 0;
    __MIXDvdStreamAttenUser = 0;
    __MIXSoundMode = 1;
}

void MIXInitChannel(AXVPB* p, u32 mode, int input, int auxA, int auxB, int pan, int span,
                    int fader)
{
    MIXChannel* c = &__MIXChannel[p->index];
    u16 mixerCtrl;
    int old;

    c->axvpb = p;
    c->mode = mode & 0xF;
    c->input = input;
    c->auxA = auxA;
    c->auxB = auxB;
    c->pan = pan;
    c->span = span;
    c->fader = fader;

    __MIXSetPan(c);

    if (c->mode & 4)
    {
        c->v = 0;
    }
    else
    {
        c->v = __MIXGetVolume(input);
    }

    if (__MIXSoundMode == 1)
    {
        c->vL = __MIXGetVolume(c->fader + c->l + c->f);
        c->vR = __MIXGetVolume(c->fader + c->r + c->f);
        c->vS = __MIXGetVolume(c->fader + c->b);
        if (c->mode & 1)
        {
            c->vAL = __MIXGetVolume(c->auxA + c->l + c->f);
            c->vAR = __MIXGetVolume(c->auxA + c->r + c->f);
            c->vAS = __MIXGetVolume(c->auxA + c->b - 60);
        }
        else
        {
            c->vAL = __MIXGetVolume(c->fader + c->auxA + c->l + c->f);
            c->vAR = __MIXGetVolume(c->fader + c->auxA + c->r + c->f);
            c->vAS = __MIXGetVolume(c->fader + c->auxA + c->b - 60);
        }
        if (c->mode & 2)
        {
            c->vBL = __MIXGetVolume(c->auxB + c->l + c->f);
            c->vBR = __MIXGetVolume(c->auxB + c->r + c->f);
            c->vBS = __MIXGetVolume(c->auxB + c->b - 60);
        }
        else
        {
            c->vBL = __MIXGetVolume(c->fader + c->auxB + c->l + c->f);
            c->vBR = __MIXGetVolume(c->fader + c->auxB + c->r + c->f);
            c->vBS = __MIXGetVolume(c->fader + c->auxB + c->b - 60);
        }
    }
    else
    {
        c->vL = __MIXGetVolume(c->fader + c->f);
        c->vR = __MIXGetVolume(c->fader + c->f);
        c->vS = __MIXGetVolume(c->fader + c->b);
        if (c->mode & 1)
        {
            c->vAL = __MIXGetVolume(c->auxA + c->f);
            c->vAR = __MIXGetVolume(c->auxA + c->f);
            c->vAS = __MIXGetVolume(c->auxA + c->b - 60);
        }
        else
        {
            c->vAL = __MIXGetVolume(c->fader + c->auxA + c->f);
            c->vAR = __MIXGetVolume(c->fader + c->auxA + c->f);
            c->vAS = __MIXGetVolume(c->fader + c->auxA + c->b - 60);
        }
        if (c->mode & 2)
        {
            c->vBL = __MIXGetVolume(c->auxB + c->f);
            c->vBR = __MIXGetVolume(c->auxB + c->f);
            c->vBS = __MIXGetVolume(c->auxB + c->b - 60);
        }
        else
        {
            c->vBL = __MIXGetVolume(c->fader + c->auxB + c->f);
            c->vBR = __MIXGetVolume(c->fader + c->auxB + c->f);
            c->vBS = __MIXGetVolume(c->fader + c->auxB + c->b - 60);
        }
    }

    old = OSDisableInterrupts();

    mixerCtrl = 0;
    p->currentVolume = c->v;
    p->currentDelta = 0;
    p->vL = c->vL;
    p->vDeltaL = 0;
    p->vR = c->vR;
    p->vDeltaR = 0;
    p->vAuxAL = c->vAL;
    p->vDeltaAuxAL = 0;
    p->vAuxAR = c->vAR;
    p->vDeltaAuxAR = 0;
    p->vAuxBL = c->vBL;
    p->vDeltaAuxBL = 0;
    p->vAuxBR = c->vBR;
    p->vDeltaAuxBR = 0;
    p->vAuxBS = c->vBS;
    p->vDeltaAuxBS = 0;
    p->vS = c->vS;
    p->vDeltaS = 0;
    p->vAuxAS = c->vAS;
    p->vDeltaAuxAS = 0;

    if (p->vAuxAL || p->vAuxAR || p->vAuxAS)
    {
        mixerCtrl |= 1;
    }
    if (p->vAuxBL || p->vAuxBR || p->vAuxBS)
    {
        mixerCtrl |= 2;
    }
    if (p->vS || p->vAuxAS || p->vAuxBS)
    {
        mixerCtrl |= 4;
    }
    p->mixerCtrl = mixerCtrl;
    p->sync |= 0x212;

    OSRestoreInterrupts(old);
}

void MIXReleaseChannel(AXVPB* p)
{
    __MIXChannel[p->index].axvpb = 0;
}

void MIXAdjustInput(AXVPB* p, int dB)
{
    MIXChannel* c = &__MIXChannel[p->index];
    c->input += dB;
    c->mode |= 0x10000000;
}

int MIXGetInput(AXVPB* p)
{
    MIXChannel* c = &__MIXChannel[p->index];
    return c->input;
}

void MIXAdjustPan(AXVPB* p, int pan)
{
    MIXChannel* c = &__MIXChannel[p->index];
    c->pan = __MIXClampPan(c->pan + pan);
    __MIXSetPan(c);
    c->mode |= 0x40000000;
}

int MIXGetPan(AXVPB* p)
{
    MIXChannel* c = &__MIXChannel[p->index];
    return c->pan;
}

void MIXUnMute(AXVPB* p)
{
    MIXChannel* c = &__MIXChannel[p->index];
    c->mode &= ~4;
    c->mode |= 0x10000000;
}

void MIXAdjustFader(AXVPB* p, int dB)
{
    MIXChannel* c = &__MIXChannel[p->index];
    c->fader += dB;
    c->mode |= 0x40000000;
}

int MIXGetFader(AXVPB* p)
{
    MIXChannel* c = &__MIXChannel[p->index];
    return c->fader;
}

void MIXUpdateSettings(void)
{
    int i;
    u16 mixerCtrl;
    s16 delta;
    u16 d;
    int setMix;
    MIXChannel* c;
    AXVPB* p;
    int setInput;

    for (i = 0; i < 64; i++)
    {
        c = &__MIXChannel[i];
        p = c->axvpb;
        setInput = 0;
        setMix = 0;

        if (!p)
        {
            continue;
        }

        if (c->mode & 0x20000000)
        {
            setInput = 1;
            c->v = c->v1;
            c->mode &= ~0x20000000;
        }
        if (c->mode & 0x10000000)
        {
            if (c->mode & 4)
            {
                c->v1 = 0;
            }
            else
            {
                c->v1 = __MIXGetVolume(c->input);
            }
            setInput = 1;
            c->mode &= ~0x10000000;
            c->mode |= 0x20000000;
        }
        if (c->mode & 0x80000000)
        {
            setMix = 1;
            c->vL = c->vL1;
            c->vR = c->vR1;
            c->vS = c->vS1;
            c->vAL = c->vAL1;
            c->vAR = c->vAR1;
            c->vAS = c->vAS1;
            c->vBL = c->vBL1;
            c->vBR = c->vBR1;
            c->vBS = c->vBS1;
            c->mode &= ~0x80000000;
        }
        if (c->mode & 0x40000000)
        {
            if (__MIXSoundMode == 1)
            {
                c->vL1 = __MIXGetVolume(c->fader + c->l + c->f);
                c->vR1 = __MIXGetVolume(c->fader + c->r + c->f);
                c->vS1 = __MIXGetVolume(c->fader + c->b - 60);
                if (c->mode & 1)
                {
                    c->vAL1 = __MIXGetVolume(c->auxA + c->l + c->f);
                    c->vAR1 = __MIXGetVolume(c->auxA + c->r + c->f);
                    c->vAS1 = __MIXGetVolume(c->auxA + c->b - 60);
                }
                else
                {
                    c->vAL1 = __MIXGetVolume(c->fader + c->auxA + c->l + c->f);
                    c->vAR1 = __MIXGetVolume(c->fader + c->auxA + c->r + c->f);
                    c->vAS1 = __MIXGetVolume(c->fader + c->auxA + c->b - 60);
                }
                if (c->mode & 2)
                {
                    c->vBL1 = __MIXGetVolume(c->auxB + c->l + c->f);
                    c->vBR1 = __MIXGetVolume(c->auxB + c->r + c->f);
                    c->vBS1 = __MIXGetVolume(c->auxB + c->b - 60);
                }
                else
                {
                    c->vBL1 = __MIXGetVolume(c->fader + c->auxB + c->l + c->f);
                    c->vBR1 = __MIXGetVolume(c->fader + c->auxB + c->r + c->f);
                    c->vBS1 = __MIXGetVolume(c->fader + c->auxB + c->b - 60);
                }
            }
            else
            {
                c->vL1 = __MIXGetVolume(c->fader + c->f);
                c->vR1 = __MIXGetVolume(c->fader + c->f);
                c->vS1 = __MIXGetVolume(c->fader + c->b - 60);
                if (c->mode & 1)
                {
                    c->vAL1 = __MIXGetVolume(c->auxA + c->f);
                    c->vAR1 = __MIXGetVolume(c->auxA + c->f);
                    c->vAS1 = __MIXGetVolume(c->auxA + c->b - 60);
                }
                else
                {
                    c->vAL1 = __MIXGetVolume(c->fader + c->auxA + c->f);
                    c->vAR1 = __MIXGetVolume(c->fader + c->auxA + c->f);
                    c->vAS1 = __MIXGetVolume(c->fader + c->auxA + c->b - 60);
                }
                if (c->mode & 2)
                {
                    c->vBL1 = __MIXGetVolume(c->auxB + c->f);
                    c->vBR1 = __MIXGetVolume(c->auxB + c->f);
                    c->vBS1 = __MIXGetVolume(c->auxB + c->b - 60);
                }
                else
                {
                    c->vBL1 = __MIXGetVolume(c->fader + c->auxB + c->f);
                    c->vBR1 = __MIXGetVolume(c->fader + c->auxB + c->f);
                    c->vBS1 = __MIXGetVolume(c->fader + c->auxB + c->b - 60);
                }
            }
            setMix = 1;
            c->mode &= ~0x40000000;
            c->mode |= 0x80000000;
        }

        if (setInput && p)
        {
            p->currentVolume = c->v;
            p->currentDelta = (c->v1 - c->v) / 160;
            p->sync |= 0x200;
        }

        if (setMix && p)
        {
            p->vL = c->vL;
            delta = (c->vL1 - c->vL) / 160;
            if (c->mode & 8)
            {
                d = 0;
            }
            else if (delta > 0)
            {
                d = delta;
            }
            else
            {
                d = 0;
            }
            p->vDeltaL = d;
            p->vR = c->vR;
            delta = (c->vL1 - c->vL) / 160;
            if (c->mode & 8)
            {
                d = 0;
            }
            else if (delta > 0)
            {
                d = delta;
            }
            else
            {
                d = 0;
            }
            p->vDeltaR = d;
            mixerCtrl = 0;
            p->vAuxAL = c->vAL;
            p->vDeltaAuxAL = (c->vAL1 - c->vAL) / 160;
            p->vAuxAR = c->vAR;
            p->vDeltaAuxAR = (c->vAR1 - c->vAR) / 160;
            p->vAuxBL = c->vBL;
            p->vDeltaAuxBL = (c->vBL1 - c->vBL) / 160;
            p->vAuxBR = c->vBR;
            p->vDeltaAuxBR = (c->vBR1 - c->vBR) / 160;
            p->vAuxBS = c->vBS;
            p->vDeltaAuxBS = (c->vBS1 - c->vBS) / 160;
            p->vS = c->vS;
            p->vDeltaS = (c->vS1 - c->vS) / 160;
            p->vAuxAS = c->vAS;
            p->vDeltaAuxAS = (c->vAS1 - c->vAS) / 160;

            if (p->vAuxAL || p->vAuxAR || p->vAuxAS)
            {
                mixerCtrl |= 1;
            }
            if (p->vAuxBL || p->vAuxBR || p->vAuxBS)
            {
                mixerCtrl |= 2;
            }
            if (p->vS || p->vAuxAS || p->vAuxBS)
            {
                mixerCtrl |= 4;
            }
            if (p->vDeltaL || p->vDeltaR || p->vDeltaS || p->vDeltaAuxAL || p->vDeltaAuxAR ||
                p->vDeltaAuxAS || p->vDeltaAuxBL || p->vDeltaAuxBR || p->vDeltaAuxBS)
            {
                mixerCtrl |= 8;
            }
            p->mixerCtrl = mixerCtrl;
            p->sync |= 0x12;
        }
    }

    if (__MIXDvdStreamAttenUser > __MIXDvdStreamAttenCurrent)
    {
        __MIXDvdStreamAttenCurrent++;
        AISetStreamVolLeft(__MIXAIVolumeTable[__MIXDvdStreamAttenCurrent]);
        AISetStreamVolRight(__MIXAIVolumeTable[__MIXDvdStreamAttenCurrent]);
    }
    else if (__MIXDvdStreamAttenUser < __MIXDvdStreamAttenCurrent)
    {
        __MIXDvdStreamAttenCurrent--;
        AISetStreamVolLeft(__MIXAIVolumeTable[__MIXDvdStreamAttenCurrent]);
        AISetStreamVolRight(__MIXAIVolumeTable[__MIXDvdStreamAttenCurrent]);
    }
}

}
