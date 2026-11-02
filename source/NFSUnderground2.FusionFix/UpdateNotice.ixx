module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"


export module UpdateNotice;

import ComVars;

// The update notice in the game's own OK dialog, over the main menu
export namespace NFSU2UpdateNotice
{
    inline void Tick() { RWGTA::UpdateCheck::Tick(); }
    inline void OnExit() { RWGTA::UpdateCheck::OnExit(); }

    constexpr const char* MainMenu = "UI_Main.fng";

    inline bool Install(asi_settings::Plugin& settings, std::function<bool()> frontend,
        std::function<bool(const char*)> package, std::function<std::string(const std::string&)> encode)
    {
        // The OK dialog with a printf text (parent package, dialog package, format, arguments); the dialog shown
        auto dialog = hook::pattern("81 EC 38 03 00 00 8D 4C 24 00 E8 ? ? ? ? 8B 84 24 48 03 00 00 8B 8C 24 44 03 00 00 50 51 8D 54 24 08 52 E8 ? ? ? ? 8B 8C 24 48 03 00 00 8B 94 24 4C 03 00 00 B8 01 00 00 00");
        auto current = hook::pattern("A1 ? ? ? ? 8B 4C 24 04 50 51 68 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 83 C4 0C 3B C1 75");
        static auto showDialog = reinterpret_cast<int (__cdecl*)(const char*, const char*, const char*, const void*)>(dialog.get_first());
        static const bool initialized = [&]()
        {
            CurrentDialog.SetAddress(*current.get_first<int32_t*>(1));
            return true;
        }();
        (void)initialized;
        RWGTA::UpdateCheck::Config config;
        config.pluginId = "NFSUnderground2.FusionFix";
        config.fileVersion = rsc_FileVersion;
        config.settings = &settings;
        config.playable = [frontend, package]()
        {
            return frontend() && CurrentDialog == 0 && package(MainMenu);
        };
        config.show = [encode](const std::wstring& notice)
        {
            // An empty dialog package: the game picks the dialog's size by the text. The dialog copies the text.
            auto text = encode(asi_settings::text::Narrow(notice));
            const char* arguments[] = { text.c_str() };
            showDialog(MainMenu, "", "%s", arguments);
        };
        RWGTA::UpdateCheck::Init(std::move(config));
        return true;
    }
}
