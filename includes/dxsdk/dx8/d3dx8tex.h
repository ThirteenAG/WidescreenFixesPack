///////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:       d3dx8tex.h
//  Content:    D3DX texturing APIs
//
///////////////////////////////////////////////////////////////////////////

#include "d3dx8.h"

#ifndef __D3DX8TEX_H__
#define __D3DX8TEX_H__


//-------------------------------------------------------------------------
// D3DX_FILTER flags:
// ------------------
//
// A valid filter must contain one of these values:
//
//  D3DX_FILTER_NONE
//      No scaling or filtering will take place.  Pixels outside the bounds
//      of the source image are assumed to be transparent black.
//  D3DX_FILTER_POINT
//      Each destination pixel is computed by sampling the nearest pixel
//      from the source image.
//  D3DX_FILTER_LINEAR
//      Each destination pixel is computed by linearly interpolating between
//      the nearest pixels in the source image.  This filter works best 
//      when the scale on each axis is less than 2.
//  D3DX_FILTER_TRIANGLE
//      Every pixel in the source image contributes equally to the
//      destination image.  This is the slowest of all the filters.
//  D3DX_FILTER_BOX
//      Each pixel is computed by averaging a 2x2(x2) box pixels from 
//      the source image. Only works when the dimensions of the 
//      destination are half those of the source. (as with mip maps)
//
//
// And can be OR'd with any of these optional flags:
//
//  D3DX_FILTER_MIRROR_U
//      Indicates that pixels off the edge of the texture on the U-axis
//      should be mirrored, not wraped.
//  D3DX_FILTER_MIRROR_V
//      Indicates that pixels off the edge of the texture on the V-axis
//      should be mirrored, not wraped.
//  D3DX_FILTER_MIRROR_W
//      Indicates that pixels off the edge of the texture on the W-axis
//      should be mirrored, not wraped.
//  D3DX_FILTER_MIRROR
//      Same as specifying D3DX_FILTER_MIRROR_U, D3DX_FILTER_MIRROR_V,
//      and D3DX_FILTER_MIRROR_V
//  D3DX_FILTER_DITHER
//      Dithers the resulting image.
//
//-------------------------------------------------------------------------

#define D3DX_FILTER_NONE      (1 << 0)
#define D3DX_FILTER_POINT     (2 << 0)
#define D3DX_FILTER_LINEAR    (3 << 0)
#define D3DX_FILTER_TRIANGLE  (4 << 0)
#define D3DX_FILTER_BOX       (5 << 0)

#define D3DX_FILTER_MIRROR_U  (1 << 16)
#define D3DX_FILTER_MIRROR_V  (2 << 16)
#define D3DX_FILTER_MIRROR_W  (4 << 16)
#define D3DX_FILTER_MIRROR    (7 << 16)
#define D3DX_FILTER_DITHER    (8 << 16)


//-------------------------------------------------------------------------
// D3DXIMAGE_INFO:
// ---------------
// This structure is used to return a rough description of what the
// the original contents of an image file looked like.
// 
//  Width
//      Width of original image in pixels
//  Height
//      Height of original image in pixels
//  Depth
//      Depth of original image in pixels
//  MipLevels
//      Number of mip levels in original image
//  Format
//      D3D format which most closely describes the data in original image
//
//-------------------------------------------------------------------------


typedef struct _D3DXIMAGE_INFO
{
    UINT Width;
    UINT Height;
    UINT Depth;
    UINT MipLevels;
    D3DFORMAT Format;

} D3DXIMAGE_INFO;



#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////


//-------------------------------------------------------------------------
// D3DXLoadSurfaceFromFile/Resource:
// ---------------------------------
// Load surface from a file or resource
//
// Parameters:
//  pDestSurface
//      Destination surface, which will receive the image.
//  pDestPalette
//      Destination palette of 256 colors, or NULL
//  pDestRect
//      Destination rectangle, or NULL for entire surface
//  pSrcFile
//      File name of the source image.
//  pSrcModule
//      Module where resource is located, or NULL for module associated
//      with image the os used to create the current process.
//  pSrcResource
//      Resource name
//  pSrcData
//      Pointer to file in memory.
//  SrcDataSize
//      Size in bytes of file in memory.
//  pSrcRect
//      Source rectangle, or NULL for entire image
//  Filter
//      D3DX_FILTER flags controlling how the image is filtered.
//      Or D3DX_DEFAULT for D3DX_FILTER_TRIANGLE.
//  ColorKey
//      Color to replace with transparent black, or 0 to disable colorkey.
//      This is always a 32-bit ARGB color, independent of the source image
//      format.  Alpha is significant, and should usually be set to FF for 
//      opaque colorkeys.  (ex. Opaque black == 0xff000000)
//  pSrcInfo
//      Pointer to a D3DXIMAGE_INFO structure to be filled in with the 
//      description of the data in the source image file, or NULL.
//
//-------------------------------------------------------------------------
HRESULT WINAPI
    D3DXLoadSurfaceFromFileA(
        LPDIRECT3DSURFACE8    pDestSurface,
        CONST PALETTEENTRY*   pDestPalette,
        CONST RECT*           pDestRect,
        LPCSTR                pSrcFile,
        CONST RECT*           pSrcRect,
        DWORD                 Filter,
        D3DCOLOR              ColorKey,
        D3DXIMAGE_INFO*       pSrcInfo);

