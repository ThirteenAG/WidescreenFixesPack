#include "stdafx.h"
#include "FixSettings.hpp"
#include "Shared/D3D8Compatibility.hpp"
#include "Shared/WidescreenFixConflict.hpp"
#include <atomic>

import Misc;

ASI_SETTINGS_EXPORT(FixSettings())

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        if (WidescreenFixConflict::Found())
            return;
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime([]()
        {
            D3D8Compatibility::Initialize(false, false, []()
            {
                return PunisherFrameLimit.load(std::memory_order_relaxed);
            });
            WindowedModeWrapper::EnableDpiAwareness();
            WFP::onInitEvent().executeAll();
        }, hook::pattern("89 1D ? ? ? ? A3 ? ? ? ? 88 0D"));
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
