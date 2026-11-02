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
    config.pluginId = "SplinterCell.FusionFix";
    config.fileVersion = rsc_FileVersion;
    config.settings = &FixSettings();
    config.playable = []()
    {
        return !bPlayingVideo && UObject::GetState(L"EPCConsole") == L"UWindow" && SCMenu::NoticeReady(gViewport);
    };
    config.show = [](const std::wstring& line)
    {
        SCMenu::ShowNotice(gViewport, line);
    };
    RWGTA::UpdateCheck::Init(std::move(config));
    WFP::onGameProcessEvent() += []()
    {
        RWGTA::UpdateCheck::Tick();
    };
    if (auto exit = GetProcAddress(GetModuleHandle(L"Core"), "?appRequestExit@@YAXH@Z"))
        shAppRequestExit = safetyhook::create_inline(exit, AppRequestExit);
}
