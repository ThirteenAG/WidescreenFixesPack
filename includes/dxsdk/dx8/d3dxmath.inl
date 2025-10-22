//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 1998 Microsoft Corporation.  All Rights Reserved.
//
//  File:       d3dxmath.inl
//  Content:    D3DX math inline functions
//
//////////////////////////////////////////////////////////////////////////////

#ifndef __D3DXMATH_INL__
#define __D3DXMATH_INL__


//===========================================================================
//
// Inline Class Methods
//
//===========================================================================

#ifdef __cplusplus

//--------------------------
// 2D Vector
//--------------------------

D3DXINLINE 
D3DXVECTOR2::D3DXVECTOR2( const float *pf )
{
#ifdef D3DX_DEBUG
    if(!pf)
        return;
#endif

    x = pf[0];
    y = pf[1];
}

D3DXINLINE 
D3DXVECTOR2::D3DXVECTOR2( float fx, float fy )
{
    x = fx;
    y = fy;
}

// casting
D3DXINLINE 
D3DXVECTOR2::operator float* ()
{
    return (float *) &x;
}

D3DXINLINE 
D3DXVECTOR2::operator const float* () const
{
    return (const float *) &x;
}

// assignment operators
D3DXINLINE D3DXVECTOR2& 
D3DXVECTOR2::operator += ( const D3DXVECTOR2& v )
{
    x += v.x;
    y += v.y;
    return *this;
}

D3DXINLINE D3DXVECTOR2& 
D3DXVECTOR2::operator -= ( const D3DXVECTOR2& v )
{
    x -= v.x;
    y -= v.y;
    return *this;
}

D3DXINLINE D3DXVECTOR2& 
D3DXVECTOR2::operator *= ( float f )
{
    x *= f;
    y *= f;
    return *this;
}

D3DXINLINE D3DXVECTOR2& 
D3DXVECTOR2::operator /= ( float f )
{
    float fInv = 1.0f / f;
    x *= fInv;
    y *= fInv;
    return *this;
}

// unary operators
D3DXINLINE D3DXVECTOR2 
D3DXVECTOR2::operator + () const
{
    return *this;
}

D3DXINLINE D3DXVECTOR2 
D3DXVECTOR2::operator - () const
{
    return D3DXVECTOR2(-x, -y);
}

// binary operators
D3DXINLINE D3DXVECTOR2 
D3DXVECTOR2::operator + ( const D3DXVECTOR2& v ) const
{
    return D3DXVECTOR2(x + v.x, y + v.y);
}

D3DXINLINE D3DXVECTOR2 
D3DXVECTOR2::operator - ( const D3DXVECTOR2& v ) const
{
    return D3DXVECTOR2(x - v.x, y - v.y);
}

D3DXINLINE D3DXVECTOR2 
D3DXVECTOR2::operator * ( float f ) const
{
    return D3DXVECTOR2(x * f, y * f);
}

D3DXINLINE D3DXVECTOR2 
D3DXVECTOR2::operator / ( float f ) const
{
    float fInv = 1.0f / f;
    return D3DXVECTOR2(x * fInv, y * fInv);
}


D3DXINLINE D3DXVECTOR2 
operator * ( float f, const D3DXVECTOR2& v )
{
    return D3DXVECTOR2(f * v.x, f * v.y);
}

D3DXINLINE BOOL 
D3DXVECTOR2::operator == ( const D3DXVECTOR2& v ) const
{
    return x == v.x && y == v.y;
}

D3DXINLINE BOOL 
D3DXVECTOR2::operator != ( const D3DXVECTOR2& v ) const
{
    return x != v.x || y != v.y;
}




//--------------------------
// 3D Vector
//--------------------------
D3DXINLINE 
D3DXVECTOR3::D3DXVECTOR3( const float *pf )
{
#ifdef D3DX_DEBUG
    if(!pf)
        return;
#endif

    x = pf[0];
    y = pf[1];
    z = pf[2];
}

D3DXINLINE 
D3DXVECTOR3::D3DXVECTOR3( const D3DVECTOR& v )
{
    x = v.x;
    y = v.y;
    z = v.z;
}

D3DXINLINE 
D3DXVECTOR3::D3DXVECTOR3( float fx, float fy, float fz )
{
    x = fx;
    y = fy;
    z = fz;
}


// casting
D3DXINLINE 
D3DXVECTOR3::operator float* ()
{
    return (float *) &x;
}

D3DXINLINE 
D3DXVECTOR3::operator const float* () const
{
    return (const float *) &x;
}


D3DXINLINE 
D3DXVECTOR3::operator D3DVECTOR* ()
{
    return (D3DVECTOR *) &x;
}

D3DXINLINE 
D3DXVECTOR3::operator const D3DVECTOR* () const
{
    return (const D3DVECTOR *) &x;
}


D3DXINLINE 
D3DXVECTOR3::operator D3DVECTOR& ()
{
    return *((D3DVECTOR *) &x);
}

D3DXINLINE 
D3DXVECTOR3::operator const D3DVECTOR& () const
{
    return *((const D3DVECTOR *) &x);
}


// assignment operators
D3DXINLINE D3DXVECTOR3& 
D3DXVECTOR3::operator += ( const D3DXVECTOR3& v )
{
    x += v.x;
    y += v.y;
    z += v.z;
    return *this;
}

D3DXINLINE D3DXVECTOR3& 
D3DXVECTOR3::operator -= ( const D3DXVECTOR3& v )
{
    x -= v.x;
    y -= v.y;
    z -= v.z;
    return *this;
}

D3DXINLINE D3DXVECTOR3& 
D3DXVECTOR3::operator *= ( float f )
{
    x *= f;
    y *= f;
    z *= f;
    return *this;
}

D3DXINLINE D3DXVECTOR3& 
D3DXVECTOR3::operator /= ( float f )
{
    float fInv = 1.0f / f;
    x *= fInv;
    y *= fInv;
    z *= fInv;
    return *this;
}


// unary operators
D3DXINLINE D3DXVECTOR3 
D3DXVECTOR3::operator + () const
{
    return *this;
}

D3DXINLINE D3DXVECTOR3 
D3DXVECTOR3::operator - () const
{
    return D3DXVECTOR3(-x, -y, -z);
}


// binary operators
D3DXINLINE D3DXVECTOR3 
D3DXVECTOR3::operator + ( const D3DXVECTOR3& v ) const
{
    return D3DXVECTOR3(x + v.x, y + v.y, z + v.z);
}

