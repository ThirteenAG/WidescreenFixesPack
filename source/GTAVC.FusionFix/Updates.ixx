module;

#include <stdafx.h>
#include "common.h"
#include "FixSettings.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

export module Updates;

import Menu;
import Camera;
import CutsceneMgr;

// MAIN.CheckForUpdates: see Shared/RWGTA/UpdateCheck.hpp. The notice is the game's help box.
namespace
{
    GameRef<int32_t> GameState;                 // gGameState: 9 = playing
    void (__cdecl* SetHelpMessage)(const wchar_t*, bool, bool) = nullptr;
}

class Updates
{
public:
    Updates()
    {
        // Found before other modules hook these functions
        WFP::onInitEvent() += []()
        {
            // gGameState, compared with 9 in WndProc
            GameState.SetAddress(*hook::pattern("83 3D ? ? ? ? 09 75 0B").get_first<int32_t*>(2));
            SetHelpMessage = reinterpret_cast<decltype(SetHelpMessage)>(hook::pattern("80 3D ? ? ? ? 01 53 8B").get_first());
        };

        WFP::onGameInitEvent() += []()
        {
            RWGTA::UpdateCheck::Config config;
            config.pluginId = "GTAVC.FusionFix";
            config.fileVersion = rsc_FileVersion;
            config.settings = &FixSettings();
            config.playable = []()
            {
                return *GameState == 9 && !FrontendMenuManager->m_bMenuActive && !CCutsceneMgr::IsRunning() && !TheCamera->m_WideScreenOn;
            };
            config.show = [](const std::wstring& line)
            {
                static std::wstring text;
                text = line;
                SetHelpMessage(text.c_str(), false, false);
            };
            RWGTA::UpdateCheck::Init(std::move(config));

            WFP::onGameProcessEvent() += []()
            {
                RWGTA::UpdateCheck::Tick();
            };
            WFP::onShutdownEvent() += []()
            {
                RWGTA::UpdateCheck::OnExit();
            };
        };
    }
} Updates;
