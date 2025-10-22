//-----------------------------------------------------------------------------
// File: D3DUtil.h
//
// Desc: Helper functions and typing shortcuts for Direct3D programming.
//
// Copyright (c) 1997-2000 Microsoft Corporation. All rights reserved
//-----------------------------------------------------------------------------
#ifndef D3DUTIL_H
#define D3DUTIL_H
#include <D3D8.h>
#include <D3DX8Math.h>




//-----------------------------------------------------------------------------
// Name: D3DUtil_InitMaterial()
// Desc: Initializes a D3DMATERIAL8 structure, setting the diffuse and ambient
//       colors. It does not set emissive or specular colors.
//-----------------------------------------------------------------------------
VOID D3DUtil_InitMaterial( D3DMATERIAL8& mtrl, FLOAT r=0.0f, FLOAT g=0.0f,
                                               FLOAT b=0.0f, FLOAT a=1.0f );




//-----------------------------------------------------------------------------
// Name: D3DUtil_InitLight()
// Desc: Initializes a D3DLIGHT structure, setting the light position. The
//       diffuse color is set to white, specular and ambient left as black.
//-----------------------------------------------------------------------------
VOID D3DUtil_InitLight( D3DLIGHT8& light, D3DLIGHTTYPE ltType,
                        FLOAT x=0.0f, FLOAT y=0.0f, FLOAT z=0.0f );




//-----------------------------------------------------------------------------
// Name: D3DUtil_CreateTexture()
// Desc: Helper function to create a texture. It checks the root path first,
//       then tries the DXSDK media path (as specified in the system registry).
//-----------------------------------------------------------------------------
HRESULT D3DUtil_CreateTexture( LPDIRECT3DDEVICE8 pd3dDevice, TCHAR* strTexture,
                               LPDIRECT3DTEXTURE8* ppTexture,
                               D3DFORMAT d3dFormat = D3DFMT_UNKNOWN );




//-----------------------------------------------------------------------------
// Name: D3DUtil_SetColorKey()
// Desc: Changes all texels matching the colorkey to transparent, black.
//-----------------------------------------------------------------------------
HRESULT D3DUtil_SetColorKey( LPDIRECT3DTEXTURE8 pTexture, DWORD dwColorKey );




//-----------------------------------------------------------------------------
// Name: D3DUtil_CreateVertexShader()
// Desc: Assembles and creates a file-based vertex shader
//-----------------------------------------------------------------------------
HRESULT D3DUtil_CreateVertexShader( LPDIRECT3DDEVICE8 pd3dDevice, 
								    TCHAR* strFilename, DWORD* pdwVertexDecl,
									DWORD* pdwVertexShader );

									
									
									
//-----------------------------------------------------------------------------
// Name: D3DUtil_GetCubeMapViewMatrix()
// Desc: Returns a view matrix for rendering to a face of a cubemap.
//-----------------------------------------------------------------------------
D3DXMATRIX D3DUtil_GetCubeMapViewMatrix( DWORD dwFace );




//-----------------------------------------------------------------------------
// Name: D3DUtil_GetRotationFromCursor()
// Desc: Returns a quaternion for the rotation implied by the window's cursor
//       position.
//-----------------------------------------------------------------------------
D3DXQUATERNION D3DUtil_GetRotationFromCursor( HWND hWnd,
                                              FLOAT fTrackBallRadius=1.0f );




//-----------------------------------------------------------------------------
// Name: D3DUtil_SetDeviceCursor
// Desc: Builds and sets a cursor for the D3D device based on hCursor.
//-----------------------------------------------------------------------------
HRESULT D3DUtil_SetDeviceCursor( LPDIRECT3DDEVICE8 pd3dDevice, HCURSOR hCursor );




//-----------------------------------------------------------------------------
// Name: class CD3DArcBall
// Desc:
//-----------------------------------------------------------------------------
class CD3DArcBall
{
	INT            m_iWidth;   // ArcBall's window width
	INT            m_iHeight;  // ArcBall's window height
	FLOAT          m_fRadius;  // ArcBall's radius in screen coords
	FLOAT          m_fRadius2; // ArcBall's radius in screen coords

