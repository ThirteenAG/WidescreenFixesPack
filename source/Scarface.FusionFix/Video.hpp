#pragma once
#include "RTTI.h"
#include <d3d9.h>

namespace ScarfaceVideo
{
    struct Geometry
    {
        int x = 0, y = 0;
        float scaleX = 1.0f, scaleY = 1.0f;
    };

    inline SafetyHookInline ConfigureHook, DrawHook, DestroyHook;
    inline const int* Mode = nullptr;
    inline void** Display = nullptr;
    inline std::unordered_map<void*, float> PixelAspect;

    inline Geometry Layout(int sourceWidth, int sourceHeight, int width, int height, float pixelAspect, int mode)
    {
        if (sourceWidth <= 0 || sourceHeight <= 0 || width <= 0 || height <= 0)
            return {};
        if (!std::isfinite(pixelAspect) || pixelAspect <= 0.0f)
            pixelAspect = 1.0f;
        const double aspect = double(sourceWidth) / sourceHeight * pixelAspect;
        double drawWidth = width, drawHeight = height;
        if (mode != 2)
        {
            const bool matchHeight = mode == 1 ? aspect > double(width) / height : aspect < double(width) / height;
            if (matchHeight)
                drawWidth = height * aspect;
            else
                drawHeight = width / aspect;
        }
        return { int((width - drawWidth) * 0.5), int((height - drawHeight) * 0.5),
            float(drawWidth / sourceWidth), float(drawHeight / sourceHeight) };
    }

    inline void ClearBars(IDirect3DDevice9* device, const Geometry& geometry, int width, int height)
    {
        if (!device)
            return;
        std::array<D3DRECT, 4> bars{};
        DWORD count = 0;
        if (geometry.x > 0)
        {
            bars[count++] = { 0, 0, geometry.x, height };
            bars[count++] = { width - geometry.x, 0, width, height };
        }
        if (geometry.y > 0)
        {
            bars[count++] = { 0, 0, width, geometry.y };
            bars[count++] = { 0, height - geometry.y, width, height };
        }
        if (count)
            device->Clear(count, bars.data(), D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
    }

    inline uintptr_t __fastcall Configure(void* object, void*, unsigned width, unsigned height, int mode, float pixelAspect)
    {
        const auto result = ConfigureHook.unsafe_fastcall<uintptr_t>(object, nullptr, width, height, mode, pixelAspect);
        PixelAspect[object] = pixelAspect;
        return result;
    }

    inline uintptr_t __fastcall Draw(void* object, void*)
    {
        const auto display = Display ? *Display : nullptr;
        if (!display || !Mode)
            return DrawHook.unsafe_fastcall<uintptr_t>(object, nullptr);
        auto& left = *reinterpret_cast<int*>(static_cast<uint8_t*>(object) + 20);
        auto& top = *reinterpret_cast<int*>(static_cast<uint8_t*>(object) + 24);
        const auto sourceWidth = *reinterpret_cast<const int*>(static_cast<const uint8_t*>(object) + 28);
        const auto sourceHeight = *reinterpret_cast<const int*>(static_cast<const uint8_t*>(object) + 32);
        auto& scaleX = *reinterpret_cast<float*>(static_cast<uint8_t*>(object) + 36);
        auto& scaleY = *reinterpret_cast<float*>(static_cast<uint8_t*>(object) + 40);
        const auto width = *reinterpret_cast<const int*>(static_cast<const uint8_t*>(display) + 492);
        const auto height = *reinterpret_cast<const int*>(static_cast<const uint8_t*>(display) + 488);
        if (sourceWidth <= 0 || sourceHeight <= 0 || width <= 0 || height <= 0)
            return DrawHook.unsafe_fastcall<uintptr_t>(object, nullptr);
        const Geometry previous{ left, top, scaleX, scaleY };
        const auto aspect = PixelAspect.find(object);
        const auto geometry = Layout(sourceWidth, sourceHeight, width, height,
            aspect == PixelAspect.end() ? 1.0f : aspect->second, std::clamp(*Mode, 0, 2));
        left = geometry.x;
        top = geometry.y;
        scaleX = geometry.scaleX;
        scaleY = geometry.scaleY;
        ClearBars(*reinterpret_cast<IDirect3DDevice9**>(static_cast<uint8_t*>(display) + 16), geometry, width, height);
        const auto result = DrawHook.unsafe_fastcall<uintptr_t>(object, nullptr);
        left = previous.x;
        top = previous.y;
        scaleX = previous.scaleX;
        scaleY = previous.scaleY;
        return result;
    }

    inline void* __fastcall Destroy(void* object, void*, unsigned char flags)
    {
        PixelAspect.erase(object);
        return DestroyHook.unsafe_fastcall<void*>(object, nullptr, flags);
    }

    inline bool Install(const int& mode, HMODULE module = GetModuleHandle(nullptr))
    {
        Mode = &mode;
        if (ConfigureHook && DrawHook && DestroyHook)
            return true;
        const auto tables = ScarfaceRTTI::ScanVtables(module);
        const auto table = tables.find(".?AVRenderStrategyBink@movie@@");
        if (table == tables.end())
        {
            OutputDebugStringA("Scarface: native movie strategy unavailable.\n");
            return false;
        }
        const auto methods = reinterpret_cast<uintptr_t*>(table->second);
        auto display = hook::range_pattern(methods[7], methods[7] + 128, "8B 0D ? ? ? ? 8B 01 56 57 FF 50 38");
        if (display.size() != 1)
        {
            OutputDebugStringA("Scarface: native movie display unavailable.\n");
            return false;
        }
        Display = *display.get_first<void**>(2);
        ConfigureHook = safetyhook::create_inline(reinterpret_cast<void*>(methods[7]), Configure);
        DrawHook = safetyhook::create_inline(reinterpret_cast<void*>(methods[9]), Draw);
        DestroyHook = safetyhook::create_inline(reinterpret_cast<void*>(methods[3]), Destroy);
        if (!ConfigureHook || !DrawHook || !DestroyHook)
        {
            ConfigureHook.reset();
            DrawHook.reset();
            DestroyHook.reset();
            OutputDebugStringA("Scarface: native movie hooks unavailable.\n");
            return false;
        }
        return true;
    }
}