D3DXINLINE D3DXVECTOR3 
D3DXVECTOR3::operator - ( const D3DXVECTOR3& v ) const
{
    return D3DXVECTOR3(x - v.x, y - v.y, z - v.z);
}

D3DXINLINE D3DXVECTOR3 
D3DXVECTOR3::operator * ( float f ) const
{
    return D3DXVECTOR3(x * f, y * f, z * f);
}

D3DXINLINE D3DXVECTOR3 
D3DXVECTOR3::operator / ( float f ) const
{
    float fInv = 1.0f / f;
    return D3DXVECTOR3(x * fInv, y * fInv, z * fInv);
}


D3DXINLINE D3DXVECTOR3 
operator * ( float f, const struct D3DXVECTOR3& v )
{
    return D3DXVECTOR3(f * v.x, f * v.y, f * v.z);
}


D3DXINLINE BOOL 
D3DXVECTOR3::operator == ( const D3DXVECTOR3& v ) const
{
    return x == v.x && y == v.y && z == v.z;
}

D3DXINLINE BOOL 
D3DXVECTOR3::operator != ( const D3DXVECTOR3& v ) const
{
    return x != v.x || y != v.y || z != v.z;
}



//--------------------------
// 4D Vector
//--------------------------
D3DXINLINE 
D3DXVECTOR4::D3DXVECTOR4( const float *pf )
{
#ifdef D3DX_DEBUG
    if(!pf)
        return;
#endif

    x = pf[0];
    y = pf[1];
    z = pf[2];
    w = pf[3];
}

D3DXINLINE 
D3DXVECTOR4::D3DXVECTOR4( float fx, float fy, float fz, float fw )
{
    x = fx;
    y = fy;
    z = fz;
    w = fw;
}


// casting
D3DXINLINE 
D3DXVECTOR4::operator float* ()
{
    return (float *) &x;
}

D3DXINLINE 
D3DXVECTOR4::operator const float* () const
{
    return (const float *) &x;
}


// assignment operators
D3DXINLINE D3DXVECTOR4& 
D3DXVECTOR4::operator += ( const D3DXVECTOR4& v )
{
    x += v.x;
    y += v.y;
    z += v.z;
    w += v.w;
    return *this;
}

D3DXINLINE D3DXVECTOR4& 
D3DXVECTOR4::operator -= ( const D3DXVECTOR4& v )
{
    x -= v.x;
    y -= v.y;
    z -= v.z;
    w -= v.w;
    return *this;
}

D3DXINLINE D3DXVECTOR4& 
D3DXVECTOR4::operator *= ( float f )
{
    x *= f;
    y *= f;
    z *= f;
    w *= f;
    return *this;
}

D3DXINLINE D3DXVECTOR4& 
D3DXVECTOR4::operator /= ( float f )
{
    float fInv = 1.0f / f;
    x *= fInv;
    y *= fInv;
    z *= fInv;
    w *= fInv;
    return *this;
}


// unary operators
D3DXINLINE D3DXVECTOR4 
D3DXVECTOR4::operator + () const
{
    return *this;
}

D3DXINLINE D3DXVECTOR4 
D3DXVECTOR4::operator - () const
{
    return D3DXVECTOR4(-x, -y, -z, -w);
}


// binary operators
D3DXINLINE D3DXVECTOR4 
D3DXVECTOR4::operator + ( const D3DXVECTOR4& v ) const
{
    return D3DXVECTOR4(x + v.x, y + v.y, z + v.z, w + v.w);
}

D3DXINLINE D3DXVECTOR4 
D3DXVECTOR4::operator - ( const D3DXVECTOR4& v ) const
{
    return D3DXVECTOR4(x - v.x, y - v.y, z - v.z, w - v.w);
}

D3DXINLINE D3DXVECTOR4 
D3DXVECTOR4::operator * ( float f ) const
{
    return D3DXVECTOR4(x * f, y * f, z * f, w * f);
}

D3DXINLINE D3DXVECTOR4 
D3DXVECTOR4::operator / ( float f ) const
{
    float fInv = 1.0f / f;
    return D3DXVECTOR4(x * fInv, y * fInv, z * fInv, w * fInv);
}


D3DXINLINE D3DXVECTOR4 
operator * ( float f, const D3DXVECTOR4& v )
{
    return D3DXVECTOR4(f * v.x, f * v.y, f * v.z, f * v.w);
}


D3DXINLINE BOOL 
D3DXVECTOR4::operator == ( const D3DXVECTOR4& v ) const
{
    return x == v.x && y == v.y && z == v.z && w == v.w;
}

D3DXINLINE BOOL 
D3DXVECTOR4::operator != ( const D3DXVECTOR4& v ) const
{
    return x != v.x || y != v.y || z != v.z || w != v.w;
}


//--------------------------
// Matrix
//--------------------------
D3DXINLINE 
D3DXMATRIX::D3DXMATRIX( const float* pf )
{
#ifdef D3DX_DEBUG
    if(!pf)
        return;
#endif

    memcpy(&m00, pf, sizeof(D3DXMATRIX));
}

D3DXINLINE 
D3DXMATRIX::D3DXMATRIX( const D3DMATRIX& mat )
{
    memcpy(&m00, &mat, sizeof(D3DXMATRIX));
}

D3DXINLINE 
D3DXMATRIX::D3DXMATRIX( float f00, float f01, float f02, float f03,
                        float f10, float f11, float f12, float f13,
                        float f20, float f21, float f22, float f23,
                        float f30, float f31, float f32, float f33 )
{
    m00 = f00; m01 = f01; m02 = f02; m03 = f03;
    m10 = f10; m11 = f11; m12 = f12; m13 = f13;
    m20 = f20; m21 = f21; m22 = f22; m23 = f23;
    m30 = f30; m31 = f31; m32 = f32; m33 = f33;
}



// access grants
D3DXINLINE float& 
D3DXMATRIX::operator () ( UINT iRow, UINT iCol )
{
    return m[iRow][iCol];
}

D3DXINLINE float  
D3DXMATRIX::operator () ( UINT iRow, UINT iCol ) const
{
    return m[iRow][iCol];
}


// casting operators
D3DXINLINE 
D3DXMATRIX::operator float* ()
{
    return (float *) &m00;
}

D3DXINLINE 
D3DXMATRIX::operator const float* () const
{
    return (const float *) &m00;
}


D3DXINLINE 
D3DXMATRIX::operator D3DMATRIX* ()
{
    return (D3DMATRIX *) &m00;
}

D3DXINLINE 
D3DXMATRIX::operator const D3DMATRIX* () const
{
    return (const D3DMATRIX *) &m00;
}


