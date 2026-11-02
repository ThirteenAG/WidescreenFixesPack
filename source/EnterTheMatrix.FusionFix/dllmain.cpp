#include "stdafx.h"
#include "FixSettings.hpp"
#include "Shared/WidescreenFixConflict.hpp"

import ComVars;
import MatrixOptions;

// The settings' runtime interface for menu hosts and editors (AsiSettings_GetApi)
ASI_SETTINGS_EXPORT(FixSettings())

// The game; the per-frame events come from the window's frame (Window)
void Init()
{
    // The options dialog keeps system scaling; the game itself gets real pixels on scaled desktops
    WindowedModeWrapper::EnableDpiAwareness();
    WFP::onInitEvent().executeAll();
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        if (WidescreenFixConflict::Found())
            return;
        FixSettings();
        asi_settings::LiveLink::Start();
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("55 8B EC 83 EC 34 56 6A 00 6A FF"));
        // The options tool
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(InitMatrixOptions, hook::pattern("8B 45 04 85 C0 74"));
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
