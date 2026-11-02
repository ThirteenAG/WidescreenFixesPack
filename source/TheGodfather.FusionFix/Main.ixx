module;

#include <stdafx.h>
#include "FloatingPoint.hpp"
#include <d3d9.h>
#include "FixSettings.hpp"


export module Main;

import VertexTransform;

import Window;
import HudAspect;
import HudPatches;

import ComVars;
import Timing;
import Cinematics;
import Camera;
import Intro;
import ControllerCamera;
import Focus;
import Vibration;
import HudModels;
import Updates;
import Movies;
import Input;
import NativeMenu;
import Overlay;
import ControllerIcons;
import VideoModes;
import Paths;

class Main
{
public:
    Main()
    {
        WFP::onInitEvent() += []()
        {
            auto& settings = FixSettings();
            asi_settings::LiveLink::Start();
            GodfatherVideoModes::Init();
            GodfatherPaths::Init();
            GodfatherMenu::Init();
            GodfatherIntro::Init();
            GodfatherUpdates::Init();
            onResChange() += GodfatherHudPatches::UpdateScreen;
            GodfatherWindow::resized = [](int width, int height)
            {
                onResChange().executeAll(width, height);
            };
            GodfatherWindow::Init(Screen.nWidth, Screen.nHeight);
            const bool timing = GodfatherTiming::Init();
            settings.Availability("MAIN.FPSLimit", [timing](std::string_view value)
            {
                return value == "30" || timing;
            });
            GodfatherFocus::Init();
            GodfatherCinematics::Init();
            GodfatherMovies::Init();
            GodfatherOverlay::Install();
            GodfatherCamera::Init();
            GodfatherIcons::Init();
            GodfatherControllerCamera::Init();
            GodfatherVibration::Init();

            GodfatherHudPatches::Install();
        };
    }
} Main;
