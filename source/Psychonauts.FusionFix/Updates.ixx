module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

// Update notice: in the help line of the fix's journal page
export module Updates;

import NativeMenu;

export namespace PsychonautsUpdates
{
    inline void WINAPI Exit(UINT code)
    {
        RWGTA::UpdateCheck::OnExit();
        ExitProcess(code);
    }

    inline void Init(bool menu)
    {
        FixSettings().Availability("MAIN.CheckForUpdates", [menu](std::string_view value)
        {
            return value == "0" || menu;
        });
        if (!menu)
            return;
        IATHook::Replace(GetModuleHandleW(nullptr), "KERNEL32.dll", std::forward_as_tuple("ExitProcess", Exit));
        RWGTA::UpdateCheck::Config config;
        config.pluginId = "Psychonauts.FusionFix";
        config.fileVersion = rsc_FileVersion;
        config.settings = &FixSettings();
        config.playable = []() { return true; };
        config.format = [](const std::string& latest, bool hub)
        {
            auto line = PsychonautsMenu::Text(hub ? "update.availablehub" : "update.availablesite");
            for (auto [key, value] : { std::pair<std::string, std::string>{ "{name}", PsychonautsMenu::Text("plugin.name") },
                std::pair<std::string, std::string>{ "{version}", latest } })
            {
                if (auto at = line.find(key); at != std::string::npos)
                    line.replace(at, key.size(), value);
            }
            return asi_settings::text::Widen(line);
        };
        config.show = [](const std::wstring& line)
        {
            PsychonautsMenu::notice = asi_settings::text::Narrow(line);
        };
        RWGTA::UpdateCheck::Init(std::move(config));
    }
}
