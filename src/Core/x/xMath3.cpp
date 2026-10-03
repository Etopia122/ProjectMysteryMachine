#define XMATH3_DEFINE_GLOBALS
#include "xMath3.h"
#include "xMath.h"
#include "iMath3.h"

extern "C" {
void* memcpy(void* dst, const void* src, unsigned int n);
double asin(double);
double acos(double);
}


void iMath3Init();
float xAngleClamp(float a);
void iMath3Exit();

extern const _xVec3 g_O3 = { 0.0f, 0.0f, 0.0f };
extern const _xVec3 g_Y3 = { 0.0f, 1.0f, 0.0f };
extern const _xVec3 g_Z3 = { 0.0f, 0.0f, 1.0f };
extern const _xVec3 g_NY3 = { 0.0f, -1.0f, 0.0f };
extern const _xMat4x3 g_I3 = {
    { 1.0f, 0.0f, 0.0f }, 0, { 0.0f, 1.0f, 0.0f }, 0, { 0.0f, 0.0f, 1.0f }, 0, { 0.0f, 0.0f, 0.0f }, 0,
};
extern const _xQuat g_IQ = { { 0.0f, 0.0f, 0.0f }, 1.0f };

static inline float xasin(float x)
{
    return asin(x);
}

static inline float xacos(float x)
{
    return acos(x);
}

static inline void xVec3Init(_xVec3* v, float x, float y, float z)
{
    v->x = x;
    v->y = y;
    v->z = z;
}

#pragma fp_contract off
static inline float xVec3Length2(const _xVec3* v)
{
    return v->x * v->x + v->y * v->y + v->z * v->z;
}
#pragma fp_contract on

static inline void xVec3Inv(_xVec3* o, const _xVec3* v)
{
    xVec3SMul(o, v, -1.0f);
}

static inline void xVec3Cross(_xVec3* o, const _xVec3* a, const _xVec3* b)
{
    _xVec3 temp;
    temp.x = a->y * b->z - b->y * a->z;
    temp.y = a->z * b->x - b->z * a->x;
    temp.z = a->x * b->y - b->x * a->y;
    xVec3Copy(o, &temp);
}

static inline void xMat3x3Copy(_xMat3x3* o, const _xMat3x3* m)
{
    memcpy(o, m, sizeof(_xMat3x3));
}

static inline void xMat3x3Identity(_xMat3x3* m)
{
    xMat3x3Copy(m, (const _xMat3x3*)&g_I3);
}

static inline void xQuatCopy(_xQuat* o, const _xQuat* q)
{
    o->s = q->s;
    o->v.x = q->v.x;
    o->v.y = q->v.y;
    o->v.z = q->v.z;
}

static inline float xQuatDot(const _xQuat* a, const _xQuat* b)
{
    return a->v.x * b->v.x + a->v.y * b->v.y + a->v.z * b->v.z + a->s * b->s;
}

static inline float xQuatLength2(const _xQuat* q)
{
    return xQuatDot(q, q);
}

static inline void xQuatSMul(_xQuat* q, const _xQuat* a, float t)
{
    q->s = a->s * t;
    xVec3SMul(&q->v, &a->v, t);
}

static inline void xQuatAdd(_xQuat* q, const _xQuat* a, const _xQuat* b)
{
    q->s = a->s + b->s;
    xVec3Add(&q->v, &a->v, &b->v);
}

static inline void xQuatFlip(_xQuat* o, const _xQuat* q)
{
    o->s = -q->s;
    xVec3Inv(&o->v, &q->v);
}

float xVec3Normalize(_xVec3* o, _xVec3* v);
void xMat3x3Euler(_xMat3x3* m, float yaw, float pitch, float roll);
void xMat3x3RotC(_xMat3x3* m, float _x, float _y, float _z, float t);
void xMat3x3Mul(_xMat3x3* o, _xMat3x3* a, _xMat3x3* b);
void xMat3x3LMulVec(_xVec3* o, _xMat3x3* m, _xVec3* v);
void xMat4x3Mul(_xMat4x3* o, _xMat4x3* a, _xMat4x3* b);
float xQuatNormalize(_xQuat* o, _xQuat* q);

