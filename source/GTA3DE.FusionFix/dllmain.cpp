#include "stdafx.h"

import Main;

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(InitFusionFix, hook::pattern("48 89 5C 24 10 48 89 6C 24 18 48 89 74 24 20 48 89 4C 24 08 57 41 54 41 55 41 56 41 57 48 83 EC 40 45 33 ED 4C 8D 3D ?"));
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
