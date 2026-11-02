#include "stdafx.h"
#include "FixSettings.hpp"
#include "Shared/WidescreenFixConflict.hpp"

import Misc;

void Init()
{
    WFP::onInitEvent().executeAll();
}

ASI_SETTINGS_EXPORT(FixSettings())

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        if (WidescreenFixConflict::Found())
            return;
        WindowedModeWrapper::EnableDpiAwareness();
        CallbackHandler::RegisterModuleLoadCallback(L"LS3DF.dll", []()
        {
            CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("C7 45 EC ? ? ? ? FF D6"));
        });
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(InitSettings, hook::pattern("0F ? ? ? ? ? 81 ? 80 02 00 00"));
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent())
            InitializeASI();
    }
    return TRUE;
}
