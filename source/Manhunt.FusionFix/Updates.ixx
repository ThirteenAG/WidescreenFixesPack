module;

#include <stdafx.h>
#include "common.h"
#include "Shared/RWGTA/FixMenu.hpp"
#include "FixSettings.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

export module Updates;

import NativeMenuABI;
import NativeMenuView;


import NativeMenu;
import Vibration;

export namespace MHUpdates
{
    inline void WINAPI Exit(UINT code)
    {
        MHRumble::Shutdown();
        RWGTA::UpdateCheck::OnExit();
        ExitProcess(code);
    }

    inline bool Ready()
    {
        auto state = MHMenu::Storage();
        return state && state->enabled && !state->opened && MHMenu::ABI::CurrentMenu() == 10;
    }

    inline void Install()
    {
        const bool available = MHMenu::Storage() && MHMenu::Storage()->enabled;
        FixSettings().Availability("MAIN.CheckForUpdates", [available](std::string_view value)
        {
            return value == "0" || available;
        });
        if (!available)
            return;

        IATHook::Replace(GetModuleHandleW(nullptr), "KERNEL32.dll", std::forward_as_tuple("ExitProcess", Exit));
        RWGTA::UpdateCheck::Config config;
        config.pluginId = "Manhunt.FusionFix";
        config.fileVersion = rsc_FileVersion;
        config.settings = &FixSettings();
        config.playable = Ready;
        config.format = [](const std::string&, bool hub)
        {
            auto line = RWGTAMenu::PluginText(FixSettings(), MHMenu::ABI::Language(),
                hub ? "update.availablehub" : "update.availablesite");
            return asi_settings::text::Widen(line);
        };
        config.show = [](const std::wstring& message)
        {
            auto state = MHMenu::Storage();
            if (state && state->Open())
            {
                state->view.notice = asi_settings::text::Narrow(message);
                state->refresh = true;
            }
        };
        RWGTA::UpdateCheck::Init(std::move(config));
    }
}
