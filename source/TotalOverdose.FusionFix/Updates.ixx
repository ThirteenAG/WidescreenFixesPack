module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

export module Updates;

// Update notice: in place of the version number at the top of the main menu
export namespace Updates
{
    char notice[128] = {};
    std::atomic<bool> available = false;

    using MakeStringFn = int(__cdecl*)(const char* text, int type, int flags);
    MakeStringFn MakeString = nullptr;
    SafetyHookInline VersionHook;
    int __stdcall Version(int* out)
    {
        if (!available)
            return VersionHook.stdcall<int>(out);
        *out = MakeString(notice, 45, 0);
        return *out;
    }

    void Install()
    {
        auto version = hook::pattern("83 EC 34 FF 15 ? ? ? ? 35 D3 3F EA 19 50 8D 44 24 04 68 ? ? ? ? 50 E8 ? ? ? ? 83 C4 0C 8D 0C 24 51 6A 00 6A 02 FF 15 ? ? ? ? 85 C0 74 24");
        MakeString = reinterpret_cast<MakeStringFn>(injector::GetBranchDestination(version.get_first(0x43)).as_int()); //0x488290
        VersionHook = safetyhook::create_inline(version.get_first(), Version);                                           //0x4856F0

        RWGTA::UpdateCheck::Config config;
        config.pluginId = "TotalOverdose.FusionFix";
        config.fileVersion = rsc_FileVersion;
        config.settings = &FixSettings();
        config.playable = []() { return true; };
        // The menu font has the Latin-1 letters
        config.show = [](const std::wstring& line)
        {
            size_t i = 0;
            for (; i < line.size() && i + 1 < std::size(notice); ++i)
                notice[i] = line[i] < 256 ? static_cast<char>(line[i]) : '?';
            notice[i] = '\0';
            available = true;
        };
        RWGTA::UpdateCheck::Init(std::move(config));
    }
}

