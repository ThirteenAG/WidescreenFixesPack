#pragma once
#include <cstdint>
#include <cmath>
#include <cstring>

#define nil nullptr

inline float Sin(float x) { return sinf(x); }
inline float Asin(float x) { return asinf(x); }
inline float Cos(float x) { return cosf(x); }
inline float Acos(float x) { return acosf(x); }
inline float Tan(float x) { return tanf(x); }
inline float Atan(float x) { return atanf(x); }
inline float Atan2(float y, float x) { return atan2f(y, x); }
inline float Abs(float x) { return fabsf(x); }
inline float Sqrt(float x) { return sqrtf(x); }
inline float RecipSqrt(float x, float y) { return x / Sqrt(y); }
inline float RecipSqrt(float x) { return RecipSqrt(1.0f, x); }
inline float Pow(float x, float y) { return powf(x, y); }
inline float Floor(float x) { return floorf(x); }
inline float Ceil(float x) { return ceilf(x); }

class CMatrix;
inline CMatrix operator*(const CMatrix& m1, const CMatrix& m2);

// ---------------------------------------------------------------------------
// RwV2d
// ---------------------------------------------------------------------------
struct RwV2d
{
    float x, y;

    RwV2d() = default;
    constexpr RwV2d(float x, float y) : x(x), y(y) {}

    RwV2d  operator+(const RwV2d& o) const { return { x + o.x, y + o.y }; }
    RwV2d  operator-(const RwV2d& o) const { return { x - o.x, y - o.y }; }
    RwV2d  operator*(float s)        const { return { x * s, y * s }; }
    RwV2d& operator+=(const RwV2d& o) { x += o.x; y += o.y; return *this; }
    RwV2d& operator-=(const RwV2d& o) { x -= o.x; y -= o.y; return *this; }

    float Dot(const RwV2d& o) const { return x * o.x + y * o.y; }
    float Length()            const { return Sqrt(x * x + y * y); }

    float Normalise()
    {
        float l = Length();
        if (l > 0.f)
        {
            x /= l;
            y /= l;
        }
        else
            x = 1.0f;
        return l;
    }

    RwV2d Perp() const { return { -y, x }; }

    static RwV2d LineNormal(const RwV2d& a, const RwV2d& b)
    {
        RwV2d n = { -(b.y - a.y), b.x - a.x };
        n.Normalise();
        return n;
    }
};

inline float RwV2dLength     (const RwV2d* v) { return v->Length(); }
inline float RwV2dNormalize  (RwV2d* o, const RwV2d* v) { *o = *v; return o->Normalise(); }
inline void  RwV2dAssign     (RwV2d* o, const RwV2d* a) { *o = *a; }
inline void  RwV2dAdd        (RwV2d* o, const RwV2d* a, const RwV2d* b) { *o = *a + *b; }
inline void  RwV2dSub        (RwV2d* o, const RwV2d* a, const RwV2d* b) { *o = *a - *b; }
inline void  RwV2dScale      (RwV2d* o, const RwV2d* a, float s) { *o = *a * s; }
inline float RwV2dDotProduct (const RwV2d* a, const RwV2d* b) { return a->Dot(*b); }
inline void  RwV2dPerp       (RwV2d* o, const RwV2d* a) { *o = a->Perp(); }
inline void  RwV2dLineNormal (RwV2d* o, const RwV2d* a, const RwV2d* b) { *o = RwV2d::LineNormal(*a, *b); }

// ---------------------------------------------------------------------------
// RwV3d
// ---------------------------------------------------------------------------
struct RwV3d
{
    float x, y, z;

    RwV3d() = default;
    constexpr RwV3d(float x, float y, float z) : x(x), y(y), z(z) {}

    RwV3d  operator+(const RwV3d& o) const { return { x + o.x, y + o.y, z + o.z }; }
    RwV3d  operator-(const RwV3d& o) const { return { x - o.x, y - o.y, z - o.z }; }
    RwV3d  operator*(float s)        const { return { x * s, y * s, z * s }; }
    RwV3d  operator-()               const { return { -x, -y, -z }; }
    RwV3d& operator+=(const RwV3d& o) { x += o.x; y += o.y; z += o.z; return *this; }
    RwV3d& operator-=(const RwV3d& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    RwV3d& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }

    float Dot(const RwV3d& o)   const { return x * o.x + y * o.y + z * o.z; }
    float LengthSq()            const { return Dot(*this); }
    float Length()              const { return Sqrt(LengthSq()); }

    float Normalise()
    {
        float l = Length();
        if (l > 0.f) { x /= l; y /= l; z /= l; }
        else x = 1.0f;
        return l;
    }

    RwV3d Cross(const RwV3d& o) const
    {
        return { y * o.z - z * o.y,
                z * o.x - x * o.z,
                x * o.y - y * o.x };
    }

    void IncrementScaled(const RwV3d& in, float s) { x += in.x * s; y += in.y * s; z += in.z * s; }
};

inline float RwV3dLength         (const RwV3d* v) { return v->Length(); }
inline float RwV3dNormalize      (RwV3d* o, const RwV3d* v) { *o = *v; return o->Normalise(); }
inline void  RwV3dAssign         (RwV3d* o, const RwV3d* a) { *o = *a; }
inline void  RwV3dAdd            (RwV3d* o, const RwV3d* a, const RwV3d* b) { *o = *a + *b; }
inline void  RwV3dSub            (RwV3d* o, const RwV3d* a, const RwV3d* b) { *o = *a - *b; }
inline void  RwV3dScale          (RwV3d* o, const RwV3d* v, float s) { *o = *v * s; }
inline void  RwV3dIncrementScaled(RwV3d* o, const RwV3d* v, float s) { o->IncrementScaled(*v, s); }
inline void  RwV3dNegate         (RwV3d* o, const RwV3d* v) { *o = -(*v); }
inline float RwV3dDotProduct     (const RwV3d* a, const RwV3d* b) { return a->Dot(*b); }
inline void  RwV3dCrossProduct   (RwV3d* o, const RwV3d* a, const RwV3d* b) { *o = a->Cross(*b); }

// ---------------------------------------------------------------------------
// RwRect / RwSphere
// ---------------------------------------------------------------------------
struct RwRect
{
    std::int32_t x, y, w, h;

