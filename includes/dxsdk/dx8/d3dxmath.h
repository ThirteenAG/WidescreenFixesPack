//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 1998 Microsoft Corporation.  All Rights Reserved.
//
//  File:       d3dxmath.h
//  Content:    D3DX math types and functions
//
//////////////////////////////////////////////////////////////////////////////

#ifndef __D3DXMATH_H__
#define __D3DXMATH_H__

#include <d3d.h>
#include <math.h>
#include <limits.h>
#include "d3dxerr.h"

#ifndef D3DXINLINE
#ifdef __cplusplus
#define D3DXINLINE inline
#else
#define D3DXINLINE _inline
#endif
#endif

#pragma warning(disable:4201) // anonymous unions warning



typedef struct ID3DXMatrixStack *LPD3DXMATRIXSTACK;

// {E3357330-CC5E-11d2-A434-00A0C90629A8}
DEFINE_GUID( IID_ID3DXMatrixStack,
             0xe3357330, 0xcc5e, 0x11d2, 0xa4, 0x34, 0x0, 0xa0, 0xc9, 0x6, 0x29, 0xa8);


//===========================================================================
//
// General purpose utilities
//
//===========================================================================
#define D3DX_PI    ((float)  3.141592654f)
#define D3DX_1BYPI ((float)  0.318309886f)

#define D3DXToRadian( degree ) ((degree) * (D3DX_PI / 180.0f))
#define D3DXToDegree( radian ) ((radian) * (180.0f / D3DX_PI))



//===========================================================================
//
// Vectors
//
//===========================================================================

//--------------------------
// 2D Vector
//--------------------------
typedef struct D3DXVECTOR2
{
#ifdef __cplusplus
public:
    D3DXVECTOR2() {};
    D3DXVECTOR2( const float * );
    D3DXVECTOR2( float x, float y );

    // casting
    operator float* ();
    operator const float* () const;

    // assignment operators
    D3DXVECTOR2& operator += ( const D3DXVECTOR2& );
    D3DXVECTOR2& operator -= ( const D3DXVECTOR2& );
    D3DXVECTOR2& operator *= ( float );
    D3DXVECTOR2& operator /= ( float );

    // unary operators
    D3DXVECTOR2 operator + () const;
    D3DXVECTOR2 operator - () const;

    // binary operators
    D3DXVECTOR2 operator + ( const D3DXVECTOR2& ) const;
    D3DXVECTOR2 operator - ( const D3DXVECTOR2& ) const;
    D3DXVECTOR2 operator * ( float ) const;
    D3DXVECTOR2 operator / ( float ) const;

    friend D3DXVECTOR2 operator * ( float, const D3DXVECTOR2& );

    BOOL operator == ( const D3DXVECTOR2& ) const;
    BOOL operator != ( const D3DXVECTOR2& ) const;


public:
#endif //__cplusplus
    float x, y;
} D3DXVECTOR2, *LPD3DXVECTOR2;


//--------------------------
// 3D Vector
//--------------------------
typedef struct D3DXVECTOR3
{
#ifdef __cplusplus
public:
    D3DXVECTOR3() {};
    D3DXVECTOR3( const float * );
    D3DXVECTOR3( const D3DVECTOR& );
    D3DXVECTOR3( float x, float y, float z );

    // casting
    operator float* ();
    operator const float* () const;

    operator D3DVECTOR* ();
    operator const D3DVECTOR* () const;

    operator D3DVECTOR& ();
    operator const D3DVECTOR& () const;

    // assignment operators
    D3DXVECTOR3& operator += ( const D3DXVECTOR3& );
    D3DXVECTOR3& operator -= ( const D3DXVECTOR3& );
    D3DXVECTOR3& operator *= ( float );
    D3DXVECTOR3& operator /= ( float );

    // unary operators
    D3DXVECTOR3 operator + () const;
    D3DXVECTOR3 operator - () const;

    // binary operators
    D3DXVECTOR3 operator + ( const D3DXVECTOR3& ) const;
    D3DXVECTOR3 operator - ( const D3DXVECTOR3& ) const;
    D3DXVECTOR3 operator * ( float ) const;
    D3DXVECTOR3 operator / ( float ) const;

    friend D3DXVECTOR3 operator * ( float, const struct D3DXVECTOR3& );

    BOOL operator == ( const D3DXVECTOR3& ) const;
    BOOL operator != ( const D3DXVECTOR3& ) const;

public:
#endif //__cplusplus
    float x, y, z;
} D3DXVECTOR3, *LPD3DXVECTOR3;


