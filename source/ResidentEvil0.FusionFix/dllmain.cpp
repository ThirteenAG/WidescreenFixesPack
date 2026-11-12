#include "stdafx.h"
#include "FixSettings.hpp"

import ComVars;
import Shaders;
import System;
import LightSync;
import NativeMenu;
import InputDrain;
import OverlayRuntime;
import SkipStart;
import Resolution;
import Video;
import ButtonIcons;
import ClassicKeys;

// The settings' runtime interface for menu hosts and editors (AsiSettings_GetApi)
ASI_SETTINGS_EXPORT(FixSettings())

void Init()
{
    static std::atomic<bool> started{ false };
    if (started.exchange(true)) return;
    // Settings described in settings.json and stored in the plugin's .cfg; FusionFixHub changes them while the game
    // runs through the live link
    FixSettings();
    HMODULE module = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&Init), &module);
    RE0Menu::RememberStartupValues(module);
    asi_settings::LiveLink::Start();

    WFP::onInitEvent().executeAll();
    RE0Resolution::Install();
    AddWindowedResolution = RE0Resolution::AddWindowedResolution;

    RE0Icons::Install(module);
    RE0Video::Install();
    RE0Start::Install(RE0Input::Blocked);

    RE0OverlayRuntime::Install();

    RE0Menu::Install(module);
    // After the menu, which reads the start of a menu reader this hooks
    RE0ClassicKeys::Install();
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        WindowedModeWrapper::EnableDpiAwareness();
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("56 8B F1 57 8B 06 8B 7C 24 0C 3B C7"));
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    return TRUE;
}
