module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <fstream>
#include <map>
#include "ResolutionHooks.hpp"

// The game's own resolution picker: it lists every display mode (each resolution once, at its highest refresh rate)
// instead of five 4:3 modes, and its choice is the resolution the game starts with. MAIN.ResX/ResY override it.

export module Resolution;

import ComVars;


export namespace SecondSightResolution
{
    // scr_width, scr_height, scr_hz as the game keeps them
    inline SafetyHookMid LoadHook;

    inline bool Saved()
    {
        wchar_t path[MAX_PATH]{};
        GetModuleFileNameW(nullptr, path, DWORD(std::size(path)));
        std::ifstream file(std::filesystem::path(path).parent_path() / L"secondsight.cfg");
        std::string line;
        while (std::getline(file, line))
        {
            if (line.rfind("scr_width=", 0) == 0)
                return true;
        }
        return false;
    }

    // Called when the game has read scr_width/scr_height, with its scr_hz; returns the refresh rate it keeps
    inline uint32_t Load(uint32_t refresh)
    {
        int width = int(ChosenWidth.get()), height = int(ChosenHeight.get());
        const int overrideWidth = FixSettings().Int("MAIN.ResX"), overrideHeight = FixSettings().Int("MAIN.ResY");
        if (overrideWidth > 0 && overrideHeight > 0)
            std::tie(width, height) = std::make_tuple(overrideWidth, overrideHeight);
        else if (!Saved() || width < 640 || height < 480)
            std::tie(width, height) = GetDesktopRes();
        if (width < 640 || height < 480 || width > 16384 || height > 16384)
            std::tie(width, height) = std::make_tuple(640, 480);
        ChosenWidth = uint32_t(width);
        ChosenHeight = uint32_t(height);
        // The picker shows the chosen mode only when its refresh rate is the listed one
        const auto& modes = Modes();
        const auto mode = modes.find({ width, height });
        return mode != modes.end() ? uint32_t(mode->second) : refresh;
    }

    // pattern: the game's scr_width/scr_height/scr_hz loader at the scr_height store
    inline bool Init(hook::pattern loader)
    {
        auto filter = hook::pattern("8B 44 24 0C 83 F8 3C 7C ? 83 F8 55 7F ? 8B 44 24 04 3D 80 02 00 00");
        ChosenWidth.SetAddress(*loader.get_first<uint32_t*>(1) - 1);
        ChosenHeight.SetAddress(ChosenWidth.get_ptr() + 1);
        ChosenRefresh.SetAddress(ChosenWidth.get_ptr() + 2);
        LoadHook = safetyhook::create_mid(loader.get_first(26), [](SafetyHookContext& context)
        {
            context.eax = Load(context.eax);
        });
        injector::MakeJMP(filter.get_first(), &ListedPreserving, true);
        return bool(LoadHook);
    }
}
