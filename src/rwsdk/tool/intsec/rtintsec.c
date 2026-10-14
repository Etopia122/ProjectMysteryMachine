/*
 * Intersection toolkit: sphere/triangle and box/triangle overlap tests used
 * by the collision plugin. Both start with trivial rejections that work on
 * the sign bits of the floats directly.
 */

#include "rwsdk/rpworld_int.h"

#define FLOATBITS(f) (*(RwInt32*)&(f))

RwReal _rwInvSqrt(RwReal num);

/* Rejects when all three vertices lie beyond the sphere on one side of an axis. */
#define SPHERETRIVIALREJECT(axis)                                                                      \
    do {                                                                                               \
        RwInt32 numNeg = (FLOATBITS(vc0.axis) >> 31) + (FLOATBITS(vc1.axis) >> 31) +              \
                         (FLOATBITS(vc2.axis) >> 31);                                                \
                                                                                                       \
        if (numNeg == 0) {                                                                             \
            RwReal t0 = vc0.axis - sphere->radius;                                                   \
            RwReal t1 = vc1.axis - sphere->radius;                                                   \
            RwReal t2 = vc2.axis - sphere->radius;                                                   \
                                                                                                       \
            if ((FLOATBITS(t0) | FLOATBITS(t1) | FLOATBITS(t2)) >= 0) {                                \
                return FALSE;                                                                          \
            }                                                                                          \
        } else if (numNeg == -3) {                                                                     \
            RwReal t0 = vc0.axis + sphere->radius;                                                   \
            RwReal t1 = vc1.axis + sphere->radius;                                                   \
            RwReal t2 = vc2.axis + sphere->radius;                                                   \
                                                                                                       \
            if ((FLOATBITS(t0) & FLOATBITS(t1) & FLOATBITS(t2)) < 0) {                                 \
                return FALSE;                                                                          \
            }                                                                                          \
        }                                                                                              \
    } while (0)

#define DOT(a, b) ((a).x * (b).x + (a).y * (b).y + (a).z * (b).z)

/* Rejects when the edge a->b separates the sphere centre from the third vertex c. */
#define SPHEREEDGEREJECT(a, b, c)                                                                      \
    do {                                                                                               \
        RwV3d edge;                                                                                    \
        RwV3d closest;                                                                                 \
        RwReal factor;                                                                                 \
                                                                                                       \
        edge.x = (b).x - (a).x;                                                                        \
        edge.y = (b).y - (a).y;                                                                        \
        edge.z = (b).z - (a).z;                                                                        \
        factor = DOT(edge, a) / DOT(edge, edge);                                                       \
        closest.x = (a).x - edge.x * factor;                                                           \
        closest.y = (a).y - edge.y * factor;                                                           \
        closest.z = (a).z - edge.z * factor;                                                           \
        length2 = DOT(closest, closest);                                                               \
        if (length2 > sphereRadiusSquared && length2 < DOT(closest, c)) {                              \
            return FALSE;                                                                              \
        }                                                                                              \
    } while (0)

