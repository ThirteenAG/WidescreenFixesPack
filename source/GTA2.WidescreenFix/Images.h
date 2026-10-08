#pragma once
#include <ddraw.h>

namespace GTA2Images
{
    inline void __stdcall DetectDirectX(DWORD* version, DWORD* platform)
    {
        *version = 0; *platform = 2;
        // The old version probe creates a primary surface with a null HWND.
        // Query the interface used by the renderer without taking display ownership.
        auto library = LoadLibraryW(L"ddraw.dll");
        if (!library) return;
        using Create = HRESULT (WINAPI*)(GUID*, LPDIRECTDRAW*, IUnknown*);
        auto create = reinterpret_cast<Create>(GetProcAddress(library, "DirectDrawCreate"));
        IDirectDraw* draw = nullptr;
        if (create && SUCCEEDED(create(nullptr, &draw, nullptr)))
        {
            constexpr GUID iid = {0x9c59509a, 0x39bd, 0x11d1, {0x8c,0x4a,0x00,0xc0,0x4f,0xd9,0x30,0xc5}};
            IDirectDraw4* draw4 = nullptr;
            if (SUCCEEDED(draw->QueryInterface(iid, reinterpret_cast<void**>(&draw4))))
            { *version = 0x601; draw4->Release(); }
            draw->Release();
        }
        FreeLibrary(library);
    }

    struct Image { DWORD used, width, height; IDirectDrawSurface4* surface; };
    static_assert(sizeof(Image) == 16);
    inline Image** Images = nullptr;
    inline int* Count = nullptr;
    inline SafetyHookInline LoadHook, BlitHook;
    // The original 1600-row table is immediately followed by palette globals.
    // Large framebuffers otherwise overwrite the alpha conversion masks.
    inline std::array<uintptr_t, 8192> ScreenRows{};

    inline int8_t __stdcall Blit(int slot, int left, int top, int right, int bottom, int x, int y)
    {
        auto result = BlitHook.stdcall<int8_t>(slot, left, top, right, bottom, x, y);
        static unsigned samples = 0;
        if (Log::Enabled && result && samples++ < 8)
        {
            auto video = *GTA2Movies::Video;
            auto surface = *reinterpret_cast<IDirectDrawSurface4**>(video + 312);
            DDSURFACEDESC2 desc{}; desc.dwSize = sizeof(desc);
            if (surface) surface->GetSurfaceDesc(&desc);
            char message[192]{};
            sprintf_s(message, "GTA2 image blit: result=%d slot=%d target=%lux%lu viewport=%dx%d", result, slot,
                desc.dwWidth, desc.dwHeight, *reinterpret_cast<int*>(video + 72), *reinterpret_cast<int*>(video + 76));
            Log::Write(message);
        }
        return result;
    }

    inline DWORD Channel(unsigned value, DWORD mask)
    {
        if (!mask) return 0;
        unsigned shift = 0;
        while (!(mask & 1)) { mask >>= 1; ++shift; }
        return ((value * mask + 15) / 31) << shift;
    }

    inline int __stdcall Load(const uint8_t* tga)
    {
        int slot = 0;
        while (slot < *Count && (*Images)[slot].used) ++slot;
        if (slot == *Count) return -1;
        if (!tga || tga[1] || tga[2] != 2 || tga[16] != 16) return -2;
        unsigned width = tga[12] | (tga[13] << 8), height = tga[14] | (tga[15] << 8);
        if (!width || !height || width > 4096 || height > 4096) return -2;
        auto video = *GTA2Movies::Video;
        auto draw = *reinterpret_cast<IDirectDraw4**>(video + 288);
        DDSURFACEDESC2 desc{};
        desc.dwSize = sizeof(desc);
        desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
        desc.dwWidth = width; desc.dwHeight = height;
        desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
        if (*reinterpret_cast<int*>(video + 460) < 0) desc.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
        IDirectDrawSurface4* surface = nullptr;
        if (FAILED(draw->CreateSurface(&desc, &surface, nullptr))) return -3;
        DDSURFACEDESC2 locked{};
        locked.dwSize = sizeof(locked);
        if (FAILED(surface->Lock(nullptr, &locked, DDLOCK_WAIT | DDLOCK_WRITEONLY, nullptr)))
        { surface->Release(); return -3; }
        const auto& format = locked.ddpfPixelFormat;
        unsigned bytes = format.dwRGBBitCount / 8;
        bool valid = (format.dwFlags & DDPF_RGB) && bytes >= 2 && bytes <= 4 &&
            locked.lpSurface && locked.lPitch >= int(width * bytes);
        if (valid)
        {
            auto pixels = tga + 18 + tga[0];
            for (unsigned y = 0; y < height; ++y)
            {
                unsigned dy = (tga[17] & 0x20) ? y : height - 1 - y;
                auto row = static_cast<uint8_t*>(locked.lpSurface) + dy * locked.lPitch;
                for (unsigned x = 0; x < width; ++x, pixels += 2)
                {
                    unsigned color = pixels[0] | (pixels[1] << 8);
                    DWORD output = Channel((color >> 10) & 31, format.dwRBitMask) |
                        Channel((color >> 5) & 31, format.dwGBitMask) |
                        Channel(color & 31, format.dwBBitMask) | format.dwRGBAlphaBitMask;
                    unsigned dx = (tga[17] & 0x10) ? width - 1 - x : x;
                    memcpy(row + dx * bytes, &output, bytes);
                }
            }
        }
        HRESULT unlocked = surface->Unlock(nullptr);
        if (!valid || FAILED(unlocked)) { surface->Release(); return -3; }
        (*Images)[slot] = {1, width, height, surface};
        return slot;
    }

    inline void Install(HMODULE module)
    {
        auto load = hook::module_pattern(module, "81 EC 5C 01 00 00 8B 0D ? ? ? ? 8B 15 ? ? ? ? 53 55 33 DB 56 85 C9 57");
        if (load.size() != 1) { Log::Write("GTA2 image loader signature unavailable."); return; }
        Count = *load.get_first<int*>(8);
        Images = *load.get_first<Image**>(14);
        // Match the surface's actual format, preserving DirectDraw's blit compatibility.
        LoadHook = safetyhook::create_inline(load.get_first(), Load);
        auto blit = hook::module_pattern(module, "8B 44 24 04 8B 0D ? ? ? ? 83 EC 20 3B C1 53 55 56 57 0F 8F");
        if (blit.size() == 1) BlitHook = safetyhook::create_inline(blit.get_first(), Blit);
        auto rows = hook::module_pattern(module, "8B 54 24 0C 56 8B 74 24 0C 89 15 ? ? ? ? 85 D2 76 13 8B 4C 24 08 B8 ? ? ? ? 89 08 03 CE 83 C0 04 4A 75 F6");
        auto bitmapRows = hook::module_pattern(module, "8B 55 EC 8B 04 95 ? ? ? ? 8B 4D F0 8D 14 48");
        if (rows.size() == 1 && bitmapRows.size() == 1 &&
            *rows.get_first<uintptr_t>(24) == *bitmapRows.get_first<uintptr_t>(6))
        {
            injector::WriteMemory(rows.get_first(24), uintptr_t(ScreenRows.data()), true);
            injector::WriteMemory(bitmapRows.get_first(6), uintptr_t(ScreenRows.data()), true);
            Log::Write("GTA2 screen-row table relocated for large framebuffers.");
        }
        else Log::Write("GTA2 screen-row table signatures unavailable.");
    }
}
