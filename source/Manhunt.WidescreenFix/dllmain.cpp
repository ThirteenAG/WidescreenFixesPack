#include "stdafx.h"
#include "../Shared/CheckedPattern.hpp"
#include "FixSettings.hpp"

// The settings' runtime interface for menu hosts and editors (AsiSettings_GetApi)
ASI_SETTINGS_EXPORT(FixSettings())

void Init()
{
    // WinMain (CApp::GameInitialise), before the window and the engine. Its first store sets the skeleton's globals;
    // when they are set already, the plugin was loaded after the game had started (Ultimate ASI Loader as ddraw.dll,
    // which the game loads late), too late for the fix to set up anything
    auto pattern = hook::pattern("C7 05 ? ? ? ? ? ? ? ? 83 3D ? ? ? ? 00 75 0F");
    if (pattern.empty())
    {
        wfp::patterns::Report("Manhunt: startup signature unavailable; fix disabled.\n");
        return;
    }
    if (**pattern.get_first<uint32_t*>(2) == *pattern.get_first<uint32_t>(6))
    {
        MessageBoxW(nullptr, L"The widescreen fix was loaded too late to work: Ultimate ASI Loader is installed as ddraw.dll, "
            L"which the game loads only after it has started.\n\nDelete ddraw.dll and use the dinput8.dll that comes with the fix."
            L"\n\nThe game will continue without the fix.", L"Manhunt.WidescreenFix", MB_ICONWARNING | MB_OK | MB_SETFOREGROUND);
        return;
    }

    FixSettings();
    asi_settings::LiveLink::Start();

    static auto WinMainHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        WFP::onInitEvent().executeAll();
    });

    // CApp::Idle, once per frame of the main loop
    pattern = hook::pattern("8B 15 ? ? ? ? 89 15 ? ? ? ? A1 ? ? ? ? 8B 48 0C");
    if (pattern.empty())
    {
        OutputDebugStringA("Manhunt.WidescreenFix: main loop not found\n");
        return;
    }
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
