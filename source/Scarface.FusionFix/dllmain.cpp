#include "stdafx.h"
#include "FixSettings.hpp"
#include "Frame.hpp"

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
import VehicleGlass;

ASI_SETTINGS_EXPORT(FixSettings())

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime([]()
        {
            WindowedModeWrapper::EnableDpiAwareness();
            auto& settings = FixSettings();
            auto publish = []()
            {
                static std::mutex mutex;
                std::lock_guard lock(mutex);
                auto& settings = FixSettings();
                const auto text = settings.Get("MAIN.FOVFactor");
                float factor = 1.0f;
                std::from_chars(text.data(), text.data() + text.size(), factor);
                ScarfaceCamera::Factor.store(std::isfinite(factor) ? std::clamp(factor, 0.5f, 2.0f) : 1.0f);
                ScarfaceCutscene::Borders.store(std::clamp(std::atoi(settings.Get("MAIN.CutsceneBorders").c_str()), 0, 3));
                ScarfaceVideo::Mode.store(std::atoi(settings.Get("MAIN.FMVWidescreenMode").c_str()));
                ScarfaceFocus::Pause.store(settings.Get("MAIN.PauseOnFocusLoss") == "1");
            };
            for (const char* key : { "MAIN.FOVFactor", "MAIN.CutsceneBorders", "MAIN.FMVWidescreenMode", "MAIN.PauseOnFocusLoss" })
                settings.OnChange(key, publish);
            publish();
            asi_settings::LiveLink::Start();
            ScarfaceCamera::Install();
            ScarfaceDisplayModes::Install();
            ScarfaceFrame::Install();
            ScarfacePacing::Install();
            ScarfaceCutscene::Install();
            ScarfaceVideo::Install();
            ScarfaceFocus::Install();
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