//--------------------------
// 4D Vector
//--------------------------
typedef struct D3DXVECTOR4
{
#ifdef __cplusplus
public:
    D3DXVECTOR4() {};
    D3DXVECTOR4( const float* );
    D3DXVECTOR4( float x, float y, float z, float w );

    // casting
    operator float* ();
    operator const float* () const;

    // assignment operators
    D3DXVECTOR4& operator += ( const D3DXVECTOR4& );
    D3DXVECTOR4& operator -= ( const D3DXVECTOR4& );
    D3DXVECTOR4& operator *= ( float );
    D3DXVECTOR4& operator /= ( float );

    // unary operators
    D3DXVECTOR4 operator + () const;
    D3DXVECTOR4 operator - () const;

    // binary operators
    D3DXVECTOR4 operator + ( const D3DXVECTOR4& ) const;
    D3DXVECTOR4 operator - ( const D3DXVECTOR4& ) const;
    D3DXVECTOR4 operator * ( float ) const;
    D3DXVECTOR4 operator / ( float ) const;

    friend D3DXVECTOR4 operator * ( float, const D3DXVECTOR4& );

    BOOL operator == ( const D3DXVECTOR4& ) const;
    BOOL operator != ( const D3DXVECTOR4& ) const;

public:
#endif //__cplusplus
    float x, y, z, w;
} D3DXVECTOR4, *LPD3DXVECTOR4;


//===========================================================================
//
// Matrices
//
//===========================================================================
typedef struct D3DXMATRIX
{
#ifdef __cplusplus
public:
    D3DXMATRIX() {};
    D3DXMATRIX( const float * );
    D3DXMATRIX( const D3DMATRIX& );
    D3DXMATRIX( float m00, float m01, float m02, float m03,
                float m10, float m11, float m12, float m13,
                float m20, float m21, float m22, float m23,
                float m30, float m31, float m32, float m33 );


    // access grants
    float& operator () ( UINT iRow, UINT iCol );
    float  operator () ( UINT iRow, UINT iCol ) const;

    // casting operators
    operator float* ();
    operator const float* () const;

    operator D3DMATRIX* ();
    operator const D3DMATRIX* () const;

    operator D3DMATRIX& ();
    operator const D3DMATRIX& () const;

    // assignment operators
    D3DXMATRIX& operator *= ( const D3DXMATRIX& );
    D3DXMATRIX& operator += ( const D3DXMATRIX& );
    D3DXMATRIX& operator -= ( const D3DXMATRIX& );
    D3DXMATRIX& operator *= ( float );
    D3DXMATRIX& operator /= ( float );

    // unary operators
    D3DXMATRIX operator + () const;
    D3DXMATRIX operator - () const;

    // binary operators
    D3DXMATRIX operator * ( const D3DXMATRIX& ) const;
    D3DXMATRIX operator + ( const D3DXMATRIX& ) const;
    D3DXMATRIX operator - ( const D3DXMATRIX& ) const;
    D3DXMATRIX operator * ( float ) const;
    D3DXMATRIX operator / ( float ) const;

    friend D3DXMATRIX operator * ( float, const D3DXMATRIX& );

    BOOL operator == ( const D3DXMATRIX& ) const;
    BOOL operator != ( const D3DXMATRIX& ) const;


#endif //__cplusplus

    union
    {
        float m[4][4];
#ifdef __cplusplus
        struct
        {
            float m00, m01, m02, m03;
            float m10, m11, m12, m13;
            float m20, m21, m22, m23;
            float m30, m31, m32, m33;
        };
#endif //__cplusplus
    };
} D3DXMATRIX, *LPD3DXMATRIX;


//===========================================================================
//
//    Quaternions
//
//===========================================================================
typedef struct D3DXQUATERNION
{
#ifdef __cplusplus
public:
    D3DXQUATERNION() {}
    D3DXQUATERNION( const float * );
    D3DXQUATERNION( float x, float y, float z, float w );

    // casting
    operator float* ();
    operator const float* () const;

    // assignment operators
    D3DXQUATERNION& operator += ( const D3DXQUATERNION& );
    D3DXQUATERNION& operator -= ( const D3DXQUATERNION& );
    D3DXQUATERNION& operator *= ( const D3DXQUATERNION& );
    D3DXQUATERNION& operator *= ( float );
    D3DXQUATERNION& operator /= ( float );

    // unary operators
    D3DXQUATERNION  operator + () const;
    D3DXQUATERNION  operator - () const;

    // binary operators
    D3DXQUATERNION operator + ( const D3DXQUATERNION& ) const;
    D3DXQUATERNION operator - ( const D3DXQUATERNION& ) const;
    D3DXQUATERNION operator * ( const D3DXQUATERNION& ) const;
    D3DXQUATERNION operator * ( float ) const;
    D3DXQUATERNION operator / ( float ) const;

    friend D3DXQUATERNION operator * (float, const D3DXQUATERNION& );

    BOOL operator == ( const D3DXQUATERNION& ) const;
    BOOL operator != ( const D3DXQUATERNION& ) const;

#endif //__cplusplus
    float x, y, z, w;
} D3DXQUATERNION, *LPD3DXQUATERNION;


//===========================================================================
//
// Planes
//
//===========================================================================
typedef struct D3DXPLANE
{
#ifdef __cplusplus
public:
    D3DXPLANE() {}
    D3DXPLANE( const float* );
    D3DXPLANE( float a, float b, float c, float d );

    // casting
    operator float* ();
    operator const float* () const;

    // unary operators
    D3DXPLANE operator + () const;
    D3DXPLANE operator - () const;

    // binary operators
    BOOL operator == ( const D3DXPLANE& ) const;
    BOOL operator != ( const D3DXPLANE& ) const;

#endif //__cplusplus
    float a, b, c, d;
} D3DXPLANE, *LPD3DXPLANE;


