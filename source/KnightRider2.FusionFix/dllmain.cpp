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
            // Per monitor DPI awareness, so the game gets real pixels on scaled desktops (the desktop resolution and a
            // full screen picture that isn't scaled up from its top left part)
            using SetContext = BOOL(WINAPI*)(HANDLE);
            if (auto set = reinterpret_cast<SetContext>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetProcessDpiAwarenessContext")))
                set(reinterpret_cast<HANDLE>(-4));
            else
                SetProcessDPIAware();

            CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("8D 4C 24 18 50 51 6A 04 8B CE"));
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
