module;

#include <stdafx.h>
#include "FixSettings.hpp"

// Cinematic framing. The game shows its realtime cutscenes in a band of a 4:3 screen: either 0.75 of its height with
// the camera height reduced to match (a 16:9 band), or 0.567 of it as a plain crop (a 2.35:1 band). The band is the
// director's framing; it fills the screen at any aspect, with bars where the screen has more room than the band.

export module Cinematics;

import ComVars;
import Layout;

export namespace NeoCinema
{
    using QuadFunction = void(__thiscall*)(void* renderer, const float* first, const float* last, void* material,
        float u0, float v0, float u1, float v1);

    inline SafetyHookInline BarsHook;
    inline SafetyHookMid BandHook, AreaHook;
    inline QuadFunction Quad = nullptr;
    inline void* BarMaterial = nullptr;
    inline std::atomic<int> Borders{ 3 };
    inline std::atomic<bool> MovieSeen{ false };
    inline bool Movie = false;  // a movie was drawn in the last frame: it brings its own framing

    struct Band
    {
        bool Active = false;
        float Aspect = 16.0f / 9.0f;  // the director's framing
        float CameraScale = 1.0f;     // camera height of the band relative to the visible band height
    };

    inline Band Current(uintptr_t camera)
    {
        if (!camera || !WideRatio.is_initialized() || !CropRatio.is_initialized() || !WideCamera.is_initialized())
            return {};
        if (*reinterpret_cast<int*>(camera + 0x4C8))
            return { true, NeoLayout::DesignAspect / WideRatio.get(), WideCamera.get() / WideRatio.get() };
        if (*reinterpret_cast<int*>(camera + 0x4C4))
            return { true, NeoLayout::DesignAspect / CropRatio.get(), 1.0f / CropRatio.get() };
        return {};
    }

    // Height of the visible band in pixels: the screen height, or less where the screen is narrower than the band and
    // a border mode keeps the director's horizontal framing.
    inline float VisibleHeight(const Band& band)
    {
        if (Borders.load() != 0 && Screen.fAspectRatio < band.Aspect)
            return Screen.fWidth / band.Aspect;
        return Screen.fHeight;
    }

    // Camera height for the lens: the band's vertical field of view spans the visible band.
    inline int CameraHeight(const Band& band)
    {
        return std::max(1, static_cast<int>(std::lround(VisibleHeight(band) * band.CameraScale)));
    }

    // Fractions of the screen covered by each top/bottom bar and each side bar
    inline float LetterboxBar(const Band& band)
    {
        if (!(Borders.load() & 1) || Screen.fAspectRatio >= band.Aspect || Screen.fHeight <= 0.0f)
            return 0.0f;
        return std::max(0.0f, (1.0f - VisibleHeight(band) / Screen.fHeight) * 0.5f);
    }

    inline float PillarboxBar(const Band& band)
    {
        if (!(Borders.load() & 2) || Screen.fAspectRatio <= band.Aspect)
            return 0.0f;
        return (1.0f - band.Aspect / Screen.fAspectRatio) * 0.5f;
    }

    inline Band ActiveBand()
    {
        if (Movie)
            return {};
        return Current(ActiveCamera.is_initialized() ? ActiveCamera.get() : 0);
    }

    // The bar quads drawn over the picture
    inline void SetBars(SafetyHookContext& context)
    {
        const auto band = ActiveBand();
        if (band.Active)
            BarHeight = LetterboxBar(band);
    }

    // The area the post-processing passes copy to the screen in the 16:9 band; the rest stays black
    inline void SetArea(SafetyHookContext& context)
    {
        const auto camera = ActiveCamera.is_initialized() ? ActiveCamera.get() : 0;
        const auto band = Movie ? Band{} : Current(camera);
        auto area = reinterpret_cast<float*>(context.eax);
        if (!band.Active || !*reinterpret_cast<int*>(camera + 0x4C8) ||
            *reinterpret_cast<uint8_t*>(context.esp + 0x28) || area[2] <= 0.0f || area[1] <= 0.0f)
            return;
        const float width = area[2], height = area[3] + area[1];
        const float top = LetterboxBar(band), side = PillarboxBar(band);
        area[0] = width * side;
        area[1] = height * top;
        area[2] = width * (1.0f - side);
        area[3] = height * (1.0f - top);
    }

