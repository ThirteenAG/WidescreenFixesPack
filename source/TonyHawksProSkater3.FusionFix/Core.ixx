module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include "Shared/WindowCursor.hpp"
#include <intrin.h>
#include <random>
#include "Shared/WidescreenFixConflict.hpp"


export module Core;

import ComVars;
import Camera;
import Hud;
import Movies;
import Frontend;
import Window;
import Misc;



void Install()
{
    WindowCursor::Initialize(FixSettings());
    auto& settings = FixSettings();
    asi_settings::LiveLink::Start();
    Screen.Width = settings.Int("MAIN.ResX");
    Screen.Height = settings.Int("MAIN.ResY");

    if (Screen.Width <= 0 || Screen.Height <= 0 || Screen.Width > 16384 || Screen.Height > 16384)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
    if (Screen.Width <= 0 || Screen.Height <= 0)
        std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);

    if (std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2) == 2)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
    if (Screen.Width <= 0 || Screen.Height <= 0)
        std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);
    THPS3Display::Init(Screen.Width, Screen.Height);

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.fAspectRatioDiff = 1.0f / (Screen.fAspectRatio / (4.0f / 3.0f));
    // One 2D unit in pixels: the 640x480 frame fits the screen, centred (pillarboxed when wider, letterboxed when narrower)
    Screen.fUnit = std::min(Screen.fWidth / 640.0f, Screen.fHeight / 480.0f);
    Screen.Width43 = static_cast<int32_t>(640.0f * Screen.fUnit);
    Screen.fHUDScaleX = Screen.fUnit / Screen.fWidth;
    // The game's 2D height is 448 units for the whole screen height
    Screen.fHUDScaleY = Screen.fUnit * (480.0f / 448.0f) / Screen.fHeight;
    Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
    Screen.fHudOffsetReal = (Screen.fWidth - 640.0f * Screen.fUnit) / 2.0f;
    Screen.fHudOffsetY = (Screen.fHeight - 480.0f * Screen.fUnit) / 2.0f;

    InstallResolution();

    THPS3Camera::Install(1.0f / Screen.fAspectRatioDiff, &settings.Float("MAIN.FOVFactor"));

    THPS3Video::Install(&settings.Int("MAIN.FMVWidescreenMode"));

    InstallHud();

    InstallMisc();

    THPS3Menu::Init();
    THPS3Updates::Init();
}


class Core
{
public:
    Core()
    {
        WFP::onInitEvent() += []() { Install(); };
        WFP::onShutdownEvent() += []() { WindowCursor::Shutdown(); };
    }
} Core;
