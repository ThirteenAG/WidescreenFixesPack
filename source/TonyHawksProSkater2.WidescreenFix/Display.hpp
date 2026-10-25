#pragma once
#include <ddraw.h>

namespace THPS2Display
{
    struct Draw
    {
        IDirectDraw7* object = nullptr;
        std::array<void*, 30> original{};
        HWND window = nullptr;
        DWORD width = 640, height = 480;
    };

    struct Surface
    {
        IDirectDrawSurface7* object = nullptr;
        IDirectDrawSurface7* back = nullptr;
        IDirectDrawSurface7* primary = nullptr;
        IDirectDrawClipper* clipper = nullptr;
        std::array<void*, 49> original{};
        HWND window = nullptr;
    };

    inline std::map<IDirectDraw7*, std::unique_ptr<Draw>> Draws;
    inline std::map<IDirectDrawSurface7*, std::unique_ptr<Surface>> Surfaces;
    inline std::map<void**, std::array<void*, 30>> DrawTables;
    inline std::map<void**, std::array<void*, 49>> SurfaceTables;
    inline decltype(&DirectDrawCreateEx) Create = nullptr;

    template <typename Object>
    auto& Tables()
    {
        if constexpr (std::is_same_v<Object, IDirectDraw7>)
            return DrawTables;
        else
            return SurfaceTables;
    }

    template <typename Object>
    const auto& Patch(Object* object, std::initializer_list<std::pair<size_t, void*>> replacements)
    {
        auto table = *reinterpret_cast<void***>(object);
        auto& tables = Tables<Object>();
        auto [it, inserted] = tables.try_emplace(table);
        if (inserted)
        {
            std::copy_n(table, it->second.size(), it->second.begin());
            for (auto [index, replacement] : replacements)
                ProtectedGameRef<void*>{ table + index } = replacement;
        }
        return it->second;
    }

    template <size_t Index, typename Result, typename Object, typename... Args>
    Result Forward(Object* object, Args... args)
    {
        auto table = *reinterpret_cast<void***>(object);
        return reinterpret_cast<Result(WINAPI*)(Object*, Args...)>(Tables<Object>().at(table)[Index])(object, args...);
    }

    template <size_t Index, typename Result, typename Object, typename... Args>
    Result Call(const Object& state, Args... args)
    {
        return reinterpret_cast<Result(WINAPI*)(decltype(state.object), Args...)>(state.original[Index])(state.object, args...);
    }

    inline HRESULT Present(Surface& surface)
    {
        if (!IsWindow(surface.window) || IsIconic(surface.window))
            return DD_OK;
        RECT destination{};
        if (!GetClientRect(surface.window, &destination) || IsRectEmpty(&destination))
            return DD_OK;
        MapWindowPoints(surface.window, nullptr, reinterpret_cast<POINT*>(&destination), 2);
        auto result = surface.primary->Blt(&destination, surface.object, nullptr, DDBLT_WAIT, nullptr);
        if (result == DDERR_SURFACELOST)
        {
            surface.primary->Restore();
            surface.object->Restore();
            surface.back->Restore();
            return DD_OK;
        }
        if (FAILED(result))
        {
            DDSURFACEDESC2 image{};
            image.dwSize = sizeof(image);
            result = surface.object->Lock(nullptr, &image, DDLOCK_WAIT | DDLOCK_READONLY, nullptr);
            if (FAILED(result))
                return result;
            struct Bitmap
            {
                BITMAPINFOHEADER header{};
                DWORD masks[3]{};
            } bitmap;
            bitmap.header.biSize = sizeof(BITMAPINFOHEADER);
            bitmap.header.biWidth = std::abs(image.lPitch) / 2;
            bitmap.header.biHeight = image.lPitch >= 0 ? -static_cast<LONG>(image.dwHeight) : static_cast<LONG>(image.dwHeight);
            bitmap.header.biPlanes = 1;
            bitmap.header.biBitCount = 16;
            bitmap.header.biCompression = BI_BITFIELDS;
            bitmap.masks[0] = image.ddpfPixelFormat.dwRBitMask;
            bitmap.masks[1] = image.ddpfPixelFormat.dwGBitMask;
            bitmap.masks[2] = image.ddpfPixelFormat.dwBBitMask;
            auto pixels = static_cast<const uint8_t*>(image.lpSurface);
            if (image.lPitch < 0)
                pixels += image.lPitch * (static_cast<int>(image.dwHeight) - 1);
            auto dc = GetDC(surface.window);
            if (dc)
            {
                RECT client{};
                GetClientRect(surface.window, &client);
                const auto copied = StretchDIBits(dc, 0, 0, client.right, client.bottom, 0, 0,
                    image.dwWidth, image.dwHeight, pixels, reinterpret_cast<BITMAPINFO*>(&bitmap), DIB_RGB_COLORS, SRCCOPY);
                ::ReleaseDC(surface.window, dc);
                result = copied && copied != GDI_ERROR ? DD_OK : DDERR_GENERIC;
            }
            else
                result = DDERR_GENERIC;
            Call<32, HRESULT>(surface, static_cast<RECT*>(nullptr));
        }
        return result;
    }