HRESULT WINAPI
    D3DXLoadSurfaceFromFileW(
        LPDIRECT3DSURFACE8    pDestSurface,
        CONST PALETTEENTRY*   pDestPalette,
        CONST RECT*           pDestRect,
        LPCWSTR               pSrcFile,
        CONST RECT*           pSrcRect,
        DWORD                 Filter,
        D3DCOLOR              ColorKey,
        D3DXIMAGE_INFO*       pSrcInfo);

#ifdef UNICODE
#define D3DXLoadSurfaceFromFile D3DXLoadSurfaceFromFileW
#else
#define D3DXLoadSurfaceFromFile D3DXLoadSurfaceFromFileA
#endif


HRESULT WINAPI
    D3DXLoadSurfaceFromResourceA(
        LPDIRECT3DSURFACE8    pDestSurface,
        CONST PALETTEENTRY*   pDestPalette,
        CONST RECT*           pDestRect,
        HMODULE               hSrcModule,
        LPCSTR                pSrcResource,
        CONST RECT*           pSrcRect,
        DWORD                 Filter,
        D3DCOLOR              ColorKey,
        D3DXIMAGE_INFO*       pSrcInfo);

HRESULT WINAPI
    D3DXLoadSurfaceFromResourceW(
        LPDIRECT3DSURFACE8    pDestSurface,
        CONST PALETTEENTRY*   pDestPalette,
        CONST RECT*           pDestRect,
        HMODULE               hSrcModule,
        LPCWSTR               pSrcResource,
        CONST RECT*           pSrcRect,
        DWORD                 Filter,
        D3DCOLOR              ColorKey,
        D3DXIMAGE_INFO*       pSrcInfo);


#ifdef UNICODE
#define D3DXLoadSurfaceFromResource D3DXLoadSurfaceFromResourceW
#else
#define D3DXLoadSurfaceFromResource D3DXLoadSurfaceFromResourceA
#endif


HRESULT WINAPI
    D3DXLoadSurfaceFromFileInMemory(
        LPDIRECT3DSURFACE8    pDestSurface,
        CONST PALETTEENTRY*   pDestPalette,
        CONST RECT*           pDestRect,
        LPCVOID               pSrcData,
        UINT                  SrcDataSize,
        CONST RECT*           pSrcRect,
        DWORD                 Filter,
        D3DCOLOR              ColorKey,
        D3DXIMAGE_INFO*       pSrcInfo);



//-------------------------------------------------------------------------
// D3DXLoadSurfaceFromSurface:
// --------------------------
// Load surface from another surface (with color conversion)
//
// Parameters:
//  pDestSurface
//      Destination surface, which will receive the image.
//  pDestPalette
//      Destination palette of 256 colors, or NULL
//  pDestRect
//      Destination rectangle, or NULL for entire surface
//  pSrcSurface
//      Source surface
//  pSrcPalette
//      Source palette of 256 colors, or NULL
//  pSrcRect
//      Source rectangle, or NULL for entire surface
//  Filter
//      D3DX_FILTER flags controlling how the image is filtered.
//      Or D3DX_DEFAULT for D3DX_FILTER_TRIANGLE.
//  ColorKey
//      Color to replace with transparent black, or 0 to disable colorkey.
//      This is always a 32-bit ARGB color, independent of the source image
//      format.  Alpha is significant, and should usually be set to FF for 
//      opaque colorkeys.  (ex. Opaque black == 0xff000000)
//
//-------------------------------------------------------------------------
HRESULT WINAPI
    D3DXLoadSurfaceFromSurface(
        LPDIRECT3DSURFACE8    pDestSurface,
        CONST PALETTEENTRY*   pDestPalette,
        CONST RECT*           pDestRect,
        LPDIRECT3DSURFACE8    pSrcSurface,
        CONST PALETTEENTRY*   pSrcPalette,
        CONST RECT*           pSrcRect,
        DWORD                 Filter,
        D3DCOLOR              ColorKey);