    RwRect() = default;
    constexpr RwRect(std::int32_t x, std::int32_t y, std::int32_t w, std::int32_t h)
        : x(x), y(y), w(w), h(h)
    {
    }

    std::int32_t Right()  const { return x + w; }
    std::int32_t Bottom() const { return y + h; }
};

struct RwSphere
{
    RwV3d center;
    float radius;

    RwSphere() = default;
    constexpr RwSphere(const RwV3d& center, float radius) : center(center), radius(radius) {}

    bool Contains(const RwV3d& p) const { return (p - center).LengthSq() <= radius * radius; }
    bool Overlaps(const RwSphere& o) const
    {
        float r = radius + o.radius;
        return (center - o.center).LengthSq() <= r * r;
    }
};

// ---------------------------------------------------------------------------
// RwMatrix
// ---------------------------------------------------------------------------
struct RwMatrix
{
    RwV3d          right; std::uint32_t flags;
    RwV3d          up;    std::uint32_t pad1;
    RwV3d          at;    std::uint32_t pad2;
    RwV3d          pos;   std::uint32_t pad3;

    enum : std::uint32_t
    {
        TYPENORMAL = 1,
        TYPEORTHOGONAL = 2,
        TYPEORTHONORMAL = 3,
        TYPEMASK = 3,
        IDENTITY = 0x20000
    };

    void SetIdentity()
    {
        right = { 1,0,0 }; flags = IDENTITY | TYPEORTHONORMAL;
        up = { 0,1,0 }; pad1 = 0;
        at = { 0,0,1 }; pad2 = 0;
        pos = { 0,0,0 }; pad3 = 0;
    }
};

inline void     RwMatrixCopy      (RwMatrix* dst, const RwMatrix* src) { *dst = *src; }
inline void     RwMatrixSetIdentity(RwMatrix* m) { m->SetIdentity(); }
inline void     RwMatrixDestroy   (RwMatrix*) {}  // no-op: stack/embedded only
inline RwMatrix* RwMatrixUpdate   (RwMatrix* m) { return m; }

inline RwMatrix* RwMatrixInvert(RwMatrix* dst, const RwMatrix* s)
{
    dst->right = { s->right.x, s->up.x, s->at.x };
    dst->up = { s->right.y, s->up.y, s->at.y };
    dst->at = { s->right.z, s->up.z, s->at.z };
    dst->pos.x = -(s->pos.x * s->right.x + s->pos.y * s->right.y + s->pos.z * s->right.z);
    dst->pos.y = -(s->pos.x * s->up.x + s->pos.y * s->up.y + s->pos.z * s->up.z);
    dst->pos.z = -(s->pos.x * s->at.x + s->pos.y * s->at.y + s->pos.z * s->at.z);
    dst->flags = s->flags;
    dst->pad1 = dst->pad2 = dst->pad3 = 0;
    return dst;
}

inline RwMatrix* RwMatrixMultiply(RwMatrix* dst, const RwMatrix* a, const RwMatrix* b)
{
    RwMatrix tmp;
    tmp.right.x = a->right.x * b->right.x + a->right.y * b->up.x + a->right.z * b->at.x;
    tmp.right.y = a->right.x * b->right.y + a->right.y * b->up.y + a->right.z * b->at.y;
    tmp.right.z = a->right.x * b->right.z + a->right.y * b->up.z + a->right.z * b->at.z;
    tmp.up.x = a->up.x * b->right.x + a->up.y * b->up.x + a->up.z * b->at.x;
    tmp.up.y = a->up.x * b->right.y + a->up.y * b->up.y + a->up.z * b->at.y;
    tmp.up.z = a->up.x * b->right.z + a->up.y * b->up.z + a->up.z * b->at.z;
    tmp.at.x = a->at.x * b->right.x + a->at.y * b->up.x + a->at.z * b->at.x;
    tmp.at.y = a->at.x * b->right.y + a->at.y * b->up.y + a->at.z * b->at.y;
    tmp.at.z = a->at.x * b->right.z + a->at.y * b->up.z + a->at.z * b->at.z;
    tmp.pos.x = a->pos.x * b->right.x + a->pos.y * b->up.x + a->pos.z * b->at.x + b->pos.x;
    tmp.pos.y = a->pos.x * b->right.y + a->pos.y * b->up.y + a->pos.z * b->at.y + b->pos.y;
    tmp.pos.z = a->pos.x * b->right.z + a->pos.y * b->up.z + a->pos.z * b->at.z + b->pos.z;
    tmp.flags = 0; tmp.pad1 = tmp.pad2 = tmp.pad3 = 0;
    *dst = tmp;
    return dst;
}

inline RwV3d* RwV3dTransformPoints(RwV3d* out, const RwV3d* in, int n, const RwMatrix* m)
{
    for (int i = 0; i < n; i++)
    {
        out[i].x = in[i].x * m->right.x + in[i].y * m->up.x + in[i].z * m->at.x + m->pos.x;
        out[i].y = in[i].x * m->right.y + in[i].y * m->up.y + in[i].z * m->at.y + m->pos.y;
        out[i].z = in[i].x * m->right.z + in[i].y * m->up.z + in[i].z * m->at.z + m->pos.z;
    }
    return out;
}

inline RwV3d* RwV3dTransformVectors(RwV3d* out, const RwV3d* in, int n, const RwMatrix* m)
{
    for (int i = 0; i < n; i++)
    {
        out[i].x = in[i].x * m->right.x + in[i].y * m->up.x + in[i].z * m->at.x;
        out[i].y = in[i].x * m->right.y + in[i].y * m->up.y + in[i].z * m->at.y;
        out[i].z = in[i].x * m->right.z + in[i].y * m->up.z + in[i].z * m->at.z;
    }
    return out;
}

// ---------------------------------------------------------------------------
// CVector forward decl (needed by CVector2D ctor)
// ---------------------------------------------------------------------------
class CVector;

// ---------------------------------------------------------------------------
// CVector2D
// ---------------------------------------------------------------------------
class CVector2D
{
public:
    float x, y;

    CVector2D() {}
    CVector2D(float x, float y) : x(x), y(y) {}
    inline CVector2D(const CVector& v);

    float Heading()      const { return Atan2(-x, y); }
    float Magnitude()    const { return Sqrt(x * x + y * y); }
    float MagnitudeSqr() const { return x * x + y * y; }

