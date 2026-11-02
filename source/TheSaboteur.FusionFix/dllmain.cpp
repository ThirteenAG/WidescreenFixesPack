#include "stdafx.h"
#include "FixSettings.hpp"
#include "RuntimeBridge.hpp"
#include <d3d9.h>


import NativeMenu;
import DeveloperOverlay;
import ComVars;

void Init()
{
    FixSettings();
    asi_settings::LiveLink::Start();
    SaboteurMenu::Install();
    SaboteurRuntime::Install();
    SaboteurOverlay::Install();
    [&]()
    {
        auto pattern = hook::pattern("8B 91 ? ? ? ? 50 FF D2 8B CE");
        static auto BeforeEndSceneHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            Direct3DDevice = (IDirect3DDevice9*)regs.eax;
            WFP::onEndScene().executeAll();
            SaboteurOverlay::Frame(Direct3DDevice);
            SaboteurMenu::Frame(SaboteurOverlay::BlocksInput());
        });

        pattern = hook::pattern("8B 85 ? ? ? ? 8D B0 ? ? ? ? 8B 85");
        static auto ResetHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            Direct3DDevice = nullptr;
            WFP::onBeforeReset().executeAll();
        });
    }();

    WFP::onInitEvent().executeAll();
}

ASI_SETTINGS_EXPORT(FixSettings())

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime([]()
        {
            SaboteurMenu::InstallEarly();
            Init();
        }, hook::pattern("D9 44 24 04 56 DC 0D ? ? ? ? 8B F1 D9 5C 24 08"));
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
