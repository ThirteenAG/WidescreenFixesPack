#include "stdafx.h"
#include "FixSettings.hpp"
#include "Camera.hpp"
#include "Frame.hpp"
#include "Focus.hpp"
#include "Video.hpp"
#include "Pacing.hpp"
#include "DisplayModes.hpp"

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
            FixSettings();
            asi_settings::LiveLink::Start();
            ScarfaceCamera::Install(FixSettings().Float("MAIN.FOVFactor"));
            ScarfaceDisplayModes::Install();
            ScarfaceFrame::Install();
            ScarfacePacing::Install(FixSettings().Int("MAIN.FPSLimit"));
            ScarfaceCutscene::Install(FixSettings().Int("MAIN.CutsceneBorders"), bool(ScarfaceCamera::ProjectionHook));
            ScarfaceVideo::Install(FixSettings().Int("MAIN.FMVWidescreenMode"));
            ScarfaceFocus::Install(FixSettings().Int("MAIN.PauseOnFocusLoss"));
            WFP::onInitEvent().executeAll();
        }, hook::pattern("B9 ? ? ? ? E8 ? ? ? ? 5E C3"));
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