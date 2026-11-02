module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

export module Updates;

import ComVars;
import NativeMenu;

// MAIN.CheckForUpdates: the notice is a message box of the main menu
SafetyHookInline AppRequestExitHook;
void __cdecl AppRequestExit(int force)
{
    RWGTA::UpdateCheck::OnExit();
    AppRequestExitHook.unsafe_ccall<void>(force);
}

export void InitUpdateCheck()
{
    AppRequestExitHook = safetyhook::create_inline(GetProcAddress(GetModuleHandleW(L"Core.dll"), "?appRequestExit@@YAXH@Z"), AppRequestExit);
    RWGTA::UpdateCheck::Config config;
    config.pluginId = "SplinterCellDoubleAgent.FusionFix";
    config.fileVersion = rsc_FileVersion;
    config.settings = &FixSettings();
    config.playable = []() { return DAMenu::NoticeReady(); };
    config.show = [](const std::wstring& text) { DAMenu::ShowNotice(text); };
    RWGTA::UpdateCheck::Init(std::move(config));
    WFP::onGameProcessEvent() += []()
    {
        RWGTA::UpdateCheck::Tick();
    };
}
