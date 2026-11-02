module;

#include "stdafx.h"
#include <d3d9.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"


export module Updates;

import ComVars;
import NativeMenu;

export namespace Driv3rUpdates
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
        const bool available = Driv3rMenu::ready;
        FixSettings().Availability("MAIN.CheckForUpdates", [available](std::string_view value)
        {
            return value == "0" || available;
        });
        if (!available) return;
        IATHook::Replace(GetModuleHandle(nullptr), "KERNEL32.dll", std::forward_as_tuple("ExitProcess", Exit));
        RWGTA::UpdateCheck::Config config;
        config.pluginId = "Driv3r.FusionFix";
        config.fileVersion = rsc_FileVersion;
        config.settings = &FixSettings();
        config.playable = Driv3rMenu::NoticeReady;
        config.show = Driv3rMenu::ShowNotice;
        RWGTA::UpdateCheck::Init(std::move(config));
    }
}
