#ifndef IFX_H
#define IFX_H

class _xEnt;
class RxPipeline;

extern _xEnt* warbEnt;

RxPipeline* iFXanimUVCreatePipe();
void iFXWarbInit(void* vp_ent, unsigned int warb_mode);
void iFXWarbSetParams(unsigned int mode, float* val);
void iFXWarbUpdate(float dt);

#endif
