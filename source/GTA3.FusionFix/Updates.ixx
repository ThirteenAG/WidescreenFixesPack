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
    void (__cdecl* SetHelpMessage)(const wchar_t*, bool) = nullptr;
}

class Updates
{
public:
    Updates()
    {
        // Found before other modules hook these functions
        WFP::onInitEvent() += []()
        {
            // gGameState, compared with 8 in RwTextureGtaStreamRead
            auto state = hook::pattern("83 3D ? ? ? ? 08 75 6E");
            auto help = hook::pattern("80 3D ? ? ? ? 01 75 07 C3");
            GameState.SetAddress(*state.get_first<int32_t*>(2));
            SetHelpMessage = reinterpret_cast<decltype(SetHelpMessage)>(help.get_first());
        };

        WFP::onGameInitEvent() += []()
        {
            RWGTA::UpdateCheck::Config config;
            config.pluginId = "GTA3.FusionFix";
            config.fileVersion = rsc_FileVersion;
            config.settings = &FixSettings();
            config.playable = []()
            {
                return *GameState == 9 && !CMenuManager::m_bIsActive && !CCutsceneMgr::IsRunning() && !TheCamera->m_WideScreenOn;
            };
            config.show = [](const std::wstring& line)
            {
                static std::wstring text;
                text = line;
                SetHelpMessage(text.c_str(), false);
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
