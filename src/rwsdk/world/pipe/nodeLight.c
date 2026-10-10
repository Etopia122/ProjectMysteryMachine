/*
 * Light.csl: accumulates the contribution of every light in the packet's
 * lights cluster into the camera space vertex colours. Lights are applied
 * in object space, through the inverse of the object's matrix when it is
 * not the identity.
 */

#include "rwsdk/rpworld_int.h"
#include "rwsdk/p2core_int.h"

extern RxClusterDefinition RxClMeshState;
extern RxClusterDefinition RxClObjSpace3DVertices;
extern RxClusterDefinition RxClCamSpace3DVertices;
extern RxClusterDefinition RxClLights;
extern RxClusterDefinition RxClVSteps;

#define rpLIGHTDIRECTIONAL 1
#define rpLIGHTAMBIENT 2
#define rpLIGHTPOINT 0x80
#define rpLIGHTSPOT 0x81
#define rpLIGHTSPOTSOFT 0x82

#define rpLIGHTPRIVATENOCHROMA 0x01

#define rxCLFLAGS_CLUSTERVALID 0x01

typedef struct RwRGBAReal {
    RwReal red;
    RwReal green;
    RwReal blue;
    RwReal alpha;
} RwRGBAReal;

typedef struct RxObjSpace3DVertex {
    /* 0x00 */ RwV3d objVertex;
    /* 0x0C */ RwV3d objNormal;
    /* 0x18 */ RwRGBA color;
    /* 0x1C */ RwReal u;
    /* 0x20 */ RwReal v;
} RxObjSpace3DVertex;

typedef struct RxCamSpace3DVertex {
    /* 0x00 */ RwV3d cameraVertex;
    /* 0x0C */ RwUInt8 clipFlags;
    /* 0x0D */ RwUInt8 pad[3];
    /* 0x10 */ RwRGBAReal col;
    /* 0x20 */ RwReal u;
    /* 0x24 */ RwReal v;
} RxCamSpace3DVertex;

RwMatrix* RwMatrixCreate(void);
RwBool RwMatrixDestroy(RwMatrix* mpMat);
RwMatrix* RwMatrixInvert(RwMatrix* dst, const RwMatrix* src);
RwMatrix* RwFrameGetLTM(RwFrame* frame);
RwV3d* RwV3dTransformPoints(RwV3d* pointsOut, const RwV3d* pointsIn, RwInt32 numPoints, const RwMatrix* matrix);
RwV3d* RwV3dTransformVectors(RwV3d* vectorsOut, const RwV3d* vectorsIn, RwInt32 numPoints, const RwMatrix* matrix);
RwReal _rwV3dNormalize(RwV3d* out, const RwV3d* in);
RwReal _rwSqrt(RwReal x);

static void _rwApplyAmbientLight(RxPipelineNodeInstance* self, RxPacket* packet, RpLight* light,
                                 const RwMatrix* inverseMat, RwReal invScale, const RwSurfaceProperties* surfaceProps);
static void _rwApplyDirectionalLight(RxPipelineNodeInstance* self, RxPacket* packet, RpLight* light,
                                     const RwMatrix* inverseMat, RwReal invScale,
                                     const RwSurfaceProperties* surfaceProps);
static void _rwApplyPointLight(RxPipelineNodeInstance* self, RxPacket* packet, RpLight* light,
                               const RwMatrix* inverseMat, RwReal invScale, const RwSurfaceProperties* surfaceProps);
static void _rwApplySpotLight(RxPipelineNodeInstance* self, RxPacket* packet, RpLight* light,
                              const RwMatrix* inverseMat, RwReal invScale, const RwSurfaceProperties* surfaceProps);
static void _rwApplySoftSpotLight(RxPipelineNodeInstance* self, RxPacket* packet, RpLight* light,
                                  const RwMatrix* inverseMat, RwReal invScale,
                                  const RwSurfaceProperties* surfaceProps);

#define RxClusterResetCursor(_cl) ((_cl)->currentData = (_cl)->data)

#define RxClusterLockRead(_packet, _clusterIndex)                                                                      \
    (((RwInt32)((_packet)->inputToClusterSlot[(_clusterIndex)]) == -1)                                                 \
         ? (RxCluster*)NULL                                                                                            \
         : (RxClusterResetCursor(&(_packet)->clusters[(_packet)->inputToClusterSlot[(_clusterIndex)]]),               \
            &(_packet)->clusters[(_packet)->inputToClusterSlot[(_clusterIndex)]]))