D3DXINLINE 
D3DXMATRIX::operator D3DMATRIX& ()
{
    return *((D3DMATRIX *) &m00);
}

D3DXINLINE 
D3DXMATRIX::operator const D3DMATRIX& () const
{
    return *((const D3DMATRIX *) &m00);
}


// assignment operators
D3DXINLINE D3DXMATRIX& 
D3DXMATRIX::operator *= ( const D3DXMATRIX& mat )
{
    D3DXMatrixMultiply(this, this, &mat);
    return *this;
}

D3DXINLINE D3DXMATRIX& 
D3DXMATRIX::operator += ( const D3DXMATRIX& mat )
{
    m00 += mat.m00; m01 += mat.m01; m02 += mat.m02; m03 += mat.m03;
    m10 += mat.m10; m11 += mat.m11; m12 += mat.m12; m13 += mat.m13;
    m20 += mat.m20; m21 += mat.m21; m22 += mat.m22; m23 += mat.m23;
    m30 += mat.m30; m31 += mat.m31; m32 += mat.m32; m33 += mat.m33;
    return *this;
}

D3DXINLINE D3DXMATRIX& 
D3DXMATRIX::operator -= ( const D3DXMATRIX& mat )
{
    m00 -= mat.m00; m01 -= mat.m01; m02 -= mat.m02; m03 -= mat.m03;
    m10 -= mat.m10; m11 -= mat.m11; m12 -= mat.m12; m13 -= mat.m13;
    m20 -= mat.m20; m21 -= mat.m21; m22 -= mat.m22; m23 -= mat.m23;
    m30 -= mat.m30; m31 -= mat.m31; m32 -= mat.m32; m33 -= mat.m33;
    return *this;
}

D3DXINLINE D3DXMATRIX& 
D3DXMATRIX::operator *= ( float f )
{
    m00 *= f; m01 *= f; m02 *= f; m03 *= f;
    m10 *= f; m11 *= f; m12 *= f; m13 *= f;
    m20 *= f; m21 *= f; m22 *= f; m23 *= f;
    m30 *= f; m31 *= f; m32 *= f; m33 *= f;
    return *this;
}

D3DXINLINE D3DXMATRIX& 
D3DXMATRIX::operator /= ( float f )
{
    float fInv = 1.0f / f;
    m00 *= fInv; m01 *= fInv; m02 *= fInv; m03 *= fInv;
    m10 *= fInv; m11 *= fInv; m12 *= fInv; m13 *= fInv;
    m20 *= fInv; m21 *= fInv; m22 *= fInv; m23 *= fInv;
    m30 *= fInv; m31 *= fInv; m32 *= fInv; m33 *= fInv;
    return *this;
}


// unary operators
D3DXINLINE D3DXMATRIX 
D3DXMATRIX::operator + () const
{
    return *this;
}

D3DXINLINE D3DXMATRIX 
D3DXMATRIX::operator - () const
{
    return D3DXMATRIX(-m00, -m01, -m02, -m03,
                      -m10, -m11, -m12, -m13,
                      -m20, -m21, -m22, -m23,
                      -m30, -m31, -m32, -m33);
}


// binary operators
D3DXINLINE D3DXMATRIX 
D3DXMATRIX::operator * ( const D3DXMATRIX& mat ) const
{
    D3DXMATRIX matT;
    D3DXMatrixMultiply(&matT, this, &mat);
    return matT;
}

D3DXINLINE D3DXMATRIX 
D3DXMATRIX::operator + ( const D3DXMATRIX& mat ) const
{
    return D3DXMATRIX(m00 + mat.m00, m01 + mat.m01, m02 + mat.m02, m03 + mat.m03, 
                      m10 + mat.m10, m11 + mat.m11, m12 + mat.m12, m13 + mat.m13, 
                      m20 + mat.m20, m21 + mat.m21, m22 + mat.m22, m23 + mat.m23, 
                      m30 + mat.m30, m31 + mat.m31, m32 + mat.m32, m33 + mat.m33);
}    

D3DXINLINE D3DXMATRIX 
D3DXMATRIX::operator - ( const D3DXMATRIX& mat ) const
{
    return D3DXMATRIX(m00 - mat.m00, m01 - mat.m01, m02 - mat.m02, m03 - mat.m03, 
                      m10 - mat.m10, m11 - mat.m11, m12 - mat.m12, m13 - mat.m13, 
                      m20 - mat.m20, m21 - mat.m21, m22 - mat.m22, m23 - mat.m23, 
                      m30 - mat.m30, m31 - mat.m31, m32 - mat.m32, m33 - mat.m33);
}

D3DXINLINE D3DXMATRIX 
D3DXMATRIX::operator * ( float f ) const
{
    return D3DXMATRIX(m00 * f, m01 * f, m02 * f, m03 * f, 
                      m10 * f, m11 * f, m12 * f, m13 * f, 
                      m20 * f, m21 * f, m22 * f, m23 * f, 
                      m30 * f, m31 * f, m32 * f, m33 * f);
}

D3DXINLINE D3DXMATRIX 
D3DXMATRIX::operator / ( float f ) const
{
    float fInv = 1.0f / f;
    return D3DXMATRIX(m00 * fInv, m01 * fInv, m02 * fInv, m03 * fInv, 
                      m10 * fInv, m11 * fInv, m12 * fInv, m13 * fInv, 
                      m20 * fInv, m21 * fInv, m22 * fInv, m23 * fInv, 
                      m30 * fInv, m31 * fInv, m32 * fInv, m33 * fInv);
}


D3DXINLINE D3DXMATRIX 
operator * ( float f, const D3DXMATRIX& mat )
{
    return D3DXMATRIX(f * mat.m00, f * mat.m01, f * mat.m02, f * mat.m03, 
                      f * mat.m10, f * mat.m11, f * mat.m12, f * mat.m13, 
                      f * mat.m20, f * mat.m21, f * mat.m22, f * mat.m23, 
                      f * mat.m30, f * mat.m31, f * mat.m32, f * mat.m33);
}


D3DXINLINE BOOL 
D3DXMATRIX::operator == ( const D3DXMATRIX& mat ) const
{
    return 0 == memcmp(this, &mat, sizeof(D3DXMATRIX));
}

D3DXINLINE BOOL 
D3DXMATRIX::operator != ( const D3DXMATRIX& mat ) const
{
    return 0 != memcmp(this, &mat, sizeof(D3DXMATRIX));
}



//--------------------------
// Quaternion
//--------------------------

