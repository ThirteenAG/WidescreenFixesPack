#include "stdafx.h"
#include "FixSettings.hpp"
#include "Shared/WidescreenFixConflict.hpp"

import Frontend;
import Core;

void Init()
{
    WFP::onInitEvent().executeAll();
}

ASI_SETTINGS_EXPORT(FixSettings())

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        if (WidescreenFixConflict::Found())
            return;
        FixSettings();
        asi_settings::LiveLink::Start();
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("6A 01 6A 01 ? 8B 15"));
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
        THUGMenu::Shutdown(lpReserved != nullptr);
        WFP::onShutdownEvent().executeAll();
    }
    return TRUE;
}