#define RxClusterIncCursor(_cl) ((_cl)->currentData = (RwUInt8*)(_cl)->currentData + (_cl)->stride)

RwBool LightNode(RxPipelineNodeInstance* self, const RxPipelineNodeParam* params)
{
    RxPacket* packet = RxPacketFetch(self);
    RxCluster* meshState = RxClusterLockRead(packet, 0);
    RxMeshStateVector* meshData = RxClusterGetCursorData(meshState, RxMeshStateVector);

    if (meshData->ClipFlagsAnd == 0) {
        RxCluster* lights = RxClusterLockRead(packet, 3);
        RwUInt32 numLights = lights->numUsed;

        if (numLights) {
            RwMatrix* inverseMat;
            RwReal invScale;

            if (meshData->Obj2World.flags & rwMATRIXINTERNALIDENTITY) {
                invScale = 1.0f;
                inverseMat = NULL;
            } else {
                inverseMat = RwMatrixCreate();
                RwMatrixInvert(inverseMat, &meshData->Obj2World);
                invScale = _rwSqrt(inverseMat->at.x * inverseMat->at.x + inverseMat->at.y * inverseMat->at.y +
                                   inverseMat->at.z * inverseMat->at.z);
            }

            do {
                RpLight* light = *RxClusterGetCursorData(lights, RpLight*);

                switch (light->object.object.subType) {
                case rpLIGHTAMBIENT:
                    _rwApplyAmbientLight(self, packet, light, inverseMat, invScale,
                                         (RwSurfaceProperties*)&meshData->SurfaceProperties);
                    break;
                case rpLIGHTDIRECTIONAL:
                    _rwApplyDirectionalLight(self, packet, light, inverseMat, invScale,
                                             (RwSurfaceProperties*)&meshData->SurfaceProperties);
                    break;
                case rpLIGHTPOINT:
                    _rwApplyPointLight(self, packet, light, inverseMat, invScale,
                                       (RwSurfaceProperties*)&meshData->SurfaceProperties);
                    break;
                case rpLIGHTSPOT:
                    _rwApplySpotLight(self, packet, light, inverseMat, invScale,
                                      (RwSurfaceProperties*)&meshData->SurfaceProperties);
                    break;
                case rpLIGHTSPOTSOFT:
                    _rwApplySoftSpotLight(self, packet, light, inverseMat, invScale,
                                          (RwSurfaceProperties*)&meshData->SurfaceProperties);
                    break;
                }
                RxClusterIncCursor(lights);
            } while (--numLights);

            if (inverseMat) {
                RwMatrixDestroy(inverseMat);
            }
        }
    }

    RxPacketDispatch(packet, 0, self);

    return TRUE;
}

RxNodeDefinition* RxNodeDefinitionGetLight(void)
{
    static RxClusterRef N1clofinterest[] = {
        { &RxClMeshState, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClObjSpace3DVertices, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClCamSpace3DVertices, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClLights, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClVSteps, rxCLALLOWABSENT, rxCLRESERVED },
    };

    static RxClusterValidityReq N1inputreqs[] = {
        rxCLREQ_REQUIRED, rxCLREQ_REQUIRED, rxCLREQ_REQUIRED, rxCLREQ_OPTIONAL, rxCLREQ_OPTIONAL,
    };

#pragma explicit_zero_data on
    static RxClusterValid N1outcl1[] = {
        rxCLVALID_NOCHANGE, rxCLVALID_NOCHANGE, rxCLVALID_VALID, rxCLVALID_NOCHANGE, rxCLVALID_NOCHANGE,
    };
#pragma explicit_zero_data reset

    static RwChar _Output[] = "Output";

    static RxOutputSpec N1outputs[] = { { _Output, N1outcl1, rxCLVALID_NOCHANGE } };

    static RwChar _Light_csl[] = "Light.csl";

    static RxNodeDefinition nodeLightCSL = {
        _Light_csl,
        { LightNode, NULL, NULL, NULL, NULL, NULL, NULL },
        { 5, N1clofinterest, N1inputreqs, 1, N1outputs },
        0,
        0,
        0,
    };

    return &nodeLightCSL;
}