D3DXINLINE 
D3DXQUATERNION::D3DXQUATERNION( const float* pf )
{
#ifdef D3DX_DEBUG
    if(!pf)
        return;
#endif

    x = pf[0];
    y = pf[1];
    z = pf[2];
    w = pf[3];
}

D3DXINLINE 
D3DXQUATERNION::D3DXQUATERNION( float fx, float fy, float fz, float fw )
{
    x = fx;
    y = fy;
    z = fz;
    w = fw;
}


// casting
D3DXINLINE 
D3DXQUATERNION::operator float* ()
{
    return (float *) &x;
}

D3DXINLINE 
D3DXQUATERNION::operator const float* () const
{
    return (const float *) &x;
}


// assignment operators
D3DXINLINE D3DXQUATERNION& 
D3DXQUATERNION::operator += ( const D3DXQUATERNION& q )
{
    x += q.x;
    y += q.y;
    z += q.z;
    w += q.w;
    return *this;
}

D3DXINLINE D3DXQUATERNION& 
D3DXQUATERNION::operator -= ( const D3DXQUATERNION& q )
{
    x -= q.x;
    y -= q.y;
    z -= q.z;
    w -= q.w;
    return *this;
}

D3DXINLINE D3DXQUATERNION& 
D3DXQUATERNION::operator *= ( const D3DXQUATERNION& q )
{
    D3DXQuaternionMultiply(this, this, &q);
    return *this;
}

D3DXINLINE D3DXQUATERNION& 
D3DXQUATERNION::operator *= ( float f )
{
    x *= f;
    y *= f;
    z *= f;
    w *= f;
    return *this;
}

D3DXINLINE D3DXQUATERNION& 
D3DXQUATERNION::operator /= ( float f )
{
    float fInv = 1.0f / f;
    x *= fInv;
    y *= fInv;
    z *= fInv;
    w *= fInv;
    return *this;
}


// unary operators
D3DXINLINE D3DXQUATERNION  
D3DXQUATERNION::operator + () const
{
    return *this;
}

D3DXINLINE D3DXQUATERNION  
D3DXQUATERNION::operator - () const
{
    return D3DXQUATERNION(-x, -y, -z, -w);
}


// binary operators
D3DXINLINE D3DXQUATERNION 
D3DXQUATERNION::operator + ( const D3DXQUATERNION& q ) const
{
    return D3DXQUATERNION(x + q.x, y + q.y, z + q.z, w + q.w);
}

D3DXINLINE D3DXQUATERNION 
D3DXQUATERNION::operator - ( const D3DXQUATERNION& q ) const
{
    return D3DXQUATERNION(x - q.x, y - q.y, z - q.z, w - q.w);
}

D3DXINLINE D3DXQUATERNION 
D3DXQUATERNION::operator * ( const D3DXQUATERNION& q ) const
{
    D3DXQUATERNION qT;
    D3DXQuaternionMultiply(&qT, this, &q);
    return qT;
}

D3DXINLINE D3DXQUATERNION 
D3DXQUATERNION::operator * ( float f ) const
{
    return D3DXQUATERNION(x * f, y * f, z * f, w * f);
}

D3DXINLINE D3DXQUATERNION 
D3DXQUATERNION::operator / ( float f ) const
{
    float fInv = 1.0f / f;
    return D3DXQUATERNION(x * fInv, y * fInv, z * fInv, w * fInv);
}


D3DXINLINE D3DXQUATERNION 
operator * (float f, const D3DXQUATERNION& q )
{
    return D3DXQUATERNION(f * q.x, f * q.y, f * q.z, f * q.w);
}


D3DXINLINE BOOL 
D3DXQUATERNION::operator == ( const D3DXQUATERNION& q ) const
{
    return x == q.x && y == q.y && z == q.z && w == q.w;
}

D3DXINLINE BOOL 
D3DXQUATERNION::operator != ( const D3DXQUATERNION& q ) const
{
    return x != q.x || y != q.y || z != q.z || w != q.w;
}



//--------------------------
// Plane
//--------------------------

D3DXINLINE 
D3DXPLANE::D3DXPLANE( const float* pf )
{
#ifdef D3DX_DEBUG
    if(!pf)
        return;
#endif

    a = pf[0];
    b = pf[1];
    c = pf[2];
    d = pf[3];
}

D3DXINLINE 
D3DXPLANE::D3DXPLANE( float fa, float fb, float fc, float fd )
{
    a = fa;
    b = fb;
    c = fc;
    d = fd;
}


// casting
D3DXINLINE 
D3DXPLANE::operator float* ()
{
    return (float *) &a;
}

D3DXINLINE 
D3DXPLANE::operator const float* () const
{
    return (const float *) &a;
}


// unary operators
D3DXINLINE D3DXPLANE 
D3DXPLANE::operator + () const
{
    return *this;
}

D3DXINLINE D3DXPLANE 
D3DXPLANE::operator - () const
{
    return D3DXPLANE(-a, -b, -c, -d);
}


// binary operators
D3DXINLINE BOOL 
D3DXPLANE::operator == ( const D3DXPLANE& p ) const
{
    return a == p.a && b == p.b && c == p.c && d == p.d;
}

D3DXINLINE BOOL 
D3DXPLANE::operator != ( const D3DXPLANE& p ) const
{
    return a != p.a || b != p.b || c != p.c || d != p.d;
}




//--------------------------
// Color
//--------------------------

D3DXINLINE 
D3DXCOLOR::D3DXCOLOR( DWORD dw )
{
    const float f = 1.0f / 255.0f;
    r = f * (float) (unsigned char) (dw >> 16);
    g = f * (float) (unsigned char) (dw >>  8);
    b = f * (float) (unsigned char) (dw >>  0);
    a = f * (float) (unsigned char) (dw >> 24);
}

D3DXINLINE 
D3DXCOLOR::D3DXCOLOR( const float* pf )
{
#ifdef D3DX_DEBUG
    if(!pf)
        return;
#endif

    r = pf[0];
    g = pf[1];
    b = pf[2];
    a = pf[3];
}

D3DXINLINE 
D3DXCOLOR::D3DXCOLOR( const D3DCOLORVALUE& c )
{
    r = c.r;
    g = c.g;
    b = c.b;
    a = c.a;
}

D3DXINLINE 
D3DXCOLOR::D3DXCOLOR( float fr, float fg, float fb, float fa )
{
    r = fr;
    g = fg;
    b = fb;
    a = fa;
}