//===========================================================================
//
// Colors
//
//===========================================================================

typedef struct D3DXCOLOR
{
#ifdef __cplusplus
public:
    D3DXCOLOR() {}
    D3DXCOLOR( DWORD argb );
    D3DXCOLOR( const float * );
    D3DXCOLOR( const D3DCOLORVALUE& );
    D3DXCOLOR( float r, float g, float b, float a );

    // casting
    operator DWORD () const;

    operator float* ();
    operator const float* () const;

    operator D3DCOLORVALUE* ();
    operator const D3DCOLORVALUE* () const;

    operator D3DCOLORVALUE& ();
    operator const D3DCOLORVALUE& () const;

    // assignment operators
    D3DXCOLOR& operator += ( const D3DXCOLOR& );
    D3DXCOLOR& operator -= ( const D3DXCOLOR& );
    D3DXCOLOR& operator *= ( float );
    D3DXCOLOR& operator /= ( float );

    // unary operators
    D3DXCOLOR operator + () const;
    D3DXCOLOR operator - () const;

    // binary operators
    D3DXCOLOR operator + ( const D3DXCOLOR& ) const;
    D3DXCOLOR operator - ( const D3DXCOLOR& ) const;
    D3DXCOLOR operator * ( float ) const;
    D3DXCOLOR operator / ( float ) const;

    friend D3DXCOLOR operator * (float, const D3DXCOLOR& );

    BOOL operator == ( const D3DXCOLOR& ) const;
    BOOL operator != ( const D3DXCOLOR& ) const;

#endif //__cplusplus
    FLOAT r, g, b, a;
} D3DXCOLOR, *LPD3DXCOLOR;



//===========================================================================
//
// D3DX math functions:
//
// NOTE:
//  * All these functions can take the same object as in and out parameters.
//
//  * Out parameters are typically also returned as return values, so that
//    the output of one function may be used as a parameter to another.
//
//===========================================================================

//--------------------------
// 2D Vector
//--------------------------

// inline

float D3DXVec2Length
    ( const D3DXVECTOR2 *pV );

float D3DXVec2LengthSq
    ( const D3DXVECTOR2 *pV );

float D3DXVec2Dot
    ( const D3DXVECTOR2 *pV1, const D3DXVECTOR2 *pV2 );

// Z component of ((x1,y1,0) cross (x2,y2,0))
float D3DXVec2CCW
    ( const D3DXVECTOR2 *pV1, const D3DXVECTOR2 *pV2 );

D3DXVECTOR2* D3DXVec2Add
    ( D3DXVECTOR2 *pOut, const D3DXVECTOR2 *pV1, const D3DXVECTOR2 *pV2 );

D3DXVECTOR2* D3DXVec2Subtract
    ( D3DXVECTOR2 *pOut, const D3DXVECTOR2 *pV1, const D3DXVECTOR2 *pV2 );

// Minimize each component.  x = min(x1, x2), y = min(y1, y2)
D3DXVECTOR2* D3DXVec2Minimize
    ( D3DXVECTOR2 *pOut, const D3DXVECTOR2 *pV1, const D3DXVECTOR2 *pV2 );

// Maximize each component.  x = max(x1, x2), y = max(y1, y2)
D3DXVECTOR2* D3DXVec2Maximize
    ( D3DXVECTOR2 *pOut, const D3DXVECTOR2 *pV1, const D3DXVECTOR2 *pV2 );

D3DXVECTOR2* D3DXVec2Scale
    ( D3DXVECTOR2 *pOut, const D3DXVECTOR2 *pV, float s );

// Linear interpolation. V1 + s(V2-V1)
D3DXVECTOR2* D3DXVec2Lerp
    ( D3DXVECTOR2 *pOut, const D3DXVECTOR2 *pV1, const D3DXVECTOR2 *pV2,
      float s );

// non-inline
#ifdef __cplusplus
extern "C" {
#endif

D3DXVECTOR2* WINAPI D3DXVec2Normalize
    ( D3DXVECTOR2 *pOut, const D3DXVECTOR2 *pV );

// Hermite interpolation between position V1, tangent T1 (when s == 0)
// and position V2, tangent T2 (when s == 1).
D3DXVECTOR2* WINAPI D3DXVec2Hermite
    ( D3DXVECTOR2 *pOut, const D3DXVECTOR2 *pV1, const D3DXVECTOR2 *pT1,
      const D3DXVECTOR2 *pV2, const D3DXVECTOR2 *pT2, float s );

// Barycentric coordinates.  V1 + f(V2-V1) + g(V3-V1)
D3DXVECTOR2* WINAPI D3DXVec2BaryCentric
    ( D3DXVECTOR2 *pOut, const D3DXVECTOR2 *pV1, const D3DXVECTOR2 *pV2,
      D3DXVECTOR2 *pV3, float f, float g);

// Transform (x, y, 0, 1) by matrix.
D3DXVECTOR4* WINAPI D3DXVec2Transform
    ( D3DXVECTOR4 *pOut, const D3DXVECTOR2 *pV, const D3DXMATRIX *pM );

// Transform (x, y, 0, 1) by matrix, project result back into w=1.
D3DXVECTOR2* WINAPI D3DXVec2TransformCoord
    ( D3DXVECTOR2 *pOut, const D3DXVECTOR2 *pV, const D3DXMATRIX *pM );

// Transform (x, y, 0, 0) by matrix.
D3DXVECTOR2* WINAPI D3DXVec2TransformNormal
    ( D3DXVECTOR2 *pOut, const D3DXVECTOR2 *pV, const D3DXMATRIX *pM );

#ifdef __cplusplus
}
#endif


