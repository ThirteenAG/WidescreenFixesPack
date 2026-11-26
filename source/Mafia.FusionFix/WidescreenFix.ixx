// Based on behar's original Mafia widescreen fix.
module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <atomic>

export module WidescreenFix;

import ComVars;
import LS3DF;
import HUD;
import Menu;
import DrawDistance;
import UI;
import Cutscene;

namespace
{
    std::atomic<bool> hudChanged{ true };
    std::atomic<bool> cutsceneChanged{ true };
}

void SetHooks()
{
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

// The fix's resolution replaces the game's own one for the back buffer, so the layout must use it too
export void ResolveResolution(int windowMode)
{
    nWindowWidth = FixSettings().Int("MAIN.ResX");
    nWindowHeight = FixSettings().Int("MAIN.ResY");
    if (windowMode == 2 || nWindowWidth <= 0 || nWindowHeight <= 0 || nWindowWidth > 16384 || nWindowHeight > 16384)
        std::tie(nWindowWidth, nWindowHeight) = GetDesktopRes();
    if (nWindowWidth <= 0 || nWindowHeight <= 0)
        std::tie(nWindowWidth, nWindowHeight) = std::make_tuple(640, 480);
}

export void InitializePresentation(int width, int height)
{
    if (!nResolutionWidth.is_initialized() || !nResolutionHeight.is_initialized() || width <= 0 || height <= 0) return;
    if (nResolutionWidth != width)
        nResolutionWidth = width;
    if (nResolutionHeight != height)
        nResolutionHeight = height;
    static bool installed = false;
    const bool resized = Screen.fWidth != width || Screen.fHeight != height;
    const bool refreshHud = hudChanged.exchange(false);
    const bool refreshCutscene = cutsceneChanged.exchange(false);
    if (installed && !resized && !refreshHud && !refreshCutscene) return;
    if (refreshCutscene)
    {
        const auto borders = FixSettings().Get("MAIN.CutsceneBorders");
        nCutsceneBorders = std::clamp(std::atoi(borders.c_str()), 0, 3);
        bNoCutsceneBorderAnimation = FixSettings().Get("MAIN.AnimatedCutsceneBorders") == "0";
    }
    if (installed && resized)
        ResizeSceneViewports(static_cast<int>(Screen.fWidth), static_cast<int>(Screen.fHeight), width, height);
    Screen.fWidth = static_cast<float>(width);
    Screen.fHeight = static_cast<float>(height);
    Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;
    Screen.fBaseWidth = 600.0f * Screen.fAspectRatio;
    Screen.fInvBaseWidth = 1.0f / Screen.fBaseWidth;
    fMovieAspect = Screen.fAspectRatio / (4.0f / 3.0f);
    if (!installed)
    {
        SetHooks();
        installed = true;
        return;
    }
    if (resized || refreshHud)
        RefreshHUD();
    if (resized)
    {
        CameraDisplayResized();
        RefreshMapAndText();
        RefreshMenuGeometry();
    }
}

// Resolves the game resolution variables and initializes the display
export void InitWidescreenFix()
{
    bWidescreenMap = FixSettings().Bool("WIDESCREEN.WidescreenMap") != 0;
    FixSettings().OnChange("MAIN.HudAspectRatioConstraint", []() { hudChanged.store(true); });
    for (const char* id : { "MAIN.CutsceneBorders", "MAIN.AnimatedCutsceneBorders" })
        FixSettings().OnChange(id, []() { cutsceneChanged.store(true); });
    bChangeDrawDistance = FixSettings().Bool("DRAW_DISTANCE.Enable");
    fDrawDistance = FixSettings().Float("GRAPHICS.DrawDistance");
    fDrawDistanceMin = FixSettings().Float("DRAW_DISTANCE.SkipMinDistance");
    fDrawDistanceMax = FixSettings().Float("DRAW_DISTANCE.SkipMaxDistance");
    auto pattern = find_pattern("A1 ? ? ? ? 8B 3D ? ? ? ? 52 89 4C 24 30");
    if (!pattern.empty()) // 1.0 resolution layout
    {
        nResolutionWidth.SetAddress(*pattern.get_first<int32_t*>(1));
        nResolutionHeight.SetAddress(nResolutionWidth.get_ptr() + 1);
    }
    else // 1.1/1.2 resolution layout
    {
        pattern = find_pattern("A1 ? ? ? ? 8B 0D ? ? ? ? 50 89 54 24 24");
        nResolutionHeight.SetAddress(*pattern.get_first<int32_t*>(1));
        nResolutionWidth.SetAddress(*pattern.get_first<int32_t*>(7));
    }
    IATHook::Replace(GetLS3DF(), "USER32.DLL", std::forward_as_tuple("LoadCursorA", LoadCursorHook));

    auto pMatroxWidth = find_pattern("75 ? B9 01 00 00 00 89 0D ? ? ? ? D9 44 24 10");
    if (!pMatroxWidth.empty()) // 1.1/1.2 Matrox layout
    {
        auto pMatroxHeight = find_pattern("75 ? B9 02 00 00 00 89 0D ? ? ? ? A1 ? ? ? ?");
        auto pMatroxCamera = find_pattern("7E ? A1 ? ? ? ? 89 7C 24 14 50 8B 10 FF 52 70");
        injector::WriteMemory<uint8_t>(pMatroxWidth.get_first(), 0xEB, true);
        injector::WriteMemory<uint8_t>(pMatroxHeight.get_first(), 0xEB, true);
        injector::WriteMemory<uint8_t>(pMatroxCamera.get_first(), 0xEB, true);
    }
    else // 1.0 has no Matrox path
    {
        auto legacyResolution = find_pattern("A1 ? ? ? ? 8B 3D ? ? ? ? 52 89 4C 24 30");
        legacyResolution.get_first();
    }
    static auto ResolutionHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        // Runs before the engine creates the device; the size saved by the game may differ in shape
        if (nWindowWidth <= 0 || nWindowHeight <= 0)
            ResolveResolution(std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2));
        InitializePresentation(nWindowWidth, nWindowHeight);
    });
}
