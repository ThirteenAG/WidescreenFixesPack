#include "stdafx.h"
#include "FixSettings.hpp"
#include "Shared/WidescreenFixConflict.hpp"

import NativeMenu;
import Core;

ASI_SETTINGS_EXPORT(FixSettings())

// The game's own exes whose display, window and HUD code the fix doesn't find; other programs in the folder load it
// silently

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
        {
        if (WidescreenFixConflict::Found())
            return;
            // Real pixels on scaled desktops (desktop resolution, full screen picture not scaled up from a corner)
            WindowedModeWrapper::EnableDpiAwareness();
            FixSettings();
            asi_settings::LiveLink::Start();
            CallbackHandler::RegisterCallback(L"suffering.rfl", SufferingMenu::Init);
            CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime([]() { WFP::onInitEvent().executeAll(); }, hook::pattern("75 4B D9 45 ? D8 5D ? DF E0 F6 C4 41"));
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
