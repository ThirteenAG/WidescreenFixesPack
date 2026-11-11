module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"


export module UpdateNotice;

import ComVars;

// The update notice in the game's own one-button dialog, over the main menu
export namespace NFSCUpdateNotice
{
    inline void Tick() { RWGTA::UpdateCheck::Tick(); }
    inline void OnExit() { RWGTA::UpdateCheck::OnExit(); }

    constexpr const char* MainMenu = "FeMainMenu.fng";

    // show: the dialog with the text (already in the font's encoding)
    inline bool Install(asi_settings::Plugin& settings, std::function<bool()> playable,
        std::function<void(const std::string&)> show, std::function<std::string(const std::string&)> encode)
    {
        RWGTA::UpdateCheck::Config config;
        config.pluginId = "NFSCarbon.FusionFix";
        config.fileVersion = rsc_FileVersion;
        config.settings = &settings;
        config.playable = std::move(playable);
        config.show = [show = std::move(show), encode = std::move(encode)](const std::wstring& notice)
        {
            show(encode(asi_settings::text::Narrow(notice)));
        };
        RWGTA::UpdateCheck::Init(std::move(config));
        return true;
    }
}
