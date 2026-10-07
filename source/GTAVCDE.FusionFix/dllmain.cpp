#include "stdafx.h"

import Main;

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(InitFusionFix);
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
