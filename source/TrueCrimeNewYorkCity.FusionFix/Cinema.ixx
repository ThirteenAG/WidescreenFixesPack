module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Cinema;

import ComVars;

// Cinematic bars of cutscenes and the title scenes. The game slides two bars of up to 68 pixels in, a size meant for
// 480 lines, which leaves a 640:344 picture of the 4:3 frame. MAIN.CutsceneBorders: Both keeps that picture with its
// original horizontal view at any aspect ratio (bars above and below or at the sides), Letterbox draws the original
// bars at the screen's height (the wider view at the sides stays), Pillarbox limits the view to the 4:3 frame without
// the bars, Off shows the whole screen.
export namespace TCCinema
{
    using Rectangle = int(__thiscall*)(void*, float, float, float, float, uint32_t, float);
    Rectangle DrawRectangle = nullptr;
    uint8_t* Bars = nullptr;          // the bars' object while it draws: state at +20
    std::atomic<int> Borders{ 0 };
    std::atomic_bool Animated{ true };

    constexpr float FullHeight = 68.0f;
    constexpr float NarrowHeight = 16.0f;

    bool Active()
    {
        if (bCutscene) return true;
        if (!pGameSettings) return false;
        // The game links the cinematic bars into its draw list when a scripted view starts. State 2 is the exit.
        const auto* bars = *reinterpret_cast<uint8_t* const*>(pGameSettings.get() + 0x2A4C);
        return bars && *reinterpret_cast<const uintptr_t*>(bars) && *reinterpret_cast<const int*>(bars + 20) != 2;
    }

    float BarHeight()
    {
        return pGameSettings && pGameSettings[0x1010] ? NarrowHeight : FullHeight;
    }

    float PictureAspect(float barHeight)
    {
        return (4.0f / 3.0f) * 480.0f / (480.0f - 2.0f * barHeight);
    }

    // The world camera's horizontal projection scale: Hor+, or in Both the original horizontal view across the
    // picture between the bars
    float Scale(float scale, float width, float height)
    {
        if (width <= 0.0f || height <= 0.0f)
            return scale;
        if (Borders.load(std::memory_order_relaxed) != 3 || !Active())
            return scale * height * (4.0f / 3.0f) / width;
        const float picture = std::min(width, height * PictureAspect(BarHeight()));
        return scale * picture / width;
    }

    // The top bar's rectangle (0, 0, width, bar height in pixels); the bottom bar's call is skipped and every bar
    // is drawn here
    int __fastcall Draw(void* batch, void*, float left, float top, float right, float bottom, uint32_t color, float depth)
    {
        const float full = BarHeight();
        const float aspect = PictureAspect(full);

        float progress = std::clamp(bottom / full, 0.0f, 1.0f);
        if (!Animated.load(std::memory_order_relaxed))
            progress = Bars && *reinterpret_cast<int*>(Bars + 20) == 2 ? 0.0f : 1.0f;

        const float width = right, height = float(nResolutionHeight);
        float bar = 0.0f, side = 0.0f;
        switch (Borders.load(std::memory_order_relaxed))
        {
            case 1:
                bar = height * full / 480.0f;
                break;
            case 2:
                side = std::max(0.0f, (width - height * (4.0f / 3.0f)) * 0.5f);
                break;
            case 3:
            {
                const float picture = std::min(width, height * aspect);
                side = (width - picture) * 0.5f;
                bar = std::max(0.0f, (height - picture / aspect) * 0.5f);
                break;
            }
        }
        bar *= progress;
        side *= progress;
        int result = 0;
        if (bar >= 0.5f)
        {
            result = DrawRectangle(batch, left, top, width, bar, color, depth);
            DrawRectangle(batch, left, height - bar, width, height, color, depth);
        }
        if (side >= 0.5f)
        {
            DrawRectangle(batch, left, top, side, height, color, depth);
            DrawRectangle(batch, width - side, top, width, height, color, depth);
        }
        return result;
    }

    int __fastcall Skip(void*, void*, float, float, float, float, uint32_t, float)
    {
        return 0;
    }

    void Install()
    {
        auto& settings = FixSettings();
        const auto publish = []()
        {
            static std::mutex mutex;
            const std::lock_guard lock(mutex);
            Borders.store(ReadNumericSetting("MAIN.CutsceneBorders", 3), std::memory_order_relaxed);
            Animated.store(FixSettings().Get("MAIN.AnimatedCutsceneBorders") == "1", std::memory_order_relaxed);
        };
        settings.OnChange("MAIN.CutsceneBorders", publish);
        settings.OnChange("MAIN.AnimatedCutsceneBorders", publish);
        publish();
        static auto object = safetyhook::create_mid(hook::get_pattern("56 8B F1 8B 46 14 83 E8 01 57 74 45", 3),
            [](SafetyHookContext& regs) { Bars = reinterpret_cast<uint8_t*>(regs.esi); });
        pGameSettings.SetAddress(*hook::pattern("8B 0D ? ? ? ? 80 B9 10 10 00 00 00 F3 0F 10 46 18").get_first<uint8_t**>(2));
        auto top = hook::pattern("F7 DE E8 ? ? ? ? 68 00 00 80 3F B9 00 00 00 80");
        DrawRectangle = reinterpret_cast<Rectangle>(injector::MakeCALL(top.get_first(2), Draw, true).as_int());
        injector::MakeCALL(hook::get_pattern("8D 4C 24 34 E8 ? ? ? ? 8D 4C 24 1C C7 84 24", 4), Skip, true);
    }
}

class Cinema
{
public:
    Cinema()
    {
        WFP::onInitEvent() += []()
        {
            TCCinema::Install();
        };
    }
} Cinema;
