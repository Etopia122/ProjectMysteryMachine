// Probe source for a few xMath / xMath3 / xString functions, written from the
// GameCube disassembly (types from the PS2 DWARF dump).

extern "C" double fmod(double, double);
extern "C" double sin(double);
extern "C" double cos(double);

struct _xVec3
{
    float x, y, z;
};

struct _xQuat
{
    _xVec3 v;
    float s;
};

struct _xMat3x3
{
    _xVec3 right;
    signed int flags;
    _xVec3 up;
    unsigned int pad1;
    _xVec3 at;
    unsigned int pad2;
};

struct _xMat4x3 : _xMat3x3
{
    _xVec3 pos;
    unsigned int pad3;
};

extern _xVec3 g_Y3;

#define PI 3.1415927f

float xAngleClamp(float a)
{
    float rem = fmod(a, 2.0f * PI);
    if (rem >= PI)
    {
        rem -= 2.0f * PI;
    }
    else if (rem < -PI)
    {
        rem += 2.0f * PI;
    }
    return rem;
}

void xQuatMul(_xQuat* o, _xQuat* a, _xQuat* b)
{
    float x = a->s * b->v.x + a->v.x * b->s + a->v.y * b->v.z - a->v.z * b->v.y;
    float y = a->s * b->v.y + a->v.y * b->s + a->v.z * b->v.x - a->v.x * b->v.z;
    float z = a->s * b->v.z + a->v.z * b->s + a->v.x * b->v.y - a->v.y * b->v.x;
    float s = a->s * b->s - a->v.x * b->v.x - a->v.y * b->v.y - a->v.z * b->v.z;
    o->v.x = x;
    o->v.y = y;
    o->v.z = z;
    o->s = s;
}

void xMat3x3RMulVec(_xVec3* o, _xMat3x3* m, _xVec3* v)
{
    float x = m->right.x * v->x + m->up.x * v->y + m->at.x * v->z;
    float y = m->right.y * v->x + m->up.y * v->y + m->at.y * v->z;
    float z = m->right.z * v->x + m->up.z * v->y + m->at.z * v->z;
    o->x = x;
    o->y = y;
    o->z = z;
}

inline void xVec3Init(_xVec3* v, float x, float y, float z)
{
    v->x = x;
    v->y = y;
    v->z = z;
}

inline void xVec3Copy(_xVec3* dst, const _xVec3* src)
{
    dst->x = src->x;
    dst->y = src->y;
    dst->z = src->z;
}

void xMat3x3RotY(_xMat3x3* m, float t)
{
    float c = cos(t);
    float s = sin(t);
    xVec3Init(&m->right, c, 0.0f, -s);
    xVec3Copy(&m->up, &g_Y3);
    xVec3Init(&m->at, s, 0.0f, c);
    m->flags = 0;
}

void xMat4x3MoveLocalAt(_xMat4x3* m, float mag)
{
    m->pos.x += m->at.x * mag;
    m->pos.y += m->at.y * mag;
    m->pos.z += m->at.z * mag;
}

inline char xToUpper(char c)
{
    return c - (c & (c >> 1) & 0x20);
}

unsigned int xStrHash(char* s)
{
    unsigned int hash = 0;
    char c;
    while ((c = *s) != 0)
    {
        s++;
        hash = xToUpper(c) + hash * 131;
    }
    return hash;
}