// casting
D3DXINLINE 
D3DXCOLOR::operator DWORD () const
{
    DWORD dwR = r >= 1.0f ? 0xff : r <= 0.0f ? 0x00 : (DWORD) (r * 255.0f + 0.5f);
    DWORD dwG = g >= 1.0f ? 0xff : g <= 0.0f ? 0x00 : (DWORD) (g * 255.0f + 0.5f);
    DWORD dwB = b >= 1.0f ? 0xff : b <= 0.0f ? 0x00 : (DWORD) (b * 255.0f + 0.5f);
    DWORD dwA = a >= 1.0f ? 0xff : a <= 0.0f ? 0x00 : (DWORD) (a * 255.0f + 0.5f);

    return (dwA << 24) | (dwR << 16) | (dwG << 8) | dwB;
}


D3DXINLINE 
D3DXCOLOR::operator float * ()
{
    return (float *) &r;
}

D3DXINLINE 
D3DXCOLOR::operator const float * () const
{
    return (const float *) &r;
}


D3DXINLINE 
D3DXCOLOR::operator D3DCOLORVALUE * ()
{
    return (D3DCOLORVALUE *) &r;
}

D3DXINLINE 
D3DXCOLOR::operator const D3DCOLORVALUE * () const
{
    return (const D3DCOLORVALUE *) &r;
}


D3DXINLINE 
D3DXCOLOR::operator D3DCOLORVALUE& ()
{
    return *((D3DCOLORVALUE *) &r);
}

D3DXINLINE 
D3DXCOLOR::operator const D3DCOLORVALUE& () const
{
    return *((const D3DCOLORVALUE *) &r);
}


// assignment operators
D3DXINLINE D3DXCOLOR& 
D3DXCOLOR::operator += ( const D3DXCOLOR& c )
{
    r += c.r;
    g += c.g;
    b += c.b;
    a += c.a;
    return *this;
}

D3DXINLINE D3DXCOLOR& 
D3DXCOLOR::operator -= ( const D3DXCOLOR& c )
{
    r -= c.r;
    g -= c.g;
    b -= c.b;
    a -= c.a;
    return *this;
}

D3DXINLINE D3DXCOLOR& 
D3DXCOLOR::operator *= ( float f )
{
    r *= f;
    g *= f;
    b *= f;
    a *= f;
    return *this;
}

D3DXINLINE D3DXCOLOR& 
D3DXCOLOR::operator /= ( float f )
{
    float fInv = 1.0f / f;
    r *= fInv;
    g *= fInv;
    b *= fInv;
    a *= fInv;
    return *this;
}


// unary operators
D3DXINLINE D3DXCOLOR 
D3DXCOLOR::operator + () const
{
    return *this;
}

D3DXINLINE D3DXCOLOR 
D3DXCOLOR::operator - () const
{
    return D3DXCOLOR(-r, -g, -b, -a);
}


// binary operators
D3DXINLINE D3DXCOLOR 
D3DXCOLOR::operator + ( const D3DXCOLOR& c ) const
{
    return D3DXCOLOR(r + c.r, g + c.g, b + c.b, a + c.a);
}

D3DXINLINE D3DXCOLOR 
D3DXCOLOR::operator - ( const D3DXCOLOR& c ) const
{
    return D3DXCOLOR(r - c.r, g - c.g, b - c.b, a - c.a);
}

D3DXINLINE D3DXCOLOR 
D3DXCOLOR::operator * ( float f ) const
{
    return D3DXCOLOR(r * f, g * f, b * f, a * f);
}

D3DXINLINE D3DXCOLOR 
D3DXCOLOR::operator / ( float f ) const
{
    float fInv = 1.0f / f;
    return D3DXCOLOR(r * fInv, g * fInv, b * fInv, a * fInv);
}


D3DXINLINE D3DXCOLOR 
operator * (float f, const D3DXCOLOR& c )
{
    return D3DXCOLOR(f * c.r, f * c.g, f * c.b, f * c.a);
}


D3DXINLINE BOOL 
D3DXCOLOR::operator == ( const D3DXCOLOR& c ) const
{
    return r == c.r && g == c.g && b == c.b && a == c.a;
}

D3DXINLINE BOOL 
D3DXCOLOR::operator != ( const D3DXCOLOR& c ) const
{
    return r != c.r || g != c.g || b != c.b || a != c.a;
}


#endif //__cplusplus



//===========================================================================
//
// Inline functions
//
//===========================================================================


//--------------------------
// 2D Vector
//--------------------------

D3DXINLINE float D3DXVec2Length
    ( const D3DXVECTOR2 *pV )
{
#ifdef D3DX_DEBUG
    if(!pV)
        return 0.0f;
#endif

#ifdef __cplusplus
    return sqrtf(pV->x * pV->x + pV->y * pV->y);
#else
    return (float) sqrt(pV->x * pV->x + pV->y * pV->y);
#endif 
}

D3DXINLINE float D3DXVec2LengthSq
    ( const D3DXVECTOR2 *pV )
{
#ifdef D3DX_DEBUG
    if(!pV)
        return 0.0f;
#endif

    return pV->x * pV->x + pV->y * pV->y;
}

D3DXINLINE float D3DXVec2Dot
    ( const D3DXVECTOR2 *pV1, const D3DXVECTOR2 *pV2 )
{
#ifdef D3DX_DEBUG
    if(!pV1 || !pV2)
        return 0.0f;
#endif

    return pV1->x * pV2->x + pV1->y * pV2->y;
}

D3DXINLINE float D3DXVec2CCW
    ( const D3DXVECTOR2 *pV1, const D3DXVECTOR2 *pV2 )
{
#ifdef D3DX_DEBUG
    if(!pV1 || !pV2)
        return 0.0f;
#endif

    return pV1->x * pV2->y - pV1->y * pV2->x;
}

D3DXINLINE D3DXVECTOR2* D3DXVec2Add
    ( D3DXVECTOR2 *pOut, const D3DXVECTOR2 *pV1, const D3DXVECTOR2 *pV2 )
{
#ifdef D3DX_DEBUG
    if(!pOut || !pV1 || !pV2)
        return NULL;
#endif

    pOut->x = pV1->x + pV2->x;
    pOut->y = pV1->y + pV2->y;
    return pOut;
}

D3DXINLINE D3DXVECTOR2* D3DXVec2Subtract
    ( D3DXVECTOR2 *pOut, const D3DXVECTOR2 *pV1, const D3DXVECTOR2 *pV2 )
{
#ifdef D3DX_DEBUG
    if(!pOut || !pV1 || !pV2)
        return NULL;
#endif

    pOut->x = pV1->x - pV2->x;
    pOut->y = pV1->y - pV2->y;
    return pOut;
}

