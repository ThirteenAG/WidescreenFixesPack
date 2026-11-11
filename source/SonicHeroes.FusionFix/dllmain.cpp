#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/WindowCursor.hpp"
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
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("0F BE 0D ? ? ? ? 8D 0C 89 8B"));
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
        WindowCursor::Shutdown();
    }
    return TRUE;
}
