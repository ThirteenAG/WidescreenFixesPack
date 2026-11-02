module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"


export module Updates;

import Menu;

export namespace BullyUpdates
{
    void WINAPI Exit(UINT code)
    {
        RWGTA::UpdateCheck::OnExit();
        ExitProcess(code);
    }

    void Install()
    {
        const bool available = BullyMenu::NoticeReady();
        FixSettings().Availability("MAIN.CheckForUpdates", [available](std::string_view value)
        {
            return value == "0" || available;
        });
        if (!available)
            return;
        IATHook::Replace(GetModuleHandle(nullptr), "KERNEL32.dll", std::forward_as_tuple("ExitProcess", Exit));
        RWGTA::UpdateCheck::Config config;
        config.pluginId = "Bully.FusionFix";
        config.fileVersion = rsc_FileVersion;
        config.settings = &FixSettings();
        config.playable = []() { return BullyMenu::NoticeReady(); };
        config.show = BullyMenu::ShowNotice;
        RWGTA::UpdateCheck::Init(std::move(config));
    }
}

class UpdateEvents
{
public:
    UpdateEvents()
    {
        WFP::onGameProcessEvent() += []()
        {
            RWGTA::UpdateCheck::Tick();
        };
    }
} UpdateEvents;
