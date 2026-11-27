module;

#include <stdafx.h>
#include "FixSettings.hpp"

// Cutscene borders. The game draws black bars above and below in scripted cutscenes (a part of the screen height)
// and, sliding in and out, for its cinematic camera moments in play. MAIN.CutsceneBorders: Letterbox keeps them,
// Off removes them (the cutscene fills the screen), Pillarbox adds bars at the sides that keep the picture at 4:3 on
// wider screens, Both does both, which shows exactly the original framing. The side bars are drawn with the game's
// own bars, under the subtitles.

export module Cutscenes;

import ComVars;


export namespace SecondSightCutscenes
{
    inline float ScriptBars = 0.145f;     // part of the screen height, set by the setting
    inline float ScriptOriginal = 0.145f;
    inline float CameraBars = 60.0f;      // pixels at full slide
    inline float CameraOriginal = 60.0f;
    inline std::atomic<int> BorderMode{ 0 };
    inline int(__cdecl* Rectangle)(int, int, int, int, int) = nullptr;
    inline SafetyHookMid ScriptHook;
    inline SafetyHookMid CameraHook;

    inline void Apply()
    {
        BorderMode.store(FixSettings().Int("MAIN.CutsceneBorders"), std::memory_order_relaxed);
    }

    // Side bars of the given part of their full width, in the game's colour value for black
    inline void Sides(int borders, float amount, int color)
    {
        if ((borders != 2 && borders != 3) || amount <= 0.0f)
            return;
        const int width = int(RenderWidth.get()), height = int(RenderHeight.get()), box = height * 4 / 3;
        if (box >= width)
            return;
        const int side = int(float((width - box) / 2) * (std::min)(amount, 1.0f));
        if (side <= 0)
            return;
        Rectangle(0, 0, side, height, color);
        Rectangle(width - side, 0, width, height, color);
    }

    inline void Init()
    {
        // Scripted cutscenes: two bars of a part of the screen height, then the subtitles
        auto top = hook::pattern("DB 05 ? ? ? ? 6A 7F D8 0D ? ? ? ? E8 ? ? ? ? 50 A1 ? ? ? ? 50 6A 00 6A 00 E8");
        auto bottom = hook::pattern("DB 05 ? ? ? ? 8B 35 ? ? ? ? 8B 0D ? ? ? ? D8 0D ? ? ? ? 6A 7F");
        // Cinematic camera in play: bars of a fixed height times the slide amount
        auto camera = hook::pattern("A1 ? ? ? ? 85 C0 0F 85 ? ? ? ? D9 05 ? ? ? ? D8 1D ? ? ? ? DF E0 F6 C4 41 0F 85 ? ? ? ? D9 05 ? ? ? ? 56 D8 0D");
        Rectangle = reinterpret_cast<decltype(Rectangle)>(injector::GetBranchDestination(top.get_first(30)).as_int());
        ScriptOriginal = **top.get_first<float*>(10);
        CameraOriginal = **camera.get_first<float*>(45);
        CameraBarAmount.SetAddress(*camera.get_first<float*>(15));
        Apply();
        FixSettings().OnChange("MAIN.CutsceneBorders", Apply);
        injector::WriteMemory<float*>(top.get_first<float*>(10), &ScriptBars, true);
        injector::WriteMemory<float*>(bottom.get_first<float*>(20), &ScriptBars, true);
        injector::WriteMemory<float*>(camera.get_first<float*>(45), &CameraBars, true);
        // The native x87 loads read these floats on the render thread.
        ScriptHook = safetyhook::create_mid(top.get_first(), [](SafetyHookContext&)
        {
            const int borders = BorderMode.load(std::memory_order_relaxed);
            ScriptBars = borders == 1 || borders == 3 ? ScriptOriginal : 0.0f;
            Sides(borders, 1.0f, 0x7F);
        });
        CameraHook = safetyhook::create_mid(camera.get_first(36), [](SafetyHookContext&)
        {
            const int borders = BorderMode.load(std::memory_order_relaxed);
            CameraBars = borders == 1 || borders == 3 ? CameraOriginal : 0.0f;
            Sides(borders, CameraBarAmount.get(), 0x80);
        });
    }
}
