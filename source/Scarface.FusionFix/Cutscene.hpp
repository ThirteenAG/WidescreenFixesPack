#pragma once
#include "RTTI.h"
#include <d3d9.h>

namespace ScarfaceCutscene
{
    struct Framing
    {
        int left = 0, top = 0;
        float projection = 1.0f;
    };

    inline SafetyHookInline UpdateHook, DestroyHook;
    inline const int* Borders = nullptr;
    inline uint8_t* Effects = nullptr;
    inline void** Display = nullptr;
    inline bool Ready = false, DrawReady = false, Active = false;
    inline Framing Current;

    inline Framing Layout(int width, int height, float visible, int mode)
    {
        if (width <= 0 || height <= 0 || !std::isfinite(visible) || visible <= 0.0f || visible > 1.0f)
            return {};
        const float aspect = float(width) / height;
        const float vertical = (mode & 1) ? 1.0f : visible;
        const float projection = (mode & 2) ? vertical : std::min(vertical, (4.0f / 3.0f) / aspect);
        const float imageAspect = (mode & 1) ? 4.0f / 3.0f : (4.0f / 3.0f) / visible;
        return { (mode & 2) ? int(std::ceil(std::max(0.0f, (width - height * imageAspect) * 0.5f) - 0.001f)) : 0,
            (mode & 1) ? int(std::ceil(std::max(0.0f, height * (1.0f - visible) * 0.5f) - 0.001f)) : 0, projection };
    }

    inline bool Visible(void* box)
    {
        if (!box)
            return false;
        auto drawable = *reinterpret_cast<const uint8_t* const*>(static_cast<const uint8_t*>(box) + 32);
        return drawable && (drawable[44] & 0x40);
    }

    inline void SetVisible(void* box, bool visible)
    {
        const auto methods = *reinterpret_cast<uintptr_t**>(box);
        reinterpret_cast<void(__thiscall*)(void*, bool)>(methods[17])(box, visible);
    }

    inline float Adjust(const uint8_t* manager, const uint8_t* source, float fov)
    {
        Active = false;
        const auto display = Display ? *Display : nullptr;
        if (!Ready || !DrawReady || !Borders || !Effects || !display || !manager || !source || manager[254] ||
            !std::isfinite(fov) || fov <= 0.0f || fov >= 3.141592654f)
            return fov;
        const int state = *reinterpret_cast<const int*>(manager + 232);
        if (state != 7 && state != 8)
            return fov;
        auto camera = *reinterpret_cast<const uint8_t* const*>(manager + 196 + state * 4);
        if (!camera || *reinterpret_cast<const uint8_t* const*>(camera + 132) != source)
            return fov;
        const int effectState = *reinterpret_cast<const int*>(Effects + 108);
        auto upper = *reinterpret_cast<const uint8_t* const*>(Effects + 124);
        auto lower = *reinterpret_cast<const uint8_t* const*>(Effects + 128);
        if (effectState < 1 || effectState > 3 || !Visible(const_cast<uint8_t*>(upper)) || !Visible(const_cast<uint8_t*>(lower)))
            return fov;
        const auto width = *reinterpret_cast<const int*>(static_cast<const uint8_t*>(display) + 492);
        const auto height = *reinterpret_cast<const int*>(static_cast<const uint8_t*>(display) + 488);
        if (width <= 0 || height <= 0)
            return fov;
        const int upperEdge = *reinterpret_cast<const int16_t*>(upper + 50);
        const int lowerEdge = *reinterpret_cast<const int16_t*>(lower + 50) + *reinterpret_cast<const int16_t*>(lower + 46);
        // The native layer centers its 640x480 coordinates and applies the 448/480 safe area.
        if (upperEdge <= lowerEdge || std::abs(upperEdge + lowerEdge - 480) > 2)
            return fov;
        const float scale = std::min(width / 640.0f, height / 480.0f) * (448.0f / 480.0f);
        const float visible = (upperEdge - lowerEdge) * scale / height;
        if (!std::isfinite(visible) || visible <= 0.0f || visible >= 1.0f)
            return fov;
        Current = Layout(width, height, visible, std::clamp(*Borders, 0, 3));
        Active = true;
        return Current.projection == 1.0f ? fov : 2.0f * std::atan(std::tan(fov * 0.5f) * Current.projection);
    }

