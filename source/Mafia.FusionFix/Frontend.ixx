module;

#include <stdafx.h>

export module Frontend;

import ComVars;
import NativeMenu;
import Controller;
import ButtonIcons;
import Updates;
import DeveloperOverlay;

// The native settings page, pad navigation, button icons and the update notice; each needs the menu set up first
class Frontend
{
public:
    Frontend()
    {
        WFP::onInitEvent() += []()
        {
            MafiaMenu::viewportWidth = []() { return Screen.fWidth; };
            MafiaMenu::Init();
            MafiaPad::Install();
            MafiaIcons::Install();
            MafiaUpdates::Install();
            MafiaMenu::blocksInput = []()
            {
                auto window = WindowedModeWrapper::GameHWND;
                return MafiaOverlay::BlocksInput() || (window && GetForegroundWindow() != window);
            };
        };

        WFP::onGameProcessEvent() += []()
        {
            MafiaPad::Frame(WindowedModeWrapper::GameHWND);
            MafiaUpdates::Tick();
            MafiaMenu::Notice();
        };
    }
} Frontend;