//-------------------------------------------------------------------------
// D3DXLoadSurfaceFromMemory:
// ------------------------
// Load surface from memory.
//
// Parameters:
//  pDestSurface
//      Destination surface, which will receive the image.
//  pDestPalette
//      Destination palette of 256 colors, or NULL
//  pDestRect
//      Destination rectangle, or NULL for entire surface
//  pSrcMemory
//      Pointer to the top-left corner of the source image in memory
//  SrcFormat
//      Pixel format of the source image.
//  SrcPitch
//      Pitch of source image, in bytes.  For DXT formats, this number
//      should represent the width of one row of cells, in bytes.
//  pSrcPalette
//      Source palette of 256 colors, or NULL
//  pSrcRect
//      Source rectangle.
//  Filter
//      D3DX_FILTER flags controlling how the image is filtered.
//      Or D3DX_DEFAULT for D3DX_FILTER_TRIANGLE.
//  ColorKey
//      Color to replace with transparent black, or 0 to disable colorkey.
//      This is always a 32-bit ARGB color, independent of the source image
//      format.  Alpha is significant, and should usually be set to FF for 
//      opaque colorkeys.  (ex. Opaque black == 0xff000000)
//
//-------------------------------------------------------------------------
HRESULT WINAPI
    D3DXLoadSurfaceFromMemory(
        LPDIRECT3DSURFACE8    pDestSurface,
        CONST PALETTEENTRY*   pDestPalette,
        CONST RECT*           pDestRect,
        LPCVOID               pSrcMemory,
        D3DFORMAT             SrcFormat,
        UINT                  SrcPitch,
        CONST PALETTEENTRY*   pSrcPalette,
        CONST RECT*           pSrcRect,
        DWORD                 Filter,
        D3DCOLOR              ColorKey);



///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////



//-------------------------------------------------------------------------
// D3DXLoadVolumeFromVolume:
// --------------------------
// Load volume from another volume (with color conversion)
//
// Parameters:
//  pDestVolume
//      Destination volume, which will receive the image.
//  pDestPalette
//      Destination palette of 256 colors, or NULL
//  pDestBox
//      Destination box, or NULL for entire volume
//  pSrcVolume
//      Source volume
//  pSrcPalette
//      Source palette of 256 colors, or NULL
//  pSrcBox
//      Source box, or NULL for entire volume
//  Filter
//      D3DX_FILTER flags controlling how the image is filtered.
//      Or D3DX_DEFAULT for D3DX_FILTER_TRIANGLE.
//  ColorKey
//      Color to replace with transparent black, or 0 to disable colorkey.
//      This is always a 32-bit ARGB color, independent of the source image
//      format.  Alpha is significant, and should usually be set to FF for 
//      opaque colorkeys.  (ex. Opaque black == 0xff000000)
//
//-------------------------------------------------------------------------
HRESULT WINAPI
    D3DXLoadVolumeFromVolume(
        LPDIRECT3DVOLUME8     pDestVolume,
        CONST PALETTEENTRY*   pDestPalette,
        CONST D3DBOX*         pDestBox,
        LPDIRECT3DVOLUME8     pSrcVolume,
        CONST PALETTEENTRY*   pSrcPalette,
        CONST D3DBOX*         pSrcBox,
        DWORD                 Filter,
        D3DCOLOR              ColorKey);