/* Vertex walking shared by the light functions: start at the first vertex
 * the vertex steps cluster asks for, then advance by its per vertex steps. */
#define LIGHTVERTSSETUP()                                                                                              \
    RxCluster* objVerts = RxClusterLockRead(packet, 1);                                                                \
    RxCluster* camVerts = RxClusterLockWrite(packet, 2, self);                                                         \
    RxCluster* vSteps = RxClusterLockRead(packet, 4);                                                                  \
    RwUInt32 objStride = objVerts->stride;                                                                             \
    RwUInt8* objV = (RwUInt8*)objVerts->currentData;                                                                   \
    RxCamSpace3DVertex* camV = (RxCamSpace3DVertex*)camVerts->currentData;                                             \
    RwUInt8* steps;                                                                                                    \
    RwUInt32 numVerts;                                                                                                 \
                                                                                                                       \
    if (vSteps && (vSteps->flags & rxCLFLAGS_CLUSTERVALID)) {                                                          \
        RwUInt32 skip;                                                                                                 \
                                                                                                                       \
        steps = (RwUInt8*)vSteps->data;                                                                                \
        numVerts = vSteps->numUsed;                                                                                    \
        skip = *steps++;                                                                                               \
        objV += skip * objStride;                                                                                      \
        camV += skip;                                                                                                  \
    } else {                                                                                                           \
        steps = NULL;                                                                                                  \
        numVerts = camVerts->numUsed;                                                                                  \
    }

#define LIGHTVERTSNEXT()                                                                                               \
    do {                                                                                                               \
        RwUInt32 _skip = 1;                                                                                            \
                                                                                                                       \
        if (steps) {                                                                                                   \
            _skip = *steps++;                                                                                          \
        }                                                                                                              \
        do {                                                                                                           \
            objV += objStride;                                                                                         \
            camV++;                                                                                                    \
        } while (--_skip);                                                                                             \
    } while (0)

static void _rwApplyAmbientLight(RxPipelineNodeInstance* self, RxPacket* packet, RpLight* light,
                                 const RwMatrix* inverseMat, RwReal invScale, const RwSurfaceProperties* surfaceProps)
{
    RxCluster* camVerts = RxClusterLockWrite(packet, 2, self);
    RxCluster* vSteps = RxClusterLockRead(packet, 4);
    RxCamSpace3DVertex* camV = (RxCamSpace3DVertex*)camVerts->currentData;
    RwUInt8* steps;
    RwUInt32 numVerts;
    RwUInt32 i;

    if (vSteps && (vSteps->flags & rxCLFLAGS_CLUSTERVALID)) {
        steps = (RwUInt8*)vSteps->data;
        numVerts = vSteps->numUsed;
        camV += *steps++;
    } else {
        steps = NULL;
        numVerts = camVerts->numUsed;
    }

    if (light->object.object.privateFlags & rpLIGHTPRIVATENOCHROMA) {
        RwReal lum = 255.0f * light->color[0] * surfaceProps->ambient;

        for (i = 0; i < numVerts; i++) {
            camV->col.red += lum;
            camV->col.green += lum;
            camV->col.blue += lum;
            if (steps) {
                camV += *steps++;
            } else {
                camV++;
            }
        }
    } else {
        RwReal scale = 255.0f * surfaceProps->ambient;

        for (i = 0; i < numVerts; i++) {
            camV->col.red += scale * light->color[0];
            camV->col.green += scale * light->color[1];
            camV->col.blue += scale * light->color[2];
            if (steps) {
                camV += *steps++;
            } else {
                camV++;
            }
        }
    }
}