RwBool RtIntersectionSphereTriangle(RwSphere* sphere, RwV3d* v0, RwV3d* v1, RwV3d* v2, RwV3d* normal,
                                    RwReal* distance)
{
    RwV3d vc0;
    RwV3d vc1;
    RwV3d vc2;
    RwReal len0;
    RwReal len1;
    RwReal nDotN;
    RwReal factor;
    RwReal len2;
    RwReal distToPlane;
    RwReal _result;
    RwReal length2;
    RwReal sphereRadiusSquared;

    vc0.x = v0->x - sphere->center.x;
    vc0.y = v0->y - sphere->center.y;
    vc0.z = v0->z - sphere->center.z;
    vc1.x = v1->x - sphere->center.x;
    vc1.y = v1->y - sphere->center.y;
    vc1.z = v1->z - sphere->center.z;
    vc2.x = v2->x - sphere->center.x;
    vc2.y = v2->y - sphere->center.y;
    vc2.z = v2->z - sphere->center.z;

    SPHERETRIVIALREJECT(x);
    SPHERETRIVIALREJECT(y);
    SPHERETRIVIALREJECT(z);

    {
        RwV3d a;
        RwV3d b;

        a.x = v1->x - v0->x;
        a.y = v1->y - v0->y;
        a.z = v1->z - v0->z;
        b.x = v2->x - v0->x;
        b.y = v2->y - v0->y;
        b.z = v2->z - v0->z;
        normal->x = a.y * b.z - a.z * b.y;
        normal->y = a.z * b.x - a.x * b.z;
        normal->z = a.x * b.y - a.y * b.x;
    }
    nDotN = normal->x * normal->x + normal->y * normal->y + normal->z * normal->z;
    if (nDotN <= 0.0f) {
        return FALSE;
    }
    _result = _rwInvSqrt(nDotN);
    normal->x *= _result;
    normal->y *= _result;
    normal->z *= _result;

    distToPlane = DOT(vc0, *normal);
    if (distToPlane < -sphere->radius || distToPlane > sphere->radius) {
        return FALSE;
    }
    *distance = -distToPlane;

    sphereRadiusSquared = sphere->radius * sphere->radius;
    len0 = DOT(vc0, vc0);
    if (len0 <= sphereRadiusSquared) {
        return TRUE;
    }
    len1 = DOT(vc1, vc1);
    if (len1 <= sphereRadiusSquared) {
        return TRUE;
    }
    len2 = DOT(vc2, vc2);
    if (len2 <= sphereRadiusSquared) {
        return TRUE;
    }

    /* The nearest vertex must be in front of both of its edges. */
    if (len0 < len1) {
        if (len2 < len0) {
            if (DOT(vc2, vc0) > len2 && DOT(vc2, vc1) > len2) {
                return FALSE;
            }
        } else {
            if (DOT(vc0, vc1) > len0 && DOT(vc0, vc2) > len0) {
                return FALSE;
            }
        }
    } else {
        if (len2 < len1) {
            if (DOT(vc2, vc0) > len2 && DOT(vc2, vc1) > len2) {
                return FALSE;
            }
        } else {
            if (DOT(vc1, vc0) > len1 && DOT(vc1, vc2) > len1) {
                return FALSE;
            }
        }
    }

    SPHEREEDGEREJECT(vc0, vc1, vc2);
    SPHEREEDGEREJECT(vc1, vc2, vc0);
    SPHEREEDGEREJECT(vc2, vc0, vc1);
    return TRUE;
}

/* Outcode of a point against the box: which faces it lies outside of. */
#define BOXOUTCODE(v)                                                                                  \
    ((((v)->x > bbox->sup.x) ? 1 : ((v)->x < bbox->inf.x) ? 2 : 0) |                                   \
     (((v)->y > bbox->sup.y) ? 4 : ((v)->y < bbox->inf.y) ? 8 : 0) |                                   \
     (((v)->z > bbox->sup.z) ? 0x10 : ((v)->z < bbox->inf.z) ? 0x20 : 0))

/*
 * Does the edge from a along d cross the face of the box on plane P (where
 * a is "rel" from that plane) within the face? Q and R are the other axes.
 */
#define BOXFACETEST(bit, d, P, Q, R, rel)                                                                  if (diff & (bit)) {                                                                                        RwReal tmp = rel.P * (d).Q;                                                                                                                                                                                   if ((d).P < 0.0f) {                                                                                        if (tmp < toSup.Q * (d).P && tmp > toInf.Q * (d).P) {                                                      tmp = rel.P * (d).R;                                                                                   if (tmp < toSup.R * (d).P && tmp > toInf.R * (d).P) {                                                      return TRUE;                                                                                       }                                                                                                  }                                                                                                  } else {                                                                                                   if (tmp > toSup.Q * (d).P && tmp < toInf.Q * (d).P) {                                                      tmp = rel.P * (d).R;                                                                                   if (tmp > toSup.R * (d).P && tmp < toInf.R * (d).P) {                                                      return TRUE;                                                                                       }                                                                                                  }                                                                                                  }                                                                                                  }