//-------------------------------------------------------------------------
// D3DXLoadVolumeFromMemory:
// ------------------------
// Load volume from memory.
//
// Parameters:
//  pDestVolume
//      Destination volume, which will receive the image.
//  pDestPalette
//      Destination palette of 256 colors, or NULL
//  pDestBox
//      Destination box, or NULL for entire volume
//  pSrcMemory
//      Pointer to the top-left corner of the source volume in memory
//  SrcFormat
//      Pixel format of the source volume.
//  SrcRowPitch
//      Pitch of source image, in bytes.  For DXT formats, this number
//      should represent the size of one row of cells, in bytes.
//  SrcSlicePitch
//      Pitch of source image, in bytes.  For DXT formats, this number
//      should represent the size of one slice of cells, in bytes.
//  pSrcPalette
//      Source palette of 256 colors, or NULL
//  pSrcBox
//      Source box.
//  Filter
//      D3DX_FILTER flags controlling how the image is filtered.
//      Or D3DX_DEFAULT for D3DX_FILTER_TRIANGLE.
//  ColorKey
//      Color to replace with transparent black, or 0 to disable colorkey.
//      This is always a 32-bit ARGB color, independent of the source image
//      format.  Alpha is significant, and should usually be set to FF for 
//      opaque colorkeys.  (ex. Opaque black == 0xff000000)
//
//-------------------------------------------------------------------------
HRESULT WINAPI
    D3DXLoadVolumeFromMemory(
        LPDIRECT3DVOLUME8     pDestVolume,
        CONST PALETTEENTRY*   pDestPalette,
        CONST D3DBOX*         pDestBox,
        LPCVOID               pSrcMemory,
        D3DFORMAT             SrcFormat,
        UINT                  SrcRowPitch,
        UINT                  SrcSlicePitch,
        CONST PALETTEENTRY*   pSrcPalette,
        CONST D3DBOX*         pSrcBox,
        DWORD                 Filter,
        D3DCOLOR              ColorKey);


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////




//-------------------------------------------------------------------------
// D3DXCheckTextureRequirements:
// -----------------------------
//
// Checks texture creation parameters.  If parameters are invalid, this
// function returns corrected parameters.
//
// Parameters:
//
//  pDevice
//      The D3D device to be used
//  pWidth
//      Desired width in pixels, or NULL.  Returns corrected width.
//  pHeight
//      Desired height in pixels, or NULL.  Returns corrected height.
//  pNumMipLevels
//      Number of desired mipmap levels, or NULL.  Returns corrected number.
//  Usage
//      Texture usage flags
//  pFormat
//      Desired pixel format, or NULL.  Returns corrected format.
//  Pool
//      Memory pool to be used to create texture
//
//-------------------------------------------------------------------------
HRESULT WINAPI
    D3DXCheckTextureRequirements(
        LPDIRECT3DDEVICE8     pDevice,
        UINT*                 pWidth,
        UINT*                 pHeight,
        UINT*                 pNumMipLevels,
        DWORD                 Usage,
        D3DFORMAT*            pFormat,
        D3DPOOL               Pool);


//-------------------------------------------------------------------------
// D3DXCreateTexture:
// ------------------
// Create an empty texture
//
// Parameters:
//
//  pDevice
//      The D3D device with which the texture is going to be used.
//  Width
//      width in pixels; must be non-zero
//  Height
//      height in pixels; must be non-zero
//  MipLevels
//      number of mip levels desired; if zero or D3DX_DEFAULT, a complete
//      mipmap chain will be created.
//  Usage
//      Texture usage flags
//  Format
//      Pixel format.
//  Pool
//      Memory pool to be used to create texture
//  ppTexture
//      The texture object that will be created
//
//-------------------------------------------------------------------------
HRESULT WINAPI
    D3DXCreateTexture(
        LPDIRECT3DDEVICE8     pDevice,
        UINT                  Width,
        UINT                  Height,
        UINT                  MipLevels,
        DWORD                 Usage,
        D3DFORMAT             Format,
        D3DPOOL               Pool,
        LPDIRECT3DTEXTURE8*   ppTexture);


//-------------------------------------------------------------------------
// D3DXCreateTextureFromFile:
// --------------------------
// Create a texture object from a file.
//
// Parameters:
//
//  pDevice
//      The D3D device with which the texture is going to be used.
//  pSrcFile
//      File name.
//  hSrcModule
//      Module handle. if NULL, current module will be used.
//  pSrcResource
//      Resource name in module
//  pvSrcData
//      Pointer to file in memory.
//  SrcDataSize
//      Size in bytes of file in memory.
//  Width
//      Width in pixels; if zero or D3DX_DEFAULT, the width will be taken
//      from the file.
//  Height
//      Height in pixels; if zero of D3DX_DEFAULT, the height will be taken
//      from the file.
//  MipLevels
//      Number of mip levels;  if zero or D3DX_DEFAULT, a complete mipmap
//      chain will be created.
//  Usage
//      Texture usage flags
//  Format
//      Desired pixel format.  If D3DFMT_UNKNOWN, the format will be
//      taken from the file.
//  Pool
//      Memory pool to be used to create texture
//  Filter
//      D3DX_FILTER flags controlling how the image is filtered.
//      Or D3DX_DEFAULT for D3DX_FILTER_TRIANGLE.
//  MipFilter
//      D3DX_FILTER flags controlling how each miplevel is filtered.
//      Or D3DX_DEFAULT for D3DX_FILTER_BOX,
//  ColorKey
//      Color to replace with transparent black, or 0 to disable colorkey.
//      This is always a 32-bit ARGB color, independent of the source image
//      format.  Alpha is significant, and should usually be set to FF for 
//      opaque colorkeys.  (ex. Opaque black == 0xff000000)
//  pSrcInfo
//      Pointer to a D3DXIMAGE_INFO structure to be filled in with the 
//      description of the data in the source image file, or NULL.
//  pPalette
//      256 color palette to be filled in, or NULL
//  ppTexture
//      The texture object that will be created
//
//-------------------------------------------------------------------------
HRESULT WINAPI
    D3DXCreateTextureFromFileA(
        LPDIRECT3DDEVICE8     pDevice,
        LPCSTR                pSrcFile,
        LPDIRECT3DTEXTURE8*   ppTexture);