D3DXINLINE D3DXVECTOR2* D3DXVec2Minimize
    ( D3DXVECTOR2 *pOut, const D3DXVECTOR2 *pV1, const D3DXVECTOR2 *pV2 )
{
#ifdef D3DX_DEBUG
    if(!pOut || !pV1 || !pV2)
        return NULL;
#endif

    pOut->x = pV1->x < pV2->x ? pV1->x : pV2->x;
    pOut->y = pV1->y < pV2->y ? pV1->y : pV2->y;
    return pOut;
}

D3DXINLINE D3DXVECTOR2* D3DXVec2Maximize
    ( D3DXVECTOR2 *pOut, const D3DXVECTOR2 *pV1, const D3DXVECTOR2 *pV2 )
{
#ifdef D3DX_DEBUG
    if(!pOut || !pV1 || !pV2)
        return NULL;
#endif

    pOut->x = pV1->x > pV2->x ? pV1->x : pV2->x;
    pOut->y = pV1->y > pV2->y ? pV1->y : pV2->y;
    return pOut;
}

D3DXINLINE D3DXVECTOR2* D3DXVec2Scale
    ( D3DXVECTOR2 *pOut, const D3DXVECTOR2 *pV, float s )
{
#ifdef D3DX_DEBUG
    if(!pOut || !pV)
        return NULL;
#endif

    pOut->x = pV->x * s;
    pOut->y = pV->y * s;
    return pOut;
}

D3DXINLINE D3DXVECTOR2* D3DXVec2Lerp
    ( D3DXVECTOR2 *pOut, const D3DXVECTOR2 *pV1, const D3DXVECTOR2 *pV2,
      float s )
{
#ifdef D3DX_DEBUG
    if(!pOut || !pV1 || !pV2)
        return NULL;
#endif

    pOut->x = pV1->x + s * (pV2->x - pV1->x);
    pOut->y = pV1->y + s * (pV2->y - pV1->y);
    return pOut;
}


//--------------------------
// 3D Vector
//--------------------------

D3DXINLINE float D3DXVec3Length
    ( const D3DXVECTOR3 *pV )
{
#ifdef D3DX_DEBUG
    if(!pV)
        return 0.0f;
#endif

#ifdef __cplusplus
    return sqrtf(pV->x * pV->x + pV->y * pV->y + pV->z * pV->z);
#else
    return (float) sqrt(pV->x * pV->x + pV->y * pV->y + pV->z * pV->z);
#endif
}

D3DXINLINE float D3DXVec3LengthSq
    ( const D3DXVECTOR3 *pV )
{
#ifdef D3DX_DEBUG
    if(!pV)
        return 0.0f;
#endif

    return pV->x * pV->x + pV->y * pV->y + pV->z * pV->z;
}

D3DXINLINE float D3DXVec3Dot
    ( const D3DXVECTOR3 *pV1, const D3DXVECTOR3 *pV2 )
{
#ifdef D3DX_DEBUG
    if(!pV1 || !pV2)
        return 0.0f;
#endif

    return pV1->x * pV2->x + pV1->y * pV2->y + pV1->z * pV2->z;
}

D3DXINLINE D3DXVECTOR3* D3DXVec3Cross
    ( D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV1, const D3DXVECTOR3 *pV2 )
{
    D3DXVECTOR3 v;

#ifdef D3DX_DEBUG
    if(!pOut || !pV1 || !pV2)
        return NULL;
#endif

    v.x = pV1->y * pV2->z - pV1->z * pV2->y;
    v.y = pV1->z * pV2->x - pV1->x * pV2->z;
    v.z = pV1->x * pV2->y - pV1->y * pV2->x;

    *pOut = v;
    return pOut;
}

D3DXINLINE D3DXVECTOR3* D3DXVec3Add
    ( D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV1, const D3DXVECTOR3 *pV2 )
{
#ifdef D3DX_DEBUG
    if(!pOut || !pV1 || !pV2)
        return NULL;
#endif

    pOut->x = pV1->x + pV2->x;
    pOut->y = pV1->y + pV2->y;
    pOut->z = pV1->z + pV2->z;
    return pOut;
}

D3DXINLINE D3DXVECTOR3* D3DXVec3Subtract
    ( D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV1, const D3DXVECTOR3 *pV2 )
{
#ifdef D3DX_DEBUG
    if(!pOut || !pV1 || !pV2)
        return NULL;
#endif

    pOut->x = pV1->x - pV2->x;
    pOut->y = pV1->y - pV2->y;
    pOut->z = pV1->z - pV2->z;
    return pOut;
}

D3DXINLINE D3DXVECTOR3* D3DXVec3Minimize
    ( D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV1, const D3DXVECTOR3 *pV2 )
{
#ifdef D3DX_DEBUG
    if(!pOut || !pV1 || !pV2)
        return NULL;
#endif

    pOut->x = pV1->x < pV2->x ? pV1->x : pV2->x;
    pOut->y = pV1->y < pV2->y ? pV1->y : pV2->y;
    pOut->z = pV1->z < pV2->z ? pV1->z : pV2->z;
    return pOut;
}

D3DXINLINE D3DXVECTOR3* D3DXVec3Maximize
    ( D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV1, const D3DXVECTOR3 *pV2 )
{
#ifdef D3DX_DEBUG
    if(!pOut || !pV1 || !pV2)
        return NULL;
#endif

    pOut->x = pV1->x > pV2->x ? pV1->x : pV2->x;
    pOut->y = pV1->y > pV2->y ? pV1->y : pV2->y;
    pOut->z = pV1->z > pV2->z ? pV1->z : pV2->z;
    return pOut;
}

D3DXINLINE D3DXVECTOR3* D3DXVec3Scale
    ( D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV, float s)
{
#ifdef D3DX_DEBUG
    if(!pOut || !pV)
        return NULL;
#endif

    pOut->x = pV->x * s;
    pOut->y = pV->y * s;
    pOut->z = pV->z * s;
    return pOut;
}

D3DXINLINE D3DXVECTOR3* D3DXVec3Lerp
    ( D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV1, const D3DXVECTOR3 *pV2,
      float s )
{
#ifdef D3DX_DEBUG
    if(!pOut || !pV1 || !pV2)
        return NULL;
#endif

    pOut->x = pV1->x + s * (pV2->x - pV1->x);
    pOut->y = pV1->y + s * (pV2->y - pV1->y);
    pOut->z = pV1->z + s * (pV2->z - pV1->z);
    return pOut;
}


//--------------------------
// 4D Vector
//--------------------------

