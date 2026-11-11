module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

export module Updates;

import ComVars;
import NativeMenu;

// MAIN.CheckForUpdates: the notice is a message box of the main menu
SafetyHookInline shAppRequestExit = {};
void __cdecl AppRequestExit(int force)
{
    RWGTA::UpdateCheck::OnExit();
    shAppRequestExit.unsafe_ccall(force);
}

export void InitUpdateCheck()
{
    RWGTA::UpdateCheck::Config config;
    config.pluginId = "SplinterCellPandoraTomorrow.FusionFix";
    config.fileVersion = rsc_FileVersion;
    config.settings = &FixSettings();
    config.playable = []()
    {
        return !bPlayingVideo && !bDisplayingBackground && PTMenu::NoticeReady();
    };
    config.show = [](const std::wstring& line)
    {
        PTMenu::ShowNotice(line);
    };
    RWGTA::UpdateCheck::Init(std::move(config));
    WFP::onGameProcessEvent() += []()
    {
        RWGTA::UpdateCheck::Tick();
    };
    shAppRequestExit = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"Core"), "?appRequestExit@@YAXH@Z"), AppRequestExit);
}
