#include "stdafx.h"
#include "FixSettings.hpp"

import ComVars;
import SystemState;

// The settings' runtime interface for menu hosts and editors (AsiSettings_GetApi)
ASI_SETTINGS_EXPORT(FixSettings())

void Init()
{
    // The game objects the modules use
    auto pattern = hook::pattern("8B 3D ? ? ? ? 6A 08");
    pRender.SetAddress(*pattern.get_first<uintptr_t*>(2));

    WFP::onInitEvent().executeAll();

    // Serve the initialized settings to editors while the game runs.
    asi_settings::LiveLink::Start();
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        using SetContext = BOOL(WINAPI*)(HANDLE);
        if (const auto set = reinterpret_cast<SetContext>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetProcessDpiAwarenessContext")))
            set(reinterpret_cast<HANDLE>(-4));
        else
            SetProcessDPIAware();

        RevSystemState::Init();
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