// Functions are in reverse DOL order (-inline deferred emits last to first).

#pragma auto_inline on

void xMath3Init()
{
    iMath3Init();
}

void xMath3Exit()
{
    iMath3Exit();
}

float xVec3LengthFast(float x, float y, float z)
{
    float ax = __fabs(x);
    float ay = __fabs(y);
    float az = __fabs(z);
    float max = ax;
    float mid = ay;
    float min = az;

    if (ay > ax)
    {
        mid = ax;
        max = ay;
    }
    if (az > mid)
    {
        min = mid;
        mid = az;
    }
    if (mid > max)
    {
        float temp = mid;
        mid = max;
        max = temp;
    }
    return max + 0.34375f * mid + 0.25f * min;
}

float xVec3Normalize(_xVec3* o, _xVec3* v)
{
    float len2 = xVec3Length2(v);
    float len;

    if (len2 == 1.0f)
    {
        if (o != v)
        {
            xVec3Copy(o, v);
        }
        return 1.0f;
    }

    if (len2 == 0.0f)
    {
        if (o != v)
        {
            xVec3Copy(o, &g_Y3);
        }
        return 0.0f;
    }

    len = xsqrt(len2);
    xVec3SMul(o, v, 1.0f / len);
    return len;
}

void xLine3VecDist2(_xVec3* p1, _xVec3* p2, _xVec3* v, _xIsect* isx)
{
    _xVec3 ldir;
    xVec3Sub(&ldir, p2, p1);
    xVec3Sub(&isx->norm, v, p1);

    float ldirdotlv = xVec3Dot(&ldir, &isx->norm);
    if (ldirdotlv <= 0.0f)
    {
        isx->dist = xVec3Length2(&isx->norm);
        return;
    }

    float ldirlen2 = xVec3Length2(&ldir);
    if (ldirdotlv >= ldirlen2)
    {
        xVec3Sub(&isx->norm, v, p2);
        isx->dist = xVec3Length2(&isx->norm);
        return;
    }

    float lvlen2 = xVec3Length2(&isx->norm);
    isx->dist = lvlen2 - ldirdotlv * ldirdotlv / ldirlen2;
}

void xMat3x3Normalize(_xMat3x3* o, _xMat3x3* m)
{
    xVec3Normalize(&o->right, &m->right);
    xVec3Normalize(&o->up, &m->up);
    xVec3Normalize(&o->at, &m->at);
}

void xMat3x3GetScale(_xMat3x3* m, _xVec3* s)
{
    s->x = xsqrt(xVec3Length2(&m->right));
    s->y = xsqrt(xVec3Length2(&m->up));
    s->z = xsqrt(xVec3Length2(&m->at));
}

void xMat3x3GetEuler(_xMat3x3* m, _xVec3* a)
{
    float yaw = -xasin(m->at.y);

    float roll;
    float pitch;

    if (yaw < (PI / 2))
    {
        if (yaw > -(PI / 2))
        {
            pitch = xAngleClamp(xatan2(m->at.x, m->at.z));
            roll = xAngleClamp(xatan2(m->right.y, m->up.y));
        }
        else
        {
            pitch = -xAngleClamp(xatan2(-m->up.x, m->right.x));
            roll = 0.0f;
        }
    }
    else
    {
        pitch = xAngleClamp(xatan2(-m->up.x, m->right.x));
        roll = 0.0f;
    }

    a->x = pitch;
    a->y = yaw;
    a->z = roll;
}

void xMat4x3MoveLocalRight(_xMat4x3* m, float mag)
{
    m->pos.x += m->right.x * mag;
    m->pos.y += m->right.y * mag;
    m->pos.z += m->right.z * mag;
}

void xMat4x3MoveLocalUp(_xMat4x3* m, float mag)
{
    m->pos.x += m->up.x * mag;
    m->pos.y += m->up.y * mag;
    m->pos.z += m->up.z * mag;
}

void xMat4x3MoveLocalAt(_xMat4x3* m, float mag)
{
    m->pos.x += m->at.x * mag;
    m->pos.y += m->at.y * mag;
    m->pos.z += m->at.z * mag;
}