//--------------------------
// 3D Vector
//--------------------------

// inline

float D3DXVec3Length
    ( const D3DXVECTOR3 *pV );

float D3DXVec3LengthSq
    ( const D3DXVECTOR3 *pV );

float D3DXVec3Dot
    ( const D3DXVECTOR3 *pV1, const D3DXVECTOR3 *pV2 );

D3DXVECTOR3* D3DXVec3Cross
    ( D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV1, const D3DXVECTOR3 *pV2 );

D3DXVECTOR3* D3DXVec3Add
    ( D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV1, const D3DXVECTOR3 *pV2 );

D3DXVECTOR3* D3DXVec3Subtract
    ( D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV1, const D3DXVECTOR3 *pV2 );

// Minimize each component.  x = min(x1, x2), y = min(y1, y2), ...
D3DXVECTOR3* D3DXVec3Minimize
    ( D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV1, const D3DXVECTOR3 *pV2 );

// Maximize each component.  x = max(x1, x2), y = max(y1, y2), ...
D3DXVECTOR3* D3DXVec3Maximize
    ( D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV1, const D3DXVECTOR3 *pV2 );

D3DXVECTOR3* D3DXVec3Scale
    ( D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV, float s);

// Linear interpolation. V1 + s(V2-V1)
D3DXVECTOR3* D3DXVec3Lerp
    ( D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV1, const D3DXVECTOR3 *pV2,
      float s );

// non-inline
#ifdef __cplusplus
extern "C" {
#endif

D3DXVECTOR3* WINAPI D3DXVec3Normalize
    ( D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV );

// Hermite interpolation between position V1, tangent T1 (when s == 0)
// and position V2, tangent T2 (when s == 1).
D3DXVECTOR3* WINAPI D3DXVec3Hermite
    ( D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV1, const D3DXVECTOR3 *pT1,
      const D3DXVECTOR3 *pV2, const D3DXVECTOR3 *pT2, float s );

// Barycentric coordinates.  V1 + f(V2-V1) + g(V3-V1)
D3DXVECTOR3* WINAPI D3DXVec3BaryCentric
    ( D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV1, const D3DXVECTOR3 *pV2,
      const D3DXVECTOR3 *pV3, float f, float g);

// Transform (x, y, z, 1) by matrix.
D3DXVECTOR4* WINAPI D3DXVec3Transform
    ( D3DXVECTOR4 *pOut, const D3DXVECTOR3 *pV, const D3DXMATRIX *pM );

// Transform (x, y, z, 1) by matrix, project result back into w=1.
D3DXVECTOR3* WINAPI D3DXVec3TransformCoord
    ( D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV, const D3DXMATRIX *pM );

// Transform (x, y, z, 0) by matrix.
D3DXVECTOR3* WINAPI D3DXVec3TransformNormal
    ( D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV, const D3DXMATRIX *pM );

#ifdef __cplusplus
}
#endif



//--------------------------
// 4D Vector
//--------------------------

// inline

float D3DXVec4Length
    ( const D3DXVECTOR4 *pV );

float D3DXVec4LengthSq
    ( const D3DXVECTOR4 *pV );

float D3DXVec4Dot
    ( const D3DXVECTOR4 *pV1, const D3DXVECTOR4 *pV2 );

D3DXVECTOR4* D3DXVec4Add
    ( D3DXVECTOR4 *pOut, const D3DXVECTOR4 *pV1, const D3DXVECTOR4 *pV2);

D3DXVECTOR4* D3DXVec4Subtract
    ( D3DXVECTOR4 *pOut, const D3DXVECTOR4 *pV1, const D3DXVECTOR4 *pV2);

// Minimize each component.  x = min(x1, x2), y = min(y1, y2), ...
D3DXVECTOR4* D3DXVec4Minimize
    ( D3DXVECTOR4 *pOut, const D3DXVECTOR4 *pV1, const D3DXVECTOR4 *pV2);

// Maximize each component.  x = max(x1, x2), y = max(y1, y2), ...
D3DXVECTOR4* D3DXVec4Maximize
    ( D3DXVECTOR4 *pOut, const D3DXVECTOR4 *pV1, const D3DXVECTOR4 *pV2);

D3DXVECTOR4* D3DXVec4Scale
    ( D3DXVECTOR4 *pOut, const D3DXVECTOR4 *pV, float s);

// Linear interpolation. V1 + s(V2-V1)
D3DXVECTOR4* D3DXVec4Lerp
    ( D3DXVECTOR4 *pOut, const D3DXVECTOR4 *pV1, const D3DXVECTOR4 *pV2,
      float s );

