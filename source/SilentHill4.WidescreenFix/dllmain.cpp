#include "stdafx.h"
#include "FixSettings.hpp"
#include "../Shared/SilentHill/Display.hpp"

import System;
import Widescreen;
import Camera;
import Input;
import Menu;

// The settings' runtime interface for menu hosts and editors (AsiSettings_GetApi)
ASI_SETTINGS_EXPORT(FixSettings())

void Init()
{
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

    auto reset = hook::pattern("83 EC 48 53 33 DB 33 C0 57 B9 0C 00 00 00 8D 7C 24 20 89 5C 24 1C F3 AB");
    auto fullscreen = hook::pattern("A1 ? ? ? ? 8B 0D ? ? ? ? 89 44 24 1C 39 1D ? ? ? ? BF 01 00 00 00");
    if (reset.size() == 1 && fullscreen.size() == 1)
    {
        static auto nativeReset = reinterpret_cast<HRESULT(__cdecl*)(int)>(reset.get_first());
        static auto nativeFullscreen = *fullscreen.get_first<int32_t*>(17);
        SHDisplay::Detail::NativeModeReset = [](SHDisplay::Mode mode)
        {
            ProtectedGameRef<int32_t>{ nativeFullscreen } = mode == SHDisplay::Mode::Fullscreen ? 1 : 0;
            return nativeReset(1);
        };
    }
    else
        OutputDebugStringA("Silent Hill 4: native window reset patterns unavailable.\n");

    // The camera and input modules find code the widescreen module hooks, so they come first
    System::Init();
    Camera::Init();
    Input::Init();
    Widescreen::Init();

    // Option 4 (Advanced Options) of the Options screen
    Menu::Init();
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
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
        CursorConfinement::Shutdown();
    }
    return TRUE;
}