float xMat3x3LookVec(_xMat3x3* m, _xVec3* at)
{
    float vec_len = xVec3Normalize(&m->at, at);
    xVec3Inv(&m->at, &m->at);

    if (__fabs(1.0f - m->at.y) < 0.00001f)
    {
        m->right.x = 1.0f;
        m->right.y = 0.0f;
        m->right.z = 0.0f;
        m->up.x = 0.0f;
        m->up.y = 0.0f;
        m->up.z = 1.0f;
        m->at.x = 0.0f;
        m->at.y = -1.0f;
        m->at.z = 0.0f;
        return vec_len;
    }
    if (__fabs(1.0f + m->at.y) < 0.00001f)
    {
        m->right.x = -1.0f;
        m->right.y = 0.0f;
        m->right.z = 0.0f;
        m->up.x = 0.0f;
        m->up.y = 0.0f;
        m->up.z = -1.0f;
        m->at.x = 0.0f;
        m->at.y = 1.0f;
        m->at.z = 0.0f;
        return vec_len;
    }
    if ((__fabs(at->z) < 0.00001f) && (__fabs(at->x) < 0.00001f))
    {
        m->right.x = 1.0f;
        m->right.y = 0.0f;
        m->right.z = 0.0f;
        m->up.x = 0.0f;
        m->up.y = 1.0f;
        m->up.z = 0.0f;
        m->at.x = 0.0f;
        m->at.y = 0.0f;
        m->at.z = 1.0f;
        return 0.0f;
    }
    m->right.x = m->at.z;
    m->right.y = 0.0f;
    m->right.z = -m->at.x;
    xVec3Normalize((_xVec3*)m, (_xVec3*)m);
    xVec3Cross(&m->up, &m->at, (_xVec3*)m);
    xVec3Cross((_xVec3*)m, &m->up, &m->at);
    m->flags = 0;
    return vec_len;
}

void xMat3x3Euler(_xMat3x3* m, _xVec3* ypr)
{
    xMat3x3Euler(m, ypr->x, ypr->y, ypr->z);
}

void xMat3x3Euler(_xMat3x3* m, float yaw, float pitch, float roll)
{
    float isy = isin(yaw);
    float icy = icos(yaw);
    float isp = isin(pitch);
    float icp = icos(pitch);
    float isr = isin(roll);
    float icr = icos(roll);
    float is_prod = icy * isp;
    float ic_prod = isy * isp;

    m->right.x = (icy * icr) + (isr * ic_prod);
    m->right.y = icp * isr;
    m->right.z = (-isy * icr) + (isr * is_prod);
    m->up.x = (-icy * isr) + (icr * ic_prod);
    m->up.y = icp * icr;
    m->up.z = (isy * isr) + (icr * is_prod);
    m->at.x = isy * icp;
    m->at.y = -isp;
    m->at.z = icy * icp;
    m->flags = 0;
}

void xMat3x3RotC(_xMat3x3* m, float _x, float _y, float _z, float t)
{
    float sin;
    float cos;
    float c;

    if (t == 0.0f)
    {
        xMat3x3Identity(m);
        return;
    }

    cos = icos(t);
    sin = isin(t);

    c = 1.0f - cos;

    m->right.x = (c * _x * _x) + cos;
    m->right.y = (sin * _z) + (c * _x * _y);
    m->right.z = (-sin * _y) + (c * _z * _x);

    m->up.x = (-sin * _z) + (c * _x * _y);
    m->up.y = (c * _y * _y) + cos;
    m->up.z = (sin * _x) + (c * _y * _z);

    m->at.x = (sin * _y) + (c * _z * _x);
    m->at.y = (-sin * _x) + (c * _y * _z);
    m->at.z = (c * _z * _z) + cos;

    m->flags = 0;
}

void xMat3x3RotY(_xMat3x3* m, float t)
{
    float cos = icos(t);
    float sin = isin(t);

    xVec3Init((_xVec3*)m, cos, 0.0f, -sin);
    xVec3Copy(&m->up, &g_Y3);
    xVec3Init(&m->at, sin, 0.0f, cos);
    m->flags = 0;
}

