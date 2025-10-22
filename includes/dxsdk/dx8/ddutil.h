//-----------------------------------------------------------------------------
// File: ddutil.cpp
//
// Desc: Routines for loading bitmap and palettes from resources
//
// Copyright (C) 1998-1999 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------
#ifndef DDUTIL_H
#define DDUTIL_H

#include <ddraw.h>
#include <d3d.h>




//-----------------------------------------------------------------------------
// Classes defined in this header file 
//-----------------------------------------------------------------------------
class CDisplay;
class CSurface;




//-----------------------------------------------------------------------------
// Flags for the CDisplay and CSurface methods
//-----------------------------------------------------------------------------
#define DSURFACELOCK_READ
#define DSURFACELOCK_WRITE




//-----------------------------------------------------------------------------
// Name: class CDisplay
// Desc: Class to handle all DDraw aspects of a display, including creation of
//       front and back buffers, creating offscreen surfaces and palettes,
//       and blitting surface and displaying bitmaps.
//-----------------------------------------------------------------------------
class CDisplay
{
protected:
    LPDIRECTDRAW7        m_pDD;
    LPDIRECTDRAWSURFACE7 m_pddsFrontBuffer;
    LPDIRECTDRAWSURFACE7 m_pddsBackBuffer;
    LPDIRECTDRAWSURFACE7 m_pddsBackBufferLeft; // For stereo modes

    HWND                 m_hWnd;
    RECT                 m_rcWindow;
    BOOL                 m_bWindowed;
    BOOL                 m_bStereo;

public:
    CDisplay();
    ~CDisplay();

    // Access functions
    HWND                 GetHWnd()           { return m_hWnd; }
    LPDIRECTDRAW7        GetDirectDraw()     { return m_pDD; }
    LPDIRECTDRAWSURFACE7 GetFrontBuffer()    { return m_pddsFrontBuffer; }
    LPDIRECTDRAWSURFACE7 GetBackBuffer()     { return m_pddsBackBuffer; }
    LPDIRECTDRAWSURFACE7 GetBackBufferLEft() { return m_pddsBackBufferLeft; }

    // Status functions
    BOOL    IsWindowed()                     { return m_bWindowed; }
    BOOL    IsStereo()                       { return m_bStereo; }

    // Creation/destruction methods
    HRESULT CreateFullScreenDisplay( HWND hWnd, DWORD dwWidth, DWORD dwHeight,
		                             DWORD dwBPP );
    HRESULT CreateWindowedDisplay( HWND hWnd, DWORD dwWidth, DWORD dwHeight );
    HRESULT InitClipper();
    HRESULT UpdateBounds();
    virtual HRESULT DestroyObjects();

    // Methods to create child objects
    HRESULT CreateSurface( CSurface** ppSurface, DWORD dwWidth,
		                   DWORD dwHeight );
    HRESULT CreateSurfaceFromBitmap( CSurface** ppSurface, TCHAR* strBMP,
		                             DWORD dwDesiredWidth,
									 DWORD dwDesiredHeight );
    HRESULT CreateSurfaceFromText( CSurface** ppSurface, HFONT hFont,
		                           TCHAR* strText, 
								   COLORREF crBackground,
								   COLORREF crForeground );
    HRESULT CreatePaletteFromBitmap( LPDIRECTDRAWPALETTE* ppPalette, const TCHAR* strBMP );

    // Display methods
    HRESULT Clear( DWORD dwColor = 0L );
    HRESULT ColorKeyBlt( DWORD x, DWORD y, LPDIRECTDRAWSURFACE7 pdds,
                         RECT* prc = NULL );
    HRESULT Blt( DWORD x, DWORD y, LPDIRECTDRAWSURFACE7 pdds,
		         RECT* prc=NULL, DWORD dwFlags=0 );
    HRESULT Blt( DWORD x, DWORD y, CSurface* pSurface, RECT* prc = NULL );
    HRESULT ShowBitmap( HBITMAP hbm, LPDIRECTDRAWPALETTE pPalette=NULL );
    HRESULT SetPalette( LPDIRECTDRAWPALETTE pPalette );
    HRESULT Present();
};




//-----------------------------------------------------------------------------
// Name: class CSurface
// Desc: Class to handle aspects of a DirectDrawSurface.
//-----------------------------------------------------------------------------
class CSurface
{
    LPDIRECTDRAWSURFACE7 m_pdds;
    DDSURFACEDESC2       m_ddsd;
    BOOL                 m_bColorKeyed;

public:
    LPDIRECTDRAWSURFACE7 GetDDrawSurface() { return m_pdds; }
    BOOL                 IsColorKeyed()    { return m_bColorKeyed; }

    HRESULT DrawBitmap( HBITMAP hBMP, DWORD dwBMPOriginX = 0, DWORD dwBMPOriginY = 0, 
		                DWORD dwBMPWidth = 0, DWORD dwBMPHeight = 0 );
    HRESULT DrawBitmap( TCHAR* strBMP, DWORD dwDesiredWidth, DWORD dwDesiredHeight );
    HRESULT DrawText( HFONT hFont, TCHAR* strText, DWORD dwOriginX, DWORD dwOriginY,
		              COLORREF crBackground, COLORREF crForeground );

    HRESULT SetColorKey( DWORD dwColorKey );
    DWORD   ConvertGDIColor( COLORREF dwGDIColor );
    static HRESULT GetBitMaskInfo( DWORD dwBitMask, DWORD* pdwShift, DWORD* pdwBits );

    HRESULT Create( LPDIRECTDRAW7 pDD, DDSURFACEDESC2* pddsd );
    HRESULT Create( LPDIRECTDRAWSURFACE7 pdds );
    HRESULT Destroy();

    CSurface();
    ~CSurface();
};




#endif // DDUTIL_H

