module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Misc;

import ComVars;
import Frontend;
import Camera;
import Hud;
import Intro;
import Window;
import Movie;

void Install()
{
    WindowedModeWrapper::EnableDpiAwareness();
    auto& settings = FixSettings();
    asi_settings::LiveLink::Start();
    (void)DeerMenu::Game();
    if (!DeerMovie::Init())
        OutputDebugStringA("Deer Avenger 4: native movie scaling contracts unavailable.\n");
    if (!DeerWindow::Init(settings.Int("MAIN.WindowMode")))
        OutputDebugStringA("Deer Avenger 4: native window mode contracts unavailable.\n");
    if (!DeerIntro::Init(settings.Bool("MAIN.SkipIntro")))
        OutputDebugStringA("Deer Avenger 4: startup video decision unavailable; native preference retained.\n");
    if (!DeerHud::Init())
        OutputDebugStringA("Deer Avenger 4: gameplay HUD correction unavailable; native layout retained.\n");
    if (!DeerCamera::Init())
        OutputDebugStringA("Deer Avenger 4: camera/FOV correction incomplete; native signatures or hook unavailable.\n");
}

export void InitWithMenu()
{
    WFP::onInitEvent().executeAll();
    HMODULE owner = nullptr;
    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&Install), &owner))
        DeerMenu::Init(owner);
}

class Misc
{
public:
    Misc()
    {
        WFP::onInitEvent() += []() { Install(); };
    }
} Misc;

