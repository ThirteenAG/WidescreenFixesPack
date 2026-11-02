module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include <atomic>


export module Misc;

import NativeMenu;
import MenuLayout;
import Updates;
import Window;
import DeveloperOverlay;
import ComVars;
import Hud;
import Frontend;
import Cutscene;
import Camera;




export std::atomic<int> PunisherFrameLimit{ 60 };

void UpdateFrameLimit()
{
    int limit = FixSettings().Int("MAIN.FPSLimit");
    if (limit == -1)
    {
        DEVMODEW mode{};
        mode.dmSize = sizeof(mode);
        limit = EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &mode) && mode.dmDisplayFrequency > 1
            ? static_cast<int>(mode.dmDisplayFrequency) : 60;
    }
    PunisherFrameLimit.store(limit > 0 ? std::clamp(limit, 30, 1000) : 0, std::memory_order_relaxed);
}

void InitPatches()
{
    hook::pattern pattern;
    auto& settings = FixSettings();
    asi_settings::LiveLink::Start();
    UpdateFrameLimit();
    settings.OnChange("MAIN.FPSLimit", UpdateFrameLimit);
    Screen.Width = settings.Int("MAIN.ResX");
    Screen.Height = settings.Int("MAIN.ResY");
    bool bSkipIntro = settings.Bool("MAIN.SkipIntro");
    static const float& fFOVFactor = settings.Float("MAIN.FOVFactor");
    auto updateHudConstraint = []()
    {
        const auto parsed = ParseWidescreenHudOffset(FixSettings().String("MAIN.HudAspectRatioConstraint"));
        const float aspect = parsed && std::isfinite(*parsed) && *parsed > 0
            ? std::clamp(*parsed, 4.0f / 3.0f, 32.0f / 9.0f) : 0.0f;
        RequestedHudAspect.store(aspect, std::memory_order_release);
        HudCoordinatesPending.store(true, std::memory_order_release);
    };
    updateHudConstraint();
    settings.OnChange("MAIN.HudAspectRatioConstraint", updateHudConstraint);

    if (settings.Int("MAIN.WindowMode") == 2 || Screen.Width <= 0 || Screen.Height <= 0)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
    if (Screen.Width <= 0 || Screen.Height <= 0)
    {
        Screen.Width = 640;
        Screen.Height = 480;
    }

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.Width43 = static_cast<int32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.fAspectRatioDiff = 1.0f / (Screen.fAspectRatio / (4.0f / 3.0f));
    Screen.fHUDScaleX = 1.0f / Screen.fWidth * (Screen.fHeight / 480.0f);
    Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
    Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;

    //Presets
    static constexpr int32_t heights[] = { 480, 600, 768, 960 };
    Screen.PresetHeight = heights[0];
    for (auto height : heights)
    {
        if (height <= Screen.Height)
            Screen.PresetHeight = height;
    }

    switch (Screen.PresetHeight)
    {
    case 480:
        Screen.PresetWidth = 640;
        break;
    case 600:
        Screen.PresetWidth = 800;
        break;
    case 768:
        Screen.PresetWidth = 1024;
        break;
    case 960:
        Screen.PresetWidth = 1280;
        break;
    default:
        Screen.PresetWidth = 1280;
        Screen.PresetHeight = 960;
        break;
    }

    Screen.PresetFactorX = Screen.fWidth / static_cast<float>(Screen.PresetWidth);
    Screen.PresetFactorY = Screen.fHeight / static_cast<float>(Screen.PresetHeight);

    InitHud();

    InitFrontend();

    if (bSkipIntro)
    {
        pattern = hook::pattern("E8 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 6A 00 E8 ? ? ? ?");
        injector::MakeNOP(pattern.get_first(), 5, true);
    }

    Camera::Init(fFOVFactor);

    Cutscene::Init();
    PunisherWindow::Init(Screen.Width, Screen.Height);
    PunisherOverlay::Init();
    PunMenuLayout::Init(Screen.Width, Screen.Height);
    PunMenu::Init();
    PunisherUpdates::Init();
}


class Misc
{
public:
    Misc()
    {
        WFP::onInitEvent() += InitPatches;
    }
} Misc;
