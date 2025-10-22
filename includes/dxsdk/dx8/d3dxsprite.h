///////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:       d3dxsprite.h
//  Content:    D3DX sprite helper functions
//
//      These functions allow you to use sprites with D3DX. A "sprite" is
//      loosely defined as a 2D image that you want to transfer to the 
//      rendering target. The source image can be a texture created
//      with the help of the D3DX texture loader; though advanced users may
//      want to create their own. A helper function (PrepareDeviceForSprite)
//      is provided to make it easy to set up render states on a device. 
//      (Again, advanced users can use their own created devices.) 
//
//      There are two general techniques for sprites; the simpler one just
//      specifies a destination rectangle and a rotation anlge. A more 
//      powerful technique supports rendering to non-rectangular quads.
//
//      Both techniques support clipping, alpha, and rotation. More
//      details are below.
//
///////////////////////////////////////////////////////////////////////////

#ifndef __D3DXSPRITE_H__
#define __D3DXSPRITE_H__

#include <d3d.h>
#include <limits.h>
#include "d3dxerr.h"

#ifdef __cplusplus
extern "C" {
#endif


//-------------------------------------------------------------------------
// D3DXPrepareDeviceForSprite:
//
// Call this function to set up all the render states necessary for
// BltSprite/WarpSprite to work correctly. (Advanced users may opt to
// not call this function first; in which case Blt/WarpSprite functions
// will use whatever render/texture states were set up on the device when
// they are called.)
//
// Warning: This function modifies render states and may impact performance
// negatively on some 3D hardware if it is called too often per frame.
//
// Warning: If the render state changes (other than through calls to 
// BltSprite or WarpSprite), you will need to call this function again before 
// calling BltSprite or WarpSprite.
//
// Details: This function modifies the the rendering first texture stage and 
// it modifies some renderstates for the entire device. Here is the exact 
// list:
// 
//   SetTextureStageState(0, D3DTSS_COLORARG1,         D3DTA_TEXTURE);
//   SetTextureStageState(0, D3DTSS_COLOROP,           D3DTOP_SELECTARG1);
//   SetTextureStageState(0, D3DTSS_ALPHAARG1,         D3DTA_TEXTURE);
//   SetTextureStageState(0, D3DTSS_ALPHAARG2,         D3DTA_DIFFUSE);
//   SetTextureStageState(0, D3DTSS_ALPHAOP,           D3DTOP_MODULATE);
//   SetTextureStageState(0, D3DTSS_MINFILTER,         D3DTFN_LINEAR);
//   SetTextureStageState(0, D3DTSS_MAGFILTER,         D3DTFG_LINEAR);
// 
//   SetRenderState(D3DRENDERSTATE_SRCBLEND,           D3DBLEND_SRCALPHA);
//   SetRenderState(D3DRENDERSTATE_DESTBLEND,          D3DBLEND_INVSRCALPHA);
//   SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE,   TRUE);
//
//   Depending on the value of ZEnable parameter, this function will
//   will either call
//   SetRenderState(D3DRENDERSTATE_ZENABLE,            FALSE);
//   - or -
//   SetRenderState(D3DRENDERSTATE_ZENABLE,            TRUE);
//
// Parameters: 
//      pd3dDevice  - a pointer to the d3d device that you wish to prepare
//                    for use with D3DX Sprite Services
//      ZEnable     - a flag indicating whether you want the sprites to
//                    check and update the Z buffer as part of rendering.
//                    If ZEnable is FALSE, OR you are using
//                    alpha-blending, then it is necessary to render your
//                    sprites from back-to-front. 
//
//-------------------------------------------------------------------------

#ifdef __cplusplus
HRESULT WINAPI
    D3DXPrepareDeviceForSprite( LPDIRECT3DDEVICE7 pd3dDevice, 
                                BOOL ZEnable = FALSE);
#else
HRESULT WINAPI
    D3DXPrepareDeviceForSprite( LPDIRECT3DDEVICE7 pd3dDevice, 
                                BOOL ZEnable);
#endif



//-------------------------------------------------------------------------
// The D3DXDrawBasicSprite() function performs blitting of source images onto 
// a 3D rendering device. This function only calls SetTexture on the first 
// renderstage with the parameter (pd3dTexture) if that parameter is non-null. 
// This function assumes that D3DXPrepareDeviceForSprite has been called on 
// the device or that caller has in some other way correctly prepared the 
// renderstates.
//
// This function supports scaling, rotations, alpha-blending, and choosing 
// a source sub-rect.
// 
// Rotation angle is specified in radians. Both rotations and scales
// are applied around the center of the sprite; where the center of the
// sprite is half the width/height of the sprite, plus the offset parameter. 
//
// Use the offset parameter if you want the sprite's center to be something 
// other than the image center.
//
// The destination point indicates where you would like the center of
// the sprite to draw to.
//
// Parameters: 
//      pd3dTexture - a pointer to the surface containing the texture
//      pd3dDevice  - a pointer to the d3d device to render to. It is
//                    assumed that render states are set up. (See
//                    D3DXPrepareDeviceForSprite)
//      ppointDest  - a pointer to the target point for the sprite. The
//                    components of the vector must be in screen
//                    space.
//      alpha       - alpha value to apply to sprite. 1.0 means totally
//                    opaque; and 0.0 means totally transparent. 
//                    WARNING: If you are using alpha, then you should render
//                    from back to front in order to avoid rendering
//                    artifacts.
//      angleRad    - angle of rotation around the 'center' of the rect
//      scale       - a uniform scale that is applied to the source rect
//                    to specify the size of the image that is rendered
//      pOffset     - offset from the center of the source rect to use as the 
//                    center of rotation
//      pSourceRect - a rect that indicates what portion of the source
//                    source texture to use. If NULL is passed, then the
//                    entire source is used. If the source texture was 
//                    created via D3DX, then the rect should be specified
//                    in the coordinates of the original image (so that you
//                    don't have to worry about stretching/scaling that D3DX
//                    may have done to make the image work with your current
//                    3D Device.) Note that horizontal or vertical mirroring 
//                    may be simply accomplished by swapping the left/right 
//                    or top/bottom fields of this RECT.
//-------------------------------------------------------------------------

#ifdef __cplusplus
HRESULT WINAPI 
    D3DXDrawSpriteSimple(LPDIRECTDRAWSURFACE7  pd3dTexture, 
                         LPDIRECT3DDEVICE7     pd3dDevice, 
                         const D3DXVECTOR3     *ppointDest, 
                         float                 alpha        = 1.0f,
                         float                 scale        = 1.0f,
                         float                 angleRad     = 0.0f,
                         const D3DXVECTOR2     *pOffset     = NULL,
                         const RECT            *pSourceRect = NULL);
#else
HRESULT WINAPI 
    D3DXDrawSpriteSimple(LPDIRECTDRAWSURFACE7  pd3dTexture, 
                         LPDIRECT3DDEVICE7     pd3dDevice, 
                         D3DXVECTOR3           *ppointDest, 
                         float                 alpha,
                         float                 scale,
                         float                 angleRad,
                         D3DXVECTOR2           *pOffset,
                         RECT                  *pSourceRect);
#endif

//-------------------------------------------------------------------------
// The D3DXDrawSprite() function transforms source images onto a 3D 
// rendering device. It takes a general 4x4 matrix which is use to transform
// the points of a default rect: (left=-.5, top=-.5, right=+.5, bottom=+.5).
// (This default rect was chosen so that it was centered around the origin
// to ease setting up rotations. And it was chosen to have a width/height of one
// to ease setting up scales.)
// 
// This function only calls SetTexture on the first 
// renderstage with the parameter (pd3dTexture) if that parameter is non-null. 
// This function assumes that D3DXPrepareDeviceForSprite has been called on 
// the device or that caller has in some other way correctly prepared the 
// renderstates.
//
// This function supports alpha-blending, and choosing 
// a source sub-rect. (A value of NULL for source sub-rect means the entire
// texture is used.)
//
// Note that if the transformed points have a value for w (the homogenous
// coordinate) that is not 1, then this function will invert it and pass
// that value to D3D as the rhw field of a TLVERTEX. If the value for w is
// zero, then it use 1 as the rhw.
//
// Parameters: 
//      pd3dTexture - a pointer to the surface containing the texture
//      pd3dDevice  - a pointer to the d3d device to render to. It is
//                    assumed that render states are set up. (See
//                    D3DXPrepareDeviceForSprite)
//      pMatrixTransform - 4x4 matrix that specifies the transformation
//                    that will be applied to the default -.5 to +.5 
//                    rectangle.
//      alpha       - alpha value to apply to sprite. 1.0 means totally
//                    opaque; and 0.0 means totally transparent. 
//                    WARNING: If you are using alpha, then you should render
//                    from back to front in order to avoid rendering
//                    artifacts.Furthermore, you should avoid scenarios where 
//                    semi-transparent objects intersect.
//      pSourceRect - a rect that indicates what portion of the source
//                    source texture to use. If NULL is passed, then the
//                    entire source is used. If the source texture was 
//                    created via D3DX, then the rect should be specified
//                    in the coordinates of the original image (so that you
//                    don't have to worry about stretching/scaling that D3DX
//                    may have done to make the image work with your current
//                    3D Device.) Note that mirroring may be simply accomplished
//                    by swapping the left/right or top/bottom fields of
//                    this RECT.
// 
//-------------------------------------------------------------------------

#ifdef __cplusplus
HRESULT WINAPI 
    D3DXDrawSpriteTransform(LPDIRECTDRAWSURFACE7  pd3dTexture, 
                            LPDIRECT3DDEVICE7     pd3dDevice, 
                            const D3DXMATRIX      *pMatrixTransform, 
                            float                 alpha         = 1.0f,
                            const RECT            *pSourceRect  = NULL);
#else
HRESULT WINAPI 
    D3DXDrawSpriteTransform(LPDIRECTDRAWSURFACE7  pd3dTexture, 
                            LPDIRECT3DDEVICE7     pd3dDevice, 
                            D3DXMATRIX            *pMatrixTransform, 
                            float                 alpha,
                            RECT                  *pSourceRect);
#endif

//-------------------------------------------------------------------------
// The D3DXBuildSpriteTransform() function is a helper provided which
// creates a matrix corresponding to simple properties. This matrix is
// set up to pass directly to D3DXTransformSprite.
//
// Parameters: 
//      pMatrix     - a pointer to the result matrix
//      prectDest   - a pointer to the target rectangle for the sprite
//      angleRad    - angle of rotation around the 'center' of the rect
//      pOffset     - offset from the center of the source rect to use as the 
//                    center of rotation
// 
//-------------------------------------------------------------------------

#ifdef __cplusplus
void WINAPI
    D3DXBuildSpriteTransform(D3DXMATRIX            *pMatrix,
                             const RECT            *prectDest,
                             float                 angleRad     = 0.0f,
                             const D3DXVECTOR2     *pOffset     = NULL);
#else
void WINAPI
    D3DXBuildSpriteTransform(D3DXMATRIX            *pMatrix,
                             RECT                  *prectDest,
                             float                 angleRad,
                             D3DXVECTOR2           *pOffset);
#endif


//-------------------------------------------------------------------------
// The D3DXDrawSprite3D() function renders a texture onto a 3D quad. The
// quad ABCD is broken into two triangles ABC and ACD which are rendered
// via DrawPrim.
//
// Parameters: 
//      pd3dTexture - a pointer to the surface containing the texture
//      pd3dDevice  - a pointer to the d3d device to render to. It is
//                    assumed that render states are set up. (See
//                    D3DXPrepareDeviceForSprite)
//      quad        - array of 4 points in the following order:
//                    upper-left, upper-right, lower-right, lower-left.
//                    If these vectors contain a W, then this function
//                    will take the reciprocal of that value to pass as
//                    as the rhw (i.e. reciprocal homogenous w).
//      alpha       - alpha value to apply to sprite. 1.0 means totally
//                    opaque; and 0.0 means totally transparent. 
//                    WARNING: If you are using alpha, then you should render
//                    from back to front in order to avoid rendering
//                    artifacts.Furthermore, you should avoid scenarios where 
//                    semi-transparent objects intersect.
//      pSourceRect - a rect that indicates what portion of the source
//                    source texture to use. If NULL is passed, then the
//                    entire source is used. If the source texture was 
//                    created via D3DX, then the rect should be specified
//                    in the coordinates of the original image (so that you
//                    don't have to worry about stretching/scaling that D3DX
//                    may have done to make the image work with your current
//                    3D Device.) Note that mirroring may be simply accomplished
//                    by swapping the left/right or top/bottom fields of
//                    this RECT.
//-------------------------------------------------------------------------

#ifdef __cplusplus
HRESULT WINAPI 
    D3DXDrawSprite3D(LPDIRECTDRAWSURFACE7  pd3dTexture, 
                     LPDIRECT3DDEVICE7     pd3dDevice, 
                     const D3DXVECTOR4     quad[4], 
                     float                 alpha         = 1.0f,
                     const RECT            *pSourceRect  = NULL);
#else
HRESULT WINAPI 
    D3DXDrawSprite3D(LPDIRECTDRAWSURFACE7  pd3dTexture, 
                     LPDIRECT3DDEVICE7     pd3dDevice, 
                     D3DXVECTOR4           quad[4], 
                     float                 alpha,
                     RECT                  *pSourceRect);
#endif



#ifdef __cplusplus
} // extern "C"
#endif

#endif // __D3DXSPRITE_H__
