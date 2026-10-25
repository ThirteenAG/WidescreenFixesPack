// Based on behar's original Mafia widescreen fix.
module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module WidescreenFix;

import ComVars;
import LS3DF;
import HUD;
import Menu;
import DrawDistance;
import UI;
import Cutscene;

void SetHooks()
{
    Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;
    Screen.fBaseWidth = 600.0f * Screen.fAspectRatio;
    Screen.fInvBaseWidth = 1.0f / Screen.fBaseWidth;
    fMenuOffset = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) * 0.5f;
    fMovieAspect = Screen.fAspectRatio / (4.0f / 3.0f);
    InitCutscenes();
    InitFOV();
    InitHUD();
    InitMap();
    InitText();
    InitMenu();
    InitMovie();
    if (bChangeDrawDistance) InitDrawDistance();
    InitUIScaling();
    InitOthers();
}

export void InitWidescreenFix()
{
    bWidescreenHUD = FixSettings().Bool("WIDESCREEN.WidescreenHUD") != 0;
    bWidescreenMap = FixSettings().Bool("WIDESCREEN.WidescreenMap") != 0;
    auto presentation = []()
    {
        nFMVWidescreenMode = std::clamp(FixSettings().Int("MAIN.FMVWidescreenMode"), 0, 2);
        nCutsceneBorders = std::clamp(FixSettings().Int("MAIN.CutsceneBorders"), 0, 3);
        bNoCutsceneBorderAnimation = !FixSettings().Bool("MAIN.AnimatedCutsceneBorders");
    };
    presentation();
    for (const char* id : { "MAIN.FMVWidescreenMode", "MAIN.CutsceneBorders", "MAIN.AnimatedCutsceneBorders" })
        FixSettings().OnChange(id, presentation);
    bChangeDrawDistance = FixSettings().Bool("DRAW_DISTANCE.Enable");
    fDrawDistance = FixSettings().Float("GRAPHICS.DrawDistance");
    fDrawDistanceMin = FixSettings().Float("DRAW_DISTANCE.SkipMinDistance");
    fDrawDistanceMax = FixSettings().Float("DRAW_DISTANCE.SkipMaxDistance");
    nFPSLimit = FixSettings().Int("MAIN.FPSLimit");
    auto pattern = find_pattern("A1 ? ? ? ? 8B 3D ? ? ? ? 52 89 4C 24 30");
    nGameVersion = MAFIA_1_0_ENG;
    if (pattern.empty())
    {
        pattern = find_pattern("A1 ? ? ? ? 8B 0D ? ? ? ? 50 89 54 24 24");
        nGameVersion = MAFIA_1_1_ENG;
    }
    if (pattern.size() != 1) return;
    if (nGameVersion == MAFIA_1_0_ENG)
    {
        pResolutionWidth = *pattern.get_first<int32_t*>(1);
        pResolutionHeight = pResolutionWidth + 1;
    }
    else
    {
        pResolutionHeight = *pattern.get_first<int32_t*>(1);
        pResolutionWidth = *pattern.get_first<int32_t*>(7);
        auto pHudCreditsText2X6 = find_pattern("68 00 00 C3 43 52 B9 ? ? ? ? E8 ? ? ? ?");
        if (pHudCreditsText2X6.size() == 1) nGameVersion = MAFIA_1_2_ENG;
    }
    if (auto hModule = GetLS3DF())
        IATHook::Replace(hModule, "USER32.DLL", std::forward_as_tuple("LoadCursorA", LoadCursorHook));

    if (nGameVersion != MAFIA_1_0_ENG)
    {
        auto pMatroxWidth = find_pattern("75 ? B9 01 00 00 00 89 0D ? ? ? ? D9 44 24 10");
        auto pMatroxHeight = find_pattern("75 ? B9 02 00 00 00 89 0D ? ? ? ? A1 ? ? ? ?");
        auto pMatroxCamera = find_pattern("7E ? A1 ? ? ? ? 89 7C 24 14 50 8B 10 FF 52 70");
        if (pMatroxWidth.size() == 1 && pMatroxHeight.size() == 1 && pMatroxCamera.size() == 1)
        {
            ProtectedGameRef<std::remove_cvref_t<decltype(uint8_t{0xEB})>>{pMatroxWidth.get_first<std::remove_cvref_t<decltype(uint8_t{0xEB})>>()} = uint8_t{0xEB};
            ProtectedGameRef<std::remove_cvref_t<decltype(uint8_t{0xEB})>>{pMatroxHeight.get_first<std::remove_cvref_t<decltype(uint8_t{0xEB})>>()} = uint8_t{0xEB};
            ProtectedGameRef<std::remove_cvref_t<decltype(uint8_t{0xEB})>>{pMatroxCamera.get_first<std::remove_cvref_t<decltype(uint8_t{0xEB})>>()} = uint8_t{0xEB};
        }
    }
    static auto ResolutionHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if (*pResolutionWidth <= 0 || *pResolutionHeight <= 0) return;
        static std::once_flag flag;
        std::call_once(flag, []()
        {
            Screen.fWidth = static_cast<float>(nWindowMode == 2 && nWindowWidth > 0 ? nWindowWidth : *pResolutionWidth);
            Screen.fHeight = static_cast<float>(nWindowMode == 2 && nWindowHeight > 0 ? nWindowHeight : *pResolutionHeight);
            SetHooks();
        });
    });
}

