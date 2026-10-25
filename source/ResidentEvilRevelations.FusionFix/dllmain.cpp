#include "stdafx.h"
#include "FixSettings.hpp"

import ComVars;

// The settings' runtime interface for menu hosts and editors (AsiSettings_GetApi)
ASI_SETTINGS_EXPORT(FixSettings())

void Init()
{
    // The game objects the modules use
    // Every module needs the renderer: without it the fix stays off.
    auto pattern = hook::pattern("8B 3D ? ? ? ? 6A 08");
    if (pattern.empty())
    {
        OutputDebugStringA("Resident Evil Revelations Fusion Fix: unsupported game version.\n");
        return;
    }
    pRender.SetAddress(*pattern.get_first<uintptr_t*>(2));

    WFP::onInitEvent().executeAll();

    // Serve the initialized settings to editors while the game runs.
    asi_settings::LiveLink::Start();
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("8B 4E 68 85 C9 75 47"));
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