HRESULT WINAPI
    D3DXCreateTextureFromFileW(
        LPDIRECT3DDEVICE8     pDevice,
        LPCWSTR               pSrcFile,
        LPDIRECT3DTEXTURE8*   ppTexture);

#ifdef UNICODE
#define D3DXCreateTextureFromFile D3DXCreateTextureFromFileW
#else
#define D3DXCreateTextureFromFile D3DXCreateTextureFromFileA
#endif


HRESULT WINAPI
    D3DXCreateTextureFromResourceA(
        LPDIRECT3DDEVICE8     pDevice,
        HMODULE               hSrcModule,
        LPCSTR                pSrcResource,
        LPDIRECT3DTEXTURE8*   ppTexture);

HRESULT WINAPI
    D3DXCreateTextureFromResourceW(
        LPDIRECT3DDEVICE8     pDevice,
        HMODULE               hSrcModule,
        LPCWSTR               pSrcResource,
        LPDIRECT3DTEXTURE8*   ppTexture);

#ifdef UNICODE
#define D3DXCreateTextureFromResource D3DXCreateTextureFromResourceW
#else
#define D3DXCreateTextureFromResource D3DXCreateTextureFromResourceA
#endif


HRESULT WINAPI
    D3DXCreateTextureFromFileExA(
        LPDIRECT3DDEVICE8     pDevice,
        LPCSTR                pSrcFile,
        UINT                  Width,
        UINT                  Height,
        UINT                  MipLevels,
        DWORD                 Usage,
        D3DFORMAT             Format,
        D3DPOOL               Pool,
        DWORD                 Filter,
        DWORD                 MipFilter,
        D3DCOLOR              ColorKey,
        D3DXIMAGE_INFO*       pSrcInfo,
        PALETTEENTRY*         pPalette,
        LPDIRECT3DTEXTURE8*   ppTexture);

HRESULT WINAPI
    D3DXCreateTextureFromFileExW(
        LPDIRECT3DDEVICE8     pDevice,
        LPCWSTR               pSrcFile,
        UINT                  Width,
        UINT                  Height,
        UINT                  MipLevels,
        DWORD                 Usage,
        D3DFORMAT             Format,
        D3DPOOL               Pool,
        DWORD                 Filter,
        DWORD                 MipFilter,
        D3DCOLOR              ColorKey,
        D3DXIMAGE_INFO*       pSrcInfo,
        PALETTEENTRY*         pPalette,
        LPDIRECT3DTEXTURE8*   ppTexture);

#ifdef UNICODE
#define D3DXCreateTextureFromFileEx D3DXCreateTextureFromFileExW
#else
#define D3DXCreateTextureFromFileEx D3DXCreateTextureFromFileExA
#endif


HRESULT WINAPI
    D3DXCreateTextureFromResourceExA(
        LPDIRECT3DDEVICE8     pDevice,
        HMODULE               hSrcModule,
        LPCSTR                pSrcResource,
        UINT                  Width,
        UINT                  Height,
        UINT                  MipLevels,
        DWORD                 Usage,
        D3DFORMAT             Format,
        D3DPOOL               Pool,
        DWORD                 Filter,
        DWORD                 MipFilter,
        D3DCOLOR              ColorKey,
        D3DXIMAGE_INFO*       pSrcInfo,
        PALETTEENTRY*         pPalette,
        LPDIRECT3DTEXTURE8*   ppTexture);