// non-inline
#ifdef __cplusplus
extern "C" {
#endif

// Cross-product in 4 dimensions.
D3DXVECTOR4* WINAPI D3DXVec4Cross
    ( D3DXVECTOR4 *pOut, const D3DXVECTOR4 *pV1, const D3DXVECTOR4 *pV2,
      const D3DXVECTOR4 *pV3);

D3DXVECTOR4* WINAPI D3DXVec4Normalize
    ( D3DXVECTOR4 *pOut, const D3DXVECTOR4 *pV );

// Hermite interpolation between position V1, tangent T1 (when s == 0)
// and position V2, tangent T2 (when s == 1).
D3DXVECTOR4* WINAPI D3DXVec4Hermite
    ( D3DXVECTOR4 *pOut, const D3DXVECTOR4 *pV1, const D3DXVECTOR4 *pT1,
      const D3DXVECTOR4 *pV2, const D3DXVECTOR4 *pT2, float s );

// Barycentric coordinates.  V1 + f(V2-V1) + g(V3-V1)
D3DXVECTOR4* WINAPI D3DXVec4BaryCentric
    ( D3DXVECTOR4 *pOut, const D3DXVECTOR4 *pV1, const D3DXVECTOR4 *pV2,
      const D3DXVECTOR4 *pV3, float f, float g);

// Transform vector by matrix.
D3DXVECTOR4* WINAPI D3DXVec4Transform
    ( D3DXVECTOR4 *pOut, const D3DXVECTOR4 *pV, const D3DXMATRIX *pM );

#ifdef __cplusplus
}
#endif


//--------------------------
// 4D Matrix
//--------------------------

// inline

D3DXMATRIX* D3DXMatrixIdentity
    ( D3DXMATRIX *pOut );

BOOL D3DXMatrixIsIdentity
    ( const D3DXMATRIX *pM );


