//----------------------------------------------------------------------
//                                                                      
//   d3dxerr.h --  0xC code definitions for the D3DX API                
//                                                                      
//   Copyright (c) 1991-1999, Microsoft Corp. All rights reserved.      
//                                                                      
//----------------------------------------------------------------------
#ifndef __D3DXERR_H__
#define __D3DXERR_H__

// 
//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//
#define FACILITY_D3DX    0x877



//
// MessageId: D3DXERR_NOMEMORY
//
// MessageText:
//
//  Out of memory.
// 
#define D3DXERR_NOMEMORY    ((HRESULT)0xC8770BB8L)


//
// MessageId: D3DXERR_NULLPOINTER
//
// MessageText:
//
//  A NULL pointer was passed as a parameter.
// 
#define D3DXERR_NULLPOINTER    ((HRESULT)0xC8770BB9L)


//
// MessageId: D3DXERR_INVALIDD3DXDEVICEINDEX
//
// MessageText:
//
//  The Device Index passed in is invalid.
// 
#define D3DXERR_INVALIDD3DXDEVICEINDEX    ((HRESULT)0xC8770BBAL)


//
// MessageId: D3DXERR_NODIRECTDRAWAVAILABLE
//
// MessageText:
//
//  DirectDraw has not been created.
// 
#define D3DXERR_NODIRECTDRAWAVAILABLE    ((HRESULT)0xC8770BBBL)


//
// MessageId: D3DXERR_NODIRECT3DAVAILABLE
//
// MessageText:
//
//  Direct3D has not been created.
// 
#define D3DXERR_NODIRECT3DAVAILABLE    ((HRESULT)0xC8770BBCL)


//
// MessageId: D3DXERR_NODIRECT3DDEVICEAVAILABLE
//
// MessageText:
//
//  Direct3D device has not been created.
// 
#define D3DXERR_NODIRECT3DDEVICEAVAILABLE    ((HRESULT)0xC8770BBDL)


//
// MessageId: D3DXERR_NOPRIMARYAVAILABLE
//
// MessageText:
//
//  Primary surface has not been created.
// 
#define D3DXERR_NOPRIMARYAVAILABLE    ((HRESULT)0xC8770BBEL)


//
// MessageId: D3DXERR_NOZBUFFERAVAILABLE
//
// MessageText:
//
//  Z buffer has not been created.
// 
#define D3DXERR_NOZBUFFERAVAILABLE    ((HRESULT)0xC8770BBFL)


//
// MessageId: D3DXERR_NOBACKBUFFERAVAILABLE
//
// MessageText:
//
//  Backbuffer has not been created.
// 
#define D3DXERR_NOBACKBUFFERAVAILABLE    ((HRESULT)0xC8770BC0L)


//
// MessageId: D3DXERR_COULDNTUPDATECAPS
//
// MessageText:
//
//  Failed to update caps database after changing display mode.
// 
#define D3DXERR_COULDNTUPDATECAPS    ((HRESULT)0xC8770BC1L)


//
// MessageId: D3DXERR_NOZBUFFER
//
// MessageText:
//
//  Could not create Z buffer.
// 
#define D3DXERR_NOZBUFFER    ((HRESULT)0xC8770BC2L)


//
// MessageId: D3DXERR_INVALIDMODE
//
// MessageText:
//
//  Display mode is not valid.
// 
#define D3DXERR_INVALIDMODE    ((HRESULT)0xC8770BC3L)


//
// MessageId: D3DXERR_INVALIDPARAMETER
//
// MessageText:
//
//  One or more of the parameters passed is invalid.
// 
#define D3DXERR_INVALIDPARAMETER    ((HRESULT)0xC8770BC4L)


//
// MessageId: D3DXERR_INITFAILED
//
// MessageText:
//
//  D3DX failed to initialize itself.
// 
#define D3DXERR_INITFAILED    ((HRESULT)0xC8770BC5L)


//
// MessageId: D3DXERR_STARTUPFAILED
//
// MessageText:
//
//  D3DX failed to start up.
// 
#define D3DXERR_STARTUPFAILED    ((HRESULT)0xC8770BC6L)


//
// MessageId: D3DXERR_D3DXNOTSTARTEDYET
//
// MessageText:
//
//  D3DXInitialize() must be called first.
// 
#define D3DXERR_D3DXNOTSTARTEDYET    ((HRESULT)0xC8770BC7L)


//
// MessageId: D3DXERR_NOTINITIALIZED
//
// MessageText:
//
//  D3DX is not initialized yet.
// 
#define D3DXERR_NOTINITIALIZED    ((HRESULT)0xC8770BC8L)


