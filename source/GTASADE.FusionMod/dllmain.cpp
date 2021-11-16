#include "stdafx.h"
#include <assembly64.hpp>

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
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