    inline void __cdecl DrawBars()
    {
        BarsHook.ccall<void>();
        Movie = MovieSeen.exchange(false);
        const auto band = ActiveBand();
        if (!band.Active || !(Borders.load() & 2) || Screen.fAspectRatio <= band.Aspect || !CinemaRenderer.is_initialized() || !CinemaRenderer.get())
            return;
        const float side = (1.0f - band.Aspect / Screen.fAspectRatio) * 0.5f;
        const float left[2]{ 0.0f, 0.0f }, leftEnd[2]{ side, 1.0f };
        const float right[2]{ 1.0f - side, 0.0f }, rightEnd[2]{ 1.0f, 1.0f };
        Quad(CinemaRenderer.get(), left, leftEnd, BarMaterial, 0.0f, 0.0f, 1.0f, 1.0f);
        Quad(CinemaRenderer.get(), right, rightEnd, BarMaterial, 0.0f, 0.0f, 1.0f, 1.0f);
    }

    inline void Install()
    {
        Borders = std::clamp(FixSettings().Int("MAIN.CutsceneBorders"), 0, 3);
        FixSettings().OnChange("MAIN.CutsceneBorders", []()
        {
            Borders = std::clamp(FixSettings().Int("MAIN.CutsceneBorders"), 0, 3);
        });

        auto bars = hook::pattern("A1 ? ? ? ? D9 05 ? ? ? ? 83 EC 10 85 C0 56 74 26 8B 88 C8 04 00 00 85 C9 74 0A DD D8 D9 05 ? ? ? ? EB 12 8B 88 C4 04 00 00 85 C9 74 08 DD D8 D9 05 ? ? ? ? D9 05 ? ? ? ? 8B 35 ? ? ? ? 8B 06 D8 E1 6A 00 8B CE D8 0D ? ? ? ? D9 1D ? ? ? ? DD D8");
        auto camera = hook::pattern("DB 05 ? ? ? ? D8 0D ? ? ? ? E8 ? ? ? ? 8B 0D ? ? ? ? 89 41 4C 8B C6");
        auto area = hook::pattern("8D 44 24 10 50 8B CE FF 92 C4 00 00 00 5F 5E 5B 83 C4 14 C2 04 00");
        auto quad = hook::pattern("68 ? ? ? ? 8D 44 24 18 50 8D 4C 24 24 51 8B CE E8");
        const auto code = bars.get_first<uint8_t>();
        ActiveCamera.SetAddress(*reinterpret_cast<uintptr_t**>(code + 1));
        WideRatio.SetAddress(*reinterpret_cast<float**>(code + 33));
        CropRatio.SetAddress(*reinterpret_cast<float**>(code + 53));
        CinemaRenderer.SetAddress(*reinterpret_cast<void***>(code + 65));
        BarHeight.SetAddress(*reinterpret_cast<float**>(code + 85));
        WideCamera.SetAddress(*camera.get_first<float*>(8));
        BarMaterial = *quad.get_first<void*>(1);
        Quad = reinterpret_cast<QuadFunction>(injector::GetBranchDestination(quad.get_first(17)).as_int());
        BandHook = safetyhook::create_mid(code + 89, SetBars);
        AreaHook = safetyhook::create_mid(area.get_first(5), SetArea);
        if (BandHook)
        {
            BarsHook = safetyhook::create_inline(code, DrawBars, SafetyHookInline::StartDisabled);
            (void)BarsHook.enable();
        }
    }
}
