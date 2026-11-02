module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"


export module Updates;

import NativeText;

export namespace NeoUpdates
{
    inline void WINAPI Exit(UINT code)
    {
        RWGTA::UpdateCheck::OnExit();
        ExitProcess(code);
    }

    inline void Install()
    {
        const bool available = static_cast<bool>(NeoText::TextHook);
        FixSettings().Availability("MAIN.CheckForUpdates", [available](std::string_view value)
        {
            return value == "0" || available;
        });
        if (!available)
            return;
        IATHook::Replace(GetModuleHandleW(nullptr), "KERNEL32.dll", std::forward_as_tuple("ExitProcess", Exit));
        RWGTA::UpdateCheck::Config config;
        config.pluginId = "TheMatrixPathOfNeo.FusionFix";
        config.fileVersion = rsc_FileVersion;
        config.settings = &FixSettings();
        config.playable = []() { return NeoText::Current.Ready; };
        config.show = [](const std::wstring& text)
        {
            NeoText::Notice = text;
            NeoText::NoticeUntil = GetTickCount64() + 10000;
        };
        RWGTA::UpdateCheck::Init(std::move(config));
        NeoText::Tick = RWGTA::UpdateCheck::Tick;
    }
}
