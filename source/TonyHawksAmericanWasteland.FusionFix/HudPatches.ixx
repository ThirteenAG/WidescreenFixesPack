module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module HudPatches;

import ComVars;
import Hud;
import Icons;
import Window;

export void InstallHud()
{
    auto& settings = FixSettings();
    hook::pattern pattern("");
    //HUD
    THAWHud::Install(Screen.fWidth, Screen.fHeight, &settings.String("MAIN.HudAspectRatioConstraint"),
                     &settings.Int("MAIN.CutsceneBorders"));
    THAWIcons::Install(&settings.Int("MAIN.GamepadIcons"));
    struct SetOffsetHook
    {
        void operator()(injector::reg_pack& regs)
        {
            dword_787D88 = static_cast<int32_t>(Screen.fHudOffsetReal);
        }
    };

    pattern = hook::pattern("D8 0D ? ? ? ? 8B 35 ? ? ? ? 85 F6 D9 1D");
    injector::WriteMemory<float*>(pattern.get_first<float*>(2), &Screen.fHUDScaleX, true);
    GameScaleX.SetAddress(*pattern.get_first<float*>(16));
    auto yScale = hook::pattern("D9 1D ? ? ? ? D9 05 ? ? ? ? D8 0D ? ? ? ? E8 ? ? ? ? A3 ? ? ? ?");
    GameScaleY.SetAddress(*yScale.get_first<float*>(2));
    GameYOffset.SetAddress(*yScale.get_first<int32_t*>(24));
    THAWDisplay::ScaleHook = safetyhook::create_mid(yScale.get_first(28), [](SafetyHookContext&) { THAWDisplay::UpdateFrame(); });
    auto letterbox = hook::pattern("8A 44 24 04 84 C0 A0 ? ? ? ? 74");
    Letterboxed.SetAddress(*letterbox.get_first<uint8_t*>(7));
    THAWDisplay::LetterboxHook = safetyhook::create_inline(letterbox.get_first(), THAWDisplay::SetLetterbox);
    dword_787D88.SetAddress(*hook::get_pattern<int32_t*>("89 1D ? ? ? ? C7 05 ? ? ? ? 10 00 00 00", 2));

    pattern = hook::pattern(pattern_str(0x89, '?', to_bytes(dword_787D88.get_ptr())));
    injector::MakeInline<SetOffsetHook>(pattern.get_first(0), pattern.get_first(6));

    pattern = hook::pattern(pattern_str(0xC7, '?', to_bytes(dword_787D88.get_ptr())));
    injector::MakeInline<SetOffsetHook>(pattern.get_first(0), pattern.get_first(10));

    // The finished frame was copied to the screen without its top 6% and bottom 4%, stretched to the full height
    // (everything 11% too tall, movies cut off): it is copied whole
    pattern = hook::pattern("68 8F C2 75 3F 68 8F C2 75 3D 68 00 00 80 3F 6A 00 6A 01 6A 01 6A 01");
    injector::WriteMemory<float>(pattern.get_first<float>(1), 1.0f, true);
    injector::WriteMemory<float>(pattern.get_first<float>(6), 0.0f, true);
}
