module;

#include <stdafx.h>

#include "common.h"
#include "FixSettings.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"
#include "Shared/RWGTA/FixMenu.hpp"

export module Updates;

import Menu;
import Camera;
import CutsceneMgr;

// MAIN.CheckForUpdates: see Shared/RWGTA/UpdateCheck.hpp. The notice is the game's help box.
namespace
{
    GameRef<int32_t> GameState;                 // gGameState: 9 = playing
    void (__cdecl* SetHelpMessage)(const char*, bool, bool, bool) = nullptr;
}

class Updates
{
public:
    Updates()
    {
        // Found before other modules hook these functions
        WFP::onInitEvent() += []()
        {
            // gGameState, read by WinMain; CHud::SetHelpMessage
            auto state = hook::pattern("A1 ? ? ? ? 83 F8 09 0F 87 ? ? ? ? FF 24 85 ? ? ? ? ? ? E8");
            auto help = hook::pattern("A0 ? ? ? ? 84 C0 0F 85 4F");
            GameState.SetAddress(*state.get_first<int32_t*>(1));
            SetHelpMessage = reinterpret_cast<decltype(SetHelpMessage)>(help.get_first());
        };

        WFP::onGameInitEvent() += []()
        {
            RWGTA::UpdateCheck::Config config;
            config.pluginId = "GTASA.FusionFix";
            config.fileVersion = rsc_FileVersion;
            config.settings = &FixSettings();
            config.playable = []()
            {
                return *GameState == 9 && !FrontendMenuManager->m_bMenuActive && !CCutsceneMgr::IsRunning() && !TheCamera->m_bWideScreenOn;
            };
            config.show = [](const std::wstring& line)
            {
                // In the game's 8-bit font
                static std::string text;
                text.clear();
                int size = WideCharToMultiByte(CP_UTF8, 0, line.c_str(), int(line.size()), nullptr, 0, nullptr, nullptr);
                std::string utf8(size_t(std::max(size, 0)), '\0');
                WideCharToMultiByte(CP_UTF8, 0, line.c_str(), int(line.size()), utf8.data(), size, nullptr, nullptr);
                for (auto c : RWGTAMenu::ToGameFont(utf8, false))
                    text += char(c < 0x100 ? c : '?');
                SetHelpMessage(text.c_str(), false, false, false);
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
