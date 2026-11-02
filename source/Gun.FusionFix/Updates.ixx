module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

export module Updates;

import Frontend;
import Imports;

export namespace GunUpdates
{
    inline void WINAPI Exit(UINT code)
    {
        RWGTA::UpdateCheck::OnExit();
        ExitProcess(code);
    }

    inline void Init()
    {
        const bool available = GunMenu::Get().enabled;
        FixSettings().Availability("MAIN.CheckForUpdates", [available](std::string_view value)
        {
            return value == "0" || available;
        });
        if (!available)
            return;
        GunImports::Replace("KERNEL32.dll", "ExitProcess", &Exit);
        RWGTA::UpdateCheck::Config config;
        config.pluginId = "Gun.FusionFix";
        config.fileVersion = rsc_FileVersion;
        config.settings = &FixSettings();
        config.playable = []()
        {
            const auto& state = GunMenu::Get();
            return state.open && !state.details && !state.busy && GunMenu::Current();
        };
        config.format = [](const std::string&, bool hub)
        {
            return asi_settings::text::Widen(GunMenu::Get().model.Text(hub ? "update.availablehub" : "update.availablesite"));
        };
        config.show = [](const std::wstring& text)
        {
            auto& state = GunMenu::Get();
            if (!state.open || !GunMenu::Current())
                return;
            state.help = asi_settings::text::Narrow(text);
            state.notice = state.details = true;
            state.helpSlice = 0;
            if (!GunMenu::BuildHelp())
                GunMenu::Close();
        };
        RWGTA::UpdateCheck::Init(std::move(config));
        GunMenu::Frame = []() { RWGTA::UpdateCheck::Tick(); };
    }
}
