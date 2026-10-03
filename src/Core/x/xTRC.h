#ifndef XTRC_H
#define XTRC_H

enum _tagPadInit
{
    PADINIT_Unknown
};

class _tagiTRCPadInfo
{
public:
    _tagPadInit pad_init; // 0x0
};

enum _tagTRCState
{
    TRC_Unknown,
    TRC_PadMissing,
    TRC_PadInserted,
    TRC_PadInvalidNoAnalog,
    TRC_PadInvalidType,
    TRC_DiskNotIdentified,
    TRC_DiskIdentified,
    TRC_DiskTrayOpen,
    TRC_DiskTrayClosed,
    TRC_DiskNoDisk,
    TRC_DiskInvalid,
    TRC_DiskRetry,
    TRC_DiskFatal,
    TRC_Total
};

class _tagTRCPadInfo : public _tagiTRCPadInfo
{
public:
    signed int id;       // 0x4
    _tagTRCState state;  // 0x8
};

extern _tagTRCPadInfo gTrcPad[4];
extern _tagTRCState gTrcDisk[2];

void xTRCInit();
void xTRCPad(signed int pad_id, _tagTRCState state);
void xTRCDisk(_tagTRCState state);
void xTRCRender();

#endif
