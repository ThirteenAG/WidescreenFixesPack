#include "stdafx.h"

import ComVars;
import Input;
import Windowed;
import HUD;
import Fixes;
import FramerateVigilante;
import PostFX;

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime([]()
        {
            WFP::onInitEvent().executeAll();
        }, hook::pattern("B9 ? ? ? ? E8 ? ? ? ? 5E C3"));
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
        WFP::onShutdownEvent().executeAll();
    }
    return TRUE;
}