    inline void __fastcall Update(void* object, void*, const void* time)
    {
        Effects = static_cast<uint8_t*>(object);
        UpdateHook.unsafe_fastcall(object, nullptr, time);
    }

    inline void* __fastcall Destroy(void* object, void*, unsigned char flags)
    {
        if (Effects == object)
        {
            Effects = nullptr;
            Active = false;
        }
        return DestroyHook.unsafe_fastcall<void*>(object, nullptr, flags);
    }

    class DrawScope
    {
        void* upper = nullptr;
        void* lower = nullptr;

    public:
        explicit DrawScope(void* display)
        {
            if (!display || !Active || !Effects || !Display || *Display != display)
                return;
            auto device = *reinterpret_cast<IDirect3DDevice9**>(static_cast<uint8_t*>(display) + 16);
            const auto width = *reinterpret_cast<const int*>(static_cast<uint8_t*>(display) + 492);
            const auto height = *reinterpret_cast<const int*>(static_cast<uint8_t*>(display) + 488);
            std::array<D3DRECT, 4> bars{};
            DWORD count = 0;
            if (Current.left > 0)
            {
                bars[count++] = { 0, 0, Current.left, height };
                bars[count++] = { width - Current.left, 0, width, height };
            }
            if (Current.top > 0)
            {
                bars[count++] = { 0, 0, width, Current.top };
                bars[count++] = { 0, height - Current.top, width, height };
            }
            if (count)
            {
                D3DVIEWPORT9 previous{};
                if (!device || FAILED(device->GetViewport(&previous)))
                    return;
                const D3DVIEWPORT9 full{ 0, 0, DWORD(width), DWORD(height), 0.0f, 1.0f };
                if (FAILED(device->SetViewport(&full)))
                    return;
                const auto result = device->Clear(count, bars.data(), D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
                device->SetViewport(&previous);
                if (FAILED(result))
                    return;
            }
            auto top = *reinterpret_cast<void**>(Effects + 124);
            auto bottom = *reinterpret_cast<void**>(Effects + 128);
            if (Visible(top))
            {
                upper = top;
                SetVisible(upper, false);
            }
            if (Visible(bottom))
            {
                lower = bottom;
                SetVisible(lower, false);
            }
        }

        DrawScope(const DrawScope&) = delete;
        DrawScope& operator=(const DrawScope&) = delete;

        ~DrawScope()
        {
            if (upper)
                SetVisible(upper, true);
            if (lower)
                SetVisible(lower, true);
        }
    };

    inline bool Install(const int& borders, bool cameraReady, HMODULE module = GetModuleHandle(nullptr))
    {
        Borders = &borders;
        if (Ready)
            return true;
        const auto tables = ScarfaceRTTI::ScanVtables(module);
        const auto effects = tables.find(".?AVHUDScreenFX@@");
        const auto movie = tables.find(".?AVRenderStrategyBink@movie@@");
        if (!cameraReady || effects == tables.end() || movie == tables.end())
        {
            OutputDebugStringA("Scarface: cutscene framing dependencies unavailable.\n");
            return false;
        }
        const auto movieConfigure = reinterpret_cast<uintptr_t*>(movie->second)[7];
        auto display = hook::range_pattern(movieConfigure, movieConfigure + 128, "8B 0D ? ? ? ? 8B 01 56 57 FF 50 38");
        if (display.size() != 1)
        {
            OutputDebugStringA("Scarface: cutscene display unavailable.\n");
            return false;
        }
        Display = *display.get_first<void**>(2);
        const auto methods = reinterpret_cast<uintptr_t*>(effects->second);
        UpdateHook = safetyhook::create_inline(reinterpret_cast<void*>(methods[54]), Update);
        DestroyHook = safetyhook::create_inline(reinterpret_cast<void*>(methods[1]), Destroy);
        Ready = UpdateHook && DestroyHook;
        if (!Ready)
        {
            UpdateHook.reset();
            DestroyHook.reset();
            OutputDebugStringA("Scarface: cutscene framing hooks unavailable.\n");
        }
        return Ready;
    }
}