    void Normalise()
    {
        float r = RecipSqrt(MagnitudeSqr());
        x *= r; y *= r;
    }
    void NormaliseSafe()
    {
        float sq = MagnitudeSqr();
        if (sq > 0.f) { float r = RecipSqrt(sq); x *= r; y *= r; }
        else x = 1.f;
    }

    const CVector2D& operator+=(CVector2D const& r) { x += r.x; y += r.y; return *this; }
    const CVector2D& operator-=(CVector2D const& r) { x -= r.x; y -= r.y; return *this; }
    const CVector2D& operator*=(float r) { x *= r;   y *= r;   return *this; }
    const CVector2D& operator/=(float r) { x /= r;   y /= r;   return *this; }
    CVector2D operator+(const CVector2D& r) const { return { x + r.x, y + r.y }; }
    CVector2D operator-(const CVector2D& r) const { return { x - r.x, y - r.y }; }
    CVector2D operator*(float t)            const { return { x * t,   y * t }; }
    CVector2D operator/(float t)            const { return { x / t,   y / t }; }
};

inline CVector2D operator*(float l, const CVector2D& r) { return { l * r.x, l * r.y }; }

inline float DotProduct2D  (const CVector2D& a, const CVector2D& b) { return a.x * b.x + a.y * b.y; }
inline float CrossProduct2D(const CVector2D& a, const CVector2D& b) { return a.x * b.y - a.y * b.x; }
inline float Distance2D    (const CVector2D& v, float x, float y) { return Sqrt((v.x - x) * (v.x - x) + (v.y - y) * (v.y - y)); }
inline float DistanceSqr2D (const CVector2D& v, float x, float y) { return (v.x - x) * (v.x - x) + (v.y - y) * (v.y - y); }

inline void NormalizeXY(float& x, float& y)
{
    float l = Sqrt(x * x + y * y);
    if (l != 0.f) { x /= l; y /= l; }
    else x = 1.f;
}

// ---------------------------------------------------------------------------
// CVector
// ---------------------------------------------------------------------------
class CVector : public RwV3d
{
public:
    CVector() {}
    CVector(float x, float y, float z) { this->x = x; this->y = y; this->z = z; }
    CVector(const RwV3d& v) { x = v.x; y = v.y; z = v.z; }

    float Heading()        const { return Atan2(-x, y); }
    float Magnitude()      const { return Sqrt(x * x + y * y + z * z); }
    float MagnitudeSqr()   const { return x * x + y * y + z * z; }
    float Magnitude2D()    const { return Sqrt(x * x + y * y); }
    float MagnitudeSqr2D() const { return x * x + y * y; }

    void Normalise()
    {
        float sq = MagnitudeSqr();
        if (sq > 0.f) { float r = RecipSqrt(sq); x *= r; y *= r; z *= r; }
    }
    void Normalise2D()
    {
        float r = RecipSqrt(MagnitudeSqr2D());
        x *= r; y *= r;
    }

    const CVector& operator+=(CVector const& r) { x += r.x; y += r.y; z += r.z; return *this; }
    const CVector& operator-=(CVector const& r) { x -= r.x; y -= r.y; z -= r.z; return *this; }
    const CVector& operator*=(float r) { x *= r;   y *= r;   z *= r;   return *this; }
    const CVector& operator/=(float r) { x /= r;   y /= r;   z /= r;   return *this; }
    CVector operator-() const { return { -x, -y, -z }; }
    bool operator==(CVector const& r) const { return x == r.x && y == r.y && z == r.z; }
    bool operator!=(CVector const& r) const { return x != r.x || y != r.y || z != r.z; }
    bool IsZero() const { return x == 0.f && y == 0.f && z == 0.f; }
};

inline CVector2D::CVector2D(const CVector& v) : x(v.x), y(v.y) {}

