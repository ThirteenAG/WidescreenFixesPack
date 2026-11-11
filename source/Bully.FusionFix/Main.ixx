module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/CursorConfinement.hpp"
#include "RawMouse.hpp"

export module Main;

import ComVars;
import DisplayModes;
import Video;
import Focus;
import ButtonIcons;
import Hud;
import Menu;
import Updates;
import Cutscene;
import Camera;
import Overlay;
import Input;
import Window;
import ScreenPatches;
import Geography;


class Main
{
public:
    Main()
    {
        WFP::onInitEvent() += []()
        {
            WindowedModeWrapper::EnableDpiAwareness();
            static auto& fFOVFactor = FixSettings().Float("MAIN.FOVFactor");
            asi_settings::LiveLink::Start();
            BullyMenu::Init();
            BullyUpdates::Install();
            BullyRawMouse::Install(FixSettings().Int("MAIN.RawInputMouse"));
            DisplayModes::Init();
            CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
            FixSettings().OnChange("MAIN.ConfineCursor", []()
            {
                CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
            });
            BullyWindow::Install(FixSettings().Int("MAIN.WindowMode"));
            BullyFocus::Install(FixSettings().Int("MAIN.PauseOnFocusLoss"));
            BullyVideo::Install();
            BullyIcons::Install(FixSettings().Int("MAIN.GamepadIcons"));
            BullyInput::Install(FixSettings().Int("MAIN.AutoGamepad"), FixSettings().Float("MAIN.GamepadCameraSpeed"), FixSettings().Float("MAIN.GamepadDeadzone"));
            BullyMenu::UsingPad = BullyInput::UsingPad;
            BullyMenu::PadHelp = BullyInput::HelpPressed;
            BullyMenu::BlocksInput = []()
            {
                return BullyOverlay::BlocksInput() || (BullyWindow::Window && GetForegroundWindow() != BullyWindow::Window);
            };
            if (FixSettings().Bool("MAIN.SkipIntro"))
            {
                // Jump over the logo movie call and its two arguments (push 2; push ecx; mov ecx, eax; call edx)
                auto logos = hook::pattern("8B 0D ? ? ? ? 8B 10 8B 52 04 6A 02 51 8B C8 FF D2 C6 05 ? ? ? ? 00");
                injector::WriteMemory<uint16_t>(logos.get_first<uint16_t>(11), 0x05EB, true);
            }


            ScreenPatches::Install();
            BullyGeography::Install();
            GameplayCamera::Install(fFOVFactor);
        };
    }
} Main;