// non-inline
#ifdef __cplusplus
extern "C" {
#endif

float WINAPI D3DXMatrixfDeterminant
    ( const D3DXMATRIX *pM );

// Matrix multiplication.  The result represents the transformation M2 
// followed by the transformation M1.  (Out = M1 * M2)
D3DXMATRIX* WINAPI D3DXMatrixMultiply
    ( D3DXMATRIX *pOut, const D3DXMATRIX *pM1, const D3DXMATRIX *pM2 );

D3DXMATRIX* WINAPI D3DXMatrixTranspose
    ( D3DXMATRIX *pOut, const D3DXMATRIX *pM );

// Calculate inverse of matrix.  Inversion my fail, in which case NULL will
// be returned.  The determinant of pM is also returned it pfDeterminant
// is non-NULL.
D3DXMATRIX* WINAPI D3DXMatrixInverse
    ( D3DXMATRIX *pOut, float *pfDeterminant, const D3DXMATRIX *pM );

// Build a matrix which scales by (sx, sy, sz)
D3DXMATRIX* WINAPI D3DXMatrixScaling
    ( D3DXMATRIX *pOut, float sx, float sy, float sz );

// Build a matrix which translates by (x, y, z)
D3DXMATRIX* WINAPI D3DXMatrixTranslation
    ( D3DXMATRIX *pOut, float x, float y, float z );

// Build a matrix which rotates around the X axis
D3DXMATRIX* WINAPI D3DXMatrixRotationX
    ( D3DXMATRIX *pOut, float angle );

// Build a matrix which rotates around the Y axis
D3DXMATRIX* WINAPI D3DXMatrixRotationY
    ( D3DXMATRIX *pOut, float angle );

// Build a matrix which rotates around the Z axis
D3DXMATRIX* WINAPI D3DXMatrixRotationZ
    ( D3DXMATRIX *pOut, float angle );

// Build a matrix which rotates around an arbitrary axis
D3DXMATRIX* WINAPI D3DXMatrixRotationAxis
    ( D3DXMATRIX *pOut, const D3DXVECTOR3 *pV, float angle );

// Build a matrix from a quaternion
D3DXMATRIX* WINAPI D3DXMatrixRotationQuaternion
    ( D3DXMATRIX *pOut, const D3DXQUATERNION *pQ);

// Yaw around the Y axis, a pitch around the X axis,
// and a roll around the Z axis.
D3DXMATRIX* WINAPI D3DXMatrixRotationYawPitchRoll
    ( D3DXMATRIX *pOut, float yaw, float pitch, float roll );


// Build transformation matrix.  NULL arguments are treated as identity.
// Mout = Msc-1 * Msr-1 * Ms * Msr * Msc * Mrc-1 * Mr * Mrc * Mt
D3DXMATRIX* WINAPI D3DXMatrixTransformation
    ( D3DXMATRIX *pOut, const D3DXVECTOR3 *pScalingCenter,
      const D3DXQUATERNION *pScalingRotation, const D3DXVECTOR3 *pScaling,
      const D3DXVECTOR3 *pRotationCenter, const D3DXQUATERNION *pRotation,
      const D3DXVECTOR3 *pTranslation);

// Build affine transformation matrix.  NULL arguments are treated as identity.
// Mout = Ms * Mrc-1 * Mr * Mrc * Mt
D3DXMATRIX* WINAPI D3DXMatrixAffineTransformation
    ( D3DXMATRIX *pOut, float Scaling, const D3DXVECTOR3 *pRotationCenter,
      const D3DXQUATERNION *pRotation, const D3DXVECTOR3 *pTranslation);

// Build a lookat matrix. (right-handed)
D3DXMATRIX* WINAPI D3DXMatrixLookAt
    ( D3DXMATRIX *pOut, const D3DXVECTOR3 *pEye, const D3DXVECTOR3 *pAt,
      const D3DXVECTOR3 *pUp );

// Build a lookat matrix. (left-handed)
D3DXMATRIX* WINAPI D3DXMatrixLookAtLH
    ( D3DXMATRIX *pOut, const D3DXVECTOR3 *pEye, const D3DXVECTOR3 *pAt,
      const D3DXVECTOR3 *pUp );

// Build a perspective projection matrix. (right-handed)
D3DXMATRIX* WINAPI D3DXMatrixPerspective
    ( D3DXMATRIX *pOut, float w, float h, float zn, float zf );

// Build a perspective projection matrix. (left-handed)
D3DXMATRIX* WINAPI D3DXMatrixPerspectiveLH
    ( D3DXMATRIX *pOut, float w, float h, float zn, float zf );

// Build a perspective projection matrix. (right-handed)
D3DXMATRIX* WINAPI D3DXMatrixPerspectiveFov
    ( D3DXMATRIX *pOut, float fovy, float aspect, float zn, float zf );

// Build a perspective projection matrix. (left-handed)
D3DXMATRIX* WINAPI D3DXMatrixPerspectiveFovLH
    ( D3DXMATRIX *pOut, float fovy, float aspect, float zn, float zf );

// Build a perspective projection matrix. (right-handed)
D3DXMATRIX* WINAPI D3DXMatrixPerspectiveOffCenter
    ( D3DXMATRIX *pOut, float l, float r, float b, float t, float zn,
      float zf );

// Build a perspective projection matrix. (left-handed)
D3DXMATRIX* WINAPI D3DXMatrixPerspectiveOffCenterLH
    ( D3DXMATRIX *pOut, float l, float r, float b, float t, float zn,
      float zf );

// Build an ortho projection matrix. (right-handed)
D3DXMATRIX* WINAPI D3DXMatrixOrtho
    ( D3DXMATRIX *pOut, float w, float h, float zn, float zf );

// Build an ortho projection matrix. (left-handed)
D3DXMATRIX* WINAPI D3DXMatrixOrthoLH
    ( D3DXMATRIX *pOut, float w, float h, float zn, float zf );

// Build an ortho projection matrix. (right-handed)
D3DXMATRIX* WINAPI D3DXMatrixOrthoOffCenter
    ( D3DXMATRIX *pOut, float l, float r, float b, float t, float zn,
      float zf );

// Build an ortho projection matrix. (left-handed)
D3DXMATRIX* WINAPI D3DXMatrixOrthoOffCenterLH
    ( D3DXMATRIX *pOut, float l, float r, float b, float t, float zn,
      float zf );

// Build a matrix which flattens geometry into a plane, as if casting
// a shadow from a light.
D3DXMATRIX* WINAPI D3DXMatrixShadow
    ( D3DXMATRIX *pOut, const D3DXVECTOR4 *pLight,
      const D3DXPLANE *pPlane );

// Build a matrix which reflects the coordinate system about a plane
D3DXMATRIX* WINAPI D3DXMatrixReflect
    ( D3DXMATRIX *pOut, const D3DXPLANE *pPlane );

#ifdef __cplusplus
}
#endif


//--------------------------
// Quaternion
//--------------------------

// inline

float D3DXQuaternionLength
    ( const D3DXQUATERNION *pQ );

// Length squared, or "norm"
float D3DXQuaternionLengthSq
    ( const D3DXQUATERNION *pQ );

float D3DXQuaternionDot
    ( const D3DXQUATERNION *pQ1, const D3DXQUATERNION *pQ2 );

// (0, 0, 0, 1)
D3DXQUATERNION* D3DXQuaternionIdentity
    ( D3DXQUATERNION *pOut );

BOOL D3DXQuaternionIsIdentity
    ( const D3DXQUATERNION *pQ );

// (-x, -y, -z, w)
D3DXQUATERNION* D3DXQuaternionConjugate
    ( D3DXQUATERNION *pOut, const D3DXQUATERNION *pQ );


