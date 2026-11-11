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
    DeerMovie::Init();
    DeerWindow::Init(settings.Int("MAIN.WindowMode"));
    DeerIntro::Init(settings.Bool("MAIN.SkipIntro"));
    DeerHud::Init();
    DeerCamera::Init();
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