    inline ULONG WINAPI ReleaseSurface(IDirectDrawSurface7* object)
    {
        auto it = Surfaces.find(object);
        if (it == Surfaces.end())
            return Forward<2, ULONG>(object);
        auto& surface = *it->second;
        const auto references = Call<2, ULONG>(surface);
        if (!references)
        {
            surface.back->Release();
            surface.primary->SetClipper(nullptr);
            surface.primary->Release();
            surface.clipper->Release();
            Surfaces.erase(it);
        }
        return references;
    }

    inline HRESULT WINAPI Attached(IDirectDrawSurface7* object, DDSCAPS2* caps, IDirectDrawSurface7** output)
    {
        if (!Surfaces.contains(object))
            return Forward<12, HRESULT>(object, caps, output);
        auto& surface = *Surfaces.at(object);
        if (caps && output && (caps->dwCaps & DDSCAPS_BACKBUFFER))
        {
            // The game releases the flip chain through its front surface.
            *output = surface.back;
            return DD_OK;
        }
        return Call<12, HRESULT>(surface, caps, output);
    }

    inline HRESULT WINAPI Flip(IDirectDrawSurface7* object, IDirectDrawSurface7* target, DWORD flags)
    {
        if (!Surfaces.contains(object))
            return Forward<11, HRESULT>(object, target, flags);
        auto& surface = *Surfaces.at(object);
        auto result = Call<5, HRESULT>(surface, static_cast<RECT*>(nullptr), surface.back, static_cast<RECT*>(nullptr),
            DWORD{ DDBLT_WAIT }, static_cast<DDBLTFX*>(nullptr));
        return SUCCEEDED(result) ? Present(surface) : result;
    }

    inline HRESULT WINAPI Blt(IDirectDrawSurface7* object, RECT* destination, IDirectDrawSurface7* source,
        RECT* sourceRect, DWORD flags, DDBLTFX* effects)
    {
        if (!Surfaces.contains(object))
            return Forward<5, HRESULT>(object, destination, source, sourceRect, flags, effects);
        auto& surface = *Surfaces.at(object);
        auto result = Call<5, HRESULT>(surface, destination, source, sourceRect, flags, effects);
        return SUCCEEDED(result) ? Present(surface) : result;
    }

    inline HRESULT WINAPI Unlock(IDirectDrawSurface7* object, RECT* rectangle)
    {
        if (!Surfaces.contains(object))
            return Forward<32, HRESULT>(object, rectangle);
        auto& surface = *Surfaces.at(object);
        auto result = Call<32, HRESULT>(surface, rectangle);
        return SUCCEEDED(result) ? Present(surface) : result;
    }

    inline HRESULT WINAPI ReleaseDC(IDirectDrawSurface7* object, HDC dc)
    {
        if (!Surfaces.contains(object))
            return Forward<26, HRESULT>(object, dc);
        auto& surface = *Surfaces.at(object);
        auto result = Call<26, HRESULT>(surface, dc);
        return SUCCEEDED(result) ? Present(surface) : result;
    }

    inline ULONG WINAPI ReleaseDraw(IDirectDraw7* object)
    {
        auto it = Draws.find(object);
        if (it == Draws.end())
            return Forward<2, ULONG>(object);
        const auto references = Call<2, ULONG>(*it->second);
        if (!references)
            Draws.erase(it);
        return references;
    }

    inline HRESULT WINAPI Cooperative(IDirectDraw7* object, HWND window, DWORD flags)
    {
        if (!Draws.contains(object))
            return Forward<20, HRESULT>(object, window, flags);
        auto& draw = *Draws.at(object);
        draw.window = window;
        return Call<20, HRESULT>(draw, window, DWORD{ DDSCL_NORMAL });
    }

    inline HRESULT WINAPI DisplayMode(IDirectDraw7* object, DWORD width, DWORD height, DWORD bits, DWORD refresh, DWORD flags)
    {
        if (!Draws.contains(object))
            return Forward<21, HRESULT>(object, width, height, bits, refresh, flags);
        auto& draw = *Draws.at(object);
        if (!width || !height || width > 16384 || height > 16384)
            return DDERR_INVALIDPARAMS;
        draw.width = width;
        draw.height = height;
        return DD_OK;
    }