HRESULT WINAPI
    D3DXCreateTextureFromResourceExW(
        LPDIRECT3DDEVICE8     pDevice,
        HMODULE               hSrcModule,
        LPCWSTR               pSrcResource,
        UINT                  Width,
        UINT                  Height,
        UINT                  MipLevels,
        DWORD                 Usage,
        D3DFORMAT             Format,
        D3DPOOL               Pool,
        DWORD                 Filter,
        DWORD                 MipFilter,
        D3DCOLOR              ColorKey,
        D3DXIMAGE_INFO*       pSrcInfo,
        PALETTEENTRY*         pPalette,
        LPDIRECT3DTEXTURE8*   ppTexture);

#ifdef UNICODE
#define D3DXCreateTextureFromResourceEx D3DXCreateTextureFromResourceExW
#else
#define D3DXCreateTextureFromResourceEx D3DXCreateTextureFromResourceExA
#endif


HRESULT WINAPI
    D3DXCreateTextureFromFileInMemory(
        LPDIRECT3DDEVICE8     pDevice,
        LPCVOID               pSrcData,
        UINT                  SrcDataSize,
        LPDIRECT3DTEXTURE8*   ppTexture);


HRESULT WINAPI
    D3DXCreateTextureFromFileInMemoryEx(
        LPDIRECT3DDEVICE8     pDevice,
        LPCVOID               pSrcData,
        UINT                  SrcDataSize,
        UINT                  Width,
        UINT                  Height,
        UINT                  MipLevels,
        DWORD                 Usage,
        D3DFORMAT             Format,
        D3DPOOL               Pool,
        DWORD                 Filter,
        DWORD                 MipFilter,
        D3DCOLOR              ColorKey,
        D3DXIMAGE_INFO*       pSrcInfo,
        PALETTEENTRY*         pPalette,
        LPDIRECT3DTEXTURE8*   ppTexture);



//-------------------------------------------------------------------------
// D3DXFilterTexture:
// ------------------
// Filters mipmaps levels of a texture.
//
// Parameters:
//  pTexture
//      The texture object to be filtered
//  pPalette
//      256 color palette to be used, or NULL for non-palettized formats
//  SrcLevel
//      The level whose image is used to generate the subsequent levels. 
//  Filter
//      D3DX_FILTER flags controlling how each miplevel is filtered.
//      Or D3DX_DEFAULT for D3DX_FILTER_BOX,
//
//-------------------------------------------------------------------------
HRESULT WINAPI
    D3DXFilterTexture(
        LPDIRECT3DTEXTURE8    pTexture,
        CONST PALETTEENTRY*   pPalette,
        UINT                  SrcLevel,
        DWORD                 Filter);





///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////





//-------------------------------------------------------------------------
// D3DXCheckCubeTextureRequirements:
// ---------------------------------
//
// Checks cube texture creation parameters.  If parameters are invalid, 
// this function returns corrected parameters.
//
// Parameters:
//
//  pDevice
//      The D3D device to be used
//  pSize
//      Desired width and height in pixels, or NULL.  Returns corrected size.
//  pNumMipLevels
//      Number of desired mipmap levels, or NULL.  Returns corrected number.
//  Usage
//      Texture usage flags
//  pFormat
//      Desired pixel format, or NULL.  Returns corrected format.
//  Pool
//      Memory pool to be used to create texture
//
//-------------------------------------------------------------------------
HRESULT WINAPI
    D3DXCheckCubeTextureRequirements(
        LPDIRECT3DDEVICE8     pDevice,
        UINT*                 pSize,
        UINT*                 pNumMipLevels,
        DWORD                 Usage,
        D3DFORMAT*            pFormat,
        D3DPOOL               Pool);


//-------------------------------------------------------------------------
// D3DXCreateCubeTexture:
// ----------------------
// Create an empty cube texture
//
// Parameters:
//
//  pDevice
//      The D3D device with which the texture is going to be used.
//  Size
//      width and height in pixels; must be non-zero
//  MipLevels
//      number of mip levels desired; if zero or D3DX_DEFAULT, a complete
//      mipmap chain will be created.
//  Usage
//      Texture usage flags
//  Format
//      Pixel format.
//  Pool
//      Memory pool to be used to create texture
//  ppCubeTexture
//      The cube texture object that will be created
//
//-------------------------------------------------------------------------
HRESULT WINAPI
    D3DXCreateCubeTexture(
        LPDIRECT3DDEVICE8         pDevice,
        UINT                      Size,
        UINT                      MipLevels,
        DWORD                     Usage,
        D3DFORMAT                 Format,
        D3DPOOL                   Pool,
        LPDIRECT3DCUBETEXTURE8*   ppCubeTexture);


