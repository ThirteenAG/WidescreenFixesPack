#include "stdafx.h"

import Main;

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(InitFusionFix, hook::pattern("41 55 41 56 48 83 EC 38 48 89 5C 24 50 45 33 ED 48 89 74 24 60 48 89 7C"));
    });
}

BOOL APIENTRY DllMain(HMODULE, DWORD reason, LPVOID reserved)
{
    if (reason == DLL_PROCESS_ATTACH && !IsUALPresent())
        InitializeASI();
    else if (reason == DLL_PROCESS_DETACH && !reserved)
        WFP::onShutdownEvent().executeAll();
    return TRUE;
}
