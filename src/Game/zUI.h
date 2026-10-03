#ifndef ZUI_H
#define ZUI_H

#include "zEnt.h"

class zScene;

class zUIAsset : public p2EntAsset
{
public:
    unsigned int uiFlags;   // 0x50
    unsigned short dim[2];  // 0x54
    unsigned int textureID; // 0x58
    float uva[2];           // 0x5C
    float uvb[2];           // 0x64
    float uvc[2];           // 0x6C
    float uvd[2];           // 0x74
};

class zUIFontAsset : public zUIAsset
{
public:
    unsigned short uiFontFlags;   // 0x7C
    unsigned char mode;           // 0x7E
    unsigned char fontID;         // 0x7F
    unsigned int textAssetID;     // 0x80
    unsigned char bcolor[4];      // 0x84
    unsigned char color[4];       // 0x88
    unsigned short inset[4];      // 0x8C
    unsigned short space[2];      // 0x94
    unsigned short cdim[2];       // 0x98
};

class _zUI : public _zEnt
{
public:
    zUIAsset* sasset;      // 0xEC
    unsigned int uiFlags;  // 0xF0
    unsigned int uiButton; // 0xF4
};

class _zUIFont : public _zUI
{
public:
    zUIFontAsset* fasset;           // 0xF8
    unsigned short uiFontFlags;     // 0xFC
    unsigned short uiFontHackFlags; // 0xFE
};

void zUI_Init(void* ent, void* asset);
void zUI_Init(_zUI* ent, p2EntAsset* asset);
void zUI_Save(_zUI* ent, xSerial* s);
void zUI_Load(_zUI* ent, xSerial* s);
void zUI_Reset(_zUI* ent);
void zUI_PreUpdate(_zUI* ent);
void zUI_Update(_zUI* ent, _xScene* sc, float dt);
xBase* zUIRenderIteratorInsert(xBase* b, zScene* sc, void* data);
signed int iRenderQSort_Face(void* a, void* b);
void zUIRenderAll();
void zUI_Render(_xEnt* ent);
signed int zUIEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                      xBase* toParamWidget);

void zUIFont_Render(_xEnt* e);

#endif
