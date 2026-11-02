module;

#include <stdafx.h>
#ifndef DIRECTDRAW_VERSION
#define DIRECTDRAW_VERSION 0x0700
#endif
#include <ddraw.h>
#include <d3d9.h>
#include <atomic>
#include <algorithm>
#include <functional>
#include <mutex>
#include <new>
#include <vector>
#include <utility>

#pragma comment(lib, "dxguid.lib")

export module MovieSurfaces;

import MovieInterfaces;

export namespace GodfatherMovies
{
    class Surface;
    std::mutex queueMutex;
    std::vector<Surface*> pending;
    std::atomic<bool> hasFrames{ false };
    std::function<std::pair<int, int>()> dimensions;
    void Queue(Surface* surface);
    void Cancel(Surface* surface);

    std::pair<int, int> Size()
    {
        const auto size = dimensions ? dimensions() : std::pair{ 640, 480 };
        return { std::clamp(size.first, 1, 8192), std::clamp(size.second, 1, 8192) };
    }

    class Surface final : public SurfaceInterface
    {
        std::atomic<ULONG> references{ 1 };
        std::atomic<bool> locked{ false };
    public:
        const int width, height, pitch;
        const bool primary;
        std::mutex pixelsMutex;
        std::vector<uint8_t> pixels;
        RECT source{}, destination{};
        bool visible = false, ready = false;
        uint64_t version = 0, uploaded = 0;
        IDirect3DTexture9* texture = nullptr;
        IDirect3DDevice9* textureDevice = nullptr;

        Surface(int w, int h, bool target) : width(w), height(h), pitch((w * 2 + 3) & ~3), primary(target),
            pixels(target ? 0 : size_t(pitch) * h), source{ 0, 0, w, h }, destination{ 0, 0, w, h }
        {
        }

        ~Surface()
        {
            if (texture)
                texture->Release();
        }

        HRESULT WINAPI QueryInterface(REFIID id, LPVOID* output) override
        {
            if (!output)
                return E_POINTER;
            *output = nullptr;
            if (!IsEqualGUID(id, IID_IUnknown) && !IsEqualGUID(id, IID_IDirectDrawSurface7))
                return E_NOINTERFACE;
            *output = static_cast<IDirectDrawSurface7*>(this);
            AddRef();
            return S_OK;
        }

        ULONG WINAPI AddRef() override
        {
            return references.fetch_add(1, std::memory_order_relaxed) + 1;
        }

        ULONG WINAPI Release() override
        {
            ULONG remaining;
            {
                std::lock_guard guard(queueMutex);
                remaining = references.fetch_sub(1, std::memory_order_acq_rel) - 1;
                if (!remaining)
                {
                    std::erase(pending, this);
                    hasFrames.store(!pending.empty(), std::memory_order_release);
                }
            }
            if (!remaining)
                delete this;
            return remaining;
        }

        HRESULT WINAPI GetSurfaceDesc(LPDDSURFACEDESC2 description) override
        {
            if (!description || description->dwSize != sizeof(*description))
                return DDERR_INVALIDPARAMS;
            *description = {};
            description->dwSize = sizeof(*description);
            description->dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH | DDSD_CAPS | DDSD_PIXELFORMAT;
            description->dwWidth = width;
            description->dwHeight = height;
            description->lPitch = primary ? width * 4 : pitch;
            description->ddsCaps.dwCaps = primary ? DDSCAPS_PRIMARYSURFACE : DDSCAPS_OVERLAY | DDSCAPS_VIDEOMEMORY;
            description->ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
            if (primary)
            {
                description->ddpfPixelFormat.dwFlags = DDPF_RGB;
                description->ddpfPixelFormat.dwRGBBitCount = 32;
                description->ddpfPixelFormat.dwRBitMask = 0x00FF0000;
                description->ddpfPixelFormat.dwGBitMask = 0x0000FF00;
                description->ddpfPixelFormat.dwBBitMask = 0x000000FF;
            }
            else
            {
                description->ddpfPixelFormat.dwFlags = DDPF_FOURCC;
                description->ddpfPixelFormat.dwFourCC = D3DFMT_YUY2;
            }
            return DD_OK;
        }

