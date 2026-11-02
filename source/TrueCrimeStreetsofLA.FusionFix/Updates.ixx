module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

// One request to fusionfix.io per start when MAIN.CheckForUpdates is on; FusionFixHub opens on its update page when
// the game closes.

export module Updates;


export namespace TrueCrimeUpdates
{
    inline void Init()
    {
        RWGTA::UpdateCheck::Config config;
        config.pluginId = "TrueCrimeStreetsofLA.FusionFix";
        config.fileVersion = rsc_FileVersion;
        config.settings = &FixSettings();
        config.playable = []() { return true; };
        config.show = [](const std::wstring&) {};
        RWGTA::UpdateCheck::Init(std::move(config));
    }
}
