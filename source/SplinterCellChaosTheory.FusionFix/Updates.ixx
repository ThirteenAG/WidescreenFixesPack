module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

export module Updates;

import ComVars;
import NativeMenu;

// MAIN.CheckForUpdates: the notice is a message box of the main menu
SafetyHookInline AppRequestExitHook;
int __cdecl AppRequestExit(int force)
{
    RWGTA::UpdateCheck::OnExit();
    return AppRequestExitHook.unsafe_ccall<int>(force);
}

export void InitUpdateCheck()
{
    auto exit = hook::pattern("8B 44 24 04 85 C0 74 08 6A 01 FF 15 ? ? ? ? C7 05 ? ? ? ? 01 00 00 00 C3");
    AppRequestExitHook = safetyhook::create_inline(exit.get_first(), AppRequestExit);
    RWGTA::UpdateCheck::Config config;
    config.pluginId = "SplinterCellChaosTheory.FusionFix";
    config.fileVersion = rsc_FileVersion;
    config.settings = &FixSettings();
    config.playable = []() { return CTMenu::NoticeReady(); };
    config.show = [](const std::wstring& line) { CTMenu::ShowNotice(line); };
    RWGTA::UpdateCheck::Init(std::move(config));
}
