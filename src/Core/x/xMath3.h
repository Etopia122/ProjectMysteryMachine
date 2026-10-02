#ifndef XMATH3_H
#define XMATH3_H

class _xVec3
{
public:
    float x;
    float y;
    float z;
};

class _xMat3x3
{
public:
    _xVec3 right;
    signed int flags;
    _xVec3 up;
    unsigned int pad1;
    _xVec3 at;
    unsigned int pad2;
};

class _xMat4x3
{
public:
    _xVec3 right;
    signed int flags;
    _xVec3 up;
    unsigned int pad1;
    _xVec3 at;
    unsigned int pad2;
    _xVec3 pos;
    unsigned int pad3;
};

class _xRot
{
public:
    _xVec3 axis;
    float angle;
};

void xMat3x3GetEuler(_xMat3x3* m, _xVec3* a);
void xMat3x3RotY(_xMat3x3* m, float t);
void xMat3x3Mul(_xMat3x3* o, _xMat3x3* a, _xMat3x3* b);
void xMat3x3RMulVec(_xVec3* o, _xMat3x3* m, _xVec3* v);
void xMat3x3Tolocal(_xVec3* o, _xMat3x3* m, _xVec3* v);

inline void xVec3Copy(_xVec3* o, _xVec3* v)
{
    o->x = v->x;
    o->y = v->y;
    o->z = v->z;
}

inline void xVec3Sub(_xVec3* o, _xVec3* a, _xVec3* b)
{
    o->x = a->x - b->x;
    o->y = a->y - b->y;
    o->z = a->z - b->z;
}

inline void xVec3AddTo(_xVec3* o, _xVec3* v)
{
    o->x += v->x;
    o->y += v->y;
    o->z += v->z;
}

inline void xVec3SMulBy(_xVec3* v, float s)
{
    v->x *= s;
    v->y *= s;
    v->z *= s;
}

inline void xMat4x3Toworld(_xVec3* o, _xMat4x3* m, _xVec3* v)
{
    xMat3x3RMulVec(o, (_xMat3x3*)m, v);
    xVec3AddTo(o, &m->pos);
}

inline void xMat4x3Tolocal(_xVec3* o, _xMat4x3* m, _xVec3* v)
{
    xVec3Sub(o, v, &m->pos);
    xMat3x3Tolocal(o, (_xMat3x3*)m, o);
}

#endif
