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

            FixSettings();

            asi_settings::LiveLink::Start();


            CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("81 EC 04 01 00 00 53 55 56"));

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
