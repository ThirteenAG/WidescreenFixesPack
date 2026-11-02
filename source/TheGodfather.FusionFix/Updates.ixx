module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"
#include <stdexcept>

export module Updates;

import NativeMenu;

namespace GodfatherUpdates
{
    void WINAPI Exit(UINT code)
    {
        RWGTA::UpdateCheck::OnExit();
        ExitProcess(code);
    }

    export void Init()
    {
        const bool available = GodfatherMenu::state.gammaHook && GodfatherMenu::state.actionsHook;
        FixSettings().Availability("MAIN.CheckForUpdates", [available](std::string_view value)
        {
            return value == "0" || available;
        });
        if (!available)
            throw std::runtime_error("Update notice dependency unavailable");
        IATHook::Replace(GetModuleHandleW(nullptr), "KERNEL32.dll", std::forward_as_tuple("ExitProcess", Exit));
        RWGTA::UpdateCheck::Config config;
        config.pluginId = "TheGodfather.FusionFix";
        config.fileVersion = rsc_FileVersion;
        config.settings = &FixSettings();
        config.playable = []()
        {
            return GodfatherMenu::state.open && GodfatherMenu::state.root && GodfatherMenu::state.view.notice.empty();
        };
        config.format = [](const std::string&, bool hub)
        {
            return asi_settings::text::Widen(GodfatherMenu::state.model.Text(hub ? "update.availablehub" : "update.availablesite"));
        };
        config.show = [](const std::wstring& text)
        {
            auto& screen = GodfatherMenu::state;
            if (!screen.open || !screen.root)
                return;
            screen.view = {};
            screen.view.notice = asi_settings::text::Narrow(text);
            screen.selected = 0;
            GodfatherMenu::Build();
        };
        RWGTA::UpdateCheck::Init(std::move(config));
        GodfatherMenu::tick = []() { RWGTA::UpdateCheck::Tick(); };
    }
}
