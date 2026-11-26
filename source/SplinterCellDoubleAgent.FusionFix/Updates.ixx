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
    config.format = [](const std::string& latest, bool hub)
    {
        auto texts = FixSettings().GetTexts();
        texts.Select(DAMenu::Language());
        auto line = texts.Resolve(hub ? "@update.availablehub" : "@update.availablesite");
        line = RWGTA::UpdateCheck::detail::Replace(line, "{name}", texts.Resolve(FixSettings().GetDescription().name));
        line = RWGTA::UpdateCheck::detail::Replace(line, "{version}", latest);
        return asi_settings::text::Widen(line);
    };
    config.tryShow = DAMenu::ShowNotice;
    RWGTA::UpdateCheck::Init(std::move(config));
    WFP::onGameProcessEvent() += []()
    {
        RWGTA::UpdateCheck::Tick();
    };
}
