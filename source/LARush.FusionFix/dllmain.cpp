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
            WindowedModeWrapper::EnableDpiAwareness();
            CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("8B 44 24 04 85 C0 75 ? 8B 44 24"));
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
