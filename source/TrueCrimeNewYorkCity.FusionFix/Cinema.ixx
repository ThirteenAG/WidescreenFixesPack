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
    std::atomic<ULONGLONG> LastDrawn{ 0 };
    std::atomic<float> Picture{ 640.0f / 344.0f };

    constexpr float FullHeight = 68.0f;
    constexpr float NarrowHeight = 16.0f;

    bool Active()
    {
        return GetTickCount64() - LastDrawn.load(std::memory_order_relaxed) < 250;
    }

    // The world camera's horizontal projection scale: Hor+, or in Both the original horizontal view across the
    // picture between the bars
    float Scale(float scale, float width, float height)
    {
        if (width <= 0.0f || height <= 0.0f)
            return scale;
        if (!Active() || FixSettings().Int("MAIN.CutsceneBorders") != 3)
            return scale * height * (4.0f / 3.0f) / width;
        const float picture = std::min(width, height * Picture.load(std::memory_order_relaxed));
        return scale * picture / width;
    }

    // The top bar's rectangle (0, 0, width, bar height in pixels); the bottom bar's call is skipped and every bar
    // is drawn here
    int __fastcall Draw(void* batch, void*, float left, float top, float right, float bottom, uint32_t color, float depth)
    {
        LastDrawn.store(GetTickCount64(), std::memory_order_relaxed);
        const bool narrow = pGameSettings && pGameSettings[0x1010];
        const float full = narrow ? NarrowHeight : FullHeight;
        const float aspect = (4.0f / 3.0f) * 480.0f / (480.0f - 2.0f * full);
        Picture.store(aspect, std::memory_order_relaxed);

        float progress = std::clamp(bottom / full, 0.0f, 1.0f);
        if (!FixSettings().Bool("MAIN.AnimatedCutsceneBorders"))
            progress = Bars && *reinterpret_cast<int*>(Bars + 20) == 2 ? 0.0f : 1.0f;

        const float width = right, height = float(nResolutionHeight);
        float bar = 0.0f, side = 0.0f;
        switch (FixSettings().Int("MAIN.CutsceneBorders"))
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