	D3DXQUATERNION m_qDown;               // Quaternion before button down
	D3DXQUATERNION m_qNow;                // Composite quaternion for current drag
	D3DXMATRIX     m_matRotation;         // Matrix for arcball's orientation
	D3DXMATRIX     m_matRotationDelta;    // Matrix for arcball's orientation
	D3DXMATRIX     m_matTranslation;      // Matrix for arcball's position
	D3DXMATRIX     m_matTranslationDelta; // Matrix for arcball's position
	BOOL           m_bDrag;               // Whether user is dragging arcball

	D3DXVECTOR3 ScreenToVector( int sx, int sy );

public:
	LRESULT     HandleMouseMessages( HWND, UINT, WPARAM, LPARAM );

	D3DXMATRIX* GetRotationMatrix()         { return &m_matRotation; }
	D3DXMATRIX* GetRotationDeltaMatrix()    { return &m_matRotationDelta; }
	D3DXMATRIX* GetTranslationMatrix()      { return &m_matTranslation; }
	D3DXMATRIX* GetTranslationDeltaMatrix() { return &m_matTranslationDelta; }
	BOOL        IsBeingDragged()            { return m_bDrag; }

	VOID        SetRadius( FLOAT fRadius );
	VOID        SetWindow( INT w, INT h, FLOAT r=0.9 );

	CD3DArcBall();
};




//-----------------------------------------------------------------------------
// Name: class CD3DCamera
// Desc:
//-----------------------------------------------------------------------------
class CD3DCamera
{
	D3DXVECTOR3 m_vEyePt;       // Attributes for view matrix
	D3DXVECTOR3 m_vLookatPt;
	D3DXVECTOR3 m_vUpVec;

	D3DXVECTOR3 m_vView;
	D3DXVECTOR3 m_vCross;

	D3DXMATRIX  m_matView;
	D3DXMATRIX  m_matBillboard; // Special matrix for billboarding effects

	FLOAT       m_fFOV;         // Attributes for projection matrix
	FLOAT       m_fAspect;
	FLOAT       m_fNearPlane;
	FLOAT       m_fFarPlane;
	D3DXMATRIX  m_matProj;

public:
	// Access functions
	D3DXVECTOR3 GetEyePt()           { return m_vEyePt; }
	D3DXVECTOR3 GetLookatPt()        { return m_vLookatPt; }
	D3DXVECTOR3 GetUpVec()           { return m_vUpVec; }
	D3DXVECTOR3 GetViewDir()         { return m_vView; }
	D3DXVECTOR3 GetCross()           { return m_vCross; }

	D3DXMATRIX  GetViewMatrix()      { return m_matView; }
	D3DXMATRIX  GetBillboardMatrix() { return m_matBillboard; }
	D3DXMATRIX  GetProjMatrix()      { return m_matProj; }

	VOID SetViewParams( D3DXVECTOR3 &vEyePt, D3DXVECTOR3& vLookatPt,
						D3DXVECTOR3& vUpVec );
	VOID SetProjParams( FLOAT fFOV, FLOAT fAspect, FLOAT fNearPlane,
						FLOAT fFarPlane );

	CD3DCamera();
};

//-----------------------------------------------------------------------------
// Helper macros for pixel shader instructions
//-----------------------------------------------------------------------------

// Parameter writemasks
#define D3DPSP_WRITEMASK_B   D3DSP_WRITEMASK_0
#define D3DPSP_WRITEMASK_G   D3DSP_WRITEMASK_1
#define D3DPSP_WRITEMASK_R   D3DSP_WRITEMASK_2
#define D3DPSP_WRITEMASK_A   D3DSP_WRITEMASK_3
#define D3DPSP_WRITEMASK_C   (D3DPSP_WRITEMASK_B|D3DPSP_WRITEMASK_G|D3DPSP_WRITEMASK_R)
#define D3DPSP_WRITEMASK_ALL (D3DSP_WRITEMASK_0|D3DSP_WRITEMASK_1|D3DSP_WRITEMASK_2|D3DSP_WRITEMASK_3)
#define D3DPSP_WRITEMASK_10  (D3DSP_WRITEMASK_0|D3DSP_WRITEMASK_1)
#define D3DPSP_WRITEMASK_32  (D3DSP_WRITEMASK_2|D3DSP_WRITEMASK_3)

