#include "stdafx.h"
#include "FixSettings.hpp"

import ComVars;

// The settings' runtime interface for menu hosts and editors (AsiSettings_GetApi)
ASI_SETTINGS_EXPORT(FixSettings())

void Init()
{
    // The game objects the modules use
    // Every module needs these: without them the fix stays off.
    auto gameMode = hook::pattern("8B 0D ? ? ? ? E8 ? ? ? ? 84 C0 75 0E");
    auto render = hook::pattern("8B 0D ? ? ? ? 6A 00 E8 ? ? ? ? EB 5E");
    auto viewports = hook::pattern("8B 0D ? ? ? ? 56 E8 ? ? ? ? 3B C5");
    if (gameMode.empty() || render.empty() || viewports.empty())
    {
        OutputDebugStringA("Resident Evil 6 Fusion Fix: unsupported game version.\n");
        return;
    }
    pGameMode.SetAddress(*gameMode.get_first<uintptr_t*>(2));
    pRender.SetAddress(*render.get_first<uintptr_t*>(2));
    pViewports.SetAddress(*viewports.get_first<uintptr_t*>(2));

    WFP::onInitEvent().executeAll();

    // Serve the initialized settings to editors while the game runs.
    asi_settings::LiveLink::Start();
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("85 C0 74 CB 8B 0D"));
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