// non-inline
#ifdef __cplusplus
extern "C" {
#endif

// Compute a quaternin's axis and angle of rotation. Expects unit quaternions.
void WINAPI D3DXQuaternionToAxisAngle
    ( const D3DXQUATERNION *pQ, D3DXVECTOR3 *pAxis, float *pAngle );

// Build a quaternion from a rotation matrix.
D3DXQUATERNION* WINAPI D3DXQuaternionRotationMatrix
    ( D3DXQUATERNION *pOut, const D3DXMATRIX *pM);

// Rotation about arbitrary axis.
D3DXQUATERNION* WINAPI D3DXQuaternionRotationAxis
    ( D3DXQUATERNION *pOut, const D3DXVECTOR3 *pV, float angle );

// Yaw around the Y axis, a pitch around the X axis,
// and a roll around the Z axis.
D3DXQUATERNION* WINAPI D3DXQuaternionRotationYawPitchRoll
    ( D3DXQUATERNION *pOut, float yaw, float pitch, float roll );

// Quaternion multiplication.  The result represents the rotation Q2 
// followed by the rotation Q1.  (Out = Q2 * Q1)
D3DXQUATERNION* WINAPI D3DXQuaternionMultiply
    ( D3DXQUATERNION *pOut, const D3DXQUATERNION *pQ1,
      const D3DXQUATERNION *pQ2 );

D3DXQUATERNION* WINAPI D3DXQuaternionNormalize
    ( D3DXQUATERNION *pOut, const D3DXQUATERNION *pQ );

// Conjugate and re-norm
D3DXQUATERNION* WINAPI D3DXQuaternionInverse
    ( D3DXQUATERNION *pOut, const D3DXQUATERNION *pQ );

// Expects unit quaternions.
// if q = (cos(theta), sin(theta) * v); ln(q) = (0, theta * v)
D3DXQUATERNION* WINAPI D3DXQuaternionLn
    ( D3DXQUATERNION *pOut, const D3DXQUATERNION *pQ );

// Expects pure quaternions. (w == 0)  w is ignored in calculation.
// if q = (0, theta * v); exp(q) = (cos(theta), sin(theta) * v)
D3DXQUATERNION* WINAPI D3DXQuaternionExp
    ( D3DXQUATERNION *pOut, const D3DXQUATERNION *pQ );

// Spherical linear interpolation between Q1 (s == 0) and Q2 (s == 1).
// Expects unit quaternions.
D3DXQUATERNION* WINAPI D3DXQuaternionSlerp
    ( D3DXQUATERNION *pOut, const D3DXQUATERNION *pQ1,
      const D3DXQUATERNION *pQ2, float t );

// Spherical quadrangle interpolation.
// Slerp(Slerp(Q1, Q4, t), Slerp(Q2, Q3, t), 2t(1-t))
D3DXQUATERNION* WINAPI D3DXQuaternionSquad
    ( D3DXQUATERNION *pOut, const D3DXQUATERNION *pQ1,
      const D3DXQUATERNION *pQ2, const D3DXQUATERNION *pQ3,
      const D3DXQUATERNION *pQ4, float t );

// Slerp(Slerp(Q1, Q2, f+g), Slerp(Q1, Q3, f+g), g/(f+g))
D3DXQUATERNION* WINAPI D3DXQuaternionBaryCentric
    ( D3DXQUATERNION *pOut, const D3DXQUATERNION *pQ1,
      const D3DXQUATERNION *pQ2, const D3DXQUATERNION *pQ3,
      float f, float g );

#ifdef __cplusplus
}
#endif


//--------------------------
// Plane
//--------------------------

// inline

// ax + by + cz + dw
float D3DXPlaneDot
    ( const D3DXPLANE *pP, const D3DXVECTOR4 *pV);

// ax + by + cz + d
float D3DXPlaneDotCoord
    ( const D3DXPLANE *pP, const D3DXVECTOR3 *pV);

// ax + by + cz
float D3DXPlaneDotNormal
    ( const D3DXPLANE *pP, const D3DXVECTOR3 *pV);

// non-inline
#ifdef __cplusplus
extern "C" {
#endif

// Normalize plane (so that |a,b,c| == 1)
D3DXPLANE* WINAPI D3DXPlaneNormalize
    ( D3DXPLANE *pOut, const D3DXPLANE *pP);

// Find the intersection between a plane and a line.  If the line is
// parallel to the plane, NULL is returned.
D3DXVECTOR3* WINAPI D3DXPlaneIntersectLine
    ( D3DXVECTOR3 *pOut, const D3DXPLANE *pP, const D3DXVECTOR3 *pV1,
      const D3DXVECTOR3 *pV2);

// Construct a plane from a point and a normal
D3DXPLANE* WINAPI D3DXPlaneFromPointNormal
    ( D3DXPLANE *pOut, const D3DXVECTOR3 *pPoint, const D3DXVECTOR3 *pNormal);

// Construct a plane from 3 points
D3DXPLANE* WINAPI D3DXPlaneFromPoints
    ( D3DXPLANE *pOut, const D3DXVECTOR3 *pV1, const D3DXVECTOR3 *pV2,
      const D3DXVECTOR3 *pV3);

// Transform a plane by a matrix.  The vector (a,b,c) must be normal.
// M must be an affine transform.
D3DXPLANE* WINAPI D3DXPlaneTransform
    ( D3DXPLANE *pOut, const D3DXPLANE *pP, const D3DXMATRIX *pM );

#ifdef __cplusplus
}
#endif


//--------------------------
// Color
//--------------------------

// inline

// (1-r, 1-g, 1-b, a)
D3DXCOLOR* D3DXColorNegative
    (D3DXCOLOR *pOut, const D3DXCOLOR *pC);

D3DXCOLOR* D3DXColorAdd
    (D3DXCOLOR *pOut, const D3DXCOLOR *pC1, const D3DXCOLOR *pC2);

