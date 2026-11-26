module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"
#include "Shared/Overlay/D3D9UpdateNotice.hpp"

// One request to fusionfix.io per start when MAIN.CheckForUpdates is on; FusionFixHub opens on its update page when
// the game closes.

export module Updates;

import ComVars;
import Cinema;
import Window;


export namespace TrueCrimeUpdates
{
    inline asi_overlay::D3D9UpdateNotice Notice;
    inline bool (*BlocksInput)() = nullptr;
    inline const int* NativeLanguage = nullptr;
    inline decltype(&ExitProcess) NativeExit = ExitProcess;

    inline void WINAPI Exit(UINT code)
    {
        RWGTA::UpdateCheck::OnExit();
        NativeExit(code);
    }

    inline bool Playable()
    {
        return GameMode.is_initialized() && GameMode.get() < 2 && PlayerObject.is_initialized() &&
            PlayerObject.get() && !TrueCrimeCinema::Active() && (!BlocksInput || !BlocksInput());
    }

    inline void Draw(IDirect3DDevice9* device, HMODULE module)
    {
        Notice.Present(device, module, Playable());
    }

    inline void BeforeReset()
    {
        Notice.BeforeReset();
    }

    inline void Init()
    {
        auto retail = hook::pattern("89 1D ? ? ? ? 7C ? 83 FB 05 7E ? C7 05 ? ? ? ? 00 00 00 00 68");
        if (!retail.empty())
            NativeLanguage = *retail.get_first<const int*>(2);
        else
            NativeLanguage = *hook::pattern("A1 ? ? ? ? 3B F0 74 ? 83 FE 01 7F ? 83 F8 01 7E ? 8B 04 B5").get_first<const int*>(1);

        RWGTA::UpdateCheck::Config config;
        config.pluginId = "TrueCrimeStreetsofLA.FusionFix";
        config.fileVersion = rsc_FileVersion;
        config.settings = &FixSettings();
        config.playable = Playable;
        config.format = [](const std::string& latest, bool hub)
        {
            static constexpr const char* languages[] = { "en", "en", "fr", "de", "it", "es", "ru" };
            const int language = *NativeLanguage;
            const auto fallback = language >= 0 && language < int(std::size(languages)) ? languages[language] : "en";
            return asi_overlay::D3D9UpdateNotice::Format(FixSettings(), fallback, latest, hub);
        };
        config.show = [](const std::wstring& line) { Notice.Show(line); };
        RWGTA::UpdateCheck::Init(std::move(config));
        auto imports = IATHook::Replace(GetModuleHandleW(nullptr), "KERNEL32.dll", std::forward_as_tuple("ExitProcess", Exit));
        NativeExit = reinterpret_cast<decltype(NativeExit)>(imports.at("ExitProcess").get());
        TrueCrimeWindow::BeforeForcedExit = RWGTA::UpdateCheck::OnExit;
    }
}
