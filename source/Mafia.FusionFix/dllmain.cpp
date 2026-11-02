#include "stdafx.h"
#include "FixSettings.hpp"
#include "Shared/WidescreenFixConflict.hpp"

import ComVars;
import Controller;
import Frontend;
import Misc;
import Mouse;
import PostFX;
import RestoredFeatures;
import WidescreenFix;
import Window;

// The settings' runtime interface for menu hosts and editors (AsiSettings_GetApi)
ASI_SETTINGS_EXPORT(FixSettings())

// The game; the per-frame events come from the engine's present (Window)
void Init()
{
    WindowedModeWrapper::EnableDpiAwareness();
    InitWidescreenFix();
    WFP::onInitEvent().executeAll();
}

// The engine DLL (LS3DF.dll, or LSV11.dll/LSV10.dll in renamed installs)
void InitLS3DF()
{
    WindowedModeWrapper::EnableDpiAwareness();
    InitDevicePointer();
    RestoredFeatures::InitProjectorOpacity();
    InitWindowMode();
    InitShadowZBias();
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        if (WidescreenFixConflict::Found())
            return;
        FixSettings();
        asi_settings::LiveLink::Start();
        // Game code of 1.0, and of 1.1/1.2; Setup.exe has neither
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("A1 ? ? ? ? 8B 3D ? ? ? ? 52 89 4C 24 30"));
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("A1 ? ? ? ? 8B 0D ? ? ? ? 50 89 54 24 24"));
        CallbackHandler::RegisterCallback(L"rw_data.dll", RestoredFeatures::InitEyeAnimations);
        for (auto name : { L"LS3DF.dll", L"LSV11.dll", L"LSV10.dll" })
        {
            CallbackHandler::RegisterCallback(name, +[]()
            {
                static std::once_flag flag;
                std::call_once(flag, []()
                {
                    InitLS3DF();
                    InitPostFX();
                });
            });
        }
        CallbackHandler::RegisterCallback(L"IJoy.dll", InitIJoy);
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
