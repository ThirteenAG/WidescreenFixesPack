module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include "Shared/WindowCursor.hpp"
#include "Shared/WidescreenFixConflict.hpp"


export module Core;

import Window;
import Camera;
import Hud;
import ComVars;
import Frontend;
import Movies;
import Misc;



void Install()
{
    WindowCursor::Initialize(FixSettings());
    THPS2Menu::Init();
    THPS2DeferredClose::Install();
    THPS2Updates::Init();
    THPS2Controller::Install();
    THPS2Startup::Install(FixSettings().Bool("MAIN.SkipIntro"));

    Screen.Width = FixSettings().Int("MAIN.ResX");
    Screen.Height = FixSettings().Int("MAIN.ResY");

    if (Screen.Width <= 0 || Screen.Height <= 0 || Screen.Width > 16384 || Screen.Height > 16384)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
    if (Screen.Width <= 0 || Screen.Height <= 0)
        std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);

    const auto windowMode = std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2);
    if (windowMode == 2)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
    if (Screen.Width <= 0 || Screen.Height <= 0)
        std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);
    THPS2D3D7Resolution::Install();
    THPS2ComRelease::Install();
    THPS2Display::SetWindow(0, Screen.Width, Screen.Height);
    THPS2Display::Install(windowMode, Screen.Width, Screen.Height);
    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    // The 4:3 frame of the menus and 2D art: full height, or full width with bars above and below on narrower screens
    Screen.Height43 = std::min(Screen.Height, static_cast<int32_t>(std::lround(Screen.fWidth * 3.0f / 4.0f)));
    Screen.Width43 = static_cast<int32_t>(std::lround(Screen.Height43 * 4.0f / 3.0f));
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.fHudOffsetReal = (Screen.fWidth - Screen.Width43) / 2.0f;
    Screen.fHudOffsetRealY = (Screen.fHeight - Screen.Height43) / 2.0f;
    Screen.fHudOffset = Screen.fHudOffsetReal * 640.0f / Screen.Width43;
    Screen.fHudOffsetY = Screen.fHudOffsetRealY * 480.0f / Screen.Height43;

    InstallResolution();
    InstallCameraAndMovies();
    InstallMenuWheel();
    InstallMenuLayout();


}

class Core
{
public:
    Core()
    {
        WFP::onInitEvent() += []()
        {
            Install();
        };
        WFP::onShutdownEvent() += []()
        {
            WindowCursor::Shutdown();
        };
    }
} Core;
