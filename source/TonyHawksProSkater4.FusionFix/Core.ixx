module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/WindowCursor.hpp"
#include "FrameRate.hpp"
#include "DeveloperOverlay.hpp"
#include <random>

export module Core;

import Frontend;
import Hud;
import Movies;
import Window;
import ComVars;
import Camera;

void Install()
{
    WindowCursor::Initialize(FixSettings());
    InitFrameRateLimit();
    InitDeveloperOverlay();
    Screen.Width = FixSettings().Int("MAIN.ResX");
    Screen.Height = FixSettings().Int("MAIN.ResY");
    static const float& fFOVFactor = FixSettings().Float("MAIN.FOVFactor");

    if (Screen.Width <= 0 || Screen.Height <= 0 || Screen.Width > 16384 || Screen.Height > 16384)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
    if (Screen.Width <= 0 || Screen.Height <= 0)
        std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);

    if (std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2) == 2)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
    if (Screen.Width <= 0 || Screen.Height <= 0)
        std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);
    THPS4Display::Init(Screen.Width, Screen.Height);

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.Width43 = static_cast<int32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.fAspectRatioDiff = 1.0f / (Screen.fAspectRatio / (4.0f / 3.0f));
    Screen.fHUDScaleX = 1.0f / Screen.fWidth * (Screen.fHeight / 480.0f);
    Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
    // The 2D frame keeps 4:3: pillarboxed on wider screens, letterboxed on narrower ones (5:4)
    Screen.fHudOffsetReal = std::max(0.0f, (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f);
    Screen.fHudOffsetY = std::max(0.0f, (Screen.fHeight - Screen.fWidth * (3.0f / 4.0f)) / 2.0f);
    THPS4Video::Install(&FixSettings().Int("MAIN.FMVWidescreenMode"));

    InstallResolution();

    InstallAspectRatio();

    InstallCameraFix(&fFOVFactor);

    InstallHud();

    {
        auto pattern = hook::pattern("E8 ? ? ? ? 8B E8 8B 87 D0");
        struct RandomHook
        {
            void operator()(injector::reg_pack& regs)
            {
                std::mt19937 r{ std::random_device{}() };
                std::uniform_int_distribution<uint32_t> uid(0, regs.eax);
                regs.eax = uid(r);
            }
        }; injector::MakeInline<RandomHook>(pattern.get_first(0));
    }
}

class Core
{
public:
    Core()
    {
        WFP::onInitEvent() += []()
        {
            Install();
            THPS4Menu::Init();
            THPS4Updates::Init();
        };
        WFP::onShutdownEvent() += []()
        {
            WindowCursor::Shutdown();
        };
    }
} Core;