D3DXINLINE float D3DXVec4Length
    ( const D3DXVECTOR4 *pV )
{
#ifdef D3DX_DEBUG
    if(!pV)
        return 0.0f;
#endif

#ifdef __cplusplus
    return sqrtf(pV->x * pV->x + pV->y * pV->y + pV->z * pV->z + pV->w * pV->w);
#else
    return (float) sqrt(pV->x * pV->x + pV->y * pV->y + pV->z * pV->z + pV->w * pV->w);
#endif
}

D3DXINLINE float D3DXVec4LengthSq
    ( const D3DXVECTOR4 *pV )
{
#ifdef D3DX_DEBUG
    if(!pV)
        return 0.0f;
#endif

    return pV->x * pV->x + pV->y * pV->y + pV->z * pV->z + pV->w * pV->w;
}

D3DXINLINE float D3DXVec4Dot
    ( const D3DXVECTOR4 *pV1, const D3DXVECTOR4 *pV2 )
{
#ifdef D3DX_DEBUG
    if(!pV1 || !pV2)
        return 0.0f;
#endif

    return pV1->x * pV2->x + pV1->y * pV2->y + pV1->z * pV2->z + pV1->w * pV2->w;
}

D3DXINLINE D3DXVECTOR4* D3DXVec4Add
    ( D3DXVECTOR4 *pOut, const D3DXVECTOR4 *pV1, const D3DXVECTOR4 *pV2)
{
#ifdef D3DX_DEBUG
    if(!pOut || !pV1 || !pV2)
        return NULL;
#endif

    pOut->x = pV1->x + pV2->x;
    pOut->y = pV1->y + pV2->y;
    pOut->z = pV1->z + pV2->z;
    pOut->w = pV1->w + pV2->w;
    return pOut;
}

D3DXINLINE D3DXVECTOR4* D3DXVec4Subtract
    ( D3DXVECTOR4 *pOut, const D3DXVECTOR4 *pV1, const D3DXVECTOR4 *pV2)
{
#ifdef D3DX_DEBUG
    if(!pOut || !pV1 || !pV2)
        return NULL;
#endif

    pOut->x = pV1->x - pV2->x;
    pOut->y = pV1->y - pV2->y;
    pOut->z = pV1->z - pV2->z;
    pOut->w = pV1->w - pV2->w;
    return pOut;
}

D3DXINLINE D3DXVECTOR4* D3DXVec4Minimize
    ( D3DXVECTOR4 *pOut, const D3DXVECTOR4 *pV1, const D3DXVECTOR4 *pV2)
{
#ifdef D3DX_DEBUG
    if(!pOut || !pV1 || !pV2)
        return NULL;
#endif

    pOut->x = pV1->x < pV2->x ? pV1->x : pV2->x;
    pOut->y = pV1->y < pV2->y ? pV1->y : pV2->y;
    pOut->z = pV1->z < pV2->z ? pV1->z : pV2->z;
    pOut->w = pV1->w < pV2->w ? pV1->w : pV2->w;
    return pOut;
}

D3DXINLINE D3DXVECTOR4* D3DXVec4Maximize
    ( D3DXVECTOR4 *pOut, const D3DXVECTOR4 *pV1, const D3DXVECTOR4 *pV2)
{
#ifdef D3DX_DEBUG
    if(!pOut || !pV1 || !pV2)
        return NULL;
#endif

    pOut->x = pV1->x > pV2->x ? pV1->x : pV2->x;
    pOut->y = pV1->y > pV2->y ? pV1->y : pV2->y;
    pOut->z = pV1->z > pV2->z ? pV1->z : pV2->z;
    pOut->w = pV1->w > pV2->w ? pV1->w : pV2->w;
    return pOut;
}

D3DXINLINE D3DXVECTOR4* D3DXVec4Scale
    ( D3DXVECTOR4 *pOut, const D3DXVECTOR4 *pV, float s)
{
#ifdef D3DX_DEBUG
    if(!pOut || !pV)
        return NULL;
#endif

    pOut->x = pV->x * s;
    pOut->y = pV->y * s;
    pOut->z = pV->z * s;
    pOut->w = pV->w * s;
    return pOut;
}

D3DXINLINE D3DXVECTOR4* D3DXVec4Lerp
    ( D3DXVECTOR4 *pOut, const D3DXVECTOR4 *pV1, const D3DXVECTOR4 *pV2,
      float s )
{
#ifdef D3DX_DEBUG
    if(!pOut || !pV1 || !pV2)
        return NULL;
#endif

    pOut->x = pV1->x + s * (pV2->x - pV1->x);
    pOut->y = pV1->y + s * (pV2->y - pV1->y);
    pOut->z = pV1->z + s * (pV2->z - pV1->z);
    pOut->w = pV1->w + s * (pV2->w - pV1->w);
    return pOut;
}


//--------------------------
// 4D Matrix
//--------------------------

D3DXINLINE D3DXMATRIX* D3DXMatrixIdentity
    ( D3DXMATRIX *pOut )
{
#ifdef D3DX_DEBUG
    if(!pOut)
        return NULL;
#endif

    pOut->m[0][1] = pOut->m[0][2] = pOut->m[0][3] = 
    pOut->m[1][0] = pOut->m[1][2] = pOut->m[1][3] = 
    pOut->m[2][0] = pOut->m[2][1] = pOut->m[2][3] = 
    pOut->m[3][0] = pOut->m[3][1] = pOut->m[3][2] = 0.0f;

    pOut->m[0][0] = pOut->m[1][1] = pOut->m[2][2] = pOut->m[3][3] = 1.0f;
    return pOut;
}


D3DXINLINE BOOL D3DXMatrixIsIdentity
    ( const D3DXMATRIX *pM )
{
#ifdef D3DX_DEBUG
    if(!pM)
        return FALSE;
#endif

    return pM->m[0][0] == 1.0f && pM->m[0][1] == 0.0f && pM->m[0][2] == 0.0f && pM->m[0][3] == 0.0f &&
           pM->m[1][0] == 0.0f && pM->m[1][1] == 1.0f && pM->m[1][2] == 0.0f && pM->m[1][3] == 0.0f &&
           pM->m[2][0] == 0.0f && pM->m[2][1] == 0.0f && pM->m[2][2] == 1.0f && pM->m[2][3] == 0.0f &&
           pM->m[3][0] == 0.0f && pM->m[3][1] == 0.0f && pM->m[3][2] == 0.0f && pM->m[3][3] == 1.0f;
}


//--------------------------
// Quaternion
//--------------------------

