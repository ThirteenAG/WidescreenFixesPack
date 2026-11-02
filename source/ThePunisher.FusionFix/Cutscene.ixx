module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Cutscene;

import ComVars;

export namespace Cutscene
{
    // In-engine cutscenes show a 16:9 window: the full width of the original 4:3 view with bars above and below.
    constexpr float Aspect = 16.0f / 9.0f;
    bool(__cdecl* Active)() = nullptr;
    bool(__cdecl* Bars)() = nullptr;
    void(__cdecl* Colour)(int, int, int, int) = nullptr;
    int(__cdecl* Fill)(int, int, int, int, int, int) = nullptr;
    uint32_t AnimatedOffset = 0;

    enum Borders { Off, Letterbox, Pillarbox, Both };

    int Mode()
    {
        static const auto& mode = FixSettings().Int("MAIN.CutsceneBorders");
        return std::clamp(mode, 0, 3);
    }

    // 0 outside cutscenes, 1 while the cutscene frame is fully shown, in between while the bars slide.
    float Amount()
    {
        if (!Active || !Active() || !Bars())
            return 0.0f;
        const auto data = Data.get();
        if (!data || !*reinterpret_cast<uint8_t*>(data + AnimatedOffset))
            return 1.0f;
        const float slide = std::isfinite(Slide.get()) ? std::clamp(Slide.get(), 0.0f, 1.0f) : 0.0f;
        static const auto& animated = FixSettings().Int("MAIN.AnimatedCutsceneBorders");
        return animated ? slide : (slide > 0.0f ? 1.0f : 0.0f);
    }

    bool LetterboxShown(float aspect)
    {
        return aspect < Aspect && (Mode() & Letterbox);
    }

    bool PillarboxShown(float aspect)
    {
        return aspect > Aspect && (Mode() & Pillarbox);
    }

    float FieldOfView(float fov, float aspect)
    {
        if (LetterboxShown(aspect))
            return fov;
        if (PillarboxShown(aspect))
            return AdjustFOV(fov, aspect, Aspect);
        return AdjustFOV(fov, aspect);
    }

    void DrawBars()
    {
        const float amount = Amount();
        if (amount <= 0.0f || Screen.Width <= 0 || Screen.Height <= 0)
            return;
        Colour(0, 0, 0, 255);
        if (LetterboxShown(Screen.fAspectRatio))
        {
            const auto bar = static_cast<int>((Screen.fHeight - Screen.fWidth / Aspect) * 0.5f * amount);
            if (bar > 0)
            {
                Fill(0, 0, Screen.Width, bar, FillMode.get(), FillFlags.get());
                Fill(0, Screen.Height - bar, Screen.Width, bar, FillMode.get(), FillFlags.get());
            }
        }
        else if (PillarboxShown(Screen.fAspectRatio))
        {
            const auto bar = static_cast<int>((Screen.fWidth - Screen.fHeight * Aspect) * 0.5f * amount);
            if (bar > 0)
            {
                Fill(0, 0, bar, Screen.Height, FillMode.get(), FillFlags.get());
                Fill(Screen.Width - bar, 0, bar, Screen.Height, FillMode.get(), FillFlags.get());
            }
        }
    }

    void Init()
    {
        auto block = hook::pattern("E8 ? ? ? ? 84 C0 0F 84 BD 00 00 00 E8 ? ? ? ? 84 C0 0F 84 ? ? ? ? E8 ? ? ? ? 89 44 24 20 DB 44 24 20 D8 0D ? ? ? ? E8 ? ? ? ? 68 FF 00 00 00 6A 00 6A 00 6A 00 8B F0 E8 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 8B 0D ? ? ? ? 8A 81 ? ? ? ? ? ? ? ? ? ? ? ? D8 0D ? ? ? ? ? ? ? ? ? ? ? 8B 15 ? ? ? ? A1 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? E8").get_first<uint8_t>();
        const auto end = block + 0xCA;
        Bars = reinterpret_cast<decltype(Bars)>(injector::GetBranchDestination(block + 0x0D, true).get<void>());
        Colour = reinterpret_cast<decltype(Colour)>(injector::GetBranchDestination(block + 0x3F, true).get<void>());
        Fill = reinterpret_cast<decltype(Fill)>(injector::GetBranchDestination(block + 0x9B, true).get<void>());
        Data.SetAddress(*reinterpret_cast<uintptr_t**>(block + 0x64));
        AnimatedOffset = *reinterpret_cast<uint32_t*>(block + 0x6A);
        Slide.SetAddress(*reinterpret_cast<float**>(block + 0x78));
        FillFlags.SetAddress(*reinterpret_cast<int32_t**>(block + 0x85));
        FillMode.SetAddress(*reinterpret_cast<int32_t**>(block + 0x8A));
        Active = reinterpret_cast<decltype(Active)>(injector::GetBranchDestination(block, true).get<void>());

        struct BarsHook
        {
            void operator()(injector::reg_pack&)
            {
                DrawBars();
            }
        }; injector::MakeInline<BarsHook>(block, end);
    }
}
