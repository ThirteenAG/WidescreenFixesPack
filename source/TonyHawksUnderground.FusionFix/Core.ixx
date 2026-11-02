module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/WindowCursor.hpp"
#include <random>

export module Core;

import Frontend;
import Hud;
import Frame;
import Window;
import ComVars;
import Camera;
import Movies;
import Misc;

void Install()
{
    WindowCursor::Initialize(FixSettings());
    auto& settings = FixSettings();
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
    THUGDisplay::Init(Screen.Width, Screen.Height);

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.Width43 = static_cast<int32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.fAspectRatioDiff = 1.0f / (Screen.fAspectRatio / (4.0f / 3.0f));
    // The 2D frame keeps 4:3: pillarboxed on wider screens, letterboxed on narrower ones (5:4)
    Screen.fHUDScaleX = std::min(Screen.fWidth, Screen.fHeight * (4.0f / 3.0f)) / 640.0f / Screen.fWidth;
    Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
    Screen.fHudOffsetReal = std::max(0.0f, (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f);
    THUGFrame::Install(Screen.fWidth, Screen.fHeight, &settings.Int("MAIN.CutsceneBorders"));
    THUGDevice::Install();

    InstallMovie();

    //addresses is not based on drm free exe

    InstallResolutionTable();

    InstallResolution();

    InstallAspectRatio();

    InstallCameraFix();

    InstallHud();

    InstallControls();

    InstallMusic();

}

class Core
{
public:
    Core()
    {
        WFP::onInitEvent() += []()
        {
            Install();
            THUGMenu::Init();
            THUGUpdates::Init();
        };
        WFP::onShutdownEvent() += []() { WindowCursor::Shutdown(); };
    }
} Core;
