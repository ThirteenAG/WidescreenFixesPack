module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

export module Updates;

import NativeMenu;

// The update notice in the native menu
export namespace MafiaUpdates
{
    void WINAPI Exit(UINT code)
    {
        RWGTA::UpdateCheck::OnExit();
        ExitProcess(code);
    }

    void Tick()
    {
        RWGTA::UpdateCheck::Tick();
    }

    void Install()
    {
        const bool available = MafiaMenu::ready;
        FixSettings().Availability("MAIN.CheckForUpdates", [available](std::string_view value)
        {
            return value == "0" || available;
        });
        if (!available) return;
        IATHook::Replace(GetModuleHandle(nullptr), "KERNEL32.dll", std::forward_as_tuple("ExitProcess", Exit));
        RWGTA::UpdateCheck::Config config;
        config.pluginId = "Mafia.FusionFix";
        config.fileVersion = rsc_FileVersion;
        config.settings = &FixSettings();
        config.playable = MafiaMenu::NoticeReady;
        config.show = MafiaMenu::ShowNotice;
        RWGTA::UpdateCheck::Init(std::move(config));
    }
}