static void _rwApplyDirectionalLight(RxPipelineNodeInstance* self, RxPacket* packet, RpLight* light,
                                     const RwMatrix* inverseMat, RwReal invScale,
                                     const RwSurfaceProperties* surfaceProps)
{
    LIGHTVERTSSETUP();

    if (light->object.object.parent) {
        RwV3d at = RwFrameGetLTM((RwFrame*)light->object.object.parent)->at;
        RwUInt32 i;

        if (inverseMat) {
            RwV3dTransformVectors(&at, &at, 1, inverseMat);
            _rwV3dNormalize(&at, &at);
        }
        at.x = -1.0f * at.x;
        at.y = -1.0f * at.y;
        at.z = -1.0f * at.z;

        if (light->object.object.privateFlags & rpLIGHTPRIVATENOCHROMA) {
            RwReal scale = 255.0f * light->color[0] * surfaceProps->diffuse;

            for (i = 0; i < numVerts; i++) {
                RwV3d* normal = &((RxObjSpace3DVertex*)objV)->objNormal;
                RwReal dot = at.x * normal->x + at.y * normal->y + at.z * normal->z;

                if (dot > 0.0f) {
                    RwReal lum = dot * scale;

                    camV->col.red += lum;
                    camV->col.green += lum;
                    camV->col.blue += lum;
                }
                LIGHTVERTSNEXT();
            }
        } else {
            RwReal scale = 255.0f * surfaceProps->diffuse;

            for (i = 0; i < numVerts; i++) {
                RwV3d* normal = &((RxObjSpace3DVertex*)objV)->objNormal;
                RwReal dot = at.x * normal->x + at.y * normal->y + at.z * normal->z;

                if (dot > 0.0f) {
                    RwReal lum = dot * scale;

                    camV->col.red += lum * light->color[0];
                    camV->col.green += lum * light->color[1];
                    camV->col.blue += lum * light->color[2];
                }
                LIGHTVERTSNEXT();
            }
        }
    }
}

static void _rwApplyPointLight(RxPipelineNodeInstance* self, RxPacket* packet, RpLight* light,
                               const RwMatrix* inverseMat, RwReal invScale, const RwSurfaceProperties* surfaceProps)
{
    RwV3d pos;
    RwReal radSq;
    RwReal recipRad;

    LIGHTVERTSSETUP();

    pos = RwFrameGetLTM((RwFrame*)light->object.object.parent)->pos;
    if (inverseMat) {
        RwReal radius = light->radius * invScale;

        recipRad = 1.0f / radius;
        radSq = radius * radius;
        RwV3dTransformPoints(&pos, &pos, 1, inverseMat);
    } else {
        radSq = light->radius * light->radius;
        recipRad = 1.0f / light->radius;
    }

    if (light->object.object.privateFlags & rpLIGHTPRIVATENOCHROMA) {
        RwReal scale = 255.0f * light->color[0] * surfaceProps->diffuse;

        while (numVerts--) {
            RxObjSpace3DVertex* ov = (RxObjSpace3DVertex*)objV;
            RwV3d vertToLight;
            RwReal dot;

            vertToLight.x = pos.x - ov->objVertex.x;
            vertToLight.y = pos.y - ov->objVertex.y;
            vertToLight.z = pos.z - ov->objVertex.z;
            dot = vertToLight.x * ov->objNormal.x + vertToLight.y * ov->objNormal.y + vertToLight.z * ov->objNormal.z;

            if (dot > 0.0f) {
                RwReal distSq =
                    vertToLight.x * vertToLight.x + vertToLight.y * vertToLight.y + vertToLight.z * vertToLight.z;

                if (distSq < radSq) {
                    RwReal dist = _rwSqrt(distSq);
                    RwReal lum = scale * dot;
                    RwReal recipDist = (dist > 0.0f) ? 1.0f / dist : 0.0f;

                    lum *= recipDist - recipRad;
                    camV->col.red += lum;
                    camV->col.green += lum;
                    camV->col.blue += lum;
                }
            }
            LIGHTVERTSNEXT();
        }
    } else {
        RwReal scale = 255.0f * surfaceProps->diffuse;

        while (numVerts--) {
            RxObjSpace3DVertex* ov = (RxObjSpace3DVertex*)objV;
            RwV3d vertToLight;
            RwReal dot;

            vertToLight.x = pos.x - ov->objVertex.x;
            vertToLight.y = pos.y - ov->objVertex.y;
            vertToLight.z = pos.z - ov->objVertex.z;
            dot = vertToLight.x * ov->objNormal.x + vertToLight.y * ov->objNormal.y + vertToLight.z * ov->objNormal.z;

            if (dot > 0.0f) {
                RwReal distSq =
                    vertToLight.x * vertToLight.x + vertToLight.y * vertToLight.y + vertToLight.z * vertToLight.z;

                if (distSq < radSq) {
                    RwReal dist = _rwSqrt(distSq);
                    RwReal lum = scale * dot;
                    RwReal recipDist = (dist > 0.0f) ? 1.0f / dist : 0.0f;

                    lum *= recipDist - recipRad;
                    camV->col.red += lum * light->color[0];
                    camV->col.green += lum * light->color[1];
                    camV->col.blue += lum * light->color[2];
                }
            }
            LIGHTVERTSNEXT();
        }
    }
}