        HRESULT WINAPI GetPixelFormat(LPDDPIXELFORMAT format) override
        {
            if (!format || format->dwSize != sizeof(*format))
                return DDERR_INVALIDPARAMS;
            DDSURFACEDESC2 description{ sizeof(description) };
            GetSurfaceDesc(&description);
            *format = description.ddpfPixelFormat;
            return DD_OK;
        }

        HRESULT WINAPI GetCaps(LPDDSCAPS2 caps) override
        {
            if (!caps)
                return DDERR_INVALIDPARAMS;
            *caps = {};
            caps->dwCaps = primary ? DDSCAPS_PRIMARYSURFACE : DDSCAPS_OVERLAY | DDSCAPS_VIDEOMEMORY;
            return DD_OK;
        }

        HRESULT WINAPI Lock(LPRECT rectangle, LPDDSURFACEDESC2 description, DWORD, HANDLE) override
        {
            if (primary || rectangle || !description || description->dwSize != sizeof(*description))
                return DDERR_INVALIDPARAMS;
            bool expected = false;
            if (!locked.compare_exchange_strong(expected, true))
                return DDERR_SURFACEBUSY;
            pixelsMutex.lock();
            AddRef();
            GetSurfaceDesc(description);
            description->lpSurface = pixels.data();
            description->dwFlags |= DDSD_LPSURFACE;
            return DD_OK;
        }

        HRESULT WINAPI Unlock(LPRECT rectangle) override
        {
            if (rectangle)
                return DDERR_INVALIDRECT;
            if (!locked.exchange(false))
                return DDERR_NOTLOCKED;
            ready = true;
            ++version;
            const bool show = visible;
            pixelsMutex.unlock();
            if (show)
                Queue(this);
            Release();
            return DD_OK;
        }

        HRESULT WINAPI UpdateOverlay(LPRECT from, LPDIRECTDRAWSURFACE7 target, LPRECT to, DWORD flags, LPDDOVERLAYFX) override
        {
            if (primary || !target)
                return DDERR_INVALIDPARAMS;
            if (flags & DDOVER_HIDE)
            {
                {
                    std::lock_guard guard(pixelsMutex);
                    visible = false;
                }
                Cancel(this);
                return DD_OK;
            }
            if (!(flags & DDOVER_SHOW) || !from || !to || from->left < 0 || from->top < 0
                || from->right > width || from->bottom > height || from->right <= from->left || from->bottom <= from->top
                || to->right <= to->left || to->bottom <= to->top || to->left < -65536 || to->top < -65536
                || to->right > 65536 || to->bottom > 65536)
                return DDERR_INVALIDPARAMS;
            {
                std::lock_guard guard(pixelsMutex);
                source = *from;
                destination = *to;
                visible = true;
            }
            Queue(this);
            return DD_OK;
        }

        HRESULT WINAPI Blt(LPRECT, LPDIRECTDRAWSURFACE7, LPRECT, DWORD flags, LPDDBLTFX) override
        {
            return primary && (flags & DDBLT_COLORFILL) ? DD_OK : DDERR_UNSUPPORTED;
        }

        HRESULT WINAPI IsLost() override { return DD_OK; }
        HRESULT WINAPI Restore() override { return DD_OK; }
        HRESULT WINAPI GetBltStatus(DWORD) override { return DD_OK; }
        HRESULT WINAPI GetFlipStatus(DWORD) override { return DD_OK; }
    };

    void Queue(Surface* surface)
    {
        std::lock_guard guard(queueMutex);
        if (std::find(pending.begin(), pending.end(), surface) == pending.end())
        {
            try
            {
                pending.push_back(surface);
                hasFrames.store(true, std::memory_order_release);
            }
            catch (const std::bad_alloc&)
            {
                OutputDebugStringA("The Godfather movie: unable to queue decoded frame.\n");
            }
        }
    }

    void Cancel(Surface* surface)
    {
        std::lock_guard guard(queueMutex);
        const auto found = std::find(pending.begin(), pending.end(), surface);
        if (found != pending.end())
        {
            pending.erase(found);
            hasFrames.store(!pending.empty(), std::memory_order_release);
        }
    }

