module;

#include <stdafx.h>
#ifndef DIRECTDRAW_VERSION
#define DIRECTDRAW_VERSION 0x0700
#endif
#include <ddraw.h>

export module MovieInterfaces;

export namespace GodfatherMovies
{
    class DrawInterface : public IDirectDraw7
    {
    public:
        HRESULT WINAPI QueryInterface(REFIID riid, LPVOID FAR * ppvObj) override
        {
            return DDERR_UNSUPPORTED;
        }

        ULONG WINAPI AddRef() override
        {
            return 0;
        }

        ULONG WINAPI Release() override
        {
            return 0;
        }

        HRESULT WINAPI Compact() override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI CreateClipper(DWORD, LPDIRECTDRAWCLIPPER FAR*, IUnknown FAR *) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI CreatePalette(DWORD, LPPALETTEENTRY, LPDIRECTDRAWPALETTE FAR*, IUnknown FAR *) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI CreateSurface(LPDDSURFACEDESC2, LPDIRECTDRAWSURFACE7 FAR *, IUnknown FAR *) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI DuplicateSurface(LPDIRECTDRAWSURFACE7, LPDIRECTDRAWSURFACE7 FAR *) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI EnumDisplayModes(DWORD, LPDDSURFACEDESC2, LPVOID, LPDDENUMMODESCALLBACK2) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI EnumSurfaces(DWORD, LPDDSURFACEDESC2, LPVOID,LPDDENUMSURFACESCALLBACK7) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI FlipToGDISurface() override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI GetCaps(LPDDCAPS, LPDDCAPS) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI GetDisplayMode(LPDDSURFACEDESC2) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI GetFourCCCodes(LPDWORD, LPDWORD) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI GetGDISurface(LPDIRECTDRAWSURFACE7 FAR *) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI GetMonitorFrequency(LPDWORD) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI GetScanLine(LPDWORD) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI GetVerticalBlankStatus(LPBOOL) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI Initialize(GUID FAR *) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI RestoreDisplayMode() override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI SetCooperativeLevel(HWND, DWORD) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI SetDisplayMode(DWORD, DWORD,DWORD, DWORD, DWORD) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI WaitForVerticalBlank(DWORD, HANDLE) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI GetAvailableVidMem(LPDDSCAPS2, LPDWORD, LPDWORD) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI GetSurfaceFromDC(HDC, LPDIRECTDRAWSURFACE7 *) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI RestoreAllSurfaces() override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI TestCooperativeLevel() override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI GetDeviceIdentifier(LPDDDEVICEIDENTIFIER2, DWORD) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI StartModeTest(LPSIZE, DWORD, DWORD) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI EvaluateMode(DWORD, DWORD *) override
        {
            return DDERR_UNSUPPORTED;
        }

    };

    class SurfaceInterface : public IDirectDrawSurface7
    {
    public:
        HRESULT WINAPI QueryInterface(REFIID riid, LPVOID FAR * ppvObj) override
        {
            return DDERR_UNSUPPORTED;
        }

        ULONG WINAPI AddRef() override
        {
            return 0;
        }

        ULONG WINAPI Release() override
        {
            return 0;
        }

        HRESULT WINAPI AddAttachedSurface(LPDIRECTDRAWSURFACE7) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI AddOverlayDirtyRect(LPRECT) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI Blt(LPRECT,LPDIRECTDRAWSURFACE7, LPRECT,DWORD, LPDDBLTFX) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI BltBatch(LPDDBLTBATCH, DWORD, DWORD) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI BltFast(DWORD,DWORD,LPDIRECTDRAWSURFACE7, LPRECT,DWORD) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI DeleteAttachedSurface(DWORD,LPDIRECTDRAWSURFACE7) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI EnumAttachedSurfaces(LPVOID,LPDDENUMSURFACESCALLBACK7) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI EnumOverlayZOrders(DWORD,LPVOID,LPDDENUMSURFACESCALLBACK7) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI Flip(LPDIRECTDRAWSURFACE7, DWORD) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI GetAttachedSurface(LPDDSCAPS2, LPDIRECTDRAWSURFACE7 FAR *) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI GetBltStatus(DWORD) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI GetCaps(LPDDSCAPS2) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI GetClipper(LPDIRECTDRAWCLIPPER FAR*) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI GetColorKey(DWORD, LPDDCOLORKEY) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI GetDC(HDC FAR *) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI GetFlipStatus(DWORD) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI GetOverlayPosition(LPLONG, LPLONG) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI GetPalette(LPDIRECTDRAWPALETTE FAR*) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI GetPixelFormat(LPDDPIXELFORMAT) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI GetSurfaceDesc(LPDDSURFACEDESC2) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI Initialize(LPDIRECTDRAW, LPDDSURFACEDESC2) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI IsLost() override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI Lock(LPRECT,LPDDSURFACEDESC2,DWORD,HANDLE) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI ReleaseDC(HDC) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI Restore() override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI SetClipper(LPDIRECTDRAWCLIPPER) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI SetColorKey(DWORD, LPDDCOLORKEY) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI SetOverlayPosition(LONG, LONG) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI SetPalette(LPDIRECTDRAWPALETTE) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI Unlock(LPRECT) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI UpdateOverlay(LPRECT, LPDIRECTDRAWSURFACE7,LPRECT,DWORD, LPDDOVERLAYFX) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI UpdateOverlayDisplay(DWORD) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI UpdateOverlayZOrder(DWORD, LPDIRECTDRAWSURFACE7) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI GetDDInterface(LPVOID FAR *) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI PageLock(DWORD) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI PageUnlock(DWORD) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI SetSurfaceDesc(LPDDSURFACEDESC2, DWORD) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI SetPrivateData(REFGUID, LPVOID, DWORD, DWORD) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI GetPrivateData(REFGUID, LPVOID, LPDWORD) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI FreePrivateData(REFGUID) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI GetUniquenessValue(LPDWORD) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI ChangeUniquenessValue() override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI SetPriority(DWORD) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI GetPriority(LPDWORD) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI SetLOD(DWORD) override
        {
            return DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI GetLOD(LPDWORD) override
        {
            return DDERR_UNSUPPORTED;
        }

    };

}
