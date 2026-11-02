#include "stdafx.h"
#include "FixSettings.hpp"

import ComVars;
import Resolution;
import Shaders;
import System;
import LightSync;
import FrameRate;
import Runtime;

// The settings' runtime interface for menu hosts and editors (AsiSettings_GetApi)
ASI_SETTINGS_EXPORT(FixSettings())

void Init()
{
    static std::atomic<bool> started{ false };
    if (started.exchange(true)) return;
    // Settings described in settings.json and stored in the plugin's .cfg; FusionFixHub changes them while the game
    // runs through the live link
    FixSettings();
    asi_settings::LiveLink::Start();

    WFP::onInitEvent().executeAll();
    RERuntime::Install();
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        WindowedModeWrapper::EnableDpiAwareness();
        // The logo scene's state switch, present in every known build once the exe is unpacked
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("8B 86 A8 00 00 00 83 F8 03 72"));
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    if (reason == DLL_PROCESS_DETACH) REFrameRate::Shutdown();
    return TRUE;
}
