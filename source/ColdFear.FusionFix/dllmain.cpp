#include "stdafx.h"
#include "FixSettings.hpp"
#include "Shared/CursorConfinement.hpp"
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
        // Settings described in settings.json and stored in the plugin's .cfg; editors change them through the live link
        FixSettings();
        asi_settings::LiveLink::Start();
        ReadSettings();
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("FF 74 24 10 FF 74 24 10 FF 74 24 10 FF 74 24 10 E8 ? ? ? ? C2 10 00"));
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent())
            InitializeASI();
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        CursorConfinement::Shutdown();
    }
    return TRUE;
}
