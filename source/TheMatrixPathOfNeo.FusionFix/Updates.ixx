module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"


export module Updates;

import NativeText;

export namespace NeoUpdates
{
    inline std::atomic<bool> DismissNotice = false;
    inline std::string Latest, Page;
    inline bool HasHub = false;
    inline ULONGLONG NoticeUntil = 0;

    inline std::wstring CurrentNotice()
    {
        if (DismissNotice.exchange(false))
            NoticeUntil = 0;
        RWGTA::UpdateCheck::Tick();
        if (GetTickCount64() >= NoticeUntil || FixSettings().Get("MAIN.CheckForUpdates") != "1")
            return {};

        auto& texts = FixSettings().GetTexts();
        auto line = texts.Resolve(HasHub ? "@update.availablehub" : "@update.availablesite");
        using RWGTA::UpdateCheck::detail::Replace;
        line = Replace(std::move(line), "{name}", texts.Resolve(FixSettings().GetDescription().name));
        line = Replace(std::move(line), "{version}", Latest);
        line = Replace(std::move(line), "{page}", Page);
        return asi_settings::text::Widen(line);
    }

    inline void WINAPI Exit(UINT code)
    {
        RWGTA::UpdateCheck::OnExit();
        ExitProcess(code);
    }

    inline void Install()
    {
        IATHook::Replace(GetModuleHandleW(nullptr), "KERNEL32.dll", std::forward_as_tuple("ExitProcess", Exit));
        RWGTA::UpdateCheck::Config config;
        config.pluginId = "TheMatrixPathOfNeo.FusionFix";
        config.fileVersion = rsc_FileVersion;
        config.settings = &FixSettings();
        config.playable = []() { return NeoText::Current.Ready; };
        // Keep the release data so the title or pause screen can resolve the current language.
        config.format = [](const std::string& latest, bool hub)
        {
            Latest = latest;
            HasHub = hub;
            Page = RWGTA::UpdateCheck::Page();
            return std::wstring{};
        };
        config.show = [](const std::wstring&)
        {
            NoticeUntil = GetTickCount64() + 10000;
        };
        FixSettings().OnChange("MAIN.CheckForUpdates", []()
        {
            if (FixSettings().Get("MAIN.CheckForUpdates") == "0")
                DismissNotice.store(true);
        });
        RWGTA::UpdateCheck::Init(std::move(config));
        NeoText::UpdateNotice = CurrentNotice;
    }
}
