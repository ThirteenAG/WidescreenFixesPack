///////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:       d3dx8effect.h
//  Content:    D3DX effect types and functions
//
///////////////////////////////////////////////////////////////////////////

#include "d3dx8.h"

#ifndef __D3DX8EFFECT_H__
#define __D3DX8EFFECT_H__


typedef enum _D3DXPARAMETERTYPE
{
    D3DXPT_DWORD        = 0,
    D3DXPT_FLOAT        = 1,
    D3DXPT_VECTOR       = 2,
    D3DXPT_MATRIX       = 3,
    D3DXPT_TEXTURE      = 4,
    D3DXPT_VERTEXSHADER = 5,
    D3DXPT_PIXELSHADER  = 6,
    D3DXPT_CONSTANT     = 7,
    D3DXPT_FORCE_DWORD  = 0x7fffffff /* force 32-bit size enum */

} D3DXPARAMETERTYPE;


typedef struct _D3DXEFFECT_DESC
{
    UINT Parameters;
    UINT Techniques;
    DWORD Usage;

} D3DXEFFECT_DESC;


typedef struct _D3DXPARAMETER_DESC
{
    DWORD Name;
    D3DXPARAMETERTYPE Type;

} D3DXPARAMETER_DESC;


typedef struct _D3DXTECHNIQUE_DESC
{
    DWORD Name;
    UINT Passes;

} D3DXTECHNIQUE_DESC;


typedef struct _D3DXPASS_DESC
{
    DWORD Name;

} D3DXPASS_DESC;


typedef struct ID3DXEffect *LPD3DXEFFECT;
typedef struct ID3DXTechnique *LPD3DXTECHNIQUE;




//////////////////////////////////////////////////////////////////////////////
// ID3DXTechnique ////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

// {A00F378D-AF79-4917-907E-4D635EE63844}
DEFINE_GUID( IID_ID3DXTechnique, 
0xa00f378d, 0xaf79, 0x4917, 0x90, 0x7e, 0x4d, 0x63, 0x5e, 0xe6, 0x38, 0x44);


DECLARE_INTERFACE_(ID3DXTechnique, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // ID3DXTechnique
    STDMETHOD(GetDevice)(THIS_ LPDIRECT3DDEVICE8* ppDevice) PURE;
    STDMETHOD(GetDesc)(THIS_ D3DXTECHNIQUE_DESC* pDesc) PURE;
    STDMETHOD(GetPassDesc)(THIS_ UINT Index, D3DXPASS_DESC* pDesc) PURE;

    STDMETHOD_(BOOL, IsParameterUsed)(THIS_ DWORD dwName) PURE;

    STDMETHOD(Validate)(THIS) PURE;
    STDMETHOD(Begin)(THIS_ UINT *pPasses) PURE;
    STDMETHOD(Pass)(THIS_ UINT Index) PURE;
    STDMETHOD(End)(THIS) PURE;
};
 

//////////////////////////////////////////////////////////////////////////////
// ID3DXEffect ///////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


// {281BBDD4-AEDF-4907-8650-E79CDFD45165}
DEFINE_GUID( IID_ID3DXEffect, 
0x281bbdd4, 0xaedf, 0x4907, 0x86, 0x50, 0xe7, 0x9c, 0xdf, 0xd4, 0x51, 0x65);


