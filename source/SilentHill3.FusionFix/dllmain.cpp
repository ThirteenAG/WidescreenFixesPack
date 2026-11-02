#include "stdafx.h"
#include "FixSettings.hpp"
#include "Shared/SilentHill/Display.hpp"
#include "Shared/WidescreenFixConflict.hpp"

import ComVars;
import System;

// The settings' runtime interface for menu hosts and editors (AsiSettings_GetApi)
ASI_SETTINGS_EXPORT(FixSettings())

void Init()
{
    // The game's assets are found relative to the working directory
    SetCurrentDirectoryW(GetExeModulePath<std::filesystem::path>().c_str());

    if (Registry::Patch())
        return;     // SH3Config.exe

    // Settings described in settings.json and stored in the plugin's .cfg; FusionFixHub changes them while the game
    // runs through the live link, served for all plugins of the game
    FixSettings();
    asi_settings::LiveLink::Start();
    CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
    FixSettings().OnChange("MAIN.ConfineCursor", []()
    {
        CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
    });

    // Display
    const int mode = std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2);
    SHDisplay::Install(mode == 0 ? SHDisplay::Mode::Fullscreen : (mode == 1 ? SHDisplay::Mode::Windowed : SHDisplay::Mode::Borderless));
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

    auto reset = hook::pattern("A0 ? ? ? ? 83 EC 10 53 33 DB 3A C3 56 57 74 ? 38 1D ? ? ? ? 74 ? E8 ? ? ? ? 38 1D");
    auto force = hook::pattern("38 1D ? ? ? ? 0F 84 ? ? ? ? 3A C3 8B 35 ? ? ? ? 8B 3D");
    auto endScene = hook::pattern("A1 ? ? ? ? 85 C0 75 18 E8 ? ? ? ? 8B 08 50 FF 91 8C 00 00 00 C7 05 ? ? ? ? 6C 08 76 88 33 C0 C3");
    static auto nativeReset = reinterpret_cast<int(__cdecl*)()>(reset.get_first());
    static auto nativeEndScene = reinterpret_cast<void(__cdecl*)()>(endScene.get_first());
    bFullscreen.SetAddress(*reset.get_first<uint8_t*>(19));
    bForceDisplayReset.SetAddress(*force.get_first<uint8_t*>(2));
    SHDisplay::Detail::NativeModeReset = [](SHDisplay::Mode mode) -> HRESULT
    {
        bFullscreen = mode == SHDisplay::Mode::Fullscreen ? 1 : 0;
        bForceDisplayReset = 1;
        const int result = nativeReset();
        // The native mode change reopens the scene; Present requires it closed.
        nativeEndScene();
        return result ? D3D_OK : D3DERR_INVALIDCALL;
    };

    // Game state several modules read, bound before any module patches the code it is read from
    pPlayer.SetAddress(*hook::pattern("FF A1 ? ? ? ? 89 35 ? ? ? ? 89 86 80 01 00 00 8B 0D ? ? ? ?").get_first<uint8_t**>(8));
    bAiming.SetAddress(*hook::pattern("A1 ? ? ? ? 83 EC 08 85 C0 0F 84 ? ? ? ? 8A 0D ? ? ? ? 84 C9 0F 84 ? ? ? ?").get_first<uint8_t*>(18));
    nEventId.SetAddress(*hook::pattern("E8 ? ? ? ? 8B 15 ? ? ? ? 53 52 57 E8 ? ? ? ? 83 C4 0C 83 F8 FF A3 ? ? ? ? 89 1D ? ? ? ?").get_first<int32_t*>(26));
    nCameraMode.SetAddress(*hook::pattern("A1 ? ? ? ? 83 EC 2C 83 F8 01 0F 84 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 83 F8 08").get_first<int32_t*>(1));
    nCameraFlags.SetAddress(*hook::pattern("A1 ? ? ? ? 8B 4C 24 0C 83 E0 FB 83 C8 01 85 C9 A3 ? ? ? ? 74 08 83 C8 04 A3 ? ? ? ?").get_first<uint32_t*>(1));
    nRenderMode.SetAddress(*hook::pattern("A2 ? ? ? ? A2 ? ? ? ? 74 4D 49").get_first<uint8_t*>(1));
    bPauseActive.SetAddress(*hook::pattern("A0 ? ? ? ? 83 EC 24 84 C0 75 09 B8 01 00 00 00 83 C4 24 C3 C6 05").get_first<uint8_t*>(1));
    auto projection = hook::pattern("85 C0 74 ? E8 ? ? ? ? 51 D9 1C 24 E8 ? ? ? ?");
    auto setter = injector::GetBranchDestination(projection.get_first(13)).get<uint8_t>();
    fProjectionDistance.SetAddress(*reinterpret_cast<float**>(setter + 5));
    DrawGlyphs = reinterpret_cast<decltype(DrawGlyphs)>(hook::get_pattern("8B 44 24 04 85 C0 74 41 8B 44 24 08"));
    MeasureLines = reinterpret_cast<decltype(MeasureLines)>(hook::get_pattern("83 EC 0C 55 8B 6C 24 18 33 D2 3B EA"));

    // System, Widescreen, Quality, Camera, the Advanced Options page (Menu) and Input
    WFP::onInitEvent().executeAll();
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        if (WidescreenFixConflict::Found())
            return;
        auto entry = hook::pattern("89 65 E8 8B F4 89 3E 56");
        CallbackHandler::RegisterCallback(Init, entry);
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
