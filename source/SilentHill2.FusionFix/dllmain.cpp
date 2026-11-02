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
            // Real pixels on scaled desktops (300 % scaling doesn't crop or blur the picture)
            WindowedModeWrapper::EnableDpiAwareness();
            auto entry = hook::pattern("6A 70 68 ? ? ? ? E8 ? ? ? ? 33 DB 53 8B 3D ? ? ? ? FF D7");
            CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, entry);
        });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(hModule);
    UNREFERENCED_PARAMETER(lpReserved);
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    return TRUE;
}
