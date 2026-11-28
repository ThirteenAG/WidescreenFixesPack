module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

export module Updates;

import Frontend;

export namespace FarCryUpdates
{
    inline void WINAPI Exit(UINT code)
    {
        RWGTA::UpdateCheck::OnExit();
        ExitProcess(code);
    }

    inline void BindModule(HMODULE module)
    {
        if (module)
            IATHook::Replace(module, "KERNEL32.dll", std::forward_as_tuple("ExitProcess", Exit));
    }

    inline void Init()
    {
        FixSettings().Availability("MAIN.CheckForUpdates", [](std::string_view value)
        {
            return value == "0" || FarCryMenu::ready;
        });
        RWGTA::UpdateCheck::Config config;
        config.pluginId = rsc_InternalName;
        config.fileVersion = rsc_FileVersion;
        config.settings = &FixSettings();
        config.playable = []()
        {
            const auto owner = FarCryMenu::context;
            return owner && owner->open && FarCryMenu::pending.empty() && FarCryMenu::Available(owner);
        };
        config.format = [](const std::string& version, bool hub)
        {
            const auto owner = FarCryMenu::context;
            auto text = owner->model.Text(hub ? "update.availablehub" : "update.availablesite");
            const auto name = FixSettings().GetTexts().Resolve("@plugin.name");
            for (const auto& [key, value] : { std::pair{std::string("{name}"), name}, {std::string("{version}"), version} })
            {
                size_t position = 0;
                while ((position = text.find(key, position)) != std::string::npos)
                {
                    text.replace(position, key.size(), value);
                    position += value.size();
                }
            }
            return asi_settings::text::Widen(text);
        };
        config.tryShow = [](const std::wstring& text)
        {
            const auto owner = FarCryMenu::context;
            if (!FarCryMenu::Available(owner) || !owner->open || !FarCryMenu::Snapshot(owner, "private"))
                return false;
            // The notice stays in the help box until the selection moves
            return FarCryMenu::Help(owner, owner->selected, asi_settings::text::Narrow(text)) &&
                FarCryMenu::Available(owner) && FarCryMenu::Snapshot(owner, "private");
        };
        RWGTA::UpdateCheck::Init(std::move(config));
        FarCryMenu::Tick = []() { RWGTA::UpdateCheck::Tick(); };
        for (const auto module : {GetModuleHandleW(nullptr), GetModuleHandleW(L"CrySystem.dll"), GetModuleHandleW(L"CryGame.dll")})
            BindModule(module);
    }
}
