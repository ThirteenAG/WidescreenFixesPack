#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/WidescreenFixConflict.hpp"

import Misc;

ASI_SETTINGS_EXPORT(FixSettings())

void Init()
{
    WFP::onInitEvent().executeAll();
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
        {
        if (WidescreenFixConflict::Found())
            return;
            // Real pixels on scaled desktops (desktop resolution, full screen picture not scaled up from a corner)
            WindowedModeWrapper::EnableDpiAwareness();
            FixSettings();
            asi_settings::LiveLink::Start();
            CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("55 8B EC E8 ? ? ? ? 80 7D 24 00 8B 45 08 A3 ? ? ? ? 74 21"));
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
