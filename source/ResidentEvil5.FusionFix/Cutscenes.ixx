module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <atomic>

export module Cutscenes;

// In-engine cutscenes. The game shows them in its cinema layout (game mode 7): on a 16:9 screen a full-width view with
// bars above and below (a 2.35:1 picture), on wider screens a centred 2.35:1 view. Its cutscene cameras keep their
// horizontal field of view, so the picture's height follows the view's shape. MAIN.CutsceneBorders chooses the
// borders as in the other Resident Evil fixes: Letterbox keeps the bars above and below, Pillarbox keeps the picture
// inside the 16:9 box on wider screens, Both looks like the game on a 16:9 screen, Off fills the screen. The picture
// keeps the vertical framing of the 2.35:1 view in every mode: without the bars it fills the height (nothing the bars
// hid is shown), wider views show more at the sides.

namespace
{
    constexpr float CinemaAspect = 16.0f / 9.0f * 1.3235294f;   // the game's cinema view: 2.353:1
    constexpr float BarTop = 0.12222222f;                       // the game's bars on a 16:9 screen, part of the height
    constexpr float BarBottom = 0.87777776f;

    struct Rect
    {
        int32_t left, top, right, bottom;
    };

    const int* pBorders = nullptr;
    GameRef<uintptr_t> Screen;      // +0x50 width, +0x54 height
    GameRef<uintptr_t> Main;        // +0x1042C game state, its +0x558 the game mode (7 = cutscene)
    std::atomic<uintptr_t> Views{ 0 };   // the view manager: +0x2F94 = set the views up again

    bool InCutscene()
    {
        if (!Main.is_initialized() || !Main)
            return false;
        const auto state = *reinterpret_cast<uintptr_t*>(Main + 0x1042C);
        return state && *reinterpret_cast<int32_t*>(state + 0x558) == 7;
    }

    // The cutscene view for a screen of width x height
    Rect CinemaView(int32_t width, int32_t height, int borders)
    {
        float left = 0.0f, top = 0.0f, w = float(width), h = float(height);
        if ((borders & 2) && w / h > 16.0f / 9.0f)
        {
            w = h * 16.0f / 9.0f;
            left = (float(width) - w) / 2.0f;
        }
        if (borders & 1)
        {
            // the bars of a 16:9 frame of this width, or of the frame's height on narrower screens
            const float frame = (std::min)(h, w * 9.0f / 16.0f);
            top = (h - frame) / 2.0f + frame * BarTop;
            h = frame * (BarBottom - BarTop);
        }
        return { int32_t(left), int32_t(top), int32_t(left + w), int32_t(top + h) };
    }
}

export namespace Cutscenes
{
    void Init()
    {
        pBorders = &FixSettings().Int("MAIN.CutsceneBorders");
        FixSettings().OnChange("MAIN.CutsceneBorders", []()
        {
            if (const auto views = Views.load(std::memory_order_relaxed))
                *reinterpret_cast<uint8_t*>(views + 0x2F94) = 1;
        });

        auto screen = hook::pattern("A1 ? ? ? ? 8B 70 50 8B 78 54 A1 ? ? ? ? 8B 88 2C 04 01 00 E8");
        Screen.SetAddress(*screen.get_first<uintptr_t*>(1));
        Main.SetAddress(*screen.get_first<uintptr_t*>(12));

        // The cinema layout's view (left, top, right, bottom), passed to the view setup
        auto pattern = hook::pattern("89 44 24 28 89 54 24 2C 89 7C 24 30 E8");
        static auto ViewHook = safetyhook::create_mid(pattern.get_first(12), [](SafetyHookContext& regs)
        {
            Views.store(regs.esi, std::memory_order_relaxed);
            if (!Screen)
                return;
            auto& rect = **reinterpret_cast<Rect**>(regs.esp + 4);
            rect = CinemaView(*reinterpret_cast<int32_t*>(Screen + 0x50), *reinterpret_cast<int32_t*>(Screen + 0x54), *pBorders);
        });

        // The cutscene cameras' projection: with the horizontal field of view flag (+0x104) the game scales the
        // matrix by the view's aspect (+0x20); the fix keeps the vertical extent of the 2.35:1 view
        pattern = hook::pattern("F3 0F 59 46 20 F3 0F 11 47 14 5F 5E");
        static auto ProjectionHook = safetyhook::create_mid(pattern.get_first(10), [](SafetyHookContext& regs)
        {
            const auto camera = regs.esi;
            if (!*reinterpret_cast<uint32_t*>(camera + 0x104) || !InCutscene())
                return;
            const float aspect = *reinterpret_cast<float*>(camera + 0x20);
            if (!(aspect > 0.0f))
                return;
            auto matrix = reinterpret_cast<float*>(regs.edi);
            const float scale = CinemaAspect / aspect;
            matrix[0] *= scale;
            matrix[5] *= scale;
        });
    }
}
