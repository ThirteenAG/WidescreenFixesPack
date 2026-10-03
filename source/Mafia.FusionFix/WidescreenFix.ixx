// Based on behar's original Mafia widescreen fix.
module;

#include <stdafx.h>

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
    CIniReader ini("");
    bWidescreenHUD = ini.ReadInteger("WIDESCREEN", "WidescreenHUD", 1) != 0;
    bWidescreenMap = ini.ReadInteger("WIDESCREEN", "WidescreenMap", 1) != 0;
    bZoomFMVs = ini.ReadInteger("WIDESCREEN", "ZoomFMVs", 1) != 0;
    nCutsceneBorders = std::clamp(ini.ReadInteger("CUTSCENES", "CutsceneBorders", 3), 0, 3);
    bNoCutsceneBorderAnimation = ini.ReadInteger("CUTSCENES", "NoCutsceneBorderAnimation", 0);
    bChangeDrawDistance = ini.ReadInteger("DRAW_DISTANCE", "Enable", 1);
    fDrawDistance = ini.ReadFloat("DRAW_DISTANCE", "DrawDistance", 600.0f);
    fDrawDistanceMin = ini.ReadFloat("DRAW_DISTANCE", "SkipMinDistance", 20.0f);
    fDrawDistanceMax = ini.ReadFloat("DRAW_DISTANCE", "SkipMaxDistance", 450.0f);
    nFPSLimit = ini.ReadInteger("MAIN", "FPSLimit", 0);
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
            injector::WriteMemory(pMatroxWidth.get_first(), uint8_t{0xEB}, true);
            injector::WriteMemory(pMatroxHeight.get_first(), uint8_t{0xEB}, true);
            injector::WriteMemory(pMatroxCamera.get_first(), uint8_t{0xEB}, true);
        }
    }
    static auto ResolutionHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if (*pResolutionWidth <= 0 || *pResolutionHeight <= 0) return;
        static std::once_flag flag;
        std::call_once(flag, []()
        {
            Screen.fWidth = static_cast<float>(*pResolutionWidth);
            Screen.fHeight = static_cast<float>(*pResolutionHeight);
            SetHooks();
        });
    });
}