void xMat3x3LMulRotY(_xMat3x3* o, _xMat3x3* m, float t)
{
    float cos = icos(t);
    float sin = isin(t);
    if (o == m)
    {
        float temp1 = o->right.x;
        float temp2 = o->at.x;
        o->at.x = cos * temp2 + sin * temp1;
        o->right.x = cos * temp1 - sin * temp2;

        temp1 = o->right.y;
        temp2 = o->at.y;
        o->at.y = cos * temp2 + sin * temp1;
        o->right.y = cos * temp1 - sin * temp2;

        temp1 = o->right.z;
        temp2 = o->at.z;
        o->at.z = cos * temp2 + sin * temp1;
        o->right.z = cos * temp1 - sin * temp2;
    }
    else
    {
        o->up.x = m->up.x;
        o->up.y = m->up.y;
        o->up.z = m->up.z;
        o->right.x = cos * m->right.x - sin * m->at.x;
        o->at.x = cos * m->at.x + sin * m->right.x;
        o->right.y = cos * m->right.y - sin * m->at.y;
        o->at.y = cos * m->at.y + sin * m->right.y;
        o->right.z = cos * m->right.z - sin * m->at.z;
        o->at.z = cos * m->at.z + sin * m->right.z;
        o->flags = 0;
    }
}

void xMat3x3RMulRotY(_xMat3x3* o, _xMat3x3* m, float t)
{
    float cos = icos(t);
    float sin = isin(t);
    if (o == m)
    {
        float temp1 = o->right.x;
        float temp2 = o->right.z;
        o->right.z = ((cos * temp2) - (sin * temp1));
        o->right.x = ((cos * temp1) + (sin * temp2));

        temp1 = o->up.x;
        temp2 = o->up.z;
        o->up.z = ((cos * temp2) - (sin * temp1));
        o->up.x = ((cos * temp1) + (sin * temp2));

        temp1 = o->at.x;
        temp2 = o->at.z;
        o->at.z = ((cos * temp2) - (sin * temp1));
        o->at.x = ((cos * temp1) + (sin * temp2));
    }
    else
    {
        o->right.x = (cos * m->right.x + (sin * m->right.z));
        o->right.y = m->right.y;
        o->right.z = (cos * m->right.z - (sin * m->right.x));

        o->up.x = (cos * m->up.x + (sin * m->up.z));
        o->up.y = m->up.y;
        o->up.z = (cos * m->up.z - (sin * m->up.x));

        o->at.x = (cos * m->at.x + (sin * m->at.z));
        o->at.y = m->at.y;
        o->at.z = (cos * m->at.z - (sin * m->at.x));

        o->flags = 0;
    }
}

void xMat3x3Mul(_xMat3x3* o, _xMat3x3* a, _xMat3x3* b)
{
    _xMat3x3 temp;
    _xMat3x3* tp;
    unsigned char usetemp = (o == a || o == b);

    if (usetemp)
    {
        tp = &temp;
    }
    else
    {
        tp = o;
    }

    tp->right.x = a->right.x * b->right.x + a->right.y * b->up.x + a->right.z * b->at.x;
    tp->right.y = a->right.x * b->right.y + a->right.y * b->up.y + a->right.z * b->at.y;
    tp->right.z = a->right.x * b->right.z + a->right.y * b->up.z + a->right.z * b->at.z;

    tp->up.x = a->up.x * b->right.x + a->up.y * b->up.x + a->up.z * b->at.x;
    tp->up.y = a->up.x * b->right.y + a->up.y * b->up.y + a->up.z * b->at.y;
    tp->up.z = a->up.x * b->right.z + a->up.y * b->up.z + a->up.z * b->at.z;

    tp->at.x = a->at.x * b->right.x + a->at.y * b->up.x + a->at.z * b->at.x;
    tp->at.y = a->at.x * b->right.y + a->at.y * b->up.y + a->at.z * b->at.y;
    tp->at.z = a->at.x * b->right.z + a->at.y * b->up.z + a->at.z * b->at.z;

    if (usetemp)
    {
        xMat3x3Copy(o, tp);
    }
    else
    {
        o->flags = 0;
    }
}