// Source and destination parameter token
#define D3DPS_REGNUM_MASK(_Num)   ( (1L<<31) | ((_Num)&D3DSP_REGNUM_MASK) )
#define D3DPS_DST(_Num)           ( D3DPS_REGNUM_MASK(_Num) | D3DSPR_TEMP | D3DPSP_WRITEMASK_ALL )
#define D3DPS_SRC_TEMP(_Num)      ( D3DPS_REGNUM_MASK(_Num) | D3DSP_NOSWIZZLE | D3DSPR_TEMP )
#define D3DPS_SRC_INPUT(_Num)     ( D3DPS_REGNUM_MASK(_Num) | D3DSP_NOSWIZZLE | D3DSPR_INPUT )
#define D3DPS_SRC_CONST(_Num)     ( D3DPS_REGNUM_MASK(_Num) | D3DSP_NOSWIZZLE | D3DSPR_CONST )
#define D3DPS_SRC_TEXTURE(_Num)   ( D3DPS_REGNUM_MASK(_Num) | D3DSP_NOSWIZZLE | D3DSPR_TEXTURE )
#define D3DVS_SRC_ADDR(_Num)      ( D3DPS_REGNUM_MASK(_Num) | D3DSP_NOSWIZZLE | D3DSPR_ADDR )
#define D3DVS_SRC_RASTOUT(_Num)   ( D3DPS_REGNUM_MASK(_Num) | D3DSP_NOSWIZZLE | D3DSPR_RASTOUT )
#define D3DVS_SRC_ATTROUT(_Num)   ( D3DPS_REGNUM_MASK(_Num) | D3DSP_NOSWIZZLE | D3DSPR_ATTROUT )
#define D3DVS_SRC_TEXCRDOUT(_Num) ( D3DPS_REGNUM_MASK(_Num) | D3DSP_NOSWIZZLE | D3DSPR_TEXCRDOUT )

// Temp destination registers
#define D3DS_DR0   D3DPS_DST(0)
#define D3DS_DR1   D3DPS_DST(1)
#define D3DS_DR2   D3DPS_DST(2)
#define D3DS_DR3   D3DPS_DST(3)
#define D3DS_DR4   D3DPS_DST(4)
#define D3DS_DR5   D3DPS_DST(5)
#define D3DS_DR6   D3DPS_DST(6)
#define D3DS_DR7   D3DPS_DST(7)

// Temp source registers
#define D3DS_SR0   D3DPS_SRC_TEMP(0)
#define D3DS_SR1   D3DPS_SRC_TEMP(1)
#define D3DS_SR2   D3DPS_SRC_TEMP(2)
#define D3DS_SR3   D3DPS_SRC_TEMP(3)
#define D3DS_SR4   D3DPS_SRC_TEMP(4)
#define D3DS_SR5   D3DPS_SRC_TEMP(5)
#define D3DS_SR6   D3DPS_SRC_TEMP(6)
#define D3DS_SR7   D3DPS_SRC_TEMP(7)

// Texture parameters
#define D3DS_T0   D3DPS_SRC_TEXTURE(0)
#define D3DS_T1   D3DPS_SRC_TEXTURE(1)
#define D3DS_T2   D3DPS_SRC_TEXTURE(2)
#define D3DS_T3   D3DPS_SRC_TEXTURE(3)
#define D3DS_T4   D3DPS_SRC_TEXTURE(4)
#define D3DS_T5   D3DPS_SRC_TEXTURE(5)
#define D3DS_T6   D3DPS_SRC_TEXTURE(6)
#define D3DS_T7   D3DPS_SRC_TEXTURE(7)

// Constant (factor) source parameters
#define D3DS_C0     D3DPS_SRC_CONST(0)
#define D3DS_C1     D3DPS_SRC_CONST(1)
#define D3DS_C2     D3DPS_SRC_CONST(2)
#define D3DS_C3     D3DPS_SRC_CONST(3)
#define D3DS_C4     D3DPS_SRC_CONST(4)
#define D3DS_C5     D3DPS_SRC_CONST(5)
#define D3DS_C6     D3DPS_SRC_CONST(6)
#define D3DS_C7     D3DPS_SRC_CONST(7)

// Iterated source parameters (0==Diffuse, 1==specular)
#define D3DS_V0     D3DPS_SRC_INPUT(0)
#define D3DS_V1     D3DPS_SRC_INPUT(1)




#endif // D3DUTIL_H
