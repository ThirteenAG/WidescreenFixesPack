module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

// Update notice: in the help line of the fix's journal page
export module Updates;

import NativeMenu;

export namespace PsychonautsUpdates
{
    inline std::atomic_bool dismissNotice{};

    void Tick()
    {
        if (dismissNotice.load(std::memory_order_relaxed) && dismissNotice.exchange(false, std::memory_order_relaxed))
            PsychonautsMenu::notice = {};
        RWGTA::UpdateCheck::Tick();
    }

    inline void WINAPI Exit(UINT code)
    {
        RWGTA::UpdateCheck::OnExit();
        ExitProcess(code);
    }

    inline void Init()
    {
        IATHook::Replace(GetModuleHandleW(nullptr), "KERNEL32.dll", std::forward_as_tuple("ExitProcess", Exit));
        RWGTA::UpdateCheck::Config config;
        config.pluginId = "Psychonauts.FusionFix";
        config.fileVersion = rsc_FileVersion;
        config.settings = &FixSettings();
        config.playable = []() { return true; };
        config.format = [](const std::string& latest, bool hub)
        {
            // The journal formats the result using the current settings language.
            PsychonautsMenu::notice = { latest, RWGTA::UpdateCheck::Page(), hub };
            return std::wstring{};
        };
        config.show = [](const std::wstring&) {};
        FixSettings().OnChange("MAIN.CheckForUpdates", []()
        {
            if (FixSettings().Get("MAIN.CheckForUpdates") == "0")
                dismissNotice.store(true, std::memory_order_relaxed);
        });
        RWGTA::UpdateCheck::Init(std::move(config));
    }
}
