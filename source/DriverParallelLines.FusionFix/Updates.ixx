module;

#include "stdafx.h"
#include <d3d9.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"


export module Updates;

import ComVars;
import NativeMenu;

export namespace DPLUpdates
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
        IATHook::Replace(GetModuleHandle(nullptr), "KERNEL32.dll", std::forward_as_tuple("ExitProcess", Exit));
        RWGTA::UpdateCheck::Config config;
        config.pluginId = "DriverParallelLines.FusionFix";
        config.fileVersion = rsc_FileVersion;
        config.settings = &FixSettings();
        config.playable = DPLMenu::NoticeReady;
        config.show = DPLMenu::ShowNotice;
        RWGTA::UpdateCheck::Init(std::move(config));
    }
}