    class Draw final : public DrawInterface
    {
        std::atomic<ULONG> references{ 1 };
    public:
        HRESULT WINAPI QueryInterface(REFIID id, LPVOID* output) override
        {
            if (!output)
                return E_POINTER;
            *output = nullptr;
            if (!IsEqualGUID(id, IID_IUnknown) && !IsEqualGUID(id, IID_IDirectDraw7))
                return E_NOINTERFACE;
            *output = static_cast<IDirectDraw7*>(this);
            AddRef();
            return DD_OK;
        }

        ULONG WINAPI AddRef() override
        {
            return references.fetch_add(1, std::memory_order_relaxed) + 1;
        }

        ULONG WINAPI Release() override
        {
            const auto remaining = references.fetch_sub(1, std::memory_order_acq_rel) - 1;
            if (!remaining)
                delete this;
            return remaining;
        }

        HRESULT WINAPI CreateSurface(LPDDSURFACEDESC2 description, LPDIRECTDRAWSURFACE7* output, IUnknown* outer) override
        {
            if (!output)
                return DDERR_INVALIDPARAMS;
            *output = nullptr;
            if (!description || description->dwSize != sizeof(*description) || outer || !(description->dwFlags & DDSD_CAPS))
                return DDERR_INVALIDPARAMS;
            const bool primary = (description->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) != 0;
            if (!primary && (!(description->ddsCaps.dwCaps & DDSCAPS_OVERLAY)
                || description->ddpfPixelFormat.dwFourCC != D3DFMT_YUY2))
                return DDERR_INVALIDPARAMS;
            const auto size = primary ? Size() : std::pair{ int(description->dwWidth), int(description->dwHeight) };
            if (size.first <= 0 || size.second <= 0 || size.first > 8192 || size.second > 8192)
                return DDERR_INVALIDPARAMS;
            try
            {
                *output = new Surface(size.first, size.second, primary);
                return DD_OK;
            }
            catch (const std::bad_alloc&)
            {
                return DDERR_OUTOFMEMORY;
            }
        }

        HRESULT WINAPI GetCaps(LPDDCAPS driver, LPDDCAPS emulation) override
        {
            for (auto caps : { driver, emulation })
            {
                if (!caps)
                    continue;
                if (caps->dwSize != sizeof(*caps))
                    return DDERR_INVALIDPARAMS;
                *caps = {};
                caps->dwSize = sizeof(*caps);
                caps->dwCaps = DDCAPS_OVERLAY | DDCAPS_OVERLAYFOURCC;
                caps->dwNumFourCCCodes = 1;
                caps->dwMaxVisibleOverlays = 1;
            }
            return DD_OK;
        }

        HRESULT WINAPI GetDisplayMode(LPDDSURFACEDESC2 description) override
        {
            if (!description || description->dwSize != sizeof(*description))
                return DDERR_INVALIDPARAMS;
            const auto size = Size();
            *description = {};
            description->dwSize = sizeof(*description);
            description->dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_DEPTH | DDSD_REFRESHRATE;
            description->dwWidth = size.first;
            description->dwHeight = size.second;
            description->dwDepth = 32;
            description->dwRefreshRate = 60;
            return DD_OK;
        }

        HRESULT WINAPI GetFourCCCodes(LPDWORD count, LPDWORD codes) override
        {
            if (!count)
                return DDERR_INVALIDPARAMS;
            const DWORD capacity = *count;
            *count = 1;
            if (codes && capacity)
                codes[0] = D3DFMT_YUY2;
            return codes && !capacity ? DDERR_MOREDATA : DD_OK;
        }

        HRESULT WINAPI SetCooperativeLevel(HWND, DWORD) override { return DD_OK; }
        HRESULT WINAPI RestoreDisplayMode() override { return DD_OK; }
        HRESULT WINAPI TestCooperativeLevel() override { return DD_OK; }
    };

    HRESULT WINAPI Factory(GUID*, LPVOID* output, REFIID id, IUnknown* outer)
    {
        if (!output)
            return E_POINTER;
        *output = nullptr;
        if (outer)
            return CLASS_E_NOAGGREGATION;
        if (!IsEqualGUID(id, IID_IDirectDraw7))
            return E_NOINTERFACE;
        *output = new(std::nothrow) Draw();
        return *output ? DD_OK : DDERR_OUTOFMEMORY;
    }
}
