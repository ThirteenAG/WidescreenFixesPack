module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

export module Updates;

import NativeMenu;

export namespace PunisherUpdates
{
    inline void WINAPI Exit(UINT code)
    {
        RWGTA::UpdateCheck::OnExit();
        ExitProcess(code);
    }

    inline void Init()
    {
        IATHook::Replace(GetModuleHandleW(nullptr), "KERNEL32.dll", std::forward_as_tuple("ExitProcess", Exit));
        RWGTA::UpdateCheck::Config config;
        config.pluginId = "ThePunisher.FusionFix";
        config.fileVersion = rsc_FileVersion;
        config.settings = &FixSettings();
        config.playable = []()
        {
            const auto& state = PunMenu::Get();
            return state.open && !state.busy && state.notice.empty();
        };
        config.format = [](const std::string&, bool hub)
        {
            return asi_settings::text::Widen(PunMenu::Get().model.Text(hub ? "update.availablehub" : "update.availablesite"));
        };
        config.show = [](const std::wstring& text)
        {
            auto& state = PunMenu::Get();
            if (!state.open)
                return;
            state.notice = asi_settings::text::Narrow(text);
            state.details = false;
            if (!PunMenu::Build())
                PunMenu::Close();
        };
        RWGTA::UpdateCheck::Init(std::move(config));
        PunMenu::Tick = []() { RWGTA::UpdateCheck::Tick(); };
    }
}