//
// MessageId: D3DXERR_FAILEDDRAWTEXT
//
// MessageText:
//
//  Failed to render text to the surface.
// 
#define D3DXERR_FAILEDDRAWTEXT    ((HRESULT)0xC8770BC9L)


//
// MessageId: D3DXERR_BADD3DXCONTEXT
//
// MessageText:
//
//  Bad D3DX context.
// 
#define D3DXERR_BADD3DXCONTEXT    ((HRESULT)0xC8770BCAL)


//
// MessageId: D3DXERR_CAPSNOTSUPPORTED
//
// MessageText:
//
//  The requested device capabilities are not supported.
// 
#define D3DXERR_CAPSNOTSUPPORTED    ((HRESULT)0xC8770BCBL)


//
// MessageId: D3DXERR_UNSUPPORTEDFILEFORMAT
//
// MessageText:
//
//  The image file format is unrecognized.
// 
#define D3DXERR_UNSUPPORTEDFILEFORMAT    ((HRESULT)0xC8770BCCL)


//
// MessageId: D3DXERR_IFLERROR
//
// MessageText:
//
//  The image file loading library error.
// 
#define D3DXERR_IFLERROR    ((HRESULT)0xC8770BCDL)


//
// MessageId: D3DXERR_FAILEDGETCAPS
//
// MessageText:
//
//  Could not obtain device caps.
// 
#define D3DXERR_FAILEDGETCAPS    ((HRESULT)0xC8770BCEL)


//
// MessageId: D3DXERR_CANNOTRESIZEFULLSCREEN
//
// MessageText:
//
//  Resize does not work for full-screen.
// 
#define D3DXERR_CANNOTRESIZEFULLSCREEN    ((HRESULT)0xC8770BCFL)


//
// MessageId: D3DXERR_CANNOTRESIZENONWINDOWED
//
// MessageText:
//
//  Resize does not work for non-windowed contexts.
// 
#define D3DXERR_CANNOTRESIZENONWINDOWED    ((HRESULT)0xC8770BD0L)


//
// MessageId: D3DXERR_FRONTBUFFERALREADYEXISTS
//
// MessageText:
//
//  Front buffer already exists.
// 
#define D3DXERR_FRONTBUFFERALREADYEXISTS    ((HRESULT)0xC8770BD1L)


//
// MessageId: D3DXERR_FULLSCREENPRIMARYEXISTS
//
// MessageText:
//
//  The app is using the primary in full-screen mode.
// 
#define D3DXERR_FULLSCREENPRIMARYEXISTS    ((HRESULT)0xC8770BD2L)


//
// MessageId: D3DXERR_GETDCFAILED
//
// MessageText:
//
//  Could not get device context.
// 
#define D3DXERR_GETDCFAILED    ((HRESULT)0xC8770BD3L)


//
// MessageId: D3DXERR_BITBLTFAILED
//
// MessageText:
//
//  Could not bitBlt.
// 
#define D3DXERR_BITBLTFAILED    ((HRESULT)0xC8770BD4L)


//
// MessageId: D3DXERR_NOTEXTURE
//
// MessageText:
//
//  There is no surface backing up this texture.
// 
#define D3DXERR_NOTEXTURE    ((HRESULT)0xC8770BD5L)


//
// MessageId: D3DXERR_MIPLEVELABSENT
//
// MessageText:
//
//  There is no such miplevel for this surface.
// 
#define D3DXERR_MIPLEVELABSENT    ((HRESULT)0xC8770BD6L)


//
// MessageId: D3DXERR_SURFACENOTPALETTED
//
// MessageText:
//
//  The surface is not paletted.
// 
#define D3DXERR_SURFACENOTPALETTED    ((HRESULT)0xC8770BD7L)


//
// MessageId: D3DXERR_ENUMFORMATSFAILED
//
// MessageText:
//
//  An error occured while enumerating surface formats.
// 
#define D3DXERR_ENUMFORMATSFAILED    ((HRESULT)0xC8770BD8L)


//
// MessageId: D3DXERR_COLORDEPTHTOOLOW
//
// MessageText:
//
//  D3DX only supports color depths of 16 bit or greater.
// 
#define D3DXERR_COLORDEPTHTOOLOW    ((HRESULT)0xC8770BD9L)


//
// MessageId: D3DXERR_INVALIDFILEFORMAT
//
// MessageText:
//
//  The file format is invalid.
// 
#define D3DXERR_INVALIDFILEFORMAT    ((HRESULT)0xC8770BDAL)


//
// MessageId: D3DXERR_NOMATCHFOUND
//
// MessageText:
//
//  No suitable match found.
// 
#define D3DXERR_NOMATCHFOUND    ((HRESULT)0xC8770BDBL)



#endif //__D3DXERR_H__

