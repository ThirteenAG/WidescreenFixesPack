#include "stdafx.h"
#include "FixSettings.hpp"

import Camera;
import System;
import LightSync;
import Graphics;
import AimWalk;
import Controls;
import KnifeParry;
import Menu;

// The settings' runtime interface for menu hosts and editors (AsiSettings_GetApi)
ASI_SETTINGS_EXPORT(FixSettings())

void Init()
{
    // Settings described in settings.json and stored in the plugin's .cfg; FusionFixHub changes them while the game
    // runs through the live link
    FixSettings();
    asi_settings::LiveLink::Start();

    Camera::Init();
    System::Init();
    LightSync::Init();
    Graphics::Init();
    AimWalk::Init();
    Controls::Init();
    KnifeParry::Init();
    Menu::Init();
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("E8 ? ? ? ? 83 BE ? ? ? ? ? 8B C7"));
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    return TRUE;
}