// TODO: 89.6%. Load scheduling (the original loads m before v).
void xMat3x3LMulVec(_xVec3* o, _xMat3x3* m, _xVec3* v)
{
    float x = (m->right.x * v->x) + (m->right.y * v->y) + (m->right.z * v->z);
    float y = (m->up.x * v->x) + (m->up.y * v->y) + (m->up.z * v->z);
    float z = (m->at.x * v->x) + (m->at.y * v->y) + (m->at.z * v->z);

    o->x = x;
    o->y = y;
    o->z = z;
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

// TODO: 71%. The original keeps v in f29-f31 across the inlined LMulVec.
void xMat3x3Tolocal(_xVec3* o, _xMat3x3* m, _xVec3* v)
{
    float sumRt = xVec3Length2(&m->right);
    float sumUp = xVec3Length2(&m->up);
    float sumAt = xVec3Length2(&m->at);
    xMat3x3LMulVec(o, m, v);
    o->x *= 1.0f / sumRt;
    o->y *= 1.0f / sumUp;
    o->z *= 1.0f / sumAt;
}

// TODO: 90.7%. Register allocation in the inlined normalizes.
void xMat4x3Orthonormalize(_xMat4x3* o, _xMat4x3* m)
{
    xVec3Cross(&o->right, &o->up, &o->at);
    xVec3Normalize(&o->right, &o->right);
    xVec3Cross(&o->up, &o->at, &o->right);
    xVec3Normalize(&o->up, &o->up);
    xVec3Cross(&o->at, &o->right, &o->up);
    xVec3Normalize(&o->at, &o->at);
    xVec3Copy(&o->pos, &m->pos);
}

// TODO: 93%. Scheduling around the inlined xMat4x3Mul.
void xMat4x3Rot(_xMat4x3* m, _xVec3* a, float t, _xVec3* p)
{
    _xMat4x3 temp;

    xMat3x3RotC((_xMat3x3*)m, a->x, a->y, a->z, t);
    xVec3Copy(&m->pos, p);
    xMat3x3Identity((_xMat3x3*)&temp);
    xVec3Inv(&temp.pos, p);
    xMat4x3Mul(m, &temp, m);
}

// TODO: 80%. The original keeps v on the stack (frame 0x50) instead of in
// saved registers.
void xMat4x3Mul(_xMat4x3* o, _xMat4x3* a, _xMat4x3* b)
{
    _xVec3 v;

    xMat4x3Toworld(&v, b, &a->pos);
    xMat3x3Mul((_xMat3x3*)o, (_xMat3x3*)a, (_xMat3x3*)b);
    xVec3Copy(&o->pos, &v);
}

void xMat4x3LMul3x3(_xMat4x3* o, _xMat4x3* m, _xMat3x3* m3x3)
{
    _xVec3 pos;

    xMat3x3LMulVec(&pos, m3x3, &m->pos);
    xMat3x3Mul((_xMat3x3*)o, (_xMat3x3*)m, m3x3);
    xVec3Copy(&o->pos, &pos);
}

void xMat4x3RMul3x3(_xMat4x3* o, _xMat3x3* m3x3, _xMat4x3* m)
{
    xMat3x3Mul((_xMat3x3*)o, m3x3, (_xMat3x3*)m);
    if (o != m)
    {
        xVec3Copy(&o->pos, &m->pos);
    }
}

// TODO: 83.8%. Scheduling of the trace and index selection.
void xQuatFromMat(_xQuat* q, _xMat3x3* m)
{
    float* mp = (float*)m;
    float* qvp = (float*)q;
    float tr = m->right.x + m->up.y + m->at.z;
    float root;

    if (tr > 0.0f)
    {
        root = xsqrt(1.0f + tr);
        q->s = 0.5f * root;
        root = 0.5f / root;
        q->v.x = root * (m->at.y - m->up.z);
        q->v.y = root * (m->right.z - m->at.x);
        q->v.z = root * (m->up.x - m->right.y);
    }
    else
    {
        static signed int nxt[3] = { 1, 2, 0 };

        signed int i = 0;
        if (mp[5] > mp[0])
            i = 1;
        if (mp[10] > mp[i * 5])
            i = 2;

        signed int j = nxt[i];
        signed int k = nxt[j];

        root = xsqrt(mp[i * 5] - mp[j * 5] - mp[k * 5] + 1.0f);
        if ((float)__fabs(root) < 1e-5f)
        {
            xQuatCopy(q, &g_IQ);
            return;
        }

        qvp[i] = 0.5f * root;
        root = 0.5f / root;
        q->s = root * (mp[j + k * 4] - mp[k + j * 4]);
        qvp[j] = root * (mp[i + j * 4] + mp[j + i * 4]);
        qvp[k] = root * (mp[i + k * 4] + mp[k + i * 4]);

        if (q->s < 0.0f)
        {
            xQuatFlip(q, q);
        }
    }
}

void xQuatToMat(_xQuat* q, _xMat3x3* m)
{
    float tx = (2.0f * q->v.x);
    float ty = (2.0f * q->v.y);
    float tz = (2.0f * q->v.z);
    float tsx = tx * q->s;
    float tsy = ty * q->s;
    float tsz = tz * q->s;
    float txx = tx * q->v.x;
    float txy = ty * q->v.x;
    float txz = tz * q->v.x;
    float tyy = ty * q->v.y;
    float tyz = tz * q->v.y;
    float tzz = tz * q->v.z;

    m->right.x = (1.0f - tyy) - tzz;
    m->right.y = txy - tsz;
    m->right.z = txz + tsy;

    m->up.x = txy + tsz;
    m->up.y = (1.0f - tzz) - txx;
    m->up.z = tyz - tsx;

    m->at.x = txz - tsy;
    m->at.y = tyz + tsx;
    m->at.z = (1.0f - txx) - tyy;

    m->flags = 0;
}

float xQuatNormalize(_xQuat* o, _xQuat* q)
{
    float one_len, len, len2;
    len2 = xQuatLength2(q);
    if (len2 == 1.0f)
    {
        if (o != q)
        {
            xQuatCopy(o, q);
        }
        return 1.0f;
    }

    if (len2 == 0.0f)
    {
        if (o != q)
        {
            xQuatCopy(o, &g_IQ);
        }
        return 0.0f;
    }

    len = xsqrt(len2);
    one_len = 1.0f / len;
    xQuatSMul(o, q, one_len);
    return len;
}

// TODO: 85%. b2 pointer set-up and load order.
void xQuatSlerp(_xQuat* q, _xQuat* a, _xQuat* b, float t)
{
    _xQuat b2;
    _xQuat q1;
    _xQuat q2;
    float abdot;
    float t1;
    float t2;

    abdot = xQuatDot(a, b);
    if (abdot < 0.0f)
    {
        abdot = -abdot;
        b2.v.x = -b->v.x;
        b2.v.y = -b->v.y;
        b2.v.z = -b->v.z;
        b2.s = -b->s;
        b = &b2;
    }
    if (abdot >= 0.999f)
    {
        t1 = 1.0f - t;
        t2 = t;
    }
    else
    {
        float theta = xacos(abdot);
        float one_sintheta = 1.0f / isin(theta);
        t1 = one_sintheta * isin((1.0f - t) * theta);
        t2 = one_sintheta * isin(t * theta);
    }

    xQuatSMul(&q1, a, t1);
    xQuatSMul(&q2, b, t2);
    xQuatAdd(q, &q1, &q2);
}

// TODO: 77.7%. Same operations, different register allocation/scheduling.
void xQuatMul(_xQuat* o, _xQuat* a, _xQuat* b)
{
    float x = a->s * b->v.x + a->v.x * b->s + a->v.y * b->v.z - a->v.z * b->v.y;
    float y = a->s * b->v.y + a->v.y * b->s + a->v.z * b->v.x - a->v.x * b->v.z;
    float z = a->s * b->v.z + a->v.z * b->s + a->v.x * b->v.y - a->v.y * b->v.x;
    float w = a->s * b->s - a->v.x * b->v.x - a->v.y * b->v.y - a->v.z * b->v.z;

    o->v.x = x;
    o->v.y = y;
    o->v.z = z;
    o->s = w;
}