//-------------------------------------------------------------------------
// D3DXCreateCubeTextureFromFile:
// --------------------------
// Create a cube texture object from a file.
//
// Parameters:
//
//  pDevice
//      The D3D device with which the texture is going to be used.
//  pSrcFile
//      File name.
//  pvSrcData
//      Pointer to file in memory.
//  SrcDataSize
//      Size in bytes of file in memory.
//  Size
//      Width and height in pixels; if zero or D3DX_DEFAULT, the size 
//      will be taken from the file.
//  MipLevels
//      Number of mip levels;  if zero or D3DX_DEFAULT, a complete mipmap
//      chain will be created.
//  Format
//      Desired pixel format.  If D3DFMT_UNKNOWN, the format will be
//      taken from the file.
//  Filter
//      D3DX_FILTER flags controlling how the image is filtered.
//      Or D3DX_DEFAULT for D3DX_FILTER_TRIANGLE.
//  MipFilter
//      D3DX_FILTER flags controlling how each miplevel is filtered.
//      Or D3DX_DEFAULT for D3DX_FILTER_BOX,
//  ColorKey
//      Color to replace with transparent black, or 0 to disable colorkey.
//      This is always a 32-bit ARGB color, independent of the source image
//      format.  Alpha is significant, and should usually be set to FF for 
//      opaque colorkeys.  (ex. Opaque black == 0xff000000)
//  pSrcInfo
//      Pointer to a D3DXIMAGE_INFO structure to be filled in with the 
//      description of the data in the source image file, or NULL.
//  pPalette
//      256 color palette to be filled in, or NULL
//  ppCubeTexture
//      The cube texture object that will be created
//
//-------------------------------------------------------------------------
HRESULT WINAPI
    D3DXCreateCubeTextureFromFileA(
        LPDIRECT3DDEVICE8         pDevice,
        LPCSTR                    pSrcFile,
        LPDIRECT3DCUBETEXTURE8*   ppCubeTexture);

HRESULT WINAPI
    D3DXCreateCubeTextureFromFileW(
        LPDIRECT3DDEVICE8         pDevice,
        LPCWSTR                   pSrcFile,
        LPDIRECT3DCUBETEXTURE8*   ppCubeTexture);

#ifdef UNICODE
#define D3DXCreateCubeTextureFromFile D3DXCreateCubeTextureFromFileW
#else
#define D3DXCreateCubeTextureFromFile D3DXCreateCubeTextureFromFileA
#endif


HRESULT WINAPI
    D3DXCreateCubeTextureFromFileExA(
        LPDIRECT3DDEVICE8         pDevice,
        LPCSTR                    pSrcFile,
        UINT                      Size,
        UINT                      MipLevels,
        DWORD                     Usage,
        D3DFORMAT                 Format,
        D3DPOOL                   Pool,
        DWORD                     Filter,
        DWORD                     MipFilter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo,
        PALETTEENTRY*             pPalette,
        LPDIRECT3DCUBETEXTURE8*   ppCubeTexture);

HRESULT WINAPI
    D3DXCreateCubeTextureFromFileExW(
        LPDIRECT3DDEVICE8         pDevice,
        LPCWSTR                   pSrcFile,
        UINT                      Size,
        UINT                      MipLevels,
        DWORD                     Usage,
        D3DFORMAT                 Format,
        D3DPOOL                   Pool,
        DWORD                     Filter,
        DWORD                     MipFilter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo,
        PALETTEENTRY*             pPalette,
        LPDIRECT3DCUBETEXTURE8*   ppCubeTexture);

#ifdef UNICODE
#define D3DXCreateCubeTextureFromFileEx D3DXCreateCubeTextureFromFileExW
#else
#define D3DXCreateCubeTextureFromFileEx D3DXCreateCubeTextureFromFileExA
#endif


HRESULT WINAPI
    D3DXCreateCubeTextureFromFileInMemory(
        LPDIRECT3DDEVICE8         pDevice,
        LPCVOID                   pSrcData,
        UINT                      SrcDataSize,
        LPDIRECT3DCUBETEXTURE8*   ppCubeTexture);


