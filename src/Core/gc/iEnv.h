#ifndef IENV_H
#define IENV_H

class RpWorld;
class xEnvAsset;

class iEnv
{
public:
    RpWorld* world;
    RpWorld* collision;
    RpWorld* fx;
    RpWorld* camera;
    unsigned int unk10[4];
    unsigned int memlvl; // 0x20
    unsigned int unk24[3];
};

signed int iEnvBeginRenderFX(iEnv* env);
void iEnvRender(iEnv* env);
void iEnvLightingBasics(iEnv* env, xEnvAsset* asset);
void iEnvDefaultLighting(iEnv* env);
void iEnvFree(iEnv* env);
void iEnvLoad(iEnv* env, void* data, unsigned int datasize, signed int dataType);

#endif
