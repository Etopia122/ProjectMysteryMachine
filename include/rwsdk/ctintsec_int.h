#ifndef RWSDK_CTINTSEC_INT_H
#define RWSDK_CTINTSEC_INT_H

/*
 * Collision plugin intersection types and the line/triangle test shared by
 * the geometry and world sector queries.
 */

#include "rwsdk/rpworld_int.h"

#define rtINTSECEPSILON 1e-08f
#define rtINTSECEDGEEPS 1e-05f

typedef enum RpIntersectType {
    rpINTERSECTNONE = 0,
    rpINTERSECTLINE = 1,
    rpINTERSECTPOINT = 2,
    rpINTERSECTSPHERE = 3,
    rpINTERSECTBOX = 4,
    rpINTERSECTATOMIC = 5
} RpIntersectType;

typedef union RpIntersectData {
    RwLine line;
    RwV3d point;
    RwSphere sphere;
    RwBBox box;
    void* object;
} RpIntersectData;

typedef struct RpIntersection {
    /* 0x00 */ RpIntersectData t;
    /* 0x18 */ RpIntersectType type;
} RpIntersection;

typedef struct RpCollisionTriangle {
    /* 0x00 */ RwV3d normal;
    /* 0x0C */ RwV3d point;
    /* 0x18 */ RwInt32 index;
    /* 0x1C */ RwV3d* vertices[3];
} RpCollisionTriangle;

typedef RpCollisionTriangle* (*RpIntersectionCallBackGeometryTriangle)(RpIntersection* intersection,
                                                                       RpCollisionTriangle* collTriangle,
                                                                       RwReal distance, void* data);

/*
 * Line against triangle: the line starts at lineStart and runs along
 * lineDelta. On a hit, *position is the parametric distance along the line.
 */
#define LINETRIANGLE(_result, _lineStart, _lineDelta, _v0, _v1, _v2, _position)                        \
    do {                                                                                               \
        RwV3d _edge1;                                                                                  \
        RwV3d _edge2;                                                                                  \
        RwV3d _p;                                                                                      \
        RwReal _det;                                                                                   \
                                                                                                       \
        _edge1.x = (_v1)->x - (_v0)->x;                                                                \
        _edge1.y = (_v1)->y - (_v0)->y;                                                                \
        _edge1.z = (_v1)->z - (_v0)->z;                                                                \
        _edge2.x = (_v2)->x - (_v0)->x;                                                                \
        _edge2.y = (_v2)->y - (_v0)->y;                                                                \
        _edge2.z = (_v2)->z - (_v0)->z;                                                                \
        _p.x = (_lineDelta)->y * _edge2.z - (_lineDelta)->z * _edge2.y;                                \
        _p.y = (_lineDelta)->z * _edge2.x - (_lineDelta)->x * _edge2.z;                                \
        _p.z = (_lineDelta)->x * _edge2.y - (_lineDelta)->y * _edge2.x;                                \
        _det = _edge1.x * _p.x + _edge1.y * _p.y + _edge1.z * _p.z;                                    \
        (_result) = _det > rtINTSECEPSILON;                                                            \
        if (_result) {                                                                                 \
            RwV3d _t;                                                                                  \
            RwReal _lo;                                                                                \
            RwReal _hi;                                                                                \
            RwReal _u;                                                                                 \
                                                                                                       \
            _lo = rtINTSECEDGEEPS * -_det;                                                             \
            _hi = _det - _lo;                                                                          \
            _t.x = (_lineStart)->x - (_v0)->x;                                                         \
            _t.y = (_lineStart)->y - (_v0)->y;                                                         \
            _t.z = (_lineStart)->z - (_v0)->z;                                                         \
            _u = _t.x * _p.x + _t.y * _p.y + _t.z * _p.z;                                              \
            (_result) = (_u >= _lo && _u <= _hi);                                                      \
            if (_result) {                                                                             \
                RwV3d _q;                                                                              \
                RwReal _v;                                                                             \
                                                                                                       \
                _q.x = _t.y * _edge1.z - _t.z * _edge1.y;                                              \
                _q.y = _t.z * _edge1.x - _t.x * _edge1.z;                                              \
                _q.z = _t.x * _edge1.y - _t.y * _edge1.x;                                              \
                _v = (_lineDelta)->x * _q.x + (_lineDelta)->y * _q.y + (_lineDelta)->z * _q.z;         \
                (_result) = (_v >= _lo && (_u + _v) <= _hi);                                           \
                if (_result) {                                                                         \
                    *(_position) = _edge2.x * _q.x + _edge2.y * _q.y + _edge2.z * _q.z;                \
                    (_result) = (*(_position) >= _lo && *(_position) <= _hi);                          \
                    if (_result) {                                                                     \
                        *(_position) /= _det;                                                          \
                    }                                                                                  \
                }                                                                                      \
            }                                                                                          \
        }                                                                                              \
    } while (0)

/* Unit normal of the triangle (v0, v1, v2). */
#define TRIANGLENORMAL(_normal, _v0, _v1, _v2)                                                         \
    do {                                                                                               \
        RwV3d edge1;                                                                                   \
        RwV3d edgetmp;                                                                                 \
        RwReal lengthSq;                                                                               \
        RwReal _result;                                                                                \
                                                                                                       \
        edge1.x = (_v1)->x - (_v0)->x;                                                                 \
        edge1.y = (_v1)->y - (_v0)->y;                                                                 \
        edge1.z = (_v1)->z - (_v0)->z;                                                                 \
        edgetmp.x = (_v2)->x - (_v0)->x;                                                               \
        edgetmp.y = (_v2)->y - (_v0)->y;                                                               \
        edgetmp.z = (_v2)->z - (_v0)->z;                                                               \
        (_normal)->x = edge1.y * edgetmp.z - edge1.z * edgetmp.y;                                      \
        (_normal)->y = edge1.z * edgetmp.x - edge1.x * edgetmp.z;                                      \
        (_normal)->z = edge1.x * edgetmp.y - edge1.y * edgetmp.x;                                      \
        lengthSq = (_normal)->x * (_normal)->x + (_normal)->y * (_normal)->y +                         \
                   (_normal)->z * (_normal)->z;                                                        \
        _result = _rwInvSqrt(lengthSq);                                                                \
        (_normal)->x *= _result;                                                                       \
        (_normal)->y *= _result;                                                                       \
        (_normal)->z *= _result;                                                                       \
    } while (0)

#endif