D3DXINLINE float D3DXQuaternionLength
    ( const D3DXQUATERNION *pQ )
{
#ifdef D3DX_DEBUG
    if(!pQ)
        return 0.0f;
#endif

#ifdef __cplusplus
    return sqrtf(pQ->x * pQ->x + pQ->y * pQ->y + pQ->z * pQ->z + pQ->w * pQ->w);
#else
    return (float) sqrt(pQ->x * pQ->x + pQ->y * pQ->y + pQ->z * pQ->z + pQ->w * pQ->w);
#endif
}

D3DXINLINE float D3DXQuaternionLengthSq
    ( const D3DXQUATERNION *pQ )
{
#ifdef D3DX_DEBUG
    if(!pQ)
        return 0.0f;
#endif

    return pQ->x * pQ->x + pQ->y * pQ->y + pQ->z * pQ->z + pQ->w * pQ->w;
}

D3DXINLINE float D3DXQuaternionDot
    ( const D3DXQUATERNION *pQ1, const D3DXQUATERNION *pQ2 )
{
#ifdef D3DX_DEBUG
    if(!pQ1 || !pQ2)
        return 0.0f;
#endif

    return pQ1->x * pQ2->x + pQ1->y * pQ2->y + pQ1->z * pQ2->z + pQ1->w * pQ2->w;
}


D3DXINLINE D3DXQUATERNION* D3DXQuaternionIdentity
    ( D3DXQUATERNION *pOut )
{
#ifdef D3DX_DEBUG
    if(!pOut)
        return NULL;
#endif

    pOut->x = pOut->y = pOut->z = 0.0f;
    pOut->w = 1.0f;
    return pOut;
}

D3DXINLINE BOOL D3DXQuaternionIsIdentity
    ( const D3DXQUATERNION *pQ )
{
#ifdef D3DX_DEBUG
    if(!pQ)
        return FALSE;
#endif

    return pQ->x == 0.0f && pQ->y == 0.0f && pQ->z == 0.0f && pQ->w == 1.0f;
}


D3DXINLINE D3DXQUATERNION* D3DXQuaternionConjugate
    ( D3DXQUATERNION *pOut, const D3DXQUATERNION *pQ )
{
#ifdef D3DX_DEBUG
    if(!pOut || !pQ)
        return NULL;
#endif

    pOut->x = -pQ->x;
    pOut->y = -pQ->y;
    pOut->z = -pQ->z;
    pOut->w =  pQ->w;
    return pOut;
}


//--------------------------
// Plane
//--------------------------

D3DXINLINE float D3DXPlaneDot
    ( const D3DXPLANE *pP, const D3DXVECTOR4 *pV)
{
#ifdef D3DX_DEBUG
    if(!pP || !pV)
        return 0.0f;
#endif

    return pP->a * pV->x + pP->b * pV->y + pP->c * pV->z + pP->d * pV->w;
}

D3DXINLINE float D3DXPlaneDotCoord
    ( const D3DXPLANE *pP, const D3DXVECTOR3 *pV)
{
#ifdef D3DX_DEBUG
    if(!pP || !pV)
        return 0.0f;
#endif

    return pP->a * pV->x + pP->b * pV->y + pP->c * pV->z + pP->d;
}

D3DXINLINE float D3DXPlaneDotNormal
    ( const D3DXPLANE *pP, const D3DXVECTOR3 *pV)
{
#ifdef D3DX_DEBUG
    if(!pP || !pV)
        return 0.0f;
#endif

    return pP->a * pV->x + pP->b * pV->y + pP->c * pV->z;
}


//--------------------------
// Color
//--------------------------

D3DXINLINE D3DXCOLOR* D3DXColorNegative
    (D3DXCOLOR *pOut, const D3DXCOLOR *pC)
{
#ifdef D3DX_DEBUG
    if(!pOut || !pC)
        return NULL;
#endif

    pOut->r = 1.0f - pC->r;
    pOut->g = 1.0f - pC->g;
    pOut->b = 1.0f - pC->b;
    pOut->a = pC->a;
    return pOut;
}

D3DXINLINE D3DXCOLOR* D3DXColorAdd        
    (D3DXCOLOR *pOut, const D3DXCOLOR *pC1, const D3DXCOLOR *pC2)
{
#ifdef D3DX_DEBUG
    if(!pOut || !pC1 || !pC2)
        return NULL;
#endif

    pOut->r = pC1->r + pC2->r;
    pOut->g = pC1->g + pC2->g;
    pOut->b = pC1->b + pC2->b;
    pOut->a = pC1->a + pC2->a;
    return pOut;
}

D3DXINLINE D3DXCOLOR* D3DXColorSubtract   
    (D3DXCOLOR *pOut, const D3DXCOLOR *pC1, const D3DXCOLOR *pC2)
{
#ifdef D3DX_DEBUG
    if(!pOut || !pC1 || !pC2)
        return NULL;
#endif

    pOut->r = pC1->r - pC2->r;
    pOut->g = pC1->g - pC2->g;
    pOut->b = pC1->b - pC2->b;
    pOut->a = pC1->a - pC2->a;
    return pOut;
}

D3DXINLINE D3DXCOLOR* D3DXColorScale      
    (D3DXCOLOR *pOut, const D3DXCOLOR *pC, float s)
{
#ifdef D3DX_DEBUG
    if(!pOut || !pC)
        return NULL;
#endif

    pOut->r = pC->r * s;
    pOut->g = pC->g * s;
    pOut->b = pC->b * s;
    pOut->a = pC->a * s;
    return pOut;
} 

D3DXINLINE D3DXCOLOR* D3DXColorModulate   
    (D3DXCOLOR *pOut, const D3DXCOLOR *pC1, const D3DXCOLOR *pC2)
{
#ifdef D3DX_DEBUG
    if(!pOut || !pC1 || !pC2)
        return NULL;
#endif

    pOut->r = pC1->r * pC2->r;
    pOut->g = pC1->g * pC2->g;
    pOut->b = pC1->b * pC2->b;
    pOut->a = pC1->a * pC2->a;
    return pOut;
}

D3DXINLINE D3DXCOLOR* D3DXColorLerp       
    (D3DXCOLOR *pOut, const D3DXCOLOR *pC1, const D3DXCOLOR *pC2, float s)
{
#ifdef D3DX_DEBUG
    if(!pOut || !pC1 || !pC2)
        return NULL;
#endif

    pOut->r = pC1->r + s * (pC2->r - pC1->r);
    pOut->g = pC1->g + s * (pC2->g - pC1->g);
    pOut->b = pC1->b + s * (pC2->b - pC1->b);
    pOut->a = pC1->a + s * (pC2->a - pC1->a);
    return pOut;
}


#endif // __D3DXMATH_INL__
