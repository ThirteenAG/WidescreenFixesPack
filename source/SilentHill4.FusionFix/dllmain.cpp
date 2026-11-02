#include "stdafx.h"
#include "FixSettings.hpp"
#include "Shared/SilentHill/Display.hpp"
#include "Shared/WidescreenFixConflict.hpp"

import ComVars;

// The settings' runtime interface for menu hosts and editors (AsiSettings_GetApi)
ASI_SETTINGS_EXPORT(FixSettings())

void Init()
{
    SetCurrentDirectoryW(GetExeModulePath<std::filesystem::path>().c_str());
    // Settings described in settings.json and stored in the plugin's .cfg; FusionFixHub changes them while the game
    // runs through the live link, served for all plugins of the game
    FixSettings();
    asi_settings::LiveLink::Start();
    CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
    FixSettings().OnChange("MAIN.ConfineCursor", []()
    {
        CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
    });

    SHDisplay::Install(FixSettings().Int("MAIN.WindowMode") == 0 ? SHDisplay::Mode::Fullscreen : (FixSettings().Int("MAIN.WindowMode") == 1 ? SHDisplay::Mode::Windowed : SHDisplay::Mode::Borderless));

    SHDisplay::OnPresent().push_back([]()
    {
        auto window = SHDisplay::Get().window;
        while (window && IsWindow(window) && GetForegroundWindow() != window && FixSettings().Bool("MAIN.PauseOnFocusLoss"))
        {
            MsgWaitForMultipleObjects(0, nullptr, FALSE, 50, QS_ALLINPUT);
            MSG message{};
            while (PeekMessageW(&message, window, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }
        }
    });

    auto reset = hook::pattern("83 EC 48 53 33 DB 33 C0 57 B9 0C 00 00 00 8D 7C 24 20 89 5C 24 1C F3 AB");
    auto fullscreen = hook::pattern("A1 ? ? ? ? 8B 0D ? ? ? ? 89 44 24 1C 39 1D ? ? ? ? BF 01 00 00 00");
    static auto nativeReset = reinterpret_cast<HRESULT(__cdecl*)(int)>(reset.get_first());
    bFullscreen.SetAddress(*fullscreen.get_first<int32_t*>(17));
    SHDisplay::Detail::NativeModeReset = [](SHDisplay::Mode mode)
    {
        bFullscreen = mode == SHDisplay::Mode::Fullscreen ? 1 : 0;
        return nativeReset(1);
    };

    // Game state the camera and the widescreen code both read, bound before the widescreen code hooks the camera
    // code it is read from
    aPlayerStatus.SetAddress(*hook::pattern("55 8B EC 83 E4 F0 83 EC 14 8B 0D ? ? ? ? A1 ? ? ? ? 8B 15 ? ? ? ? 53 56 89 0D").get(0).get<uint8_t*>(0x10));
    aMoveIntent.SetAddress(*hook::pattern("B8 ? ? ? ? C3 90 90 90 90 90 90 90 90 90 90 A1 ? ? ? ? C3 90 90 90 90 90 90 90 90 90 90 8B 15").get(0).get<float*>(1));
    auto gameCamera = hook::pattern("68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 50 E8 ? ? ? ? 83 C4 10 E9");
    aCameraMatrix.SetAddress(*gameCamera.get(0).get<float*>(1));
    fCameraFov.SetAddress(*gameCamera.get(0).get<float*>(0x1A));
    ApplyCamera = reinterpret_cast<decltype(ApplyCamera)>(gameCamera.get(0).get<void>(0));
    nGameMode.SetAddress(*hook::pattern("A1 ? ? ? ? 8B 34 85 ? ? ? ? 33 FF").get(0).get<int32_t*>(1));
    nSceneFlags.SetAddress(*hook::pattern("A1 ? ? ? ? 83 E0 01 C3 90 90 90 90 90 90 90 55 8B EC 83 EC 1C").get(0).get<int32_t*>(1));

    // System, Camera, Input, Widescreen and Option 4 (Advanced Options) of the Options screen (Menu)
    WFP::onInitEvent().executeAll();
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        if (WidescreenFixConflict::Found())
            return;
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("8B 0D ? ? ? ? 6A 00 50 51"));
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
        CursorConfinement::Shutdown();
    }
    return TRUE;
}
