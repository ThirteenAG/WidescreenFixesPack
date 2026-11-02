module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include "Frame.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

export module Updates;

import NativeMenu;

export namespace ScarfaceUpdates
{
    inline void WINAPI Exit(UINT code)
    {
        RWGTA::UpdateCheck::OnExit();
        ExitProcess(code);
    }

    inline void Install()
    {
        const bool available = ScarfaceMenu::enabled && bool(ScarfaceFrame::PresentHook);
        FixSettings().Availability("MAIN.CheckForUpdates", [available](std::string_view value)
        {
            return value == "0" || available;
        });
        if (!available)
            return;

        IATHook::Replace(GetModuleHandleW(nullptr), "KERNEL32.dll", std::forward_as_tuple("ExitProcess", Exit));
        RWGTA::UpdateCheck::Config config;
        config.pluginId = "Scarface.FusionFix";
        config.fileVersion = rsc_FileVersion;
        config.settings = &FixSettings();
        config.playable = []() { return ScarfaceMenu::active && !ScarfaceMenu::opened && !ScarfaceMenu::refresh; };
        config.format = [](const std::string&, bool hub)
        {
            return ScarfaceMenu::Words(hub ? "update.availablehub" : "update.availablesite");
        };
        config.show = [](const std::wstring& message)
        {
            ScarfaceMenu::shownLanguage = ScarfaceMenu::Language();
            ScarfaceMenu::model.OpenPlugins(ScarfaceMenu::Module(), ScarfaceMenu::shownLanguage, 32, {}, true);
            ScarfaceMenu::view = {};
            ScarfaceMenu::view.notice = asi_settings::text::Narrow(message);
            ScarfaceMenu::opened = ScarfaceMenu::view.Build(ScarfaceMenu::model, true);
            ScarfaceMenu::refresh = true;
        };
        RWGTA::UpdateCheck::Init(std::move(config));
        ScarfaceFrame::Handlers.push_back([](void*) { RWGTA::UpdateCheck::Tick(); });
    }
}
