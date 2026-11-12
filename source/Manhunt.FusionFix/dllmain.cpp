#include "stdafx.h"
#include "FixSettings.hpp"
#include "Shared/WidescreenFixConflict.hpp"

import Skeleton;
import Misc;

// The settings' runtime interface for menu hosts and editors (AsiSettings_GetApi)
ASI_SETTINGS_EXPORT(FixSettings())

void Init()
{
    InitSkeletonRequiredRefs();
    auto pattern = hook::pattern("89 35 ? ? ? ? 89 3D ? ? ? ? E8 ? ? ? ? 3D 01 08 00 00 73 ? A1 ? ? ? ? C7 40 1C");

    FixSettings();
    asi_settings::LiveLink::Start();

    InitMiscAtLoad();

    static auto WinMainHook = safetyhook::create_mid(pattern.get_first(17), [](SafetyHookContext& regs)
    {
        WFP::onInitEvent().executeAll();
    });

    // CApp::Idle, once per frame of the main loop
    pattern = hook::pattern("8B 15 ? ? ? ? 89 15 ? ? ? ? A1 ? ? ? ? 8B 48 0C");
    static auto MainLoopHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        static std::once_flag of;
        std::call_once(of, []()
        {
            WFP::onGameInitEvent().executeAll();
        });

        RWGTA::GameThread::Run();
        WFP::onGameProcessEvent().executeAll();
    });
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        if (WidescreenFixConflict::Found())
            return;
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("C7 05 ? ? ? ? ? ? ? ? 83 3D ? ? ? ? 00 75 0F"));
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
