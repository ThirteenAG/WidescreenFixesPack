module;

#include "stdafx.h"
#include "FixSettings.hpp"

export module Main;

import ComVars;
import Hud;
import Frontend;
import Movies;
import Misc;
import Window;
import Updates;

void InitializeFix()
{
    auto& settings = FixSettings();
    asi_settings::LiveLink::Start();
    Screen.Width = settings.Int("MAIN.ResX");
    Screen.Height = settings.Int("MAIN.ResY");

    if (Screen.Width <= 0 || Screen.Height <= 0 || Screen.Width > 16384 || Screen.Height > 16384)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
    if (Screen.Width <= 0 || Screen.Height <= 0 || Screen.Width > 32767 || Screen.Height > 32767)
    {
        Screen.Width = 640;
        Screen.Height = 480;
    }

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.Width43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fWidth43 = static_cast<float>(Screen.Width43);

    InstallHud();
    InstallScreenSpace();
    InstallFullScreenPictures();
    InstallMovieLayout();
    InstallWindowMode();
    InstallFieldOfView();
    InstallFrame();
    Updates::Install();

    if (settings.Bool("MAIN.SkipIntro"))
        SkipIntro();

    auto outputResolution = hook::pattern("B9 ? ? ? ? 89 8E 28 03 00 00 B8 ? ? ? ? 89 86 2C 03 00 00");
    auto cameraAspect = hook::pattern("84 C0 F3 0F 10 05 ? ? ? ? 75 08 F3 0F 10 05");
    const char* resolutions[] = {
        "C7 00 80 02 00 00 C7 40 04 E0 01 00 00 88 58 08",
        "C7 00 20 03 00 00 C7 40 04 58 02 00 00 88 58 08",
        "C7 00 00 04 00 00 C7 40 04 00 03 00 00 88 58 08",
        "C7 00 00 05 00 00 C7 40 04 00 04 00 00 88 58 08",
        "C7 00 40 06 00 00 C7 40 04 B0 04 00 00 88 58 08"
    };
    // The menus apply the saved resolution choice when they open; every choice gives the fix's resolution
    const char* menuResolutions[] = {
        "C7 44 24 04 80 02 00 00 C7 44 24 08 E0 01 00 00 E8 ? ? ? ? 83 C4 08 C3",
        "C7 44 24 04 20 03 00 00 C7 44 24 08 58 02 00 00 E8 ? ? ? ? 83 C4 08 C3",
        "C7 44 24 04 00 04 00 00 C7 44 24 08 00 03 00 00 E8 ? ? ? ? 83 C4 08 C3",
        "C7 44 24 04 00 05 00 00 C7 44 24 08 00 04 00 00 E8 ? ? ? ? 83 C4 08 C3",
        "C7 44 24 04 40 06 00 00 C7 44 24 08 B0 04 00 00 E8 ? ? ? ? 83 C4 08 C3"
    };
    std::array<uint8_t*, 5> resolutionSites{}, menuSites{};
    for (size_t i = 0; i < resolutionSites.size(); ++i)
    {
        resolutionSites[i] = hook::pattern(resolutions[i]).get_first<uint8_t>();
        menuSites[i] = hook::pattern(menuResolutions[i]).get_first<uint8_t>();
    }
    for (size_t i = 0; i < resolutionSites.size(); ++i)
    {
        injector::WriteMemory<int32_t>(resolutionSites[i] + 2, Screen.Width, true);
        injector::WriteMemory<int32_t>(resolutionSites[i] + 9, Screen.Height, true);
        injector::WriteMemory<int32_t>(menuSites[i] + 4, Screen.Width, true);
        injector::WriteMemory<int32_t>(menuSites[i] + 12, Screen.Height, true);
    }
    // All native resolution choices use the configured output size, so do not save it over the game's choice.
    auto resetSave = hook::pattern("E8 ? ? ? ? 8B CE E8 ? ? ? ? A0 ? ? ? ? 84 C0 F3 0F 2A C5");
    auto startupSave = hook::pattern("89 BE 24 03 00 00 8B CE E8 ? ? ? ? F6 44 24 28 02");
    injector::MakeNOP(resetSave.get_first(7), 5, true);
    injector::MakeNOP(startupSave.get_first(8), 5, true);
    injector::WriteMemory<int32_t>(outputResolution.get_first<int32_t>(1), Screen.Width, true);
    injector::WriteMemory<int32_t>(outputResolution.get_first<int32_t>(12), Screen.Height, true);
    // The camera's view area, for the widescreen mode and for 4:3 and narrower
    injector::WriteMemory<float>(*cameraAspect.get_first<float*>(6), Screen.fAspectRatio, true);
    injector::WriteMemory<float>(*cameraAspect.get_first<float*>(16), Screen.fAspectRatio, true);
}


class Main
{
public:
    Main()
    {
        WFP::onInitEvent() += InitializeFix;
    }
} Main;
