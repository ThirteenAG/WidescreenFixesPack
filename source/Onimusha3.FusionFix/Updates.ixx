module;
#include <stdafx.h>
#include "Shared/RWGTA/UpdateCheck.hpp"
#include "FixSettings.hpp"

export module Updates;
import NativeMenu;


export namespace OnimushaUpdates
{
    inline void WINAPI Exit(UINT code)
    {
        RWGTA::UpdateCheck::OnExit();
        ExitProcess(code);
    }

    inline void Init()
    {
        IATHook::Replace(GetModuleHandleW(nullptr), "KERNEL32.dll", std::forward_as_tuple("ExitProcess", Exit));
        RWGTA::UpdateCheck::Config config;
        config.pluginId = "Onimusha3.FusionFix";
        config.fileVersion = rsc_FileVersion;
        config.settings = &FixSettings();
        config.playable = []()
        {
            return OnimushaMenu::Opened && OnimushaMenu::Page.notice.empty() && !OnimushaMenu::Page.details;
        };
        config.format = [](const std::string&, bool hub)
        {
            return asi_settings::text::Widen(OnimushaMenu::Model.Text(hub ? "update.availablehub" : "update.availablesite"));
        };
        config.show = [](const std::wstring& text)
        {
            OnimushaMenu::Page.ShowNotice(asi_settings::text::Narrow(text));
        };
        RWGTA::UpdateCheck::Init(std::move(config));
        OnimushaMenu::Tick = []() { RWGTA::UpdateCheck::Tick(); };
    }
}
