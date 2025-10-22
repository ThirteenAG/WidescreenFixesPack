///////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:       d3dxshapes.h
//  Content:    D3DX simple shapes
//
///////////////////////////////////////////////////////////////////////////

#ifndef __D3DXSHAPES_H__
#define __D3DXSHAPES_H__

#include <d3d.h>
#include <limits.h>
#include "d3dxerr.h"


typedef struct ID3DXSimpleShape *LPD3DXSIMPLESHAPE;

// {CFCD4602-EB7B-11d2-A440-00A0C90629A8}
DEFINE_GUID( IID_ID3DXSimpleShape, 
0xcfcd4602, 0xeb7b, 0x11d2, 0xa4, 0x40, 0x0, 0xa0, 0xc9, 0x6, 0x29, 0xa8 );


///////////////////////////////////////////////////////////////////////////
// Interfaces:
///////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------
// ID3DXSimpleShape interface: 
//-------------------------------------------------------------------------

DECLARE_INTERFACE_(ID3DXSimpleShape, IUnknown)
{
    // IUnknown methods 
    STDMETHOD(QueryInterface)(THIS_ REFIID  riid, LPVOID* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    // ID3DXSimpleShape methods
    STDMETHOD_(LPDIRECT3DVERTEXBUFFER7, GetVB)(THIS) PURE;
    STDMETHOD_(DWORD, GetIndices)(THIS_ LPWORD *ppIndices) PURE;
    STDMETHOD(Draw)(THIS) PURE;
};



///////////////////////////////////////////////////////////////////////////
// Functions:
///////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


//-------------------------------------------------------------------------
// D3DXCreatePolygon: Creates an 'n' sided polygon using the device
// ----------------  specified. It returns a vertex buffer that can be used
//                   for drawing or manipulation by the program later on.
//
// Params: 
//     [in]  LPDIRECT3DDEVICE7 pDevice: The device to create off. 
//     [in]  float sideSize: Length of a side.
//     [in]  DWORD numTexCoords:   The number of texture coordinates desired
//                                 in the vertex-buffer. (Default is 1)
//                                 D3DX_DEFAULT is a valid input.
//     [out] IDirect3DVertexBuffer7** ppVB: The output shape interface.
//-------------------------------------------------------------------------
HRESULT WINAPI 
    D3DXCreatePolygon(LPDIRECT3DDEVICE7  pDevice,
                      float              sideSize, 
                      DWORD              numSides, 
                      DWORD              numTexCoords, 
                      LPD3DXSIMPLESHAPE* ppShape );

//-------------------------------------------------------------------------
// D3DXCreateBox: Creates a box (cuboid) of given dimensions using the  
// ------------  device. It returns a vertex buffer that can
//               be used for drawing or manipulation by the program later on.
//
// Params: 
//     [in]  LPDIRECT3DDEVICE7 pDevice: The device to create off. 
//     [in]  float width: Width of the box (along x-axis)
//     [in]  float height: Height of the box (along y-axis)
//     [in]  float depth: Depth of the box (along z-axis)
//     [in]  DWORD numTexCoords: The number of texture coordinates desired
//                               in the vertex-buffer. Default is 1. 
//                               D3DX_DEFAULT is a valid input here.
//     [out] LPD3DXSIMPLESHAPE* ppShape: The output vertex-buffer.
//-------------------------------------------------------------------------
HRESULT WINAPI 
    D3DXCreateBox(LPDIRECT3DDEVICE7  pDevice, 
                  float              width,
                  float              height,
                  float              depth,
                  DWORD              numTexCoords, 
                  LPD3DXSIMPLESHAPE* ppShape );

//-------------------------------------------------------------------------
// D3DXCreateCylinder: Creates a cylinder of given dimensions using the  
// -----------------  device. It returns a vertex buffer that
//                    can be used for drawing or manipulation by the program
//                    later on.
//
// Params: 
//     [in]  LPDIRECT3DDEVICE7 pDevice: The device to create off. 
//     [in]  float baseRadius: Base-radius (default is 1.0f, shd be >= 0.0f)
//     [in]  float topRadius: Top-radius (default is 1.0f, shd be >= 0.0f)
//     [in]  float height: Height (default is 1.0f, shd be >= 0.0f)
//     [in]  DWORD numSlices: Number of slices about the main axis.
//                            (default is 8) D3DX_DEFAULT is a valid input.
//     [in]  DWORD numStacks: Number of stacks along the main axis. 
//                            (default is 8) D3DX_DEFAULT is a valid input.
//     [in]  DWORD numTexCoords: The number of texture coordinates desired
//                               in the vertex-buffer. Default is 1. 
//                               D3DX_DEFAULT is a valid input here.
//     [out] LPD3DXSIMPLESHAPE* ppShape: The output shape interface.
//-------------------------------------------------------------------------
HRESULT WINAPI 
    D3DXCreateCylinder(LPDIRECT3DDEVICE7  pDevice,
                       float              baseRadius, 
                       float              topRadius, 
                       float              height, 
                       DWORD              numSlices, 
                       DWORD              numStacks,   
                       DWORD              numTexCoords, 
                       LPD3DXSIMPLESHAPE* ppShape );


//-------------------------------------------------------------------------
// D3DXCreateTorus: Creates a torus of given dimensions using the  
// --------------  device specified. It returns a vertex buffer that can
//                 be used for drawing or manipulation by the program later
//                 on. It draws a doughnut, centered at (0, 0, 0) whose axis 
//                 is aligned with the z-axis. With the innerRadius used
//                 as the radius of the cross-section (minor-Radius) and 
//                 the outerRadius used as the radius of the central 'hole'. 
//
// Params: 
//     [in]  LPDIRECT3DDEVICE7 pDevice: The device to create off. 
//     [in]  float innerRadius: inner radius (default is 1.0f, shd be >= 0.0f)
//     [in]  float outerRadius: outer radius (default is 2.0f, shd be >= 0.0f)
//     [in]  DWORD numSides: Number of sides in the cross-section 
//                           (default is 8). D3DX_DEFAULT is a valid input.
//     [in]  DWORD numRings: Number of rings making up the torus 
//                           (default is 8) D3DX_DEFAULT is a valid input.
//     [in]  DWORD numTexCoords: The number of texture coordinates desired
//                                 in the vertex-buffer. Default is 1. 
//                                 D3DX_DEFAULT is a valid input here.
//     [out] LPD3DXSIMPLESHAPE* ppShape: The output shape interface.
//-------------------------------------------------------------------------
HRESULT WINAPI
    D3DXCreateTorus(LPDIRECT3DDEVICE7  pDevice,
                    float              innerRadius,
                    float              outerRadius, 
                    DWORD              numSides,
                    DWORD              numRings, 
                    DWORD              numTexCoords,  
                    LPD3DXSIMPLESHAPE* ppShape );

//-------------------------------------------------------------------------
// D3DXCreateTeapot: Creates a teapot using the device specified. 
// ----------------  It returns a vertex buffer that can be used for
//                   drawing or manipulation by the program later on.
//
// Params: 
//     [in]  LPDIRECT3DDEVICE7 pDevice: The device to create off. 
//     [in]  DWORD numTexCoords: The number of texture coordinates desired
//                               in the vertex-buffer. Default is 1. 
//                               D3DX_DEFAULT is a valid input here.
//     [out] LPD3DXSIMPLESHAPE* ppShape: The output shape interface.
//-------------------------------------------------------------------------
HRESULT WINAPI
    D3DXCreateTeapot(LPDIRECT3DDEVICE7  pDevice,
                     DWORD              numTexCoords, 
                     LPD3DXSIMPLESHAPE* ppShape);

//-------------------------------------------------------------------------
// D3DXCreateSphere: Creates a cylinder of given dimensions using the
// ----------------  device specified. 
//                   It returns a vertex buffer that can be used for
//                   drawing or manipulation by the program later on.
//
// Params: 
//     [in]  LPDIRECT3DDEVICE7 pDevice: The device to create off. 
//     [in]  float radius: radius (default is 1.0f, shd be >= 0.0f)
//     [in]  float height: Height (default is 1.0f, shd be >= 0.0f)
//     [in]  DWORD numSlices: Number of slices about the main axis
//                            (default is 8) D3DX_DEFAULT is a valid input.
//     [in]  DWORD numStacks: Number of stacks along the main axis
//                            (default is 8) D3DX_DEFAULT is a valid input.
//     [in]  DWORD numTexCoords: The number of texture coordinates desired
//                               in the vertex-buffer. Default is 1. 
//                               D3DX_DEFAULT is a valid input here.
//     [out] LPD3DXSIMPLESHAPE* ppShape: The output shape interface.
//-------------------------------------------------------------------------
HRESULT WINAPI
    D3DXCreateSphere(LPDIRECT3DDEVICE7  pDevice, 
                     float              radius, 
                     DWORD              numSlices, 
                     DWORD              numStacks,
                     DWORD              numTexCoords, 
                     LPD3DXSIMPLESHAPE* ppShape);

#ifdef __cplusplus
}
#endif //__cplusplus    
#endif //__D3DXSHAPES_H__
