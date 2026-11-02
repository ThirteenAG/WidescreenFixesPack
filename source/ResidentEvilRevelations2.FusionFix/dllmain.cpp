#include "stdafx.h"
#include "FixSettings.hpp"

import ComVars;
import Runtime;

// The settings' runtime interface for menu hosts and editors (AsiSettings_GetApi)
ASI_SETTINGS_EXPORT(FixSettings())

void Init()
{
    // The game objects the modules use
    auto gameMode = hook::pattern("8B 0D ? ? ? ? 55 E8 ? ? ? ? 39 86 E0 0C 00 00");
    auto render = hook::pattern("8B 0D ? ? ? ? 56 8B 81 28 0D 00 00");
    pGameMode.SetAddress(*gameMode.get_first<uintptr_t*>(2));
    pRender.SetAddress(*render.get_first<uintptr_t*>(2));

    Rev2Runtime::Init();
    WFP::onInitEvent().executeAll();

    // Serve the initialized settings to editors while the game runs.
    asi_settings::LiveLink::Start();
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("F3 0F 5C 15 ? ? ? ? F3 0F 59 E5"));
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