D3DXCOLOR* D3DXColorSubtract
    (D3DXCOLOR *pOut, const D3DXCOLOR *pC1, const D3DXCOLOR *pC2);

D3DXCOLOR* D3DXColorScale
    (D3DXCOLOR *pOut, const D3DXCOLOR *pC, float s);

// (r1*r2, g1*g2, b1*b2, a1*a2)
D3DXCOLOR* D3DXColorModulate
    (D3DXCOLOR *pOut, const D3DXCOLOR *pC1, const D3DXCOLOR *pC2);

// Linear interpolation of r,g,b, and a. C1 + s(C2-C1)
D3DXCOLOR* D3DXColorLerp
    (D3DXCOLOR *pOut, const D3DXCOLOR *pC1, const D3DXCOLOR *pC2, float s);

// non-inline
#ifdef __cplusplus
extern "C" {
#endif

// Interpolate r,g,b between desaturated color and color.
// DesaturatedColor + s(Color - DesaturatedColor)
D3DXCOLOR* WINAPI D3DXColorAdjustSaturation
    (D3DXCOLOR *pOut, const D3DXCOLOR *pC, float s);

// Interpolate r,g,b between 50% grey and color.  Grey + s(Color - Grey)
D3DXCOLOR* WINAPI D3DXColorAdjustContrast
    (D3DXCOLOR *pOut, const D3DXCOLOR *pC, float c);

#ifdef __cplusplus
}
#endif






//===========================================================================
//
//    Matrix Stack
//
//===========================================================================

DECLARE_INTERFACE_(ID3DXMatrixStack, IUnknown)
{
    //
    // IUnknown methods
    //
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    //
    // ID3DXMatrixStack methods
    //

    // Pops the top of the stack, returns the current top
    // *after* popping the top.
    STDMETHOD(Pop)(THIS) PURE;

    // Pushes the stack by one, duplicating the current matrix.
    STDMETHOD(Push)(THIS) PURE;

    // Loads identity in the current matrix.
    STDMETHOD(LoadIdentity)(THIS) PURE;

    // Loads the given matrix into the current matrix
    STDMETHOD(LoadMatrix)(THIS_ const D3DXMATRIX* pM ) PURE;

    // Right-Multiplies the given matrix to the current matrix.
    // (transformation is about the current world origin)
    STDMETHOD(MultMatrix)(THIS_ const D3DXMATRIX* pM ) PURE;

    // Left-Multiplies the given matrix to the current matrix
    // (transformation is about the local origin of the object)
    STDMETHOD(MultMatrixLocal)(THIS_ const D3DXMATRIX* pM ) PURE;

    // Right multiply the current matrix with the computed rotation
    // matrix, counterclockwise about the given axis with the given angle.
    // (rotation is about the current world origin)
    STDMETHOD(RotateAxis)
        (THIS_ const D3DXVECTOR3* pV, float angle) PURE;

    // Left multiply the current matrix with the computed rotation
    // matrix, counterclockwise about the given axis with the given angle.
    // (rotation is about the local origin of the object)
    STDMETHOD(RotateAxisLocal)
        (THIS_ const D3DXVECTOR3* pV, float angle) PURE;

    // Right multiply the current matrix with the computed rotation
    // matrix. All angles are counterclockwise. (rotation is about the
    // current world origin)

    // The rotation is composed of a yaw around the Y axis, a pitch around
    // the X axis, and a roll around the Z axis.
    STDMETHOD(RotateYawPitchRoll)
        (THIS_ float yaw, float pitch, float roll) PURE;

    // Left multiply the current matrix with the computed rotation
    // matrix. All angles are counterclockwise. (rotation is about the
    // local origin of the object)

    // The rotation is composed of a yaw around the Y axis, a pitch around
    // the X axis, and a roll around the Z axis.
    STDMETHOD(RotateYawPitchRollLocal)
        (THIS_ float yaw, float pitch, float roll) PURE;

    // Right multiply the current matrix with the computed scale
    // matrix. (transformation is about the current world origin)
    STDMETHOD(Scale)(THIS_ float x, float y, float z) PURE;

    // Left multiply the current matrix with the computed scale
    // matrix. (transformation is about the local origin of the object)
    STDMETHOD(ScaleLocal)(THIS_ float x, float y, float z) PURE;

    // Right multiply the current matrix with the computed translation
    // matrix. (transformation is about the current world origin)
    STDMETHOD(Translate)(THIS_ float x, float y, float z ) PURE;

    // Left multiply the current matrix with the computed translation
    // matrix. (transformation is about the local origin of the object)
    STDMETHOD(TranslateLocal)(THIS_ float x, float y, float z) PURE;

    // Obtain the current matrix at the top of the stack
    STDMETHOD_(D3DXMATRIX*, GetTop)(THIS) PURE;
};

#ifdef __cplusplus
extern "C" {
#endif

HRESULT WINAPI D3DXCreateMatrixStack( DWORD flags, LPD3DXMATRIXSTACK *ppStack );

#ifdef __cplusplus
}
#endif

#include "d3dxmath.inl"

#pragma warning(default:4201)

#endif // __D3DXMATH_H__