#define BOXEDGETEST(a, b, codeA, codeB)                                                                    if (!((codeA) & (codeB))) {                                                                                RwInt32 diff = (codeA) ^ (codeB);                                                                      RwV3d d;                                                                                               RwV3d toSup;                                                                                           RwV3d toInf;                                                                                                                                                                                                  d.x = (b)->x - (a)->x;                                                                                 d.y = (b)->y - (a)->y;                                                                                 d.z = (b)->z - (a)->z;                                                                                 toInf.x = (a)->x - bbox->inf.x;                                                                        toInf.y = (a)->y - bbox->inf.y;                                                                        toInf.z = (a)->z - bbox->inf.z;                                                                        toSup.x = (a)->x - bbox->sup.x;                                                                        toSup.y = (a)->y - bbox->sup.y;                                                                        toSup.z = (a)->z - bbox->sup.z;                                                                        BOXFACETEST(0x01, d, x, y, z, toSup)                                                                   BOXFACETEST(0x02, d, x, y, z, toInf)                                                                   BOXFACETEST(0x04, d, y, z, x, toSup)                                                                   BOXFACETEST(0x08, d, y, z, x, toInf)                                                                   BOXFACETEST(0x10, d, z, x, y, toSup)                                                                   BOXFACETEST(0x20, d, z, x, y, toInf)                                                               }

RwBool RtIntersectionBBoxTriangle(RwBBox* bbox, RwV3d* v0, RwV3d* v1, RwV3d* v2)
{
    RwInt32 v0Test;
    RwInt32 v1Test;
    RwInt32 v2Test;
    RwV3d norm;
    RwReal det;
    RwReal dist;

    v0Test = BOXOUTCODE(v0);
    if (!v0Test) {
        return TRUE;
    }
    v1Test = BOXOUTCODE(v1);
    if (!v1Test) {
        return TRUE;
    }
    v2Test = BOXOUTCODE(v2);
    if (!v2Test) {
        return TRUE;
    }
    if (v0Test & v1Test & v2Test) {
        return FALSE;
    }

    BOXEDGETEST(v0, v1, v0Test, v1Test)
    BOXEDGETEST(v0, v2, v0Test, v2Test)
    BOXEDGETEST(v1, v2, v1Test, v2Test)

    {
        RwV3d e1;
        RwV3d e2;
        RwV3d dir;
        RwV3d org;

        e1.x = v1->x - v0->x;
        e1.y = v1->y - v0->y;
        e1.z = v1->z - v0->z;
        e2.x = v2->x - v0->x;
        e2.y = v2->y - v0->y;
        e2.z = v2->z - v0->z;
        norm.x = e1.y * e2.z - e1.z * e2.y;
        norm.y = e1.z * e2.x - e1.x * e2.z;
        norm.z = e1.x * e2.y - e1.y * e2.x;

        /* The box diagonal most aligned with the normal. */
        dir.x = bbox->sup.x - bbox->inf.x;
        org.x = bbox->inf.x - v0->x;
        if ((FLOATBITS(norm.x) ^ FLOATBITS(norm.y)) < 0) {
            dir.y = bbox->inf.y - bbox->sup.y;
            org.y = bbox->sup.y - v0->y;
        } else {
            dir.y = bbox->sup.y - bbox->inf.y;
            org.y = bbox->inf.y - v0->y;
        }
        if ((FLOATBITS(norm.x) ^ FLOATBITS(norm.z)) < 0) {
            dir.z = bbox->inf.z - bbox->sup.z;
            org.z = bbox->sup.z - v0->z;
        } else {
            dir.z = bbox->sup.z - bbox->inf.z;
            org.z = bbox->inf.z - v0->z;
        }

        det = -DOT(dir, norm);
        dist = DOT(org, norm);
        if (det < 0.0f) {
            if (0.0f > dist && dist > det) {
                RwV3d c;
                RwReal u;
                RwReal v;

                c.x = org.y * dir.z - org.z * dir.y;
                c.y = org.z * dir.x - org.x * dir.z;
                c.z = org.x * dir.y - org.y * dir.x;
                u = DOT(e2, c);
                if (0.0f > u && u > det) {
                    v = -DOT(e1, c);
                    if (0.0f > v && u + v > det) {
                        return TRUE;
                    }
                }
            }
        } else {
            if (0.0f < dist && dist < det) {
                RwV3d c;
                RwReal u;
                RwReal v;

                c.x = org.y * dir.z - org.z * dir.y;
                c.y = org.z * dir.x - org.x * dir.z;
                c.z = org.x * dir.y - org.y * dir.x;
                u = DOT(e2, c);
                if (0.0f < u && u < det) {
                    v = -DOT(e1, c);
                    if (0.0f < v && u + v < det) {
                        return TRUE;
                    }
                }
            }
        }
    }
    return FALSE;
}