static void _rwApplySpotLight(RxPipelineNodeInstance* self, RxPacket* packet, RpLight* light,
                              const RwMatrix* inverseMat, RwReal invScale, const RwSurfaceProperties* surfaceProps)
{
    RwV3d at;
    RwV3d pos;
    RwReal radSq;
    RwReal recipRad;

    LIGHTVERTSSETUP();

    pos = RwFrameGetLTM((RwFrame*)light->object.object.parent)->pos;
    at = RwFrameGetLTM((RwFrame*)light->object.object.parent)->at;
    if (inverseMat) {
        RwReal radius = light->radius * invScale;

        recipRad = 1.0f / radius;
        radSq = radius * radius;
        RwV3dTransformPoints(&pos, &pos, 1, inverseMat);
        RwV3dTransformVectors(&at, &at, 1, inverseMat);
        _rwV3dNormalize(&at, &at);
    } else {
        radSq = light->radius * light->radius;
        recipRad = 1.0f / light->radius;
    }

    if (light->object.object.privateFlags & rpLIGHTPRIVATENOCHROMA) {
        RwReal scale = 255.0f * light->color[0] * surfaceProps->diffuse;

        while (numVerts--) {
            RxObjSpace3DVertex* ov = (RxObjSpace3DVertex*)objV;
            RwV3d vertToLight;
            RwReal dot;

            vertToLight.x = pos.x - ov->objVertex.x;
            vertToLight.y = pos.y - ov->objVertex.y;
            vertToLight.z = pos.z - ov->objVertex.z;
            dot = vertToLight.x * ov->objNormal.x + vertToLight.y * ov->objNormal.y + vertToLight.z * ov->objNormal.z;

            if (dot <= 0.0f) {
                continue;
            }
            {
                RwReal distSq =
                    vertToLight.x * vertToLight.x + vertToLight.y * vertToLight.y + vertToLight.z * vertToLight.z;

                if (distSq < radSq) {
                    RwReal dist = _rwSqrt(distSq);

                    if (vertToLight.x * at.x + vertToLight.y * at.y + vertToLight.z * at.z <
                        dist * light->minusCosAngle) {
                        RwReal lum = scale * dot;
                        RwReal recipDist = (dist > 0.0f) ? 1.0f / dist : 0.0f;

                        lum *= recipDist - recipRad;
                        camV->col.red += lum;
                        camV->col.green += lum;
                        camV->col.blue += lum;
                    }
                }
            }
            LIGHTVERTSNEXT();
        }
    } else {
        RwReal scale = 255.0f * surfaceProps->diffuse;

        while (numVerts--) {
            RxObjSpace3DVertex* ov = (RxObjSpace3DVertex*)objV;
            RwV3d vertToLight;
            RwReal dot;

            vertToLight.x = pos.x - ov->objVertex.x;
            vertToLight.y = pos.y - ov->objVertex.y;
            vertToLight.z = pos.z - ov->objVertex.z;
            dot = vertToLight.x * ov->objNormal.x + vertToLight.y * ov->objNormal.y + vertToLight.z * ov->objNormal.z;

            if (dot <= 0.0f) {
                continue;
            }
            {
                RwReal distSq =
                    vertToLight.x * vertToLight.x + vertToLight.y * vertToLight.y + vertToLight.z * vertToLight.z;

                if (distSq < radSq) {
                    RwReal dist = _rwSqrt(distSq);

                    if (vertToLight.x * at.x + vertToLight.y * at.y + vertToLight.z * at.z <
                        dist * light->minusCosAngle) {
                        RwReal lum = scale * dot;
                        RwReal recipDist = (dist > 0.0f) ? 1.0f / dist : 0.0f;

                        lum *= recipDist - recipRad;
                        camV->col.red += lum * light->color[0];
                        camV->col.green += lum * light->color[1];
                        camV->col.blue += lum * light->color[2];
                    }
                }
            }
            LIGHTVERTSNEXT();
        }
    }
}