HRESULT WINAPI
    D3DXCreateCubeTextureFromFileInMemoryEx(
        LPDIRECT3DDEVICE8         pDevice,
        LPCVOID                   pSrcData,
        UINT                      SrcDataSize,
        UINT                      Size,
        UINT                      MipLevels,
        DWORD                     Usage,
        D3DFORMAT                 Format,
        D3DPOOL                   Pool,
        DWORD                     Filter,
        DWORD                     MipFilter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo,
        PALETTEENTRY*             pPalette,
        LPDIRECT3DCUBETEXTURE8*   ppCubeTexture);


//-------------------------------------------------------------------------
// D3DXFilterCubeTexture:
// ----------------------
// Filters mipmaps levels of a cube texture map.
//
// Parameters:
//  pCubeTexture
//      The cube texture object to be filtered
//  pPalette
//      256 color palette to be used, or NULL
//  SrcLevel
//      The level whose image is used to generate the subsequent levels. 
//  Filter
//      D3DX_FILTER flags controlling how each miplevel is filtered.
//      Or D3DX_DEFAULT for D3DX_FILTER_BOX,
//
//-------------------------------------------------------------------------
HRESULT WINAPI
    D3DXFilterCubeTexture(
        LPDIRECT3DCUBETEXTURE8    pCubeTexture,
        CONST PALETTEENTRY*       pPalette,
        UINT                      SrcLevel,
        DWORD                     Filter);




///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////




//-------------------------------------------------------------------------
// D3DXCheckVolumeTextureRequirements:
// -----------------------------------
//
// Checks volume texture creation parameters.  If parameters are invalid, 
// this function returns corrected parameters.
//
// Parameters:
//
//  pDevice
//      The D3D device to be used
//  pWidth
//      Desired width in pixels, or NULL.  Returns corrected size.
//  pHeight
//      Desired height in pixels, or NULL.  Returns corrected size.
//  pDepth
//      Desired depth in pixels, or NULL.  Returns corrected size.
//  pNumMipLevels
//      Number of desired mipmap levels, or NULL.  Returns corrected number.
//  pFormat
//      Desired pixel format, or NULL.  Returns corrected format.
//
//-------------------------------------------------------------------------
HRESULT WINAPI
    D3DXCheckVolumeTextureRequirements(
        LPDIRECT3DDEVICE8     pDevice,
        UINT*                 pWidth,
        UINT*                 pHeight,
        UINT*                 pDepth,
        UINT*                 pNumMipLevels,
        DWORD                 Usage,
        D3DFORMAT*            pFormat,
        D3DPOOL               Pool);


//-------------------------------------------------------------------------
// D3DXCreateVolumeTexture:
// ----------------------
// Create an empty volume texture
//
// Parameters:
//
//  pDevice
//      The D3D device with which the texture is going to be used.
//  Width
//      width in pixels; must be non-zero
//  Height
//      height in pixels; must be non-zero
//  Depth
//      depth in pixels; must be non-zero
//  MipLevels
//      number of mip levels desired; if zero or D3DX_DEFAULT, a complete
//      mipmap chain will be created.
//  Format
//      pixel format.
//  ppVolumeTexture
//      The volume texture object that will be created
//
//-------------------------------------------------------------------------
HRESULT WINAPI
    D3DXCreateVolumeTexture(
        LPDIRECT3DDEVICE8         pDevice,
        UINT                      Width,
        UINT                      Height,
        UINT                      Depth,
        UINT                      MipLevels,
        DWORD                     Usage,
        D3DFORMAT                 Format,
        D3DPOOL                   Pool,
        LPDIRECT3DVOLUMETEXTURE8* ppVolumeTexture);


//-------------------------------------------------------------------------
// D3DXFilterVolumeTexture:
// ------------------------
// Filters mipmaps levels of a volume texture map.
//
// Parameters:
//  pVolumeTexture
//      The volume texture object to be filtered
//  pPalette
//      256 color palette to be used, or NULL
//  SrcLevel
//      The level whose image is used to generate the subsequent levels. 
//  Filter
//      D3DX_FILTER flags controlling how each miplevel is filtered.
//      Or D3DX_DEFAULT for D3DX_FILTER_BOX,
//
//-------------------------------------------------------------------------
HRESULT WINAPI
    D3DXFilterVolumeTexture(
        LPDIRECT3DVOLUMETEXTURE8  pVolumeTexture,
        CONST PALETTEENTRY*       pPalette,
        UINT                      SrcLevel,
        DWORD                     Filter);



#ifdef __cplusplus
}
#endif //__cplusplus

#endif //__D3DX8TEX_H__
