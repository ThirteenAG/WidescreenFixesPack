#include "stdafx.h"
#include "FixSettings.hpp"
#include "Shared/WidescreenFixConflict.hpp"

import ComVars;
import Settings;
import Menu;

// The settings' runtime interface for menu hosts and editors (AsiSettings_GetApi)
ASI_SETTINGS_EXPORT(FixSettings())

// The game; Direct3D 9 hooks are set up when d3d9.dll loads (FusionDxHook)
void Init()
{
    FixSettings();

    // The edition, told by its cinematic bars function; several handlers read it at init
    if (hook::pattern("8B 41 10 6A 00").empty())
        Screen.bSignatureEdition = true;

    WFP::onInitEvent().executeAll();
    InitMenu();
    asi_settings::LiveLink::Start();
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        if (WidescreenFixConflict::Found())
            return;
        WindowedModeWrapper::EnableDpiAwareness();
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("33 DB 89 5D E0 53"));
        // The settings application
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(InitSettings, hook::pattern("75 66 8D 4C 24 04 51"));
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        WFP::onShutdownEvent().executeAll();
    }
    return TRUE;
}
