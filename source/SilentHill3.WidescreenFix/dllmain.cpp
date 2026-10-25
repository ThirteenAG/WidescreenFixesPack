#include "stdafx.h"
#include "FixSettings.hpp"
#include "../Shared/SilentHill/Display.hpp"

import System;
import Widescreen;
import Quality;
import Camera;
import Input;
import Menu;
import AntiAliasing;

// The settings' runtime interface for menu hosts and editors (AsiSettings_GetApi)
ASI_SETTINGS_EXPORT(FixSettings())

void Init()
{
    // The game's assets are found relative to the working directory
    SetCurrentDirectoryW(GetExeModulePath<std::filesystem::path>().c_str());

    if (System::PatchRegistry())
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

    // Display and device events.
    AntiAliasing::Init();
    SHDisplay::OnDeviceReady().push_back([](IDirect3DDevice8* device) { AntiAliasing::DeviceReady(device); });
    SHDisplay::OnBeforeReset().push_back([](IDirect3DDevice8*) { AntiAliasing::BeforeReset(); });
    SHDisplay::OnPresent().push_back([]() { AntiAliasing::Present(); });
    const int mode = std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2);
    SHDisplay::Install(mode == 0 ? SHDisplay::Mode::Fullscreen : (mode == 1 ? SHDisplay::Mode::Windowed : SHDisplay::Mode::Borderless));

    auto reset = hook::pattern("A0 ? ? ? ? 83 EC 10 53 33 DB 3A C3 56 57 74 ? 38 1D ? ? ? ? 74 ? E8 ? ? ? ? 38 1D");
    auto force = hook::pattern("38 1D ? ? ? ? 0F 84 ? ? ? ? 3A C3 8B 35 ? ? ? ? 8B 3D");
    auto endScene = hook::pattern("A1 ? ? ? ? 85 C0 75 18 E8 ? ? ? ? 8B 08 50 FF 91 8C 00 00 00 C7 05 ? ? ? ? 6C 08 76 88 33 C0 C3");
    if (reset.size() == 1 && force.size() == 1 && endScene.size() == 1)
    {
        static auto nativeReset = reinterpret_cast<int(__cdecl*)()>(reset.get_first());
        static auto nativeEndScene = reinterpret_cast<void(__cdecl*)()>(endScene.get_first());
        static auto nativeFullscreen = *reset.get_first<uint8_t*>(19);
        static auto forceReset = *force.get_first<uint8_t*>(2);
        SHDisplay::Detail::NativeModeReset = [](SHDisplay::Mode mode) -> HRESULT
        {
            ProtectedGameRef<uint8_t>{ nativeFullscreen } = mode == SHDisplay::Mode::Fullscreen ? 1 : 0;
            ProtectedGameRef<uint8_t>{ forceReset } = 1;
            const int result = nativeReset();
            // The native mode change reopens the scene; Present requires it closed.
            nativeEndScene();
            return result ? D3D_OK : D3DERR_INVALIDCALL;
        };
    }
    else
        OutputDebugStringA("Silent Hill 3: native window reset patterns unavailable.\n");

    System::Init();
    Widescreen::Init();
    Quality::Init();
    Camera::Init();
    Input::Init();

    // Advanced Options, a page of the game's Options screen
    Menu::Init();
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(Init, hook::pattern("89 65 E8 8B F4 89 3E 56"));
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
        CursorConfinement::Shutdown();
        AntiAliasing::Shutdown();
    }
    return TRUE;
}
