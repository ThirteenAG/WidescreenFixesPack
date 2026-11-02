module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/WindowCursor.hpp"
#include "Shared/TonyHawk/Camera.hpp"
#include <random>

export module Core;

import Frontend;
import Hud;
import Controller;
import Vibration;
import Icons;
import Cinema;
import Movies;
import Window;
import RawMouse;
import ComVars;
import Camera;
import HudPatches;
import Misc;

void Install()
{
    THAWController::Install(&FixSettings().Float("MAIN.GamepadCameraSpeed"), &FixSettings().Int("MAIN.InvertGamepadY"));
    WindowCursor::Initialize(FixSettings());
    auto& settings = FixSettings();
    THAWVibration::Install(&settings.Int("MAIN.Vibration"));
    asi_settings::LiveLink::Start();
    THAWMenu::Init();
    THAWRawMouse::Install(&settings.Int("MAIN.RawInputMouse"), &settings.Float("MAIN.MouseSensitivity"),
                          &settings.Int("MAIN.InvertMouseY"));
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
    THAWDisplay::Init(Screen.Width, Screen.Height);
    THAWMovies::Install(&settings.Int("MAIN.FMVWidescreenMode"));
    THAWDevice::Install();
    THAWCinema::Install(&settings.Int("MAIN.CutsceneBorders"), Screen.Width, Screen.Height);
    THAWController::BlockKeys = THAWDevice::BlocksInput;

    THAWDisplay::UpdateDimensions(Screen.Width, Screen.Height);

    InstallResolutionTable();

    InstallResolution();

    InstallAspectRatio();

    InstallCameraFix();

    InstallHud();

    InstallMusic();

    // Resize only after the client dimensions settle, on the render thread where
    // the game's own device-reset routine releases and recreates its resources.
    auto reset = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 8B 08 68 ? ? ? ? 50 FF 51 40 85 C0");
    THAWDisplay::Reset = reinterpret_cast<void(__cdecl*)()>(reset.get_first());
    Presentation.SetAddress(*reset.get_first<D3DPRESENT_PARAMETERS*>(23));
    THAWDisplay::Resizable = std::clamp(settings.Int("MAIN.WindowMode"), 0, 2) == 1;
    THAWCinema::BeforePresent = []()
    {
        WFP::onGameProcessEvent().executeAll();
    };

}

class Core
{
public:
    Core()
    {
        WFP::onInitEvent() += []()
        {
            Install();
            THAWUpdates::Init();
        };
        WFP::onGameProcessEvent() += []()
        {
            THAWDevice::Attach();
            THAWDisplay::UpdateResize();
            THAWVibration::Frame();
            const bool gameplayCamera = std::any_of(TonyCamera::Cameras.begin(), TonyCamera::Cameras.end(),
                [](const auto& entry) { return entry.second.gameplay; });
            THAWRawMouse::Frame(gameplayCamera);
        };
        WFP::onShutdownEvent() += []() { WindowCursor::Shutdown(); };
    }
} Core;
