#include "stdafx.h"
#include "FixSettings.hpp"
#include "Shared/D3D8Compatibility.hpp"
#include "Shared/WidescreenFixConflict.hpp"

import ComVars;
import Overlay;

// The settings' runtime interface for menu hosts and editors (AsiSettings_GetApi)
ASI_SETTINGS_EXPORT(FixSettings())

void Init()
{
    // Settings described in settings.json and stored in the plugin's .cfg
    FixSettings();
    // FusionFixHub changes the values while the game runs (the live link, served for all plugins of the game)
    asi_settings::LiveLink::Start();

    auto pattern = hook::pattern("88 15 ? ? ? ? 8D 45");
    bPause.SetAddress(*pattern.get_first<bool*>(2));

    pattern = hook::pattern("32 C0 88 81 ? ? ? ? A2 ? ? ? ? E8 ? ? ? ? 33 C0 C3");
    bCutscene.SetAddress(*pattern.get_first<bool*>(9));

    pattern = hook::pattern("83 3D ? ? ? ? ? 74 ? 84 DB");
    nLoading.SetAddress(*pattern.get_first<uint32_t*>(2));

    TCOverlay::Init();

    WFP::onInitEvent().executeAll();
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        if (WidescreenFixConflict::Found())
            return;
        WindowedModeWrapper::EnableDpiAwareness();
        D3D8Compatibility::Initialize(true, true);
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("BF 94 00 00 00 8B C7"));
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