DECLARE_INTERFACE_(ID3DXEffect, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // ID3DXEffect
    STDMETHOD(GetDevice)(THIS_ LPDIRECT3DDEVICE8* ppDevice) PURE;
    STDMETHOD(GetDesc)(THIS_ D3DXEFFECT_DESC* pDesc) PURE;

    STDMETHOD(GetParameterDesc)(THIS_ UINT Index, D3DXPARAMETER_DESC* pDesc) PURE;
    STDMETHOD(GetTechniqueDesc)(THIS_ UINT Index, D3DXTECHNIQUE_DESC* pDesc) PURE;

    STDMETHOD(SetDword)(THIS_ DWORD Name, DWORD dw) PURE;
    STDMETHOD(GetDword)(THIS_ DWORD Name, DWORD* pdw) PURE; 
    STDMETHOD(SetFloat)(THIS_ DWORD Name, FLOAT f) PURE;
    STDMETHOD(GetFloat)(THIS_ DWORD Name, FLOAT* pf) PURE;
    STDMETHOD(SetVector)(THIS_ DWORD Name, D3DXVECTOR4* pVector) PURE;
    STDMETHOD(GetVector)(THIS_ DWORD Name, D3DXVECTOR4* pVector) PURE;
    STDMETHOD(SetMatrix)(THIS_ DWORD Name, D3DXMATRIX* pMatrix) PURE;
    STDMETHOD(GetMatrix)(THIS_ DWORD Name, D3DXMATRIX* pMatrix) PURE;
    STDMETHOD(SetTexture)(THIS_ DWORD Name, LPDIRECT3DBASETEXTURE8 pTexture) PURE;
    STDMETHOD(GetTexture)(THIS_ DWORD Name, LPDIRECT3DBASETEXTURE8 *ppTexture) PURE;
    STDMETHOD(SetVertexShader)(THIS_ DWORD Name, DWORD Handle) PURE;
    STDMETHOD(GetVertexShader)(THIS_ DWORD Name, DWORD* pHandle) PURE;
    STDMETHOD(SetPixelShader)(THIS_ DWORD Name, DWORD Handle) PURE;
    STDMETHOD(GetPixelShader)(THIS_ DWORD Name, DWORD* pHandle) PURE;

    STDMETHOD(GetTechnique)(THIS_ UINT Index, LPD3DXTECHNIQUE* ppTechnique) PURE;
    STDMETHOD(CloneEffect)(THIS_ LPDIRECT3DDEVICE8 pDevice, DWORD Usage, LPD3DXEFFECT* ppEffect) PURE;
};



//////////////////////////////////////////////////////////////////////////////
// APIs //////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


//----------------------------------------------------------------------------
// D3DXCompileEffect:
// ------------------
// Compiles an ascii effect description into a binary form usable by
// D3DXCreateEffect.
//
// Parameters:
//  pSrcFile
//      Name of the file containing the ascii effect description
//  pSrcData
//      Pointer to ascii effect description
//  SrcDataSize
//      Size of the effect description in bytes
//  ppCompiledEffect
//      Returns a buffer containing compiled effect.
//  ppCompilationErrors
//      Returns a buffer containing any error messages which occurred during
//      compile.  Or NULL if you do not care about the error messages.
//
//----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXCompileEffectFromFileA(
        LPCSTR            pSrcFile,
        LPD3DXBUFFER*     ppCompiledEffect,
        LPD3DXBUFFER*     ppCompilationErrors);

HRESULT WINAPI
    D3DXCompileEffectFromFileW(
        LPCWSTR           pSrcFile,
        LPD3DXBUFFER*     ppCompiledEffect,
        LPD3DXBUFFER*     ppCompilationErrors);

#ifdef UNICODE
#define D3DXCompileEffectFromFile D3DXCompileEffectFromFileW
#else
#define D3DXCompileEffectFromFile D3DXCompileEffectFromFileA
#endif


HRESULT WINAPI
    D3DXCompileEffect(
        LPCVOID           pSrcData,
        UINT              SrcDataSize,
        LPD3DXBUFFER*     ppCompiledEffect,
        LPD3DXBUFFER*     ppCompilationErrors);



//----------------------------------------------------------------------------
// D3DXCreateEffect:
// -----------------
// Creates an effect object, given compiled binary effect data
//
// Parameters:
//  pDevice
//      Pointer to the device to be used.
//  pCompiledEffect
//      Pointer to compiled effect data
//  CompiledEffectSize
//      Size of compiled effect data in bytes
//  Usage
//      Allows the specification of D3DUSAGE_SOFTWAREPROCESSING 
//  ppEffect
//      Returns the created effect object
//----------------------------------------------------------------------------


HRESULT WINAPI
    D3DXCreateEffect(
        LPDIRECT3DDEVICE8 pDevice,
        LPCVOID           pCompiledEffect,
        UINT              CompiledEffectSize,
        DWORD             Usage,
        LPD3DXEFFECT*     ppEffect);



#ifdef __cplusplus
}
#endif //__cplusplus

#endif //__D3DX8EFFECT_H__