    inline HRESULT WINAPI CreateSurface(IDirectDraw7* object, DDSURFACEDESC2* description,
        IDirectDrawSurface7** output, IUnknown* outer)
    {
        if (!Draws.contains(object))
            return Forward<6, HRESULT>(object, description, output, outer);
        auto& draw = *Draws.at(object);
        if (!description || !output || !(description->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE))
            return Call<6, HRESULT>(draw, description, output, outer);
        if (outer)
            return DDERR_INVALIDPARAMS;
        *output = nullptr;
        auto surface = std::make_unique<Surface>();
        surface->window = draw.window;
        DDSURFACEDESC2 primary{};
        primary.dwSize = sizeof(primary);
        primary.dwFlags = DDSD_CAPS;
        primary.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
        auto result = Call<6, HRESULT>(draw, &primary, &surface->primary, static_cast<IUnknown*>(nullptr));
        if (FAILED(result))
            return result;
        result = object->CreateClipper(0, &surface->clipper, nullptr);
        if (SUCCEEDED(result))
            result = surface->clipper->SetHWnd(0, draw.window);
        if (SUCCEEDED(result))
            result = surface->primary->SetClipper(surface->clipper);
        DDSURFACEDESC2 buffer{};
        buffer.dwSize = sizeof(buffer);
        buffer.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
        buffer.dwWidth = draw.width;
        buffer.dwHeight = draw.height;
        buffer.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY;
        buffer.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
        buffer.ddpfPixelFormat.dwFlags = DDPF_RGB;
        buffer.ddpfPixelFormat.dwRGBBitCount = 16;
        buffer.ddpfPixelFormat.dwRBitMask = 0xF800;
        buffer.ddpfPixelFormat.dwGBitMask = 0x07E0;
        buffer.ddpfPixelFormat.dwBBitMask = 0x001F;
        if (SUCCEEDED(result))
            result = Call<6, HRESULT>(draw, &buffer, &surface->object, static_cast<IUnknown*>(nullptr));
        if (SUCCEEDED(result))
            result = Call<6, HRESULT>(draw, &buffer, &surface->back, static_cast<IUnknown*>(nullptr));
        if (FAILED(result))
        {
            if (surface->back)
                surface->back->Release();
            if (surface->object)
                surface->object->Release();
            surface->primary->Release();
            if (surface->clipper)
                surface->clipper->Release();
            return result;
        }
        auto front = surface->object;
        surface->original = Patch(front,
        {
            { 2, reinterpret_cast<void*>(ReleaseSurface) },
            { 5, reinterpret_cast<void*>(Blt) },
            { 11, reinterpret_cast<void*>(Flip) },
            { 12, reinterpret_cast<void*>(Attached) },
            { 26, reinterpret_cast<void*>(ReleaseDC) },
            { 32, reinterpret_cast<void*>(Unlock) }
        });
        Surfaces[front] = std::move(surface);
        *output = front;
        return DD_OK;
    }

    inline HRESULT WINAPI CreateDraw(GUID* device, LPVOID* output, REFIID iid, IUnknown* outer)
    {
        auto result = Create(device, output, iid, outer);
        if (FAILED(result) || !output || !*output)
            return result;
        auto draw = std::make_unique<Draw>();
        draw->object = static_cast<IDirectDraw7*>(*output);
        draw->original = Patch(draw->object,
        {
            { 2, reinterpret_cast<void*>(ReleaseDraw) },
            { 6, reinterpret_cast<void*>(CreateSurface) },
            { 20, reinterpret_cast<void*>(Cooperative) },
            { 21, reinterpret_cast<void*>(DisplayMode) }
        });
        Draws[draw->object] = std::move(draw);
        return result;
    }

    inline bool Install(int mode, int width, int height)
    {
        if (!mode)
            return true;
        auto library = GetModuleHandleW(L"ddraw.dll");
        Create = library ? reinterpret_cast<decltype(Create)>(GetProcAddress(library, "DirectDrawCreateEx")) : nullptr;
        if (!Create)
        {
            OutputDebugStringA("THPS2: DirectDraw window adapter unavailable.\n");
            return false;
        }
        IATHook::Replace(GetModuleHandleW(nullptr), "DDRAW.dll", std::forward_as_tuple("DirectDrawCreateEx", CreateDraw));
        TonyDisplay::Window(mode, width, height);
        return true;
    }
}
