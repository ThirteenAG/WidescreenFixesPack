module;

#include <stdafx.h>
#include <atomic>
#include "FixSettings.hpp"
#include <ddraw.h>
#include "Shared/TonyHawk/Display.hpp"

export module Window;

import ComVars;

/**
* Copyright (C) 2025 Elisha Riedlinger
*
* This software is  provided 'as-is', without any express  or implied  warranty. In no event will the
* authors be held liable for any damages arising from the use of this software.
* Permission  is granted  to anyone  to use  this software  for  any  purpose,  including  commercial
* applications, and to alter it and redistribute it freely, subject to the following restrictions:
*
*   1. The origin of this software must not be misrepresented; you must not claim that you  wrote the
*      original  software. If you use this  software  in a product, an  acknowledgment in the product
*      documentation would be appreciated but is not required.
*   2. Altered source versions must  be plainly  marked as such, and  must not be  misrepresented  as
*      being the original software.
*   3. This notice may not be removed or altered from any source distribution.
*
* DDrawResolutionHack taken from source code found in LegacyD3DResolutionHack
* https://github.com/UCyborg/LegacyD3DResolutionHack
*/

export namespace THPS2D3D7Resolution
{
    inline void Install()
    {
        // Adapted for this game: both Direct3D 7 device paths contain a 2048-pixel check.
        auto module = LoadLibraryExW(L"d3dim700.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!module)
            std::terminate();
        auto device = hook::module_pattern(module,
            "B8 00 08 00 00 39 45 ? 0F 87 ? ? ? ? 39 45 ? 0F 87");
        auto alternate = hook::module_pattern(module,
            "B8 00 08 00 00 39 85 ? ? ? ? 0F 87 ? ? ? ? 39 85 ? ? ? ? 0F 87");
        auto patch = [](void* address)
        {
            injector::WriteMemory<uint32_t>(address, UINT32_MAX, true);
            FlushInstructionCache(GetCurrentProcess(), address, sizeof(uint32_t));
        };
        patch(device.get_first(1));
        patch(alternate.get_first(1));
    }
}

export namespace THPS2ComRelease
{
    inline void Install(HMODULE module = GetModuleHandleW(nullptr))
    {
        // IUnknown::Release returns the remaining reference count, not an HRESULT.
        // Keep each native release, but do not treat a nonzero count as a failure.
        const char* patterns[] = {
            "FF 51 08 8B F0 3B F3 74 19 68 7E 02 00 00",
            "FF 51 08 8B F0 3B F3 74 19 68 9A 02 00 00",
            "FF 52 08 8B F0 3B F3 74 19 68 A3 02 00 00",
            "FF 51 08 3B C3 74 13 68 B1 02 00 00",
            "FF 52 08 8B F0 3B F3 74 19 68 BC 02 00 00"
        };
        for (size_t i = 0; i < std::size(patterns); ++i)
        {
            auto match = hook::module_pattern(module, patterns[i]);
            injector::WriteMemory<uint8_t>(match.get_first(i == 3 ? 5 : 7), 0xEB, true);
        }
    }
}

export namespace THPS2Display
{
    inline std::atomic<int> FrameLimit{ 60 };

    inline void UpdateFrameLimit()
    {
        int limit = FixSettings().Int("MAIN.FPSLimit");
        if (limit == -1)
        {
            DEVMODEW mode{};
            mode.dmSize = sizeof(mode);
            limit = EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &mode) && mode.dmDisplayFrequency > 1
                ? static_cast<int>(mode.dmDisplayFrequency) : 60;
        }
        FrameLimit.store(limit > 0 ? std::clamp(limit, 30, 1000) : 0, std::memory_order_relaxed);
    }

    inline void LimitFrameRate()
    {
        struct Pacer
        {
            HANDLE timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
            double frequency = []() { LARGE_INTEGER value{}; QueryPerformanceFrequency(&value); return static_cast<double>(value.QuadPart); }();
            double next = 0.0;
            int previous = 0;
            ~Pacer() { if (timer) CloseHandle(timer); }
            double Now() const { LARGE_INTEGER value{}; QueryPerformanceCounter(&value); return static_cast<double>(value.QuadPart); }
        };
        static Pacer pacer;
        const int limit = FrameLimit.load(std::memory_order_relaxed);
        if (limit <= 0 || pacer.frequency <= 0.0)
        {
            pacer.next = 0.0;
            pacer.previous = 0;
            return;
        }
        if (pacer.previous != limit)
        {
            pacer.next = 0.0;
            pacer.previous = limit;
        }
        const double period = pacer.frequency / limit;
        const double now = pacer.Now();
        if (pacer.next == 0.0 || now > pacer.next + period)
            pacer.next = now + period;
        const double seconds = (pacer.next - now) / pacer.frequency;
        constexpr double spinTime = 0.0015;
        if (seconds > spinTime)
        {
            LARGE_INTEGER due{};
            due.QuadPart = -static_cast<LONGLONG>((seconds - spinTime) * 10000000.0);
            if (pacer.timer && SetWaitableTimer(pacer.timer, &due, 0, nullptr, nullptr, FALSE))
                WaitForSingleObject(pacer.timer, INFINITE);
            else
                Sleep(static_cast<DWORD>((seconds - spinTime) * 1000.0));
        }
        while (pacer.Now() < pacer.next)
            YieldProcessor();
        pacer.next += period;
    }

