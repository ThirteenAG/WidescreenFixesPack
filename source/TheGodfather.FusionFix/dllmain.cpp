#include "stdafx.h"
#include "FixSettings.hpp"
#include "Shared/WidescreenFixConflict.hpp"

import Main;

ASI_SETTINGS_EXPORT(FixSettings())

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        if (WidescreenFixConflict::Found())
            return;
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime([]()
        {
            WindowedModeWrapper::EnableDpiAwareness();
            WFP::onInitEvent().executeAll();
        }, hook::pattern("8B 74 24 10 0F BE 06 85 C0 57"));
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
        WFP::onShutdownEvent().executeAll();
    }
    return TRUE;
}
