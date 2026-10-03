#ifndef XSHADOW_H
#define XSHADOW_H

#include "xMath3.h"

class _xEnt;
class RpWorld;
class RwCamera;
class RwRaster;
class RpIntersection;
class RpWorldSector;
class RpCollisionTriangle;

void SetupShadow();
void xShadowSetWorld(RpWorld* world);
void xShadowSetLight(_xVec3* target_pos, _xVec3* in_vec, float shadow_depth);
void xShadowRenderWorld(_xVec3* center, float radius);
unsigned int xShadowReceiveShadowSetup(_xEnt* ent);
void xShadowReceiveShadow(_xEnt* ent);
void xShadowRender(_xEnt* ent);
signed int Im2DRenderQuad(float x1, float y1, float x2, float y2, float z, float recipCamZ,
                          float uvOffset);
void InvertRaster(RwCamera* cam);
RpCollisionTriangle* ShadowRenderTriangleCB(RpIntersection* intersection, RpWorldSector* sector,
                                            RpCollisionTriangle* collTriangle, float distance,
                                            void* data);
signed int ShadowRender(RwCamera* shadowCamera, RwRaster* shadow, RpWorld* world,
                        RpIntersection* shadowZone, float shadowStrength, float fadeDist);

#endif