    inline void SetWindow(int mode, int width, int height)
    {
        UpdateFrameLimit();
        FixSettings().OnChange("MAIN.FPSLimit", UpdateFrameLimit);
        auto flip = hook::pattern("A1 ? ? ? ? 6A 01 6A 00 50 8B 08 FF 51 2C 8B F0 81 FE C2 01 76 88");
        static auto flipHook = safetyhook::create_mid(flip.get_first(12), [](SafetyHookContext&)
        {
            LimitFrameRate();
        });
        TonyDisplay::Window(mode, width, height);
    }

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
    inline std::array<SafetyHookMid, 2> CacheHooks;

    inline void BorrowAttached(IDirectDrawSurface7* back)
    {
        if (!back)
            return;
        for (const auto& [front, surface] : Surfaces)
        {
            if (surface->back == back)
            {
                // The native cache is borrowed until its front surface is destroyed.
                // Balance GetAttachedSurface's caller reference; the adapter retains its own.
                back->Release();
                return;
            }
        }
    }

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
                injector::WriteMemory<void*>(table + index, replacement, true);
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
            // GetAttachedSurface returns a reference owned by its caller.
            surface.back->AddRef();
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
        auto initial = hook::pattern("68 ? ? ? ? 89 44 24 14 52 89 44 24 1C A1 ? ? ? ? 50 8B 08 FF 51 30");
        auto recreate = hook::pattern("68 ? ? ? ? 89 54 24 18 89 54 24 1C 8B 08 8D 54 24 10 52 50 FF 51 30");
        CachedBack.SetAddress(*initial.get_first<IDirectDrawSurface7**>(1));
        auto borrowed = [](SafetyHookContext& context)
        {
            if (SUCCEEDED(static_cast<HRESULT>(context.eax)))
                BorrowAttached(CachedBack.get());
        };
        CacheHooks[0] = safetyhook::create_mid(initial.get_first(25), borrowed);
        CacheHooks[1] = safetyhook::create_mid(recreate.get_first(24), borrowed);
        IATHook::Replace(GetModuleHandleW(nullptr), "DDRAW.dll", std::forward_as_tuple("DirectDrawCreateEx", CreateDraw));
        TonyDisplay::Window(mode, width, height);
        return true;
    }
}

export void InstallResolution()
{
    hook::pattern pattern("");
    pattern = hook::pattern("A3 ? ? ? ? E8 ? ? ? ? 8B 15 ? ? ? ? A1 ? ? ? ? 6A 10"); //4F3FEE
    dword_29D6FE8.SetAddress(*pattern.get_first<int32_t*>(1));
    dword_29D6FE4.SetAddress(*pattern.get_first<int32_t*>(17));

    struct SetResHook1
    {
        void operator()(injector::reg_pack& regs)
        {
            dword_29D6FE4 = Screen.Width;
            dword_29D6FE8 = Screen.Height;
        }
    }; injector::MakeInline<SetResHook1>(pattern.get_first(0));

    pattern = hook::pattern("A3 ? ? ? ? 8B 0D ? ? ? ? 6A 10 51 50"); //4F54A4
    struct SetResHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            dword_29D6FE4 = Screen.Width;
            dword_29D6FE8 = Screen.Height;
        }
    }; injector::MakeInline<SetResHook2>(pattern.get_first(0));

    pattern = hook::pattern("89 0D ? ? ? ? 8D 34 49"); //46463F
    dword_55ED00.SetAddress(*pattern.get_first<int32_t*>(2));
    pattern = hook::pattern("A3 ? ? ? ? ? ? 8D 04 49 33 D2");
    dword_55ED18.SetAddress(*pattern.get_first<int32_t*>(1));
    pattern = hook::pattern("C7 05 ? ? ? ? 00 10 00 00 5E A3");
    dword_5606CC.SetAddress(*pattern.get_first<int32_t*>(2));
    dword_5606D0.SetAddress(*pattern.get_first<int32_t*>(12));
    struct AspectRatioHook
    {
        void operator()(injector::reg_pack& regs)
        {
            dword_55ED00 = Screen.Width;
            dword_55ED18 = Screen.Height;
            dword_5606D0 = 4096;
            dword_5606CC = 4096;
        }
    };
    pattern = hook::pattern("E8 ? ? ? ? 83 C4 08 E8 ? ? ? ? 5E 5B 83 C4 08 C3"); //0x430466
    injector::MakeInline<AspectRatioHook>(pattern.get_first(0));
    pattern = hook::pattern("E8 ? ? ? ? 83 C4 08 53 56"); //0x4645B8
    injector::MakeInline<AspectRatioHook>(pattern.get_first(0));
}
