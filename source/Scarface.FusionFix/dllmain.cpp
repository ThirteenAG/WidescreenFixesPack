#include "stdafx.h"
#include "FixSettings.hpp"
#include "Frame.hpp"
#include "Shared/D3D9Options.hpp"

import Video;
import DisplayModes;
import Pacing;
import Camera;
import DeveloperOverlay;
import Vibration;
import Updates;
import Cutscene;
import Focus;
import NativeMenu;
import Intro;
import ComVars;
import Input;
import Windowed;
import HUD;
import Fixes;
import FramerateVigilante;
import PostFX;

ASI_SETTINGS_EXPORT(FixSettings())

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime([]()
        {
            WindowedModeWrapper::EnableDpiAwareness();
            D3D9Options::presentParameters = [](D3DPRESENT_PARAMETERS* parameters, HWND)
            {
                // The presentation limiter owns pacing, including rates above the monitor refresh rate.
                parameters->PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
            };
            D3D9Options::HookCreation();
            FixSettings();
            asi_settings::LiveLink::Start();
            ScarfaceCamera::Install(FixSettings().Float("MAIN.FOVFactor"));
            ScarfaceDisplayModes::Install();
            ScarfaceFrame::Install();
            ScarfacePacing::Install(FixSettings().Int("MAIN.FPSLimit"));
            ScarfaceCutscene::Install(FixSettings().Int("MAIN.CutsceneBorders"), bool(ScarfaceCamera::ProjectionHook));
            ScarfaceVideo::Install(FixSettings().Int("MAIN.FMVWidescreenMode"));
            ScarfaceFocus::Install(FixSettings().Int("MAIN.PauseOnFocusLoss"));
            ScarfaceIntro::Install();
            ScarfaceMenu::Install();
            ScarfaceFrame::Handlers.push_back([](void*) { ScarfaceMenu::Tick(); });
            ScarfaceUpdates::Install();
            ScarfaceOverlay::Install();
            ScarfaceVibration::Install();
            WFP::onInitEvent().executeAll();
        }, hook::pattern("53 56 8B F1 33 DB F6 46 60 08 0F 84 ? ? ? ? 6A FF 53 53 53"));
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