static void _rwApplySoftSpotLight(RxPipelineNodeInstance* self, RxPacket* packet, RpLight* light,
                                  const RwMatrix* inverseMat, RwReal invScale,
                                  const RwSurfaceProperties* surfaceProps)
{
    RwV3d at;
    RwV3d pos;
    RwReal radSq;
    RwReal recipRad;

    LIGHTVERTSSETUP();

    pos = RwFrameGetLTM((RwFrame*)light->object.object.parent)->pos;
    at = RwFrameGetLTM((RwFrame*)light->object.object.parent)->at;
    if (inverseMat) {
        RwReal radius = light->radius * invScale;

        recipRad = 1.0f / radius;
        radSq = radius * radius;
        RwV3dTransformPoints(&pos, &pos, 1, inverseMat);
        RwV3dTransformVectors(&at, &at, 1, inverseMat);
        _rwV3dNormalize(&at, &at);
    } else {
        radSq = light->radius * light->radius;
        recipRad = 1.0f / light->radius;
    }

    if (light->object.object.privateFlags & rpLIGHTPRIVATENOCHROMA) {
        RwReal scale = 255.0f * light->color[0] * surfaceProps->diffuse;

        while (numVerts--) {
            RxObjSpace3DVertex* ov = (RxObjSpace3DVertex*)objV;
            RwV3d vertToLight;
            RwReal dot;

            vertToLight.x = pos.x - ov->objVertex.x;
            vertToLight.y = pos.y - ov->objVertex.y;
            vertToLight.z = pos.z - ov->objVertex.z;
            dot = vertToLight.x * ov->objNormal.x + vertToLight.y * ov->objNormal.y + vertToLight.z * ov->objNormal.z;

            if (dot > 0.0f) {
                RwReal distSq =
                    vertToLight.x * vertToLight.x + vertToLight.y * vertToLight.y + vertToLight.z * vertToLight.z;

                if (distSq < radSq) {
                    RwReal dist = _rwSqrt(distSq);
                    RwReal cosDist = vertToLight.x * at.x + vertToLight.y * at.y + vertToLight.z * at.z;
                    RwReal coneDist = dist * light->minusCosAngle;

                    if (cosDist < coneDist) {
                        RwReal lum = scale * dot;
                        RwReal recipDist = (dist > 0.0f) ? 1.0f / dist : 0.0f;
                        RwReal range;

                        lum *= recipDist - recipRad;
                        range = dist + coneDist;
                        if (range > 0.0f) {
                            RwReal fade = (dist + cosDist) / range;

                            lum *= 1.0f - fade * fade;
                        }
                        camV->col.red += lum;
                        camV->col.green += lum;
                        camV->col.blue += lum;
                    }
                }
            }
            LIGHTVERTSNEXT();
        }
    } else {
        RwReal scale = 255.0f * surfaceProps->diffuse;

        while (numVerts--) {
            RxObjSpace3DVertex* ov = (RxObjSpace3DVertex*)objV;
            RwV3d vertToLight;
            RwReal dot;

            vertToLight.x = pos.x - ov->objVertex.x;
            vertToLight.y = pos.y - ov->objVertex.y;
            vertToLight.z = pos.z - ov->objVertex.z;
            dot = vertToLight.x * ov->objNormal.x + vertToLight.y * ov->objNormal.y + vertToLight.z * ov->objNormal.z;

            if (dot > 0.0f) {
                RwReal distSq =
                    vertToLight.x * vertToLight.x + vertToLight.y * vertToLight.y + vertToLight.z * vertToLight.z;

                if (distSq < radSq) {
                    RwReal dist = _rwSqrt(distSq);
                    RwReal cosDist = vertToLight.x * at.x + vertToLight.y * at.y + vertToLight.z * at.z;
                    RwReal coneDist = dist * light->minusCosAngle;

                    if (cosDist < coneDist) {
                        RwReal lum = scale * dot;
                        RwReal recipDist = (dist > 0.0f) ? 1.0f / dist : 0.0f;
                        RwReal range;

                        lum *= recipDist - recipRad;
                        range = dist + coneDist;
                        if (range > 0.0f) {
                            RwReal fade = (dist + cosDist) / range;

                            lum *= 1.0f - fade * fade;
                        }
                        camV->col.red += lum * light->color[0];
                        camV->col.green += lum * light->color[1];
                        camV->col.blue += lum * light->color[2];
                    }
                }
            }
            LIGHTVERTSNEXT();
        }
    }
}