inline CVector operator+(const CVector& a, const CVector& b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
inline CVector operator-(const CVector& a, const CVector& b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
inline CVector operator*(const CVector& a, float s) { return { a.x * s, a.y * s, a.z * s }; }
inline CVector operator*(float s, const CVector& a) { return { s * a.x, s * a.y, s * a.z }; }
inline CVector operator/(const CVector& a, float s) { return { a.x / s, a.y / s, a.z / s }; }

inline float  DotProduct(const CVector& a, const CVector& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline CVector CrossProduct(const CVector& a, const CVector& b)
{
    return { a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x };
}
inline float Distance  (const CVector& a, const CVector& b) { return (b - a).Magnitude(); }
inline float Distance2D(const CVector& a, const CVector& b)
{
    float dx = b.x - a.x, dy = b.y - a.y;
    return Sqrt(dx * dx + dy * dy);
}

// ---------------------------------------------------------------------------
// CMatrix
// ---------------------------------------------------------------------------
class CMatrix
{
public:
    union
    {
        float f[4][4];
        struct
        {
            float rx, ry, rz, rw;
            float fx, fy, fz, fw;
            float ux, uy, uz, uw;
            float px, py, pz, pw;
        };
    };

    RwMatrix* m_attachment;
    bool m_hasRwMatrix; // are we the owner?

    CMatrix(void);
    CMatrix(CMatrix const& m);
    CMatrix(RwMatrix* matrix, bool owner = false);
    CMatrix(float scale)
    {
        m_attachment = nil;
        m_hasRwMatrix = false;
        SetScale(scale);
    }
    ~CMatrix(void);
    void Attach(RwMatrix* matrix, bool owner = false);
    void AttachRW(RwMatrix* matrix, bool owner = false);
    void Detach(void);
    void Update(void);
    void UpdateRW(void);
    void operator=(CMatrix const& rhs);
    CMatrix& operator+=(CMatrix const& rhs);
    CMatrix& operator*=(CMatrix const& rhs);

    CVector& GetPosition(void) { return *(CVector*)&px; }
    CVector& GetRight(void) { return *(CVector*)&rx; }
    CVector& GetForward(void) { return *(CVector*)&fx; }
    CVector& GetUp(void) { return *(CVector*)&ux; }

    const CVector& GetPosition(void) const { return *(CVector*)&px; }
    const CVector& GetRight(void) const { return *(CVector*)&rx; }
    const CVector& GetForward(void) const { return *(CVector*)&fx; }
    const CVector& GetUp(void) const { return *(CVector*)&ux; }


    void SetTranslate(float x, float y, float z);
    void SetTranslate(const CVector& trans) { SetTranslate(trans.x, trans.y, trans.z); }
    void Translate(float x, float y, float z)
    {
        px += x;
        py += y;
        pz += z;
    }
    void Translate(const CVector& trans) { Translate(trans.x, trans.y, trans.z); }

    void SetScale(float s);
    void Scale(float scale)
    {
        for (int i = 0; i < 3; i++)
            #ifdef FIX_BUGS // BUGFIX from VC
            for (int j = 0; j < 3; j++)
                #else
            for (int j = 0; j < 4; j++)
                #endif
                f[i][j] *= scale;
    }


    void SetRotateXOnly(float angle);
    void SetRotateYOnly(float angle);
    void SetRotateZOnly(float angle);
    void SetRotateX(float angle);
    void SetRotateY(float angle);
    void SetRotateZ(float angle);
    void SetRotate(float xAngle, float yAngle, float zAngle);
    void Rotate(float x, float y, float z);
    void RotateX(float x);
    void RotateY(float y);
    void RotateZ(float z);

    void Reorthogonalise(void);
    void CopyOnlyMatrix(const CMatrix& other);
    void SetUnity(void);
    void ResetOrientation(void);
    void SetTranslateOnly(float x, float y, float z)
    {
        px = x;
        py = y;
        pz = z;
    }
    void SetTranslateOnly(const CVector& pos)
    {
        SetTranslateOnly(pos.x, pos.y, pos.z);
    }
    void CheckIntegrity() {}
};

inline CVector MultiplyInverse(const CMatrix& mat, const CVector& vec)
{
    CVector v(vec.x - mat.px, vec.y - mat.py, vec.z - mat.pz);
    return CVector(
        mat.rx * v.x + mat.ry * v.y + mat.rz * v.z,
        mat.fx * v.x + mat.fy * v.y + mat.fz * v.z,
        mat.ux * v.x + mat.uy * v.y + mat.uz * v.z);
}

inline CVector Multiply3x3(const CMatrix& mat, const CVector& vec)
{
    // TODO: VU0 code
    return CVector(mat.rx * vec.x + mat.fx * vec.y + mat.ux * vec.z,
        mat.ry * vec.x + mat.fy * vec.y + mat.uy * vec.z,
        mat.rz * vec.x + mat.fz * vec.y + mat.uz * vec.z);
}

inline CVector Multiply3x3(const CVector& vec, const CMatrix& mat)
{
    return CVector(mat.rx * vec.x + mat.ry * vec.y + mat.rz * vec.z,
        mat.fx * vec.x + mat.fy * vec.y + mat.fz * vec.z,
        mat.ux * vec.x + mat.uy * vec.y + mat.uz * vec.z);
}

inline CVector operator*(const CMatrix& mat, const CVector& vec)
{
    // TODO: VU0 code
    return CVector(mat.rx * vec.x + mat.fx * vec.y + mat.ux * vec.z + mat.px,
        mat.ry * vec.x + mat.fy * vec.y + mat.uy * vec.z + mat.py,
        mat.rz * vec.x + mat.fz * vec.y + mat.uz * vec.z + mat.pz);
}

inline CMatrix::CMatrix(void)
{
    m_attachment = nil;
    m_hasRwMatrix = false;
}

inline CMatrix::CMatrix(CMatrix const& m)
{
    m_attachment = nil;
    m_hasRwMatrix = false;
    *this = m;
}

inline CMatrix::CMatrix(RwMatrix* matrix, bool owner)
{
    m_attachment = nil;
    Attach(matrix, owner);
}

inline CMatrix::~CMatrix(void)
{
    if (m_hasRwMatrix && m_attachment)
        RwMatrixDestroy(m_attachment);
}

inline void CMatrix::Attach(RwMatrix* matrix, bool owner)
{
    if (m_attachment && m_hasRwMatrix)
        RwMatrixDestroy(m_attachment);
    m_attachment = matrix;
    m_hasRwMatrix = owner;
    Update();
}

inline void CMatrix::AttachRW(RwMatrix* matrix, bool owner)
{
    if (m_hasRwMatrix && m_attachment)
        RwMatrixDestroy(m_attachment);
    m_attachment = matrix;
    m_hasRwMatrix = owner;
    UpdateRW();
}

inline void CMatrix::Detach(void)
{
    if (m_hasRwMatrix && m_attachment)
        RwMatrixDestroy(m_attachment);
    m_attachment = nil;
}

inline void CMatrix::Update(void)
{
    GetRight() = m_attachment->right;
    GetForward() = m_attachment->up;
    GetUp() = m_attachment->at;
    GetPosition() = m_attachment->pos;
}

inline void CMatrix::UpdateRW(void)
{
    if (m_attachment)
    {
        m_attachment->right = GetRight();
        m_attachment->up = GetForward();
        m_attachment->at = GetUp();
        m_attachment->pos = GetPosition();
        RwMatrixUpdate(m_attachment);
    }
}

inline void CMatrix::operator=(CMatrix const& rhs)
{
    memcpy(this, &rhs, sizeof(f));
    if (m_attachment)
        UpdateRW();
}

inline void CMatrix::CopyOnlyMatrix(const CMatrix& other)
{
    memcpy(this, &other, sizeof(f));
}

inline CMatrix& CMatrix::operator+=(CMatrix const& rhs)
{
    GetRight() += rhs.GetRight();
    GetForward() += rhs.GetForward();
    GetUp() += rhs.GetUp();
    GetPosition() += rhs.GetPosition();
    return *this;
}

inline void CMatrix::SetUnity(void)
{
    rx = 1.0f;
    ry = 0.0f;
    rz = 0.0f;
    fx = 0.0f;
    fy = 1.0f;
    fz = 0.0f;
    ux = 0.0f;
    uy = 0.0f;
    uz = 1.0f;
    px = 0.0f;
    py = 0.0f;
    pz = 0.0f;
}

inline void CMatrix::ResetOrientation(void)
{
    rx = 1.0f;
    ry = 0.0f;
    rz = 0.0f;
    fx = 0.0f;
    fy = 1.0f;
    fz = 0.0f;
    ux = 0.0f;
    uy = 0.0f;
    uz = 1.0f;
}

inline void CMatrix::SetScale(float s)
{
    rx = s;
    ry = 0.0f;
    rz = 0.0f;

    fx = 0.0f;
    fy = s;
    fz = 0.0f;

    ux = 0.0f;
    uy = 0.0f;
    uz = s;

    px = 0.0f;
    py = 0.0f;
    pz = 0.0f;
}

inline void CMatrix::SetTranslate(float x, float y, float z)
{
    rx = 1.0f;
    ry = 0.0f;
    rz = 0.0f;

    fx = 0.0f;
    fy = 1.0f;
    fz = 0.0f;

    ux = 0.0f;
    uy = 0.0f;
    uz = 1.0f;

    px = x;
    py = y;
    pz = z;
}

inline void CMatrix::SetRotateXOnly(float angle)
{
    float c = Cos(angle);
    float s = Sin(angle);

    rx = 1.0f;
    ry = 0.0f;
    rz = 0.0f;

    fx = 0.0f;
    fy = c;
    fz = s;

    ux = 0.0f;
    uy = -s;
    uz = c;
}

inline void CMatrix::SetRotateYOnly(float angle)
{
    float c = Cos(angle);
    float s = Sin(angle);

    rx = c;
    ry = 0.0f;
    rz = -s;

    fx = 0.0f;
    fy = 1.0f;
    fz = 0.0f;

    ux = s;
    uy = 0.0f;
    uz = c;
}

inline void CMatrix::SetRotateZOnly(float angle)
{
    float c = Cos(angle);
    float s = Sin(angle);

    rx = c;
    ry = s;
    rz = 0.0f;

    fx = -s;
    fy = c;
    fz = 0.0f;

    ux = 0.0f;
    uy = 0.0f;
    uz = 1.0f;
}

inline void CMatrix::SetRotateX(float angle)
{
    SetRotateXOnly(angle);
    px = 0.0f;
    py = 0.0f;
    pz = 0.0f;
}


inline void CMatrix::SetRotateY(float angle)
{
    SetRotateYOnly(angle);
    px = 0.0f;
    py = 0.0f;
    pz = 0.0f;
}

inline void CMatrix::SetRotateZ(float angle)
{
    SetRotateZOnly(angle);
    px = 0.0f;
    py = 0.0f;
    pz = 0.0f;
}

inline void CMatrix::SetRotate(float xAngle, float yAngle, float zAngle)
{
    float cX = Cos(xAngle);
    float sX = Sin(xAngle);
    float cY = Cos(yAngle);
    float sY = Sin(yAngle);
    float cZ = Cos(zAngle);
    float sZ = Sin(zAngle);

    rx = cZ * cY - (sZ * sX) * sY;
    ry = (cZ * sX) * sY + sZ * cY;
    rz = -cX * sY;

    fx = -sZ * cX;
    fy = cZ * cX;
    fz = sX;

    ux = (sZ * sX) * cY + cZ * sY;
    uy = sZ * sY - (cZ * sX) * cY;
    uz = cX * cY;

    px = 0.0f;
    py = 0.0f;
    pz = 0.0f;
}

inline void CMatrix::RotateX(float x)
{
    float c = Cos(x);
    float s = Sin(x);

    float ry = this->ry;
    float rz = this->rz;
    float uy = this->fy;
    float uz = this->fz;
    float ay = this->uy;
    float az = this->uz;
    float py = this->py;
    float pz = this->pz;

    this->ry = c * ry - s * rz;
    this->rz = c * rz + s * ry;
    this->fy = c * uy - s * uz;
    this->fz = c * uz + s * uy;
    this->uy = c * ay - s * az;
    this->uz = c * az + s * ay;
    this->py = c * py - s * pz;
    this->pz = c * pz + s * py;
}

inline void CMatrix::RotateY(float y)
{
    float c = Cos(y);
    float s = Sin(y);

    float rx = this->rx;
    float rz = this->rz;
    float ux = this->fx;
    float uz = this->fz;
    float ax = this->ux;
    float az = this->uz;
    float px = this->px;
    float pz = this->pz;

    this->rx = c * rx + s * rz;
    this->rz = c * rz - s * rx;
    this->fx = c * ux + s * uz;
    this->fz = c * uz - s * ux;
    this->ux = c * ax + s * az;
    this->uz = c * az - s * ax;
    this->px = c * px + s * pz;
    this->pz = c * pz - s * px;
}

inline void CMatrix::RotateZ(float z)
{
    float c = Cos(z);
    float s = Sin(z);

    float ry = this->ry;
    float rx = this->rx;
    float uy = this->fy;
    float ux = this->fx;
    float ay = this->uy;
    float ax = this->ux;
    float py = this->py;
    float px = this->px;

    this->rx = c * rx - s * ry;
    this->ry = c * ry + s * rx;
    this->fx = c * ux - s * uy;
    this->fy = c * uy + s * ux;
    this->ux = c * ax - s * ay;
    this->uy = c * ay + s * ax;
    this->px = c * px - s * py;
    this->py = c * py + s * px;
}

inline void CMatrix::Rotate(float x, float y, float z)
{
    float cX = Cos(x);
    float sX = Sin(x);
    float cY = Cos(y);
    float sY = Sin(y);
    float cZ = Cos(z);
    float sZ = Sin(z);

    float rx = this->rx;
    float ry = this->ry;
    float rz = this->rz;
    float ux = this->fx;
    float uy = this->fy;
    float uz = this->fz;
    float ax = this->ux;
    float ay = this->uy;
    float az = this->uz;
    float px = this->px;
    float py = this->py;
    float pz = this->pz;

    float x1 = cZ * cY - (sZ * sX) * sY;
    float x2 = (cZ * sX) * sY + sZ * cY;
    float x3 = -cX * sY;
    float y1 = -sZ * cX;
    float y2 = cZ * cX;
    float y3 = sX;
    float z1 = (sZ * sX) * cY + cZ * sY;
    float z2 = sZ * sY - (cZ * sX) * cY;
    float z3 = cX * cY;

    this->rx = x1 * rx + y1 * ry + z1 * rz;
    this->ry = x2 * rx + y2 * ry + z2 * rz;
    this->rz = x3 * rx + y3 * ry + z3 * rz;
    this->fx = x1 * ux + y1 * uy + z1 * uz;
    this->fy = x2 * ux + y2 * uy + z2 * uz;
    this->fz = x3 * ux + y3 * uy + z3 * uz;
    this->ux = x1 * ax + y1 * ay + z1 * az;
    this->uy = x2 * ax + y2 * ay + z2 * az;
    this->uz = x3 * ax + y3 * ay + z3 * az;
    this->px = x1 * px + y1 * py + z1 * pz;
    this->py = x2 * px + y2 * py + z2 * pz;
    this->pz = x3 * px + y3 * py + z3 * pz;
}

inline CMatrix& CMatrix::operator*=(CMatrix const& rhs)
{
    // TODO: VU0 code
    *this = *this * rhs;
    return *this;
}

inline void CMatrix::Reorthogonalise(void)
{
    CVector& r = GetRight();
    CVector& f = GetForward();
    CVector& u = GetUp();
    u = CrossProduct(r, f);
    u.Normalise();
    r = CrossProduct(f, u);
    r.Normalise();
    f = CrossProduct(u, r);
}

inline CMatrix operator*(const CMatrix& m1, const CMatrix& m2)
{
    // TODO: VU0 code
    CMatrix out;
    out.rx = m1.rx * m2.rx + m1.fx * m2.ry + m1.ux * m2.rz;
    out.ry = m1.ry * m2.rx + m1.fy * m2.ry + m1.uy * m2.rz;
    out.rz = m1.rz * m2.rx + m1.fz * m2.ry + m1.uz * m2.rz;
    out.fx = m1.rx * m2.fx + m1.fx * m2.fy + m1.ux * m2.fz;
    out.fy = m1.ry * m2.fx + m1.fy * m2.fy + m1.uy * m2.fz;
    out.fz = m1.rz * m2.fx + m1.fz * m2.fy + m1.uz * m2.fz;
    out.ux = m1.rx * m2.ux + m1.fx * m2.uy + m1.ux * m2.uz;
    out.uy = m1.ry * m2.ux + m1.fy * m2.uy + m1.uy * m2.uz;
    out.uz = m1.rz * m2.ux + m1.fz * m2.uy + m1.uz * m2.uz;
    out.px = m1.rx * m2.px + m1.fx * m2.py + m1.ux * m2.pz + m1.px;
    out.py = m1.ry * m2.px + m1.fy * m2.py + m1.uy * m2.pz + m1.py;
    out.pz = m1.rz * m2.px + m1.fz * m2.py + m1.uz * m2.pz + m1.pz;
    return out;
}

inline CMatrix& Invert(const CMatrix& src, CMatrix& dst)
{
    dst.f[3][0] = dst.f[3][1] = dst.f[3][2] = 0.0f;

    dst.f[0][0] = src.f[0][0];
    dst.f[0][1] = src.f[1][0];
    dst.f[0][2] = src.f[2][0];
    dst.f[1][0] = src.f[0][1];
    dst.f[1][1] = src.f[1][1];
    dst.f[1][2] = src.f[2][1];
    dst.f[2][0] = src.f[0][2];
    dst.f[2][1] = src.f[1][2];
    dst.f[2][2] = src.f[2][2];

    dst.f[3][0] += dst.f[0][0] * src.f[3][0];
    dst.f[3][1] += dst.f[0][1] * src.f[3][0];
    dst.f[3][2] += dst.f[0][2] * src.f[3][0];

    dst.f[3][0] += dst.f[1][0] * src.f[3][1];
    dst.f[3][1] += dst.f[1][1] * src.f[3][1];
    dst.f[3][2] += dst.f[1][2] * src.f[3][1];

    dst.f[3][0] += dst.f[2][0] * src.f[3][2];
    dst.f[3][1] += dst.f[2][1] * src.f[3][2];
    dst.f[3][2] += dst.f[2][2] * src.f[3][2];

    dst.f[3][0] = -dst.f[3][0];
    dst.f[3][1] = -dst.f[3][1];
    dst.f[3][2] = -dst.f[3][2];

    return dst;
}

inline CMatrix Invert(const CMatrix& matrix)
{
    CMatrix inv;
    return Invert(matrix, inv);
}

// ---------------------------------------------------------------------------
// CCompressedMatrix
// ---------------------------------------------------------------------------
class CCompressedMatrixNotAligned
{
    CVector     m_vecPos;
    std::int8_t m_rightX, m_rightY, m_rightZ;
    std::int8_t m_upX, m_upY, m_upZ;
public:
    void CompressFromFullMatrix(CMatrix& other)
    {
        m_rightX = (std::int8_t)(127.f * other.GetRight().x);
        m_rightY = (std::int8_t)(127.f * other.GetRight().y);
        m_rightZ = (std::int8_t)(127.f * other.GetRight().z);
        m_upX = (std::int8_t)(127.f * other.GetForward().x);
        m_upY = (std::int8_t)(127.f * other.GetForward().y);
        m_upZ = (std::int8_t)(127.f * other.GetForward().z);
        m_vecPos = other.GetPosition();
    }
    void DecompressIntoFullMatrix(CMatrix& other)
    {
        other.GetRight().x = m_rightX / 127.f;
        other.GetRight().y = m_rightY / 127.f;
        other.GetRight().z = m_rightZ / 127.f;
        other.GetForward().x = m_upX / 127.f;
        other.GetForward().y = m_upY / 127.f;
        other.GetForward().z = m_upZ / 127.f;
        other.GetUp() = CrossProduct(other.GetRight(), other.GetForward());
        other.GetPosition() = m_vecPos;
        other.Reorthogonalise();
    }
};

class CCompressedMatrix : public CCompressedMatrixNotAligned { int _alignment; };

// ---------------------------------------------------------------------------
// CRect
// ---------------------------------------------------------------------------
class CRect
{
public:
    float left, bottom, right, top;

    CRect() {}
    CRect(float l, float b, float r, float t) : left(l), bottom(b), right(r), top(t) {}

    void ContainPoint(CVector const& v)
    {
        if (v.x < left)   left = v.x;
        if (v.x > right)  right = v.x;
        if (v.y < top)    top = v.y;
        if (v.y > bottom) bottom = v.y;
    }
    void ContainRect(const CRect& r)
    {
        if (r.left < left)   left = r.left;
        if (r.right > right)  right = r.right;
        if (r.top < top)    top = r.top;
        if (r.bottom > bottom) bottom = r.bottom;
    }
    bool IsPointInside(const CVector2D& p) const
    {
        return p.x >= left && p.x <= right && p.y >= top && p.y <= bottom;
    }
    bool IsPointInside(const CVector2D& p, float r) const
    {
        return p.x >= left - r && p.x <= right + r && p.y >= top - r && p.y <= bottom + r;
    }
    void Translate(float x, float y) { left += x; right += x; bottom += y; top += y; }
    void Grow(float r) { left -= r; right += r; top -= r; bottom += r; }
    void Grow(float l, float r) { left -= l; top -= l; right += r; bottom += r; }
    void Grow(float l, float r, float t, float b) { left -= l; top -= t; right += r; bottom += b; }
    float GetWidth() { return right - left; }
    float GetHeight() { return bottom - top; }
};

struct RwRaster
{
    RwRaster* parent;        // +0x00
    uint8_t* pixels;         // +0x04
    uint8_t* palette;        // +0x08
    int32_t    width;        // +0x0C
    int32_t    height;       // +0x10
    int32_t    depth;        // +0x14
    int32_t    stride;       // +0x18
    int16_t    offsetX;      // +0x1C
    int16_t    offsetY;      // +0x1E
    uint8_t    format;       // +0x20
    uint8_t    privateFlags; // +0x21
    uint8_t    type;         // +0x22
    uint8_t    flags;        // +0x23
    ///...
};

struct RwTexture
{
    RwRaster* raster;            // +0x00
    void* dict;                  // +0x04  (TexDictionary*)
    // LLLink for dict linkage   // +0x08, +0x0C
    uint8_t    _dictLink[8];
    char       name[32];         // +0x10
    char       mask[32];         // +0x30
    uint32_t   filterAddressing; // +0x50
    int32_t    refCount;         // +0x54
};

inline int32_t RwTextureGetWidth(const RwTexture* tex) { return tex->raster ? tex->raster->width : 0; }
inline int32_t RwTextureGetHeight(const RwTexture* tex) { return tex->raster ? tex->raster->height : 0; }
inline RwRaster* RwTextureGetRaster(const RwTexture* texture) { return texture->raster; }
inline int32_t RwRasterGetWidth(const RwRaster* raster) { return raster->width; }
inline int32_t RwRasterGetHeight(const RwRaster* raster) { return raster->height; }

class CRGBA
{
public:
    union
    {
        uint32_t color32;
        struct
        {
            uint8_t r, g, b, a;
        };
        struct
        {
            uint8_t red, green, blue, alpha;
        };
    };

    CRGBA(void) {}
    CRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
        : r(r)
        , g(g)
        , b(b)
        , a(a)
    {
    }

    bool operator==(const CRGBA& right)
    {
        return this->r == right.r && this->g == right.g && this->b == right.b && this->a == right.a;
    }

    bool operator!=(const CRGBA& right)
    {
        return !(*this == right);
    }

    CRGBA& operator=(const CRGBA& right)
    {
        this->r = right.r;
        this->g = right.g;
        this->b = right.b;
        this->a = right.a;
        return *this;
    }
};

struct RwImage
{
    int32_t flags;
    int32_t width;
    int32_t height;
    int32_t depth;
    int32_t stride;
    uint8_t* pixels;
    uint8_t* palette;
};

struct RwRGBA
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t alpha;
};

enum RwObjectType
{
    rwNAOBJECT = 0,
    rwCAMERA = 1,
    rwLIGHT = 2,
    rwATOMIC = 3,
    rwCLUMP = 4
};

struct RwObject
{
    uint8_t type;
    uint8_t subType;
    uint8_t flags;
    uint8_t privateFlags;
    void* parent;  // pointer to RwFrame
};

enum RwRenderState
{
    rwRENDERSTATENARENDERSTATE = 0,
    rwRENDERSTATETEXTURERASTER,
    rwRENDERSTATETEXTUREADDRESS,
    rwRENDERSTATETEXTUREADDRESSU,
    rwRENDERSTATETEXTUREADDRESSV,
    rwRENDERSTATETEXTUREPERSPECTIVE,
    rwRENDERSTATEZTESTENABLE,
    rwRENDERSTATESHADEMODE,
    rwRENDERSTATEZWRITEENABLE,
    rwRENDERSTATETEXTUREFILTER,
    rwRENDERSTATESRCBLEND,
    rwRENDERSTATEDESTBLEND,
    rwRENDERSTATEVERTEXALPHAENABLE,
    rwRENDERSTATEBORDERCOLOR,
    rwRENDERSTATEFOGENABLE,
    rwRENDERSTATEFOGCOLOR,
    rwRENDERSTATEFOGTYPE,
    rwRENDERSTATEFOGDENSITY,
    rwRENDERSTATECULLMODE = 20,
    rwRENDERSTATESTENCILENABLE,
    rwRENDERSTATESTENCILFAIL,
    rwRENDERSTATESTENCILZFAIL,
    rwRENDERSTATESTENCILPASS,
    rwRENDERSTATESTENCILFUNCTION,
    rwRENDERSTATESTENCILFUNCTIONREF,
    rwRENDERSTATESTENCILFUNCTIONMASK,
    rwRENDERSTATESTENCILFUNCTIONWRITEMASK,
    rwRENDERSTATEALPHATESTFUNCTION,
    rwRENDERSTATEALPHATESTFUNCTIONREF,
    rwRENDERSTATEFORCEENUMSIZEINT = ((int32_t)((~((uint32_t)0)) >> 1))
};

enum RwBlendFunction
{
    rwBLENDNABLEND = 0,
    rwBLENDZERO,
    rwBLENDONE,
    rwBLENDSRCCOLOR,
    rwBLENDINVSRCCOLOR,
    rwBLENDSRCALPHA,
    rwBLENDINVSRCALPHA,
    rwBLENDDESTALPHA,
    rwBLENDINVDESTALPHA,
    rwBLENDDESTCOLOR,
    rwBLENDINVDESTCOLOR,
    rwBLENDSRCALPHASAT,
    rwBLENDFUNCTIONFORCEENUMSIZEINT = ((int32_t)((~((uint32_t)0)) >> 1))
};

struct RwLinkList { void* link_next; void* link_prev; };

typedef int32_t (__cdecl* RwDebugHandler)(const char* message);
typedef void* (__cdecl* RwMemoryAllocFn)(void* memoryManager, uint32_t size, uint32_t hint);
typedef void (__cdecl* RwMemoryFreeFn)(void* memoryManager, void* mem);
typedef void* (__cdecl* RwMallocFn)(size_t size, uint32_t hint);
typedef void (__cdecl* RwFreeFn)(void* mem);
typedef void* (__cdecl* RwReallocFn)(void* mem, size_t newSize, uint32_t hint);
typedef void* (__cdecl* RwCallocFn)(size_t numObj, size_t sizeObj, uint32_t hint);

typedef int32_t (__cdecl* RwSystemFunc)(int32_t nOption, void* pOut, void* pInOut, int32_t nIn);
typedef int32_t (__cdecl* RwRenderStateSetFunction)(int32_t state, void* value);
typedef int32_t (__cdecl* RwRenderStateGetFunction)(int32_t state, void* value);
typedef int32_t (__cdecl* RwIm2DRenderLineFunction)(void* vertices, int32_t numVertices, int32_t vert1, int32_t vert2);
typedef int32_t (__cdecl* RwIm2DRenderTriangleFunction)(void* vertices, int32_t numVertices, int32_t vert1, int32_t vert2, int32_t vert3);
typedef int32_t (__cdecl* RwIm2DRenderPrimitiveFunction)(int32_t primType, void* vertices, int32_t numVertices);
typedef int32_t (__cdecl* RwIm2DRenderIndexedPrimitiveFunction)(int32_t primType, void* vertices, int32_t numVertices, void* indices, int32_t numIndices);
typedef int32_t (__cdecl* RwIm3DRenderLineFunction)(int32_t vert1, int32_t vert2);
typedef int32_t (__cdecl* RwIm3DRenderTriangleFunction)(int32_t vert1, int32_t vert2, int32_t vert3);
typedef int32_t (__cdecl* RwIm3DRenderPrimitiveFunction)(int32_t primType);
typedef int32_t (__cdecl* RwIm3DRenderIndexedPrimitiveFunction)(int32_t primType, void* indices, int32_t numIndices);

struct RwDevice
{
    float gammaCorrection;
    RwSystemFunc fpSystem;
    float zBufferNear;
    float zBufferFar;

    RwRenderStateSetFunction fpRenderStateSet;
    RwRenderStateGetFunction fpRenderStateGet;

    RwIm2DRenderLineFunction fpIm2DRenderLine;
    RwIm2DRenderTriangleFunction fpIm2DRenderTriangle;
    RwIm2DRenderPrimitiveFunction fpIm2DRenderPrimitive;
    RwIm2DRenderIndexedPrimitiveFunction fpIm2DRenderIndexedPrimitive;

    RwIm3DRenderLineFunction fpIm3DRenderLine;
    RwIm3DRenderTriangleFunction fpIm3DRenderTriangle;
    RwIm3DRenderPrimitiveFunction fpIm3DRenderPrimitive;
    RwIm3DRenderIndexedPrimitiveFunction fpIm3DRenderIndexedPrimitive;
};

struct RwStringFunctions
{
    void* vecSprintf;
    void* vecVsprintf;
    void* vecStrcpy;
    void* vecStrncpy;
    void* vecStrcat;
    void* vecStrncat;
    void* vecStrrchr;
    void* vecStrchr;
    void* vecStrstr;
    void* vecStrcmp;
    void* vecStrncmp;
    void* vecStricmp;
    void* vecStrlen;
    void* vecStrupr;
    void* vecStrlwr;
    void* vecStrtok;
    void* vecSscanf;
};

struct RwMemoryFunctions
{
    RwMallocFn rwmalloc;
    RwFreeFn rwfree;
    RwReallocFn rwrealloc;
    RwCallocFn rwcalloc;
};

struct RwStandardFunc
{
    void* func;
};

struct RwMetrics
{
    void* data;
};

enum RwEngineStatus
{
    rwENGINESTATUSIDLE = 0,
    rwENGINESTATUSINITED = 1,
    rwENGINESTATUSOPENED = 2,
    rwENGINESTATUSSTARTED = 3
};

struct RwGlobals
{
    void* curCamera;
    void* curWorld;
    uint16_t renderFrame;
    uint16_t lightFrame;
    uint16_t pad[2];
    RwDevice dOpenDevice;
    RwStandardFunc stdFunc[29];
    RwLinkList dirtyFrameList;
    RwStringFunctions stringFuncs;
    RwMemoryFunctions memoryFuncs;
    RwMemoryAllocFn memoryAlloc;
    RwMemoryFreeFn memoryFree;
    RwMetrics* metrics;
    RwEngineStatus engineStatus;
    uint32_t resArenaInitSize;
};

struct RwBBox { RwV3d sup, inf; };

struct RwFrustumPlane
{
    float a, b, c, d;
    uint8_t closestX;
    uint8_t closestY;
    uint8_t closestZ;
    uint8_t pad;
};

enum RwCameraProjection
{
    rwNACAMERAPROJECTION = 0,
    rwPERSPECTIVE = 1,
    rwPARALLEL = 2,
    rwCAMERAPROJECTIONFORCEENUMSIZEINT = ((int32_t)((~((uint32_t)0)) >> 1))
};

struct RwLLLink
{
    RwLLLink* next;
    RwLLLink* prev;
};

struct RwObjectHasFrame
{
    RwObject object;
    RwLLLink lFrame;
    RwObjectHasFrame* (*sync)(RwObjectHasFrame* object);
};

struct RwCamera
{
    RwObjectHasFrame    object;
    RwCameraProjection  projectionType;
    void (*beginUpdate)(RwCamera*);
    void (*endUpdate)(RwCamera*);
    RwMatrix viewMatrix;
    RwRaster* frameBuffer;
    RwRaster* zBuffer;
    RwV2d viewWindow;
    RwV2d recipViewWindow;
    RwV2d viewOffset;
    float nearPlane;
    float farPlane;
    float fogPlane;
    float zScale, zShift;
    RwFrustumPlane frustumPlanes[6];
    RwBBox frustumBoundBox;
    RwV3d frustumCorners[8];
};

struct RpWorld
{
    RwObject object;
    uint32_t flags;
    //...
};

struct CScene
{
    RpWorld* m_pRwWorld;
    RwCamera* m_pRwCamera;
};

struct RwRGBAReal
{
    float red;
    float green;
    float blue;
    float alpha;
};

struct RpClump
{
    RwObject object;
    RwLinkList atomicList;
    RwLinkList lightList;
    RwLinkList cameraList;
    RwLLLink inWorldLink;
    void* callback;
};